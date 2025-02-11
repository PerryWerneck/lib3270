/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2008 Banco do Brasil S.A.
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
 #include <trace_dsc.h>
 #include <private/session.h>

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
		lib3270_remove_poll(hSession,context->xio.read);
		context->xio.read = NULL;
	}

	if(context->xio.except) {
		lib3270_remove_poll(hSession,context->xio.except);
		context->xio.except = NULL;
	}

	if(context->xio.write) {
		lib3270_remove_poll(hSession,context->xio.write);
		context->xio.write = NULL;
	}

	if(context->parent.sock != -1) {
		close(context->parent.sock);
		context->parent.sock = -1;
	}

	return 0;
 
 }

 static void on_input(H3270 *hSession, int sock, LIB3270_IO_FLAG GNUC_UNUSED(flag), Context *context) {
 

 }

 void on_exception(H3270 *hSession, int GNUC_UNUSED(fd), LIB3270_IO_FLAG GNUC_UNUSED(flag), Context *context) {

	debug("%s",__FUNCTION__);
	trace_dsn(hSession,"RCVD urgent data indication\n");

	if (!hSession->syncing) {
		hSession->syncing = 1;
		if(context->xio.except) {
			lib3270_remove_poll(hSession, context->xio.except);
			context->xio.except = NULL;
		}
	}

 }

 LIB3270_INTERNAL LIB3270_NET_CONTEXT * setup_non_ssl_context(H3270 *hSession, int sock) {

	set_ssl_state(hSession,LIB3270_SSL_UNSECURE);

	Context *context = lib3270_malloc(sizeof(Context));
	memset(context,0,sizeof(Context));

	context->parent.sock = sock;
	context->parent.disconnect = (void *) disconnect;

	context->xio.read = lib3270_add_poll_fd(hSession,sock,LIB3270_IO_FLAG_READ,on_input,context);
	context->xio.except = lib3270_add_poll_fd(hSession,sock,LIB3270_IO_FLAG_EXCEPTION,on_exception,context);

	return (LIB3270_NET_CONTEXT *) context;
 }
