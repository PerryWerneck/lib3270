/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2025 Banco do Brasil S.A.
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
 #include <winsock2.h>
 #include <windows.h>
 #include <wininet.h>

 #include <lib3270/defs.h>
 #include <lib3270/memory.h>

 #include <private/network.h>
 #include <private/intl.h>
 #include <private/mainloop.h>
 #include <private/session.h>
 #include <private/host.h>
 #include <private/popup.h>
 #include <private/trace.h>
 #include <private/win32_poll.h>

 typedef struct {
	LIB3270_NET_CONTEXT parent;
	SOCKET sock;
	void *timer;
	void *connected;
 } Context;

 static void net_connected(H3270 *hSession, SOCKET sock, Context *context) {

	debug("%s: CONNECTED",__FUNCTION__);

	if(context->connected) {
		win32_poll_remove(context->connected);
		context->connected = NULL;
	}

	if(context->timer) {
		hSession->timer.remove(hSession,context->timer);
		context->timer = NULL;
	}

	WSASetLastError(0);

	// Check for connection errors.
	{
		int 		err;
		socklen_t	len		= sizeof(err);

		if(getsockopt(context->sock,SOL_SOCKET,SO_ERROR,(char *) &err,&len)) {

			static const LIB3270_POPUP popup = {
				.name		= "connect-error",
				.type		= LIB3270_NOTIFY_CONNECTION_ERROR,
				.title		= N_("Connection error"),
				.summary	= N_("Unable to get connection state."),
				.body		= "",
				.label		= N_("OK")
			};

			err = WSAGetLastError(); 

			debug("Sending error popup %s - %d\n",popup.name,(int) err);
			popup_wsa_error(hSession,err,&popup);
			return;
	
		} else if(err) {

			PostMessage(hSession->hwnd,WM_CONNECTION_FAILED,err,0);
			connection_close(hSession,err);
			return;

		}

	}

	// Check connection status with getpeername
	{
		struct sockaddr_storage addr;
		socklen_t len = sizeof(addr);

		if(getpeername(sock, (struct sockaddr *)&addr, &len) != 0) {
	
			int err = WSAGetLastError();
			PostMessage(hSession->hwnd,WM_CONNECTION_FAILED,err,0);
			connection_close(hSession,err);
			return;

		}

		char host[NI_MAXHOST];
		if (getnameinfo((struct sockaddr *) &addr, sizeof(addr), host, sizeof(host), NULL, 0, NI_NUMERICHOST) == 0) {
			trace_network(
				hSession,
				"Established connection to %s\n",
				host
			);
		}
	
	}

	debug("---> %s","Connected to host");
	PostMessage(hSession->hwnd,WM_CONNECTION_SUCCESS,sock,sock);

 }

 static int net_timeout(H3270 *hSession, Context *context) {

	context->timer = NULL;
	connection_close(hSession,ETIMEDOUT);
	PostMessage(hSession->hwnd,WM_CONNECTION_FAILED,ERROR_INTERNET_TIMEOUT,0);

	return 0;
 }

 static int net_disconnect(H3270 *hSession, Context *context) {

	if(context->connected) {
		win32_poll_remove(context->connected);
		context->connected = NULL;
	}

	if(context->timer) {
		hSession->timer.remove(hSession,context->timer);
		context->timer = NULL;
	}

	if(context->sock != INVALID_SOCKET) {
		closesocket(context->sock);
		context->sock = INVALID_SOCKET;
	}

	return 0;
 }

 static int net_finalize(H3270 *hSession, Context *context) {

	if(context->connected) {
		win32_poll_remove(context->connected);
		context->connected = NULL;
	}

	if(context->timer) {
		hSession->timer.remove(hSession,context->timer);
		context->timer = NULL;
	}

	lib3270_free(context);
	return 0;
 }

 /// @brief Connnection was started, wait for it to be available.
 /// @param hSession The session handle.
 /// @param sock The socket
 LIB3270_INTERNAL void set_resolved(H3270 *hSession, SOCKET sock) {

	debug("%s socket=%lu ----------------------------------------------------",__FUNCTION__,(unsigned long) sock);

	Context *context = lib3270_new(Context);
	memset(context,0,sizeof(Context));

	context->parent.disconnect = (void *) net_disconnect;
	context->parent.finalize = (void *) net_finalize;
	context->sock = sock;
	set_network_context(hSession,(LIB3270_NET_CONTEXT *) context);

	hSession->ever_3270 = 0;
	set_cstate(hSession, LIB3270_PENDING);
	notify_new_state(hSession, LIB3270_STATE_HALF_CONNECT, 1);

	context->timer = hSession->timer.add(hSession,hSession->connection.timeout*1000,(void *) net_timeout,context);
	context->connected = win32_poll_add(hSession,sock,FD_CONNECT|FD_WRITE,(void *) net_connected,context);

	
}

 