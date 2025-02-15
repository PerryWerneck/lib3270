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
 #include <lib3270/malloc.h>

 #include <internals.h>
 #include <hostc.h>
 #include <statusc.h>
 #include <private/trace.h>

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
		.type		= LIB3270_NOTIFY_CONNECTION_ERROR,
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

	if(context->except) {
		hSession->poll.remove(hSession,context->except);
		context->except = NULL;
	}

	if(context->connected) {
		hSession->poll.remove(hSession,context->connected);
		context->connected = NULL;
	}

	if(context->timer) {
		hSession->timer.remove(hSession,context->timer);
		context->timer = NULL;
	}

	// Check for connection errors.
	{
		int 		error	= 0;
		socklen_t	errlen	= sizeof(error);

		if(getsockopt(sock, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen) < 0) {

			error = errno;
			lib3270_connection_close(hSession, error);

			LIB3270_POPUP popup = {
				.name		= "connect-error",
				.type		= LIB3270_NOTIFY_CONNECTION_ERROR,
				.title		= _("Connection error"),
				.summary	= _("Unable to get connection state."),
				.body		= strerror(error),
				.label		= _("OK")
			};

			lib3270_popup(hSession, &popup, 0);
			return;

		} else if(error) {

			lib3270_connection_close(hSession, error);

			lib3270_autoptr(char) summary =
				lib3270_strdup_printf(
					_( "Can't connect to %s"),
					lib3270_get_url(hSession)
				);

			LIB3270_POPUP popup = {
				.name		= "cant-connect",
				.type		= LIB3270_NOTIFY_CONNECTION_ERROR,
				.title		= _("Connection error"),
				.summary	= summary,
				.body		= "",
				.label		= _("OK")
			};

			set_popup_body(&popup,error);

			lib3270_popup(hSession, &popup, 0);
			return;

		}

	}

	// Check connection status with getpeername
	struct sockaddr_storage addr;
	socklen_t len = sizeof(addr);
	if (getpeername(sock, (struct sockaddr *)&addr, &len) == -1) {

		int error = errno;
		lib3270_connection_close(hSession, error);

		lib3270_autoptr(char) summary =
			lib3270_strdup_printf(
				_("Failed to establish connection to %s"),
				lib3270_get_url(hSession)
			);

		LIB3270_POPUP popup = {
			.name		= "connect-error",
			.type		= LIB3270_NOTIFY_CONNECTION_ERROR,
			.title		= _("Connection error"),
			.summary	= summary,
			.body		= "",
			.label		= _("OK")
		};

		set_popup_body(&popup,error);

		lib3270_popup(hSession, &popup, 0);
		return;
	}

	char host[NI_MAXHOST];
	if (getnameinfo((struct sockaddr *) &addr, sizeof(addr), host, sizeof(host), NULL, 0, NI_NUMERICHOST) == 0) {
		trace_dsn(
			hSession,
			"Established connection to %s\n",
			host
		);
	}

	lib3270_set_connected_socket(hSession,sock);

 }

 static int net_timeout(H3270 *hSession, Context *context) {

	context->timer = NULL;
	lib3270_connection_close(hSession,ETIMEDOUT);

	LIB3270_POPUP popup = {
		.name		= "connect-error",
		.type		= LIB3270_NOTIFY_CONNECTION_ERROR,
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
		hSession->poll.remove(hSession,context->except);
		context->except = NULL;
	}

	if(context->connected) {
		hSession->poll.remove(hSession,context->connected);
		context->connected = NULL;
	}

	if(context->timer) {
		hSession->timer.remove(hSession,context->timer);
		context->timer = NULL;
	}

	if(context->parent.sock != -1) {
		close(context->parent.sock);
		context->parent.sock = -1;
	}

	return 0;
 }

 LIB3270_INTERNAL int lib3270_connect_socket(H3270 *hSession, int sock, const struct sockaddr *addr, socklen_t addrlen) {

	// Set non blocking mode
	if(lib3270_set_block_mode(hSession,sock,0)) {
		return -1;
	}

	set_ssl_state(hSession,LIB3270_SSL_UNDEFINED);
	lib3270_set_cstate(hSession,LIB3270_CONNECTING);

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

	hSession->ever_3270 = False;

	// Setup socket 
	// set options for inline out-of-band data and keepalives
	{
		// set options for inline out-of-band data and keepalives
		int optval = 1;
		if(setsockopt(sock, SOL_SOCKET, SO_OOBINLINE, &optval, sizeof(optval)) < 0) {
			int rc = errno;
			lib3270_popup_dialog(	hSession,
									LIB3270_NOTIFY_ERROR,
									_( "Connection error" ),
									_( "setsockopt(SO_OOBINLINE) has failed" ),
									"%s",
									strerror(rc));
			return rc;
		}

		optval = lib3270_get_toggle(hSession,LIB3270_TOGGLE_KEEP_ALIVE) ? 1 : 0;
		if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) < 0) {
			int rc = errno;

			char buffer[4096];
			snprintf(buffer,4095,_( "Can't %s network keep-alive" ), optval ? _( "enable" ) : _( "disable" ));

			lib3270_popup_dialog(	hSession,
									LIB3270_NOTIFY_ERROR,
									_( "Connection error" ),
									buffer,
									"%s",
									strerror(rc));
			return rc;
		} else {
			trace_dsn(hSession,"Network keep-alive is %s\n",optval ? "enabled" : "disabled" );
		}

#if defined(OMTU)
		if (setsockopt(hSession->sock, SOL_SOCKET, SO_SNDBUF, (char *)&mtu,sizeof(mtu)) < 0)
		{
			popup_a_sockerr(hSession, _( "setsockopt(%s)" ), "SO_SNDBUF");
			SOCK_CLOSE(hSession);
		}
#endif

	}

	lib3270_set_cstate(hSession, LIB3270_PENDING);
	lib3270_st_changed(hSession, LIB3270_STATE_HALF_CONNECT, True);

	Context *context = lib3270_malloc(sizeof(Context));
	memset(context,0,sizeof(Context));

	context->parent.disconnect = (void *) net_disconnect;

	context->timer = hSession->timer.add(hSession,hSession->connection.timeout*1000,(void *) net_timeout,context);
	context->except = hSession->poll.add(hSession,sock,LIB3270_IO_FLAG_EXCEPTION,(void *) net_except,context);
	context->connected = hSession->poll.add(hSession,sock,LIB3270_IO_FLAG_WRITE,(void *) net_connected,context);

	hSession->connection.context = (LIB3270_NET_CONTEXT *) context;

	return 0;

 }
