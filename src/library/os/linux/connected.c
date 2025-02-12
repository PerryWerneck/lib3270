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
 #include <private/network.h>
 #include <private/intl.h>
 #include <private/session.h>
 #include <private/intl.h>
 #include <lib3270/popup.h>
 #include <trace_dsc.h>

 #include <string.h>

 typedef struct {
	LIB3270_NET_CONTEXT parent;
	void *except;
	void *recv;
 } Context;

 static int net_close(H3270 *hSession, Context *context) {
	lib3270_remove_poll_fd(hSession,context->parent.sock);
	close(context->parent.sock);
	return 0;
 }

 static void net_except(H3270 *hSession, int sock, LIB3270_IO_FLAG flag, Context *context) {

	// Connection error.
    int error = ETIMEDOUT;
    socklen_t errlen = sizeof(error);
    if(getsockopt(sock, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen) < 0) {
        error = errno;
    }

	trace_dsn(hSession,"RCVD network exception: %s\n",strerror(error));

	debug("%s: failed: %s",__FUNCTION__,strerror(error));
	lib3270_connection_close(hSession,error);

	LIB3270_POPUP popup = {
		.name		= "network-error",
		.type		= LIB3270_NOTIFY_ERROR,
		.title		= _("Network error"),
		.summary	= _("A network error occurred while attempting to communicate with the server. This could be due to a timeout, network congestion, or server issues."),
		.body		= strerror(error),
		.label		= _("OK")
	};

	lib3270_popup(hSession, &popup, 0);
	lib3270_connection_close(hSession,-1);

 }

 static void net_recv(H3270 *hSession, int sock, LIB3270_IO_FLAG flag, Context *context) {

	char buffer[16384];
	ssize_t bytes = recv(context->parent.sock, buffer, 16384, 0);

	if(bytes < 0) {

		if(errno == EWOULDBLOCK) {
			return;
		}

		int error = errno;
		debug("%s: %s",__FUNCTION__,strerror(error));

		LIB3270_POPUP popup = {
			.name		= "recv-error",
			.type		= LIB3270_NOTIFY_ERROR,
			.title		= _("Network error"),
			.summary	= _("Error receiving data from host. This may be due to network issues, server unavailability, or other connection problems."),
			.body		= strerror(error),
			.label		= _("OK")
		};

		lib3270_popup(hSession, &popup, 0);
		lib3270_connection_close(hSession,error);

	} else if(bytes == 0) {

		debug("%s: Connection closed by peer",__FUNCTION__);
		trace_dsn(hSession,"RCVD network exception: %s\n",strerror(ECONNRESET));

		LIB3270_POPUP popup = {
			.name		= "network-error",
			.type		= LIB3270_NOTIFY_ERROR,
			.title		= _("Network error"),
			.summary	= _("Connection closed by peer"),
			.body		= _("The connection was closed by the remote host."),
			.label		= _("OK")
		};

		lib3270_popup(hSession, &popup, 0);
		lib3270_connection_close(hSession,ECONNRESET);

	} else {

		// Process data
		debug("%s: Received %d bytes",__FUNCTION__, (int) bytes);

	}

 }

 static int net_send(H3270 *hSession, const void *buffer, size_t length, Context *context) {

	ssize_t bytes = send(context->parent.sock,buffer,length,0);

	if(bytes >= 0)
		return bytes;
		
	int error = errno;
	trace_dsn(hSession,"SND socket error: %s\n", strerror(error));

	LIB3270_POPUP popup = {
		.name		= "recv-error",
		.type		= LIB3270_NOTIFY_ERROR,
		.title		= _("Network error"),
		.summary	= _("Error sending data to host. This may be due to network issues, server unavailability, or other connection problems."),
		.body		= strerror(error),
		.label		= _("OK")
	};

	lib3270_popup(hSession, &popup, 0);
	lib3270_connection_close(hSession,error);

	return -error;
 }

 LIB3270_INTERNAL LIB3270_NET_CONTEXT * connected_insecure(H3270 *hSession, int sock) {

	Context *context = lib3270_malloc(sizeof(Context));
	memset(context,0,sizeof(Context));

	context->parent.sock = sock;
	context->parent.disconnect = (void *) net_close;
	context->except = lib3270_add_poll_fd(hSession,sock,LIB3270_IO_FLAG_EXCEPTION,(void *) net_except, context);
	context->recv = lib3270_add_poll_fd(hSession,sock,LIB3270_IO_FLAG_READ,(void *) net_recv, context);

	hSession->connection.write = (void *) net_send;

	return (LIB3270_NET_CONTEXT *) context;
 }
