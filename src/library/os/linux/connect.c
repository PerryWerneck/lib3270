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

 #define _GNU_SOURCE
 #include <config.h>

 #include <netdb.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <sys/socket.h>
 #include <sys/types.h>
 #include <unistd.h>
 #include <string.h>
 #include <sys/ioctl.h>
 #include <fcntl.h>

 #include <private/network.h>
 #include <private/intl.h>
 #include <private/mainloop.h>
 #include <private/session.h>

 #include <lib3270.h>
 #include <lib3270/mainloop.h>
 #include <lib3270/log.h>
 #include <lib3270/popup.h>

 #include <internals.h>
 #include <hostc.h>
 #include <statusc.h>
 #include <trace_dsc.h>

 typedef struct {
	LIB3270_NET_CONTEXT parent;
	void *timer;
	void *connected;
	void *except;
 } Context;

 static void net_except(H3270 *hSession, int sock, LIB3270_IO_FLAG flag, Context *context) {

	// Connection error.
    int error = ETIMEDOUT;
    socklen_t errlen = sizeof(error);
    if(getsockopt(sock, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen) < 0) {
        error = errno;
    }

	debug("%s: failed: %s",__FUNCTION__,strerror(error));
	lib3270_connection_close(hSession,error);

	LIB3270_POPUP popup = {
		.name		= "connect-error",
		.type		= LIB3270_NOTIFY_ERROR,
		.title		= _("Connection error"),
		.summary	= _("Unable to connect to host"),
		.body		= strerror(error),
		.label		= _("OK")
	};

	lib3270_popup(hSession, &popup, 0);
	lib3270_connection_close(hSession,error);

 }

 static void net_connected(H3270 *hSession, int sock, LIB3270_IO_FLAG flag, Context *context) {
	debug("%s: CONNECTED",__FUNCTION__);
	lib3270_set_connected_socket(hSession,sock);
 }

 static int net_timeout(H3270 *hSession, Context *context) {

	context->timer = NULL;
	lib3270_connection_close(hSession,ETIMEDOUT);

	LIB3270_POPUP popup = {
		.name		= "connect-error",
		.type		= LIB3270_NOTIFY_ERROR,
		.title		= _("Connection error"),
		.summary	= _("Unable to connect to host"),
		.body		= strerror(ETIMEDOUT),
		.label		= _("OK")
	};

	lib3270_popup(hSession, &popup, 0);

	return 0;
 }

static int net_disconnect(H3270 *hSession, Context *context) {

	if(context->except) {
		lib3270_remove_poll(hSession,context->except);
		context->except = NULL;
	}

	if(context->connected) {
		lib3270_remove_poll(hSession,context->connected);
		context->connected = NULL;
	}

	if(context->timer) {
		lib3270_remove_timer(hSession,context->timer);
		context->timer = NULL;
	}

	if(context->parent.sock != -1) {
		close(context->parent.sock);
		context->parent.sock = -1;
	}

	return 0;
 }

 LIB3270_INTERNAL int lib3270_connect_socket(H3270 *hSession, int sock, const struct sockaddr *addr, socklen_t addrlen) {

	{
		int f;
		if ((f = fcntl(sock, F_GETFL, 0)) == -1) {

			int error = errno;
			lib3270_connection_close(hSession,error);

			LIB3270_POPUP popup = {
				.name		= "connect-error",
				.type		= LIB3270_NOTIFY_ERROR,
				.title		= _("Connection error"),
				.summary	= _( "fcntl() error when getting socket state." ),
				.body		= strerror(error),
				.label		= _("OK")
			};

			lib3270_popup(hSession, &popup, 0);

			return error;
		}

		f |= O_NDELAY;

		if (fcntl(sock, F_SETFL, f) < 0) {

			int error = errno;
			lib3270_connection_close(hSession,error);

			LIB3270_POPUP popup = {
				.name		= "connect-error",
				.type		= LIB3270_NOTIFY_ERROR,
				.title		= _("Connection error"),
				.summary	= _( "fcntl() error when setting non blocking state." ),
				.body		= strerror(error),
				.label		= _("OK")
			};

			lib3270_popup(hSession, &popup, 0);

			return error;

		}

	}

	if(connect(sock,addr,addrlen) && errno != EINPROGRESS) {

		int error = errno;
		close(sock);

		trace_dsn(
			hSession,
			"Connection failed: %s\n",
			strerror(error)
		);

		return error;
	}

	// Wait for connection.
	lib3270_st_changed(hSession, LIB3270_STATE_CONNECTING, 1);
	status_changed(hSession, LIB3270_MESSAGE_CONNECTING);

	Context *context = lib3270_malloc(sizeof(Context));
	memset(context,0,sizeof(Context));

	context->parent.disconnect = (void *) net_disconnect;

	context->timer = lib3270_add_timer(hSession->connection.timeout*1000,hSession,(void *) net_timeout,context);
	context->except = lib3270_add_poll_fd(hSession,sock,LIB3270_IO_FLAG_EXCEPTION,(void *) net_except,context);
	context->connected = lib3270_add_poll_fd(hSession,sock,LIB3270_IO_FLAG_WRITE,(void *) net_connected,context);

	hSession->connection.context = (LIB3270_NET_CONTEXT *) context;

	return 0;

 }


