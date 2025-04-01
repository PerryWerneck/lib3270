/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2008 Banco do Brasil S.A.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * Contatos:
 *
 * perry.werneck@gmail.com      (Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com     (Erico Mascarenhas Mendonça)
 *
 */

#include <config.h>

#ifdef HAVE_UNISTD_H
	#include <unistd.h>
#endif

#include <internals.h>

#include "telnetc.h"
#include <private/host.h>
#include <private/status.h>
#include <errno.h>
#include <lib3270/log.h>
#include <lib3270/trace.h>
#include <lib3270/toggle.h>
#include <lib3270/ssl.h>
#include <private/session.h>
#include <private/intl.h>
#include <private/trace.h>
#include <private/util.h>

#include <private/network.h>

#include <uriparser/Uri.h>

 ///
 /// @param hSession The TN3270 session.
 /// @param seconds Timeout in seconds.
 /// @return 0 if suceeded, non zero if failed
 /// @retval 0 The connection is running.
 /// @retval EBUSY The session already has a connection.
 /// @retval -1 The connection has failed and an error popup was sent.
 ///
 /// This function starts the process of establishing a connection to remote
 /// host without blocking the calling thread. It sets up necessary parameters
 /// and begins the connection attempt, allowing other operations to proceed
 /// concurrently.
 ///
 LIB3270_EXPORT int lib3270_connect(H3270 *hSession, int seconds) {

	if(!(hSession->connection.url && *hSession->connection.url)) {
		trace_event(hSession,"No URL to connect to\n");
		return ENODATA;
	}

	if(!lib3270_allow_connect(hSession)) {
		debug("%s:%s",__FUNCTION__,strerror(errno));
		trace_event(hSession,"Unable to connect: %s\n",strerror(errno));
		return errno == 0 ? -1 : errno;
	}

	// https://uriparser.github.io/doc/api/latest/
	UriUriA uri;
	const char * errorPos;
	if(uriParseSingleUriA(&uri, hSession->connection.url, &errorPos) != URI_SUCCESS) {
		return EINVAL;	
	}

	// Extract scheme
	size_t szscheme = (uri.scheme.afterLast-uri.scheme.first);
	if(!szscheme) {
		return EINVAL;
	}
	char scheme[szscheme+2];
	strncpy(scheme,uri.scheme.first,szscheme);
	scheme[szscheme] = 0;

	debug("Scheme is '%s'",scheme);

	// Extract hostname
	size_t szhost = (uri.hostText.afterLast-uri.hostText.first);
	if(!szhost) {
		return EINVAL;
	}
	char hostname[szhost+2];
	strncpy(hostname,uri.hostText.first,szhost);
	hostname[szhost] = 0;

	// Extract port
	size_t szport = (uri.portText.afterLast-uri.portText.first);
	char port[szport+2];
	if(szport) {
		strncpy(port,uri.portText.first,szport);
	}
	port[szport] = 0;

	uriFreeUriMembersA(&uri);

	debug("%s(%s,%s,%u)",__FUNCTION__,hostname,port,seconds);

	if(hSession->connection.context) {
		debug("%s:%s (context: %p)",__FUNCTION__,strerror(EBUSY),hSession->connection.context);
		trace_event(
			hSession,
			"Connection to %s:%s with %d seconds rejected: %s\n",
				hostname,
				port,
				seconds,
				strerror(EISCONN)
		);
		return errno = EISCONN;
	}

	//
	// Prepare to connect.
	//
	set_ssl_state(hSession,LIB3270_SSL_UNDEFINED);

	snprintf(
	    hSession->full_model_name,
	    LIB3270_FULL_MODEL_NAME_LENGTH,
	    "IBM-327%c-%d%s",
	    hSession->m3279 ? '9' : '8',
	    hSession->model_num,
	    hSession->extended ? "-E" : ""
	);

	hSession->ever_3270	= 0;

	//
	// Starting 'connect' task.
	//
	trace_event(
		hSession,
		"Connecting to %s:%s with %d seconds timeout\n",
			hostname,
			port,
			seconds
	);

	if(!strcasecmp(scheme,"tn3270s")) {

		// Use SSL
		hSession->ssl.host = 1;
		trace_dsn(hSession,"TLS/SSL is enabled\n");

	} else if(!strcasecmp(scheme,"tn3270")) {

		// Dont use SSL
		hSession->ssl.host = 0;
		trace_dsn(hSession,"TLS/SSL is disabled\n");

	} else {

		static const LIB3270_POPUP failed = {
			.name		= "invalid-scheme",
			.type		= LIB3270_NOTIFY_CRITICAL,
			.title		= N_("Connection error"),
			.summary	= N_("Invalid connection scheme"),
			.body		= "",
			.label		= N_("OK")
		};

		lib3270_autoptr(LIB3270_POPUP) popup =
		    lib3270_popup_clone_printf(
		        &failed,
				_("Unsupported scheme '%s' in URL %s"),
		        scheme,
		        hSession->connection.url
		    );

		lib3270_popup(hSession, popup, 0);
		connection_close(hSession,ENOTSUP);
		return ENOTSUP;

	}

	hSession->cbk.cursor(hSession,LIB3270_POINTER_LOCKED & 0x03);
	notify_new_state(hSession, LIB3270_STATE_RESOLVING, 1);
	message_changed(hSession, LIB3270_MESSAGE_RESOLVING);

	hSession->connection.timeout = seconds;
	hSession->connection.context = resolv_hostname(hSession,hostname,port,seconds);

	if(!hSession->connection.context) {
		// No context, the DNS query has failed, call disconnect to clear flags.
		trace_event(hSession,"Unable to start DNS resolver\n");
		connection_close(hSession,ENOTCONN);
		return errno = ENOTCONN;
	}

	// Got context, the connect is running.
	return 0;

 }

 static void connection_complete(H3270 *hSession) {
	
	setup_session(hSession);
	set_connected_initial(hSession);

#ifdef _WIN32
	{

		// Set timeouts for windows sockets

	}
#else 
	{

		// Set timeouts for linux sockets

		struct timeval timeout;      
		timeout.tv_sec = hSession->connection.auto_disconnect * 60;
		timeout.tv_usec = 0;

		if (setsockopt(hSession->connection.sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout) < 0) {
			LIB3270_POPUP popup = {
				.name		= "socket-timeout-failed",
				.type		= LIB3270_NOTIFY_NETWORK_ERROR,
				.title		= _("Network I/O error"),
				.summary	= _("Unable to set socket receive timeout"),
				.body		= strerror(errno),
				.label		= _("OK")
			};
			connection_close(hSession,errno);
			lib3270_popup(hSession, &popup, 0);
			return;
		}

		if (setsockopt(hSession->connection.sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof timeout) < 0) {
			LIB3270_POPUP popup = {
				.name		= "socket-timeout-failed",
				.type		= LIB3270_NOTIFY_NETWORK_ERROR,
				.title		= _("Network I/O error"),
				.summary	= _("Unable to set socket send timeout"),
				.body		= strerror(errno),
				.label		= _("OK")
			};
			connection_close(hSession,errno);
			lib3270_popup(hSession, &popup, 0);
			return;
		}

	}
#endif


 }

#ifdef _WIN32
 void set_connected_socket(H3270 *hSession, SOCKET sock) {
#else
 void set_connected_socket(H3270 *hSession, int sock) {
#endif //_WIN32

	debug("%s(%d)",__FUNCTION__,sock);

	trace_dsn(
		hSession,
		"Connected to %s\n", 
		hSession->connection.url
	);
	
	if(hSession->connection.context) {
		trace_network(hSession,"Connection established, cleaning 'connect' context\n");
		hSession->connection.context->finalize(hSession,hSession->connection.context);
		hSession->connection.context = NULL;
	}

	hSession->connection.sock = sock;

	if(hSession->ssl.host) {

		start_tls(hSession,connection_complete);

	} else {

		trace_network(hSession,"Starting non-tls context\n");
		setup_non_tls_context(hSession);
		connection_complete(hSession);

	}

 }

 int lib3270_allow_connect(const H3270 *hSession) {
	//
	// Can't reconnect if already reconnecting *OR* there's an open popup
	// (to avoid open more than one connect error popup).
	//
	if(hSession->auto_reconnect_inprogress) {
		debug("%s: auto_reconnect_inprogress",__FUNCTION__);
		errno = EBUSY;
		return 0;
	}

	// Is the session disconnected?
	if(!lib3270_is_disconnected(hSession)) {
		debug("%s: is_disconnected=FALSE",__FUNCTION__);
		errno = EISCONN;
		return 0;
	}

	// Do I have a defined host?
	if(!(hSession->connection.url && *hSession->connection.url)) {
		debug("%s('%s')",__FUNCTION__,hSession->connection.url);
		errno = ENODATA;
		return 0;
	}

	// Do I have a connection context?
	if(hSession->connection.context) {
		errno = EBUSY;
		return 0;
	}

	return 1;
}


