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

 #ifdef HAVE_UNISTD_H
	#include <unistd.h>
 #endif

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
 #include <openssl/opensslv.h>

 /// @brief Connection context for OpenSSL connections.
 typedef struct {

	LIB3270_NET_CONTEXT parent;
	
	char state;
	H3270 *hSession;
	SSL *ssl;

	void (*complete)(H3270 *hSession);

 } Context;

 static void * ssl_thread(Context *context);

 static int disconnect(H3270 *hSession, Context *context) {

	context->state = 1;

	SSL_shutdown(context->ssl);
	hSession->connection.sock = -1;

	return 0;
 }

 static int finalize(H3270 *hSession, Context *context) {

	context->state = 2;

	if(context->ssl) {
		SSL_free(context->ssl);
		context->ssl = NULL;
	}

	return 0;
 }

 LIB3270_INTERNAL int openssl_setup(H3270 *hSession) {

	hSession->ssl.host = 1;
	hSession->ssl.start = openssl_start_tls;
	trace_dsn(hSession,"TLS/SSL is enabled using OpenSSL " OPENSSL_VERSION_STR "\n");

	return 0;
 }

 LIB3270_INTERNAL int openssl_start_tls(H3270 *hSession, void (*complete)(H3270 *hSession)) {

	memset(&hSession->ssl.message,0,sizeof(hSession->ssl.message));

	hSession->ssl.host = 1;	// Set host type as SSL.
	set_ssl_state(hSession,LIB3270_SSL_NEGOTIATING);

	Context *context = lib3270_new(Context);
	memset(context,0,sizeof(Context));
	context->hSession = hSession;
	context->complete = complete;
	context->parent.disconnect = (void *) disconnect;
	context->parent.finalize = (void *) finalize;

	set_network_context(hSession,(LIB3270_NET_CONTEXT *) context);

	// Initialize SSL
	lib3270_autoptr(SSL_CTX) ctx = openssl_context(hSession);
	if(!ctx) {

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
		context->ssl = SSL_new(ctx);
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

 static void complete(H3270 *hSession, Context *context) {

	debug("--------------> %s",SSL_state_string_long(context->ssl));

	if(!hSession->ssl.message.name) {
		set_ssl_message(hSession,openssl_message_from_code(X509_V_OK));
		set_ssl_state(hSession,LIB3270_SSL_SECURE);
	} else {
		set_ssl_state(hSession,LIB3270_SSL_NEGOTIATED);
	}

	set_blocking_mode(hSession,hSession->connection.sock,0);
	context->complete(hSession);

	{
		pthread_mutex_lock(&ssl_guard);

		SSL_up_ref(context->ssl);
		openssl_tls_complete(hSession,context->ssl);
	
		context_free(context);
	
		pthread_mutex_unlock(&ssl_guard);
	}
	
 }

 static void * ssl_thread(Context *context) {

	// Disable non-blocking mode.
	if(set_blocking_mode(context->hSession,context->hSession->connection.sock,1)) {
		context_free(context);
		return 0;
	}

	if(context->hSession->connection.timeout) {
		
		struct timeval timeout;      
		timeout.tv_sec = context->hSession->connection.timeout;
		timeout.tv_usec = 0;		

		if(setsockopt(context->hSession->connection.sock, SOL_SOCKET, SO_RCVTIMEO, &timeout,sizeof timeout) < 0)
		{
			LIB3270_POPUP popup = {
				.name		= "connect-error",
				.type		= LIB3270_NOTIFY_CONNECTION_ERROR,
				.title		= _("Connection error"),
				.summary	= _("Unable to set socket timeout"),
				.body		= strerror(errno),
				.label		= _("OK")
			};
		
			lib3270_popup(context->hSession, &popup, 0);
			connection_close(context->hSession,errno);
			context_free(context);
			return 0;
		
		}

		if(setsockopt(context->hSession->connection.sock, SOL_SOCKET, SO_SNDTIMEO, &timeout,sizeof timeout) < 0)
		{
			LIB3270_POPUP popup = {
				.name		= "connect-error",
				.type		= LIB3270_NOTIFY_CONNECTION_ERROR,
				.title		= _("Connection error"),
				.summary	= _("Unable to set socket timeout"),
				.body		= strerror(errno),
				.label		= _("OK")
			};
		
			lib3270_popup(context->hSession, &popup, 0);
			connection_close(context->hSession,errno);
			context_free(context);
			return 0;
		
		}

	}

	// Protect openssl from concurrent access.
	pthread_mutex_lock(&ssl_guard);

	lib3270_autoptr(char) server_name = lib3270_get_server_name(context->hSession);

    // Set the server name in the SSL context so that it'll be sent
    // in the server name extension in the client hello.
	SSL_set_tlsext_host_name(context->ssl, server_name);

    // Pass the socket to the BIO interface, which OpenSSL uses to create the TLS session.
    BIO *tcp = BIO_new_socket(context->hSession->connection.sock, BIO_CLOSE);
    if(tcp == NULL) {
		openssl_failed(
			context->hSession,
			-1,
			_("Unexpected error associating network socket with TLS/SSL context")
		);
		context_free(context);
		pthread_mutex_unlock(&ssl_guard);
		return 0;
	}        

    SSL_set_bio(context->ssl, tcp, tcp);

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
				context->hSession,
				code,
				summary
			);

		} else if(connect_result < 0) {

			trace_ssl(context->hSession,"The TLS/SSL handshake was not successful, because a fatal error occurred (rc=%d)\n",connect_result);

			int code = SSL_get_error(context->ssl,connect_result);

			lib3270_autoptr(char) body = openssl_errors();
	
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

	if(!context->state) {

		// Now that we've established a TLS session with the server,
		// we need to verify that the FQDN in the server cert matches
		// the server name we used to establish the connection.

		lib3270_autoptr(X509) cert = SSL_get_peer_certificate(context->ssl);
		lib3270_autoptr(char) server_name = lib3270_get_server_name(context->hSession);
		const LIB3270_SSL_MESSAGE *ssl_message = NULL;

		if(cert && server_name && *server_name) {

			ssl_message = openssl_check_fqdn(context->hSession,cert,server_name);

    	} else {

			// No Peer certificate
			ssl_message = openssl_message_from_name("NO_PEER_CERTIFICATE");

		}

		if(ssl_message) {

			trace_ssl(
				context->hSession,
				"%s\n",
				ssl_message->summary
			);

			set_ssl_message(context->hSession,ssl_message);

			if(!context->hSession->cbk.check_policy(context->hSession,ssl_message->name,EINVAL)) {

				trace_ssl(
					context->hSession,
					"Aproving by policy '%s'\n",
						ssl_message->name
				);
				
			} else {

				LIB3270_POPUP popup = {
					.title		= _("FQDN validation error"),
					.summary	= ssl_message->summary,
					.body		= ssl_message->body,
					.type		= LIB3270_NOTIFY_TLS_ERROR,
					.label		= _("OK")
				};

				connection_close(context->hSession, -1);
				lib3270_popup_async(context->hSession, &popup);
	
			}
	
		}
	}

	// TODO: Check CRL

	// Finalize
	if(context->state) {

		// Failed to establish the TLS session, so close the connection.
		// This will also free the context.
		if(context->hSession->connection.sock != -1) {
			close(context->hSession->connection.sock);
			context->hSession->connection.sock = -1;
		}

		context_free(context);
		pthread_mutex_unlock(&ssl_guard);

	} else {

		// The TLS session was established successfully.
		// Now we can go back to the main thread, start the network I/O and free context.
		pthread_mutex_unlock(&ssl_guard);

		context->hSession->post(
			context->hSession,
			(void(*)(H3270 *, void *)) complete,
			context,
			sizeof(Context *)
		);

	}

	return 0;

 }

