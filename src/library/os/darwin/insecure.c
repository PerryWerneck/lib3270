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
 #include <private/popup.h>
 #include <private/trace.h>
 #include <private/session.h>
 #include <private/network.h>
 #include <private/intl.h>
 #include <string.h>
 #include <errno.h>

 #include <internals.h>

 #ifdef HAVE_UNISTD_H
 #include <unistd.h>
 #endif // HAVE_UNISTD_H

 
 /// @brief Connection context for insecure (non SSL) connections.
 typedef struct {
	
	LIB3270_NET_CONTEXT parent;

	// I/O handlers.
	struct {
		void *except;
		void *read;
		void *write;
	} xio;

 } Context;

 static int disconnect(H3270 *hSession, Context *context) {

	debug("%s",__FUNCTION__);

	if(context->xio.read) {
		hSession->poll.remove(hSession,context->xio.read);
		context->xio.read = NULL;
	}

	if(context->xio.except) {
		hSession->poll.remove(hSession,context->xio.except);
		context->xio.except = NULL;
	}

	if(context->xio.write) {
		hSession->poll.remove(hSession,context->xio.write);
		context->xio.write = NULL;
	}

	if(hSession->connection.sock != -1) {
		close(hSession->connection.sock);
		hSession->connection.sock = -1;
	}

	return 0;
 
 }

 static int finalize(H3270 *hSession, Context *context) {
	lib3270_free(context);
	return 0;
 }

 static void on_input(H3270 *hSession, int sock, LIB3270_IO_FLAG GNUC_UNUSED(flag), Context *context) {
 
	unsigned char buffer[NETWORK_BUFFER_LENGTH];

	debug("%s",__FUNCTION__);
	ssize_t length = recv(sock,buffer,NETWORK_BUFFER_LENGTH,MSG_DONTWAIT);

	if(length < 0) {

		LIB3270_POPUP popup = {
			.name		= "recv-failed",
			.type		= LIB3270_NOTIFY_NETWORK_ERROR,
			.title		= _("Network I/O error"),
			.summary	= _("Failed to receive data from the host"),
			.body		= "",
			.label		= _("OK")
		};

		if(errno == EAGAIN || errno == EWOULDBLOCK) {
			return;
		}

		lib3270_popup(hSession, &popup, 0);
		connection_close(hSession,errno);

		return;

	}

	debug("Recv %ld bytes",length);
 	net_input(hSession, buffer, length);

 }

 static void on_exception(H3270 *hSession, int GNUC_UNUSED(fd), LIB3270_IO_FLAG GNUC_UNUSED(flag), Context *context) {

	debug("%s",__FUNCTION__);
	trace_dsn(hSession,"RCVD urgent data indication\n");

	if (!hSession->syncing) {
		hSession->syncing = 1;
		if(context->xio.except) {
			hSession->poll.remove(hSession, context->xio.except);
			context->xio.except = NULL;
		}
	}

 }

 static int enable_exception(H3270 *hSession, Context *context) {
	if(context->xio.except) {
		return EBUSY;
	}
	context->xio.except = hSession->poll.add(hSession,hSession->connection.sock,LIB3270_IO_FLAG_EXCEPTION,(void *) on_exception,context);
	return 0;
 }

 static int on_write(H3270 *hSession, const void *buffer, size_t length, Context *context) {

	ssize_t bytes = send(hSession->connection.sock,buffer,length,0);

	if(bytes >= 0)
		return bytes;

	LIB3270_POPUP popup = {
		.name		= "send-failed",
		.type		= LIB3270_NOTIFY_NETWORK_ERROR,
		.title		= _("Network I/O error"),
		.summary	= _("Failed to send data to the host"),
		.body		= "",
		.label		= _("OK")
	};

	int error = errno;

	if(error == EWOULDBLOCK || error == EAGAIN) {
		return 0;
	}
	
	set_popup_body(&popup,error);
	lib3270_popup(hSession, &popup, 0);

	return -error;

 }

 LIB3270_INTERNAL void setup_non_tls_context(H3270 *hSession) {

	set_ssl_state(hSession,LIB3270_SSL_UNSECURE);

	hSession->ssl.message.icon = "dialog-error";
	hSession->ssl.message.summary = _( "The session is not secure" );
	hSession->ssl.message.body = _( "No TLS/SSL support on this session" );
	
	Context *context = lib3270_new(Context);
	memset(context,0,sizeof(Context));

	context->parent.disconnect = (void *) disconnect;
	context->parent.finalize = (void *) finalize;

	context->xio.read = hSession->poll.add(hSession,hSession->connection.sock,LIB3270_IO_FLAG_READ,(void *) on_input,context);
	context->xio.except = hSession->poll.add(hSession,hSession->connection.sock,LIB3270_IO_FLAG_EXCEPTION,(void *) on_exception,context);

	hSession->connection.except = (void *) enable_exception;
	hSession->connection.write = (void *) on_write;

	set_network_context(hSession, (LIB3270_NET_CONTEXT *) context);

}
