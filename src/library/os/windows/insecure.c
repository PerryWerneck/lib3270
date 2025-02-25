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
 #include <string.h>
 #include <errno.h>

 #include <internals.h>
 
 /// @brief Connection context for insecure (non SSL) connections.
 typedef struct {
	LIB3270_NET_CONTEXT parent;

 } Context;


static int disconnect(H3270 *hSession, Context *context) {

	debug("%s",__FUNCTION__);


	if(hSession->connection.sock != INVALID_SOCKET) {
		closesocket(hSession->connection.sock);
		hSession->connection.sock = INVALID_SOCKET;
	}

	return 0;
 
 }

 static int finalize(H3270 *hSession, Context *context) {
	lib3270_free(context);
	return 0;
 }

 LIB3270_INTERNAL LIB3270_NET_CONTEXT * setup_non_ssl_context(H3270 *hSession) {

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

	return (LIB3270_NET_CONTEXT *) context;
 }
