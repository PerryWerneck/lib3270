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
 #include <lib3270/ssl.h>
 #include <lib3270/properties.h>

 #include <pthread.h>
 #include <openssl/ssl.h>
 #include <openssl/err.h>

 static void * ssl_thread(Context *context);

 static int disconnect(H3270 *hSession, Context *context) {

	context->state = 1;

	if(hSession->connection.sock != -1) {
		shutdown(hSession->connection.sock,SHUT_RDWR);
		hSession->connection.sock = -1;
	}

	return 0;
 }

 static int finalize(H3270 *hSession, Context *context) {

	context->state = 2;

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

 LIB3270_INTERNAL int start_tls(H3270 *hSession) {

	set_ssl_message(hSession,NULL);
	set_ssl_state(hSession,LIB3270_SSL_NEGOTIATING);

	Context *context = lib3270_new(Context);
	memset(context,0,sizeof(Context));
	context->hSession = hSession;
	context->parent.disconnect = (void *) disconnect;
	context->parent.finalize = (void *) finalize;

	set_network_context(hSession,(LIB3270_NET_CONTEXT *) context);

	// Initialize SSL
	context->ctx = openssl_context(hSession);
	if(!context->ctx) {

		LIB3270_POPUP popup = {
			.name		= "ssl-context-error",
			.type		= LIB3270_NOTIFY_ERROR,
			.title		= _("Internal error"),
			.summary	= _("Error creating SSL context"),
			.body		= _("Unexpected error getting global SSL context"),
			.label		= _("OK")
		};

		connection_close(hSession,errno);

		lib3270_free(context);
		context = NULL;

		lib3270_popup_async(hSession, &popup);

		return -1;

	}

	{
		pthread_mutex_lock(&ssl_guard);
		context->ssl = SSL_new(context->ctx);
		pthread_mutex_unlock(&ssl_guard);
	}

	if(!context->ssl) {

		LIB3270_POPUP popup = {
			.name		= "ssl-ssl-error",
			.type		= LIB3270_NOTIFY_ERROR,
			.title		= _("Internal error"),
			.summary	= _("Error creating SSL connection"),
			.body		= _("Unexpected error on SSL_new()"),
			.label		= _("OK")
		};

		connection_close(hSession,errno);

		lib3270_free(context);
		context = NULL;

		lib3270_popup_async(hSession, &popup);

		return -1;

	}

	{
		pthread_mutex_lock(&ssl_guard);
		SSL_set_ex_data(context->ssl, e_ctx_ssl_exdata_index, hSession);
		pthread_mutex_unlock(&ssl_guard);
	}

	pthread_t thread;
	if(pthread_create(&thread, NULL, (void *) ssl_thread, context)){

		hSession->connection.context = NULL;

		LIB3270_POPUP popup = {
			.name		= "ssl-thread-error",
			.type		= LIB3270_NOTIFY_ERROR,
			.title		= _("Internal error"),
			.summary	= _("Error creating SSL negotiation thread"),
			.body		= strerror(errno),
			.label		= _("OK")
		};

		connection_close(hSession,errno);

		lib3270_free(context);
		context = NULL;

		lib3270_popup_async(hSession, &popup);

		return -1;

	}	

	pthread_detach(thread);

	return 0;

 }

 static void context_free(Context *context) {

	// TODO: Use instance count.

	finalize(context->hSession,context);
	lib3270_free(context);
 
 }

 static void * ssl_thread(Context *context) {

	// Disable non-blocking mode.
	if(set_blocking_mode(context->hSession,context->hSession->connection.sock,1)) {
		context_free(context);
		return 0;
	}

	// Protect openssl from concurrent access.
	pthread_mutex_lock(&ssl_guard);

	lib3270_autoptr(char) server_name = lib3270_get_server_name(context->hSession);

    // Set the server name in the SSL context so that it'll be sent
    // in the server name extension in the client hello.
	SSL_set_tlsext_host_name(context->ssl, server_name);

    // Pass the socket to the BIO interface, which OpenSSL uses to create the TLS session.
    context->tcp = BIO_new_socket(context->hSession->connection.sock, BIO_CLOSE);
    if(context->tcp == NULL) {
		openssl_failed(
			context,
			-1,
			_("Unexpected error associating network socket with TLS/SSL context")
		);
		context_free(context);
		pthread_mutex_unlock(&ssl_guard);
		return 0;
	}        

    SSL_set_bio(context->ssl, context->tcp, context->tcp);

	// Establish the TLS session.
	{
		int connect_result = SSL_connect(context->ssl);
		debug("Connect result: %d",connect_result);

		if(connect_result == 1) {

			trace_ssl(context->hSession,"TLS/SSL connection established\n");
			if(!context->hSession->ssl.message.name) {
				set_ssl_message(context->hSession,openssl_message_from_code(X509_V_OK));
			}

		} else if(context->hSession->ssl.message.name) {

			connection_close(context->hSession,-1);
			lib3270_popup_async(context->hSession, (LIB3270_POPUP *) &context->hSession->ssl.message);

		} else if(connect_result == 0) {

			// The TLS/SSL handshake was not successful but was shut down controlled 
			// and by the specifications of the TLS/SSL protocol. 
			// Call SSL_get_error() with the return value ret to find out the reason.
			int code = SSL_get_error(context->ssl,connect_result);
			lib3270_autoptr(char) summary =
				lib3270_strdup_printf(
					_("The TLS/SSL handshake has failed with error %d"),
					code
				);

			trace_ssl(context->hSession,"%s\n",summary);

			openssl_failed(
				context,
				code,
				summary
			);

		} else if(connect_result < 0) {

			trace_ssl(context->hSession,"The TLS/SSL handshake was not successful, because a fatal error occurred (rc=%d)\n",connect_result);

			int code = SSL_get_error(context->ssl,connect_result);

			lib3270_autoptr(char) body = openssl_errors(context);
	
			LIB3270_POPUP popup = {
				.body		= body,
				.type		= LIB3270_NOTIFY_TLS_ERROR,
				.title		= _("TLS/SSL error"),
				.label		= _("OK")
			};

			connection_close(context->hSession, -1);

			switch(code) {
			case SSL_ERROR_SSL:
				{
					debug("%s","SSL_ERROR_SSL");

					const char *mask = "ssl-error-%d";

					lib3270_autoptr(char) name = lib3270_strdup_printf(mask,code);
					popup.summary = _("A fatal error occurred during the TLS/SSL handshake process.");
					popup.name = name;

					lib3270_popup_async(context->hSession, &popup);
				}
				break;

			case SSL_ERROR_SYSCALL:

				debug("%s","SSL_ERROR_SYSCALL");
				popup.title	= _("TLS/SSL system error");
				popup.summary = _("A system I/O error occurred during the SSL handshake process.");

				if(connect_result == -1) {

					lib3270_autoptr(char) name = lib3270_strdup_printf("ssl-syscall-%d",errno);
					popup.name = name;
					lib3270_popup_async(context->hSession, &popup);

				} else {

					lib3270_autoptr(char) name = lib3270_strdup_printf("ssl-connect-syscall-%d",code);
					popup.name = name;
					lib3270_popup_async(context->hSession, &popup);

				}
				break;
				
			default:
				{
					lib3270_autoptr(char) summary = lib3270_strdup_printf("Fatal error %d in TLS/SSL handshake",code);
					lib3270_autoptr(char) name = lib3270_strdup_printf("ssl-connect-%d",code);
					popup.name = name;
					popup.summary = summary;
					lib3270_popup_async(context->hSession, &popup);
				}
			}
		
		}

	}

    // Now that we've established a TLS session with the server,
	// we need to verify that the FQDN in the server cert matches
    // the server name we used to establish the connection.

	// TODO: Implement FQDN check.
	// check_fqdn(context, server_name);

	// Finalize
	if(context->state) {

		// Failed to establish the TLS session, so close the connection.
		// This will also free the context.
		if(context->hSession->connection.sock != -1) {
			close(context->hSession->connection.sock);
			context->hSession->connection.sock = -1;
		}

		finalize(context->hSession,context);
		context_free(context);

	} else {

		// The TLS session was established successfully.
		// Now we can go back to the main thread and start the network I/O.
		context->hSession->post(
			context,
			(void(*)(H3270 *, void *)) openssl_success,
			context,
			sizeof(Context *)
		);

	}

	// Cleanup
	pthread_mutex_unlock(&ssl_guard);
	
	return 0;

 }

