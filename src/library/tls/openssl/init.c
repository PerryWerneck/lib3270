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

 #include <pthread.h>
 #include <openssl/ssl.h>

 static void * ssl_thread(Context *context);


 static int disconnect(H3270 *hSession, Context *context) {


	return 0;
 }

 static int finalize(H3270 *hSession, Context *context) {

	void *retval = 0;
	pthread_join(&context->thread,&retval);


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
	hSession->connection.context = context;

	if(pthread_create(&context->thread, NULL, ssl_thread, context)){

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

	return 0;

 }

 static void * ssl_thread(Context *context) {

	// Disable non-blocking mode.
	set_blocking_mode(context->hSession,context->hSession->connection.sock,0);

	// Protect openssl from concurrent access.
	pthread_mutex_lock(&ssl_guard);






	// Cleanup
	SSL_free(context->ssl);
	SSL_CTX_free(context->ctx);
	lib3270_free(context);

	pthread_mutex_unlock(&ssl_guard);
	return 0;

 }

