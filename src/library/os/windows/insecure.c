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


/*
 * Contatos:
 *
 * perry.werneck@gmail.com      (Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com     (Erico Mascarenhas Mendonça)
 *
 */

/**
 * @brief The default (non ssl) network context.
 *
 */

 #include <config.h>
 #include <private/defs.h>
 #include <private/network.h>
 #include <lib3270/ssl.h>
 #include <lib3270/mainloop.h>
 #include <lib3270/memory.h>
 #include <private/trace.h>
 #include <private/session.h>
 #include <private/network.h>
 #include <private/intl.h>
 #include <private/mainloop.h>
 #include <private/popup.h>
 #include <string.h>
 #include <errno.h>

 #include <internals.h>

 typedef struct _context Context;

 /// @brief Connection context for insecure (non SSL) connections.
 typedef struct _context {
	LIB3270_NET_CONTEXT parent;
	void *recv;
} Context;

static int disconnect(H3270 *hSession, Context *context) {

	debug("%s",__FUNCTION__);

	if(context->recv) {
		hSession->poll.remove(hSession,context->recv);
		context->recv = NULL;
	}

	if(hSession->connection.sock != INVALID_SOCKET) {
		closesocket(hSession->connection.sock);
		hSession->connection.sock = INVALID_SOCKET;
	}

	return 0;
 
 }

 static int finalize(H3270 *hSession, Context *context) {

	if(context->recv) {
		hSession->poll.remove(hSession,context->recv);
		context->recv = NULL;
	}

	lib3270_free(context);
	return 0;
 }

 static void on_input(H3270 *hSession, SOCKET GNUC_UNUSED(sock), LIB3270_IO_FLAG GNUC_UNUSED(flag), Context *context) {
 
	// https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-wsarecv

	debug("%s",__FUNCTION__);
	
	unsigned char buffer[NETWORK_BUFFER_LENGTH];
	WSABUF wsaBuffer = {
		.len = NETWORK_BUFFER_LENGTH,
		.buf = (char *) buffer
	};

	DWORD received = 0;
	DWORD flags = 0;

	int rc = WSARecv(
		hSession->connection.sock,
		&wsaBuffer,
		1,
		&received,
		&flags,
		NULL,
		NULL
	  );

	if(rc) {
		int error = WSAGetLastError();
		if(error == WSAEWOULDBLOCK || error == WSAEINPROGRESS) {
			return;
		}
		PostMessage(hSession->hwnd,WM_RECV_FAILED,error,0);
		connection_close(hSession,error);
	}

	debug("Recv %ld bytes",received);
 	net_input(hSession, buffer, received);

 }

 static int enable_exception(H3270 *hSession, Context *context) {
	return 0;
 }

 static int on_write(H3270 *hSession, const void *buffer, size_t length, Context *context) {

	return 0;
 }
 
 LIB3270_INTERNAL LIB3270_NET_CONTEXT * setup_non_ssl_context(H3270 *hSession) {

	debug("----------------------------------------- %s",__FUNCTION__);

	WSASetLastError(0);

	// Set the socket I/O mode: In this case FIONBIO
	// enables or disables the blocking mode for the 
	// socket based on the numerical value of iMode.
	// If iMode = 0, blocking is enabled; 
	// If iMode != 0, non-blocking mode is enabled.
	// https://learn.microsoft.com/pt-br/windows/win32/api/winsock/nf-winsock-ioctlsocket
	u_long iMode= 1;

	if(ioctlsocket(hSession->connection.sock,FIONBIO,&iMode)) {

		int err = WSAGetLastError();

		static const LIB3270_POPUP popup = {
			.name		= "ioctl",
			.type		= LIB3270_NOTIFY_CONNECTION_ERROR,
			.title		= N_("Connection error"),
			.summary	= N_("Unable to set non-blocking mode."),
			.body		= "",
			.label		= N_("OK")
		};

		PostMessage(hSession->hwnd,WM_POPUP_WSA_ERROR,err,(LPARAM) &popup);
		connection_close(hSession,err);
		return NULL;
	}

	set_ssl_state(hSession,LIB3270_SSL_UNSECURE);

	static const LIB3270_SSL_MESSAGE message = {
		.icon = "dialog-error",
		.summary = N_( "The session is not secure" ),
		.body = N_( "No TLS/SSL support on this session" )
	};
	hSession->ssl.message = &message;

	Context *context = lib3270_malloc(sizeof(Context));
	memset(context,0,sizeof(Context));

	context->parent.disconnect = (void *) disconnect;
	context->parent.finalize = (void *) finalize;

	//context->read = hSession->poll.add(hSession,hSession->connection.sock,LIB3270_IO_FLAG_READ,(void *) on_input,context);

	hSession->connection.except = (void *) enable_exception;
	hSession->connection.write = (void *) on_write;

	context->recv = hSession->poll.add(hSession,hSession->connection.sock,LIB3270_IO_FLAG_READ,(void *) on_input,context);

	return (LIB3270_NET_CONTEXT *) context;
 }
