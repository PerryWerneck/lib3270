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
	return 0;
 }

 static int finalize(H3270 *hSession, Context *context) {

	context->state = 2;

	if(context->tcp) {
		BIO_free(context->tcp);
		context->tcp = NULL;
	}

	if(context->ssl) {
		SSL_free(context->ssl);
		context->ssl = NULL;
	}

	if(context->ctx) {
		SSL_CTX_free(context->ctx);
		context->ctx = NULL;
	}

	return 0;
 }

 LIB3270_INTERNAL int start_tls(H3270 *hSession) {

	set_ssl_state(hSession,LIB3270_SSL_NEGOTIATING);

	Context *context = lib3270_new(Context);
	memset(context,0,sizeof(Context));

	// Initialize SSL
	context->ctx = get_openssl_context(hSession);
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

	// Start negotiation thread.
	context->hSession = hSession;
	context->parent.disconnect = (void *) disconnect;
	context->parent.finalize = (void *) finalize;
	hSession->connection.context = (LIB3270_NET_CONTEXT *) context;

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

 static void ssl_thread_failed(Context *context, const char *message, int code) {

	lib3270_autoptr(char) name = lib3270_strdup_printf("openssl-%d",code);

	LIB3270_POPUP popup = {
		.name		= name,
		.type		= LIB3270_NOTIFY_TLS_ERROR,
		.title		= _("TLS/SSL error"),
		.label		= _("OK")
	};

	BIO * e = BIO_new(BIO_s_mem());
	if(e) {
		ERR_print_errors(e);
		(void)BIO_flush(e);
		BUF_MEM *bptr = NULL;
		BIO_get_mem_ptr(e, &bptr);
		popup.body = bptr->data;
	} else {
		popup.body = _("BIO_new failed");
	}

	connection_close(context->hSession, code ? code : -1);
	context_free(context);

	if(code) {
		lib3270_autoptr(char) summary = 
			lib3270_strdup_printf(message,SSL_get_error(context->ssl, code));

		popup.summary = summary;
		trace_ssl(context->hSession,"TLS/SSL failed with error '%s'\n%s\n",popup.summary,popup.body);

		lib3270_popup_async(context->hSession, &popup);

	} else {

		popup.summary = message;
		trace_ssl(context->hSession,"TLS/SSL failed with error '%s'\n%s\n",popup.summary,popup.body);

		lib3270_popup_async(context->hSession, &popup);

	}

	if(e) {
		BIO_free_all(e);
	}

 }

 static void * ssl_thread(Context *context) {

	int rc;

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
    context->tcp = BIO_new_socket(context->hSession->connection.sock, BIO_NOCLOSE);
    if(context->tcp == NULL) {
		ssl_thread_failed(context,_("Unexpected error associating network socket with TLS/SSL context"),0);
		pthread_mutex_unlock(&ssl_guard);
		return 0;
	}        

    SSL_set_bio(context->ssl, context->tcp, context->tcp);

	// Establish the TLS session.
    if ((rc = SSL_connect(context->ssl)) <= 0) {
		ssl_thread_failed(context,"%s",rc);
		pthread_mutex_unlock(&ssl_guard);
		return 0;
	}

	if(context->state) {
		context_free(context);
		pthread_mutex_unlock(&ssl_guard);
		return 0;
	}

    // Now that we've established a TLS session with the server,
	// we need to verify that the FQDN in the server cert matches
    // the server name we used to establish the connection.
	// check_fqdn(context, server_name);



	// Cleanup
	context_free(context);
	pthread_mutex_unlock(&ssl_guard);

	return 0;

 }

