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
 #include <private/session.h>
 #include <private/mainloop.h>

 #include <lib3270.h>

 #include <lib3270/log.h>
 #include <lib3270/popup.h>

 struct _lib3270_net_context {
	int sock;
	void *timer;
	void *poll;
 };

 static void net_connected(H3270 *hSession, int sock, LIB3270_IO_FLAG flag, LIB3270_NET_CONTEXT *context) {

	if(flag & LIB3270_IO_FLAG_WRITE) {
		debug("%s: CONNECTED",__FUNCTION__);
//		lib3270_set_socket(hSession,sock);
		return;
	}

	// Connection error.
    int error = ETIMEDOUT;
    socklen_t errlen = sizeof(error);
    if(getsockopt(sock, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen) < 0) {
        error = errno;
    }

//	lib3270_disconnect(hSession);

	LIB3270_POPUP popup = {
		.name		= "connect-error",
		.type		= LIB3270_NOTIFY_ERROR,
		.title		= _("Connection error"),
		.summary	= _("Unable to connect to host"),
		.body		= strerror(error),
		.label		= _("OK")
	};

	lib3270_popup(hSession, &popup, 0);

 }

/*
 static void net_disconnect(H3270 *hSession, LIB3270_NET_CONTEXT *context) {
	lib3270_remove_timer(hSession,context->timer);
	lib3270_remove_poll_fd(hSession,context->sock);
	close(context->sock);
 }
*/

 static int timer_expired(H3270 *hSession, LIB3270_NET_CONTEXT *data) {

//	lib3270_disconnect(hSession);

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

 LIB3270_NET_CONTEXT * connect_insecure(H3270 *hSession, const char *hostname, const char *service, time_t timeout) {

	debug("%s(%s,%s,%u)",__FUNCTION__,hostname,service,timeout);

	struct addrinfo	  hints;
	struct addrinfo * result	= NULL;
	memset(&hints,0,sizeof(hints));

	hints.ai_family 	= AF_UNSPEC;	// Allow IPv4 or IPv6
	hints.ai_socktype	= SOCK_STREAM;	// Stream socket
	hints.ai_flags		= AI_PASSIVE;	// For wildcard IP address
	hints.ai_protocol	= 0;			// Any protocol

	int rc = getaddrinfo(hostname, service, &hints, &result);
	if(rc) {

		lib3270_disconnect(hSession);

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
	LIB3270_NET_CONTEXT *context = NULL;

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

			lib3270_disconnect(hSession);

			LIB3270_POPUP popup = {
				.name		= "connect-error",
				.type		= LIB3270_NOTIFY_ERROR,
				.title		= _("Connection error"),
				.summary	= _( "fcntl() error when getting socket state." ),
				.body		= strerror(errno),
				.label		= _("OK")
			};

			lib3270_popup(hSession, &popup, 0);

			return NULL;
		}

		f |= O_NDELAY;

		if (fcntl(sock, F_SETFL, f) < 0) {

			lib3270_disconnect(hSession);

			LIB3270_POPUP popup = {
				.name		= "connect-error",
				.type		= LIB3270_NOTIFY_ERROR,
				.title		= _("Connection error"),
				.summary	= _( "fcntl() error when setting non blocking state." ),
				.body		= strerror(errno),
				.label		= _("OK")
			};

			lib3270_popup(hSession, &popup, 0);

			return NULL;

		}

		if(connect(sock,rp->ai_addr, rp->ai_addrlen) && errno != EINPROGRESS) {
			error = errno;
			continue;
		}

		// Wait for connection.
		context = lib3270_malloc(sizeof(LIB3270_NET_CONTEXT));
		memset(context,0,sizeof(LIB3270_NET_CONTEXT));

		context->sock = sock;
		context->timer = lib3270_add_timer(timeout*1000,hSession,(void *) timer_expired,context);
		context->poll = lib3270_add_poll_fd(hSession,sock,LIB3270_IO_FLAG_WRITE|LIB3270_IO_FLAG_EXCEPTION,(void *) net_connected,context);

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

	return context;

 }
