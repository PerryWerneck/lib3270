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
#include <internals.h>
#include "telnetc.h"
#include "hostc.h"
#include "statusc.h"
#include <errno.h>
#include <lib3270/log.h>
#include <lib3270/trace.h>
#include <lib3270/toggle.h>
#include <lib3270/ssl.h>
#include <private/session.h>
#include <private/intl.h>
#include <trace_dsc.h>
#include "utilc.h"
#include <trace_dsc.h>

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

	if(!(hSession->host.url && *hSession->host.url)) {
		return ENODATA;
	}

	if(!lib3270_allow_connect(hSession)) {
		debug("%s:%s",__FUNCTION__,strerror(errno));
		return errno == 0 ? -1 : errno;
	}

	// https://uriparser.github.io/doc/api/latest/
	UriUriA uri;
	const char * errorPos;
	if(uriParseSingleUriA(&uri, hSession->host.url, &errorPos) != URI_SUCCESS) {
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
		lib3270_write_event_trace(
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

	hSession->ever_3270	= False;

	//
	// Starting 'connect' task.
	//
	lib3270_write_event_trace(
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
			.type		= LIB3270_NOTIFY_ERROR,
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
		        hSession->host.url
		    );

		lib3270_popup(hSession, popup, 0);
		lib3270_connection_close(hSession,ENOTSUP);
		return ENOTSUP;

	}

	hSession->cbk.cursor(hSession,LIB3270_POINTER_LOCKED & 0x03);
	lib3270_st_changed(hSession, LIB3270_STATE_RESOLVING, True);
	status_changed(hSession, LIB3270_MESSAGE_RESOLVING);

	hSession->connection.timeout = seconds;
	hSession->connection.context = resolv_hostname(hSession,hostname,port,seconds);

	if(!hSession->connection.context) {
		// No context, the DNS query has failed, call disconnect to clear flags.
		lib3270_connection_close(hSession,ENOTCONN);
		return errno = ENOTCONN;
	}

	// Got context, the connect is running.
	return 0;

 }

 void lib3270_set_connected_socket(H3270 *hSession, int sock) {

	if(hSession->connection.context) {
		free(hSession->connection.context);
		hSession->connection.context = NULL;
	}

	trace_dsn(
		hSession,
		"Connected to %s%s.\n", 
		hSession->host.url,hSession->ssl.host ? " using SSL": ""
	);

	//if(lib3270_start_tls(hSession)) {
	//	lib3270_connection_close(hSession,-1);
	//	return;
	//}

	lib3270_setup_session(hSession);
	lib3270_set_connected_initial(hSession);

	if(hSession->ssl.host) {

		set_ssl_state(hSession,LIB3270_SSL_NEGOTIATING);

		// FIX-ME: Add ssl support.

		close(sock);
		lib3270_connection_close(hSession,ENOTSUP);
		return;

	} else {

		if(hSession->connection.context) {
			free(hSession->connection.context);
			hSession->connection.context = NULL;
		}

		hSession->connection.context = setup_non_ssl_context(hSession,sock);

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
	if(!(hSession->host.url && *hSession->host.url)) {
		debug("%s('%s')",__FUNCTION__,hSession->host.url);
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

/*
void lib3270_notify_tls(H3270 *hSession) {

	// Negotiation complete is the connection secure?
	if(hSession->ssl.message->type != LIB3270_NOTIFY_INFO) {
		// Ask user what I can do!
		if(lib3270_popup_translated(hSession,(const LIB3270_POPUP *) hSession->ssl.message,1) == ECANCELED) {
			lib3270_disconnect(hSession);
		}
	}

}
*/

int lib3270_start_tls(H3270 *hSession) {

	debug("%s: NEED REFACTOR----------------------------------------------------------------",__FUNCTION__);
/*

	hSession->ssl.message = NULL;	// Reset message.
	set_ssl_state(hSession,LIB3270_SSL_NEGOTIATING);

	non_blocking(hSession,False);

#pragma GCC diagnostic push
#ifdef _WIN32
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif // _WIN32
	int rc = lib3270_run_task(
	             hSession,
				 "StartTLS",
	             (int(*)(H3270 *h, void *)) hSession->network.module->start_tls,
	             NULL
	         );
#pragma GCC diagnostic pop

	if(rc == ENOTSUP) {

		// No support for TLS/SSL in the active network module, the connection is insecure
		set_ssl_state(hSession,LIB3270_SSL_UNSECURE);
		return 0;

	}

	// The network module SHOULD set the status message.
	if(!hSession->ssl.message) {

		static const LIB3270_POPUP message = {
			.type = LIB3270_NOTIFY_CRITICAL,
			.summary = N_( "Can't determine the TLS/SSL state"),
			.body = N_("The network module didn't set the TLS/SSL state message, this is not supposed to happen and can be a coding error")
		};

		set_ssl_state(hSession,LIB3270_SSL_UNSECURE);
		lib3270_popup_translated(hSession,&message,0);
		return EINVAL;

	}

	if(rc) {

		// Negotiation has failed. Will disconnect
		set_ssl_state(hSession,LIB3270_SSL_UNSECURE);

		if(hSession->ssl.message) {
			lib3270_popup_translated(hSession,(const LIB3270_POPUP *) hSession->ssl.message,0);
		}

		return rc;
	}

	set_ssl_state(hSession,(hSession->ssl.message->type == LIB3270_NOTIFY_INFO ? LIB3270_SSL_SECURE : LIB3270_SSL_NEGOTIATED));
	non_blocking(hSession,True);
*/

	return 0;
}


