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
 #include <private/mainloop.h>
 #include <lib3270/memory.h>
 #include <private/trace.h>
 #include <private/session.h>
 #include <private/network.h>
 #include <private/intl.h>
 #include <private/mainloop.h>
 #include <private/popup.h>
 #include <private/win32_poll.h>
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
		win32_poll_remove(context->recv);
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
		win32_poll_remove(context->recv);
		context->recv = NULL;
	}

	lib3270_free(context);
	return 0;
 }

 static void on_input(H3270 *hSession, SOCKET GNUC_UNUSED(sock), Context *context) {
 
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

	// https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-wsasend

	debug("%s",__FUNCTION__);


	WSABUF wsaBuffer = {
		.len = length,
		.buf = (char *) buffer
	};

	DWORD sent = 0;
	int rc = WSASend(
		hSession->connection.sock,
		&wsaBuffer,
		1,
		&sent,
		0,
		NULL,
		NULL
	);

	if(rc == SOCKET_ERROR) {
		int error = WSAGetLastError();
		if(error == WSAEWOULDBLOCK || error == WSAEINPROGRESS) {
			return 0;
		}
		PostMessage(hSession->hwnd,WM_SEND_FAILED,error,0);
		connection_close(hSession,error);
	}

	return (int) sent;
 }
 
 LIB3270_INTERNAL LIB3270_NET_CONTEXT * setup_non_tls_context(H3270 *hSession) {

	set_blocking_mode(hSession, hSession->connection.sock, 0);

	set_ssl_state(hSession,LIB3270_SSL_UNSECURE);
	hSession->ssl.message.icon = "dialog-error";
	hSession->ssl.message.summary = _( "The session is not secure" );
	hSession->ssl.message.body = _( "No TLS/SSL support on this session" );

	Context *context = lib3270_malloc(sizeof(Context));
	memset(context,0,sizeof(Context));

	context->parent.disconnect = (void *) disconnect;
	context->parent.finalize = (void *) finalize;

	hSession->connection.except = (void *) enable_exception;
	hSession->connection.write = (void *) on_write;

	context->recv = win32_poll_add(hSession,hSession->connection.sock,FD_READ,(void *) on_input,context);

	return (LIB3270_NET_CONTEXT *) context;
 }
