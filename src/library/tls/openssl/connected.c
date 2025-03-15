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

 #include <config.h>
 #include <lib3270/defs.h>
 #include <private/defs.h>
 #include <private/session.h>
 #include <private/network.h>
 #include <private/intl.h>
 #include <private/mainloop.h>
 #include <private/popup.h>
 #include <private/trace.h>
 #include <private/openssl.h>
 #include <lib3270/memory.h>

 #include <openssl/ssl.h>
 #include <openssl/err.h>
 #include <openssl/bio.h>

 static int disconnect(H3270 *hSession, Context *context) {

	context->state = 3;

	if(context->xio.read) {
		hSession->poll.remove(hSession,context->xio.read);
		context->xio.read = NULL;
	}

	if(context->xio.except) {
		hSession->poll.remove(hSession,context->xio.except);
		context->xio.except = NULL;
	}

	if(context->ssl) {
		SSL_shutdown(context->ssl);
		SSL_free(context->ssl);
		context->ssl = NULL;
		context->tcp = NULL;
		hSession->connection.sock = -1;
	}

	if(context->ctx) {
		SSL_CTX_free(context->ctx);
		context->ctx = NULL;
	}

	return 0;
 }

 static int finalize(H3270 *hSession, Context *context) {

	disconnect(hSession,context);
	lib3270_free(context);

	return 0;
 }


 static void on_receive(H3270 *hSession, int GNUC_UNUSED(sock), LIB3270_IO_FLAG GNUC_UNUSED(flag), Context *context) {
 
	unsigned char buffer[NETWORK_BUFFER_LENGTH];

	// https://docs.openssl.org/3.2/man3/SSL_read/
	size_t readbytes = 0;
	int rc = SSL_read_ex(context->ssl,buffer,NETWORK_BUFFER_LENGTH,&readbytes);
	if(rc == 1) {
		debug("Recv %ld bytes",length);
		net_input(hSession, buffer, readbytes);   
		return;
	}

	// https://docs.openssl.org/3.2/man3/SSL_get_error/	
	int ssl_error = SSL_get_error(context->ssl, rc);

	// https://docs.openssl.org/3.2/man3/SSL_get_error/
	switch(ssl_error) {
	case SSL_ERROR_ZERO_RETURN:
		{
			lib3270_autoptr(char) errors = openssl_errors(context);
			trace_ssl(hSession,"%s\n%s","The secure connection has been closed cleanly",errors);

			LIB3270_POPUP popup = {
				.summary	= _("Disconnected from host."),
				.body		= _("The secure connection has been closed cleanly."),
				.type		= LIB3270_NOTIFY_DISCONNECTED,
				.title		= _("TLS/SSL error"),
				.label		= _("OK")
			};

			connection_close(context->hSession, -1);

			lib3270_popup(hSession,&popup,0);

		}
		return;

	case SSL_ERROR_WANT_READ:
	case SSL_ERROR_WANT_X509_LOOKUP:
		return;	// Force a new loop.

	case SSL_ERROR_SYSCALL:
		{
			int code = errno;
			lib3270_autoptr(char) errors = openssl_errors(context);
			trace_ssl(hSession,"System error %d on secure connection\n%s",code,errors);

			LIB3270_POPUP popup = {
				.summary	= _("System error occurred during secure connection."),
				.body		= strerror(code),
				.type		= LIB3270_NOTIFY_NETWORK_IO_ERROR,
				.title		= _("Network I/O error"),
				.label		= _("OK")
			};

			connection_close(context->hSession, -1);

			lib3270_popup(hSession,&popup,0);

		}
		return;

	}

	lib3270_autoptr(char) errors = openssl_errors(context);

	trace_dsn(hSession,"RCVD SSL_read error %d\n%s\n", ssl_error, errors);

	LIB3270_POPUP popup = {
		.summary	= _("Error occurred while receiving data securely"),
		.body		= errors,
		.type		= LIB3270_NOTIFY_NETWORK_IO_ERROR,
		.title		= _("Network I/O error"),
		.label		= _("OK")
	};

	connection_close(context->hSession, -1);

	lib3270_popup(hSession,&popup,0);

 }

 static int on_send(H3270 *hSession, const void *buffer, size_t length, Context *context) {

	/*
	int rc = SSL_write(hSession->network.context->con, (const char *) buffer, length);
	if(rc > 0)
		return rc;

	// https://www.openssl.org/docs/man1.0.2/man3/SSL_get_error.html
	int ssl_error = SSL_get_error(hSession->network.context->con, rc);
	switch(ssl_error) {
	case SSL_ERROR_ZERO_RETURN:

		trace_ssl(hSession,"%s","The secure connection has been closed cleanly");

		lib3270_set_network_error(
			hSession,
		    _("Disconnected from host."),
		    "%s",
		    _("The secure connection has been closed cleanly.")
		);

		return 0;

	case SSL_ERROR_WANT_READ:
	case SSL_ERROR_WANT_X509_LOOKUP:
		return -EWOULDBLOCK;	// Force a new loop.

	case SSL_ERROR_SYSCALL:
		return lib3270_socket_send_failed(hSession);

	}

	// Build error message.
	char err_buf[120];
	(void) ERR_error_string(ssl_error, err_buf);
	trace_dsn(hSession,"RCVD SSL_write error %d (%s)\n", ssl_error, err_buf);

	lib3270_set_network_error(
		hSession,
		_("Error writing to host."),
		_("The SSL error message was %s"),
		err_buf
	);

	return -1;
	*/

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

 LIB3270_INTERNAL void openssl_success(H3270 *hSession, Context *context) {

	set_blocking_mode(hSession,hSession->connection.sock,0);

	context->parent.disconnect = (void *) disconnect;
	context->parent.finalize = (void *) finalize;

	context->xio.read = hSession->poll.add(hSession,hSession->connection.sock,LIB3270_IO_FLAG_READ,(void *) on_receive,context);
	context->xio.except = hSession->poll.add(hSession,hSession->connection.sock,LIB3270_IO_FLAG_EXCEPTION,(void *) on_exception,context);

	hSession->connection.except = (void *) enable_exception;
	hSession->connection.write = (void *) on_send;

	// set_ssl_state(session,LIB3270_SSL_SECURE);

	setup_session(hSession);
	set_connected_initial(hSession);

 }