/*
 LIB3270_NET_CONTEXT * connect_insecure(H3270 *hSession, const char *hostname, const char *service, time_t timeout) {

	debug("%s(%s,%s,%lu)",__FUNCTION__,hostname,service,timeout);

	struct addrinfo	  hints;
	struct addrinfo * result	= NULL;
	memset(&hints,0,sizeof(hints));

	hints.ai_family 	= AF_UNSPEC;	// Allow IPv4 or IPv6
	hints.ai_socktype	= SOCK_STREAM;	// Stream socket
	hints.ai_flags		= AI_PASSIVE;	// For wildcard IP address
	hints.ai_protocol	= 0;			// Any protocol

	// TODO: Use async DNS resolution.
	// https://github.com/kazuho/examples/blob/master/getaddrinfo_a%2Bsignalfd.c

	int rc = getaddrinfo(hostname, service, &hints, &result);
	if(rc) {

		lib3270_connection_close(hSession,rc);

		LIB3270_POPUP popup = {
			.name		= "connect-error",
			.type		= LIB3270_NOTIFY_ERROR,
			.title		= _("Connection error"),
			.summary	= _("Unable to connect to host"),
			.body		= gai_strerror(rc),
			.label		= _("OK")
		};

		lib3270_popup(hSession, &popup, 0);

		return NULL;
	}

	int sock = -1;
	int error = -1;
	Context *context = NULL;

	struct addrinfo * rp;
	for(rp = result; sock < 0 && rp != NULL && !context; rp = rp->ai_next) {

		// Got socket from host definition.
		sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(sock < 0) {
			// Can't get socket.
			error = errno;
			continue;
		}

		int f;
		if ((f = fcntl(sock, F_GETFL, 0)) == -1) {

			int error = errno;
			lib3270_connection_close(hSession,error);

			LIB3270_POPUP popup = {
				.name		= "connect-error",
				.type		= LIB3270_NOTIFY_ERROR,
				.title		= _("Connection error"),
				.summary	= _( "fcntl() error when getting socket state." ),
				.body		= strerror(error),
				.label		= _("OK")
			};

			lib3270_popup(hSession, &popup, 0);

			return NULL;
		}

		f |= O_NDELAY;

		if (fcntl(sock, F_SETFL, f) < 0) {

			int error = errno;
			lib3270_connection_close(hSession,error);

			LIB3270_POPUP popup = {
				.name		= "connect-error",
				.type		= LIB3270_NOTIFY_ERROR,
				.title		= _("Connection error"),
				.summary	= _( "fcntl() error when setting non blocking state." ),
				.body		= strerror(error),
				.label		= _("OK")
			};

			lib3270_popup(hSession, &popup, 0);

			return NULL;

		}

		if(connect(sock,rp->ai_addr, rp->ai_addrlen) && errno != EINPROGRESS) {
			error = errno;
			close(sock);
			continue;
		}

		// Wait for connection.
		context = lib3270_malloc(sizeof(Context));
		memset(context,0,sizeof(Context));

		context->parent.disconnect = (void *) net_disconnect;

		context->timer = lib3270_add_timer(timeout*1000,hSession,(void *) net_timeout,context);
		context->except = lib3270_add_poll_fd(hSession,sock,LIB3270_IO_FLAG_EXCEPTION,(void *) net_except,context);
		context->connected = lib3270_add_poll_fd(hSession,sock,LIB3270_IO_FLAG_WRITE,(void *) net_connected,context);

		break;

	}
	
	freeaddrinfo(result);

	if(!context) {
		// Connection error.
		LIB3270_POPUP popup = {
			.name		= "connect-error",
			.type		= LIB3270_NOTIFY_ERROR,
			.title		= _("Connection error"),
			.summary	= _("Can't connect to host"),
			.body		= strerror(error),
			.label		= _("OK")
		};

		lib3270_popup(hSession, &popup, 0);
	}

	return (LIB3270_NET_CONTEXT *) context;

 }
*/
