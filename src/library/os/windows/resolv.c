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

 #include <config.h>
 #include <winsock2.h>
 #include <windows.h>

 #include <private/network.h>
 #include <private/intl.h>
 #include <lib3270/mainloop.h>
 #include <lib3270/log.h>
 #include <lib3270/popup.h>
 #include <private/trace.h>

 typedef struct {
	LIB3270_NET_CONTEXT parent;
	SOCKET sock;

	const char *hostname;
	const char *service;

	void *timer;
	void *resolved;

 } Context;

 static int finalize(H3270 *hSession, Context *context) {	

	debug("%s: Cleaning resolver context %p",__FUNCTION__,context);

	int rc = 0;

	if(context->timer) {
		hSession->timer.remove(hSession,context->timer);
		context->timer = NULL;
	}
	
	if(context->resolved) {
		hSession->poll.remove(hSession,context->resolved);
		context->resolved = NULL;
	}

	if(context->sock != INVALID_SOCKET) {
		closesocket(context->sock);
		context->sock = INVALID_SOCKET;	
	}

	return rc;
 }

 static void failed(H3270 *hSession, Context *context) {
	if(context->sock != INVALID_SOCKET) {
		closesocket(context->sock);
		context->sock = INVALID_SOCKET;	
	}
	connection_close(hSession,-1);
 }

 static void net_response(H3270 *hSession, int sock, LIB3270_IO_FLAG flag, Context *context) {

	debug("%s: GOT response on context %p",__FUNCTION__,context);

 }

 static int net_timeout(H3270 *hSession, Context *context) {

	debug("%s: TIMEOUT",__FUNCTION__);

	context->timer = NULL;

	return 0;
 }

 ///
 /// @brief Asynchronously resolves the hostname to an IP address.
 ///
 /// This function takes a hostname as input and performs an asynchronous DNS
 /// resolution to obtain the corresponding IP address. It uses non-blocking
 /// operations to ensure that the main thread is not blocked while waiting for
 /// the DNS resolution to complete.
 ///
 /// @param hostname The hostname to be resolved.
 /// @param service The service/port name.
 /// @param timeout The connection timeout in seconds.
 /// @return The resolver network context, NULL if failed.
 ///
 LIB3270_INTERNAL LIB3270_NET_CONTEXT * resolv_hostname(H3270 *hSession, const char *hostname, const char *service, time_t timeout) {

	// Allocate and setup context
	Context *context = lib3270_malloc(sizeof(Context) + strlen(hostname) + strlen(service) + 3);
	memset(context,0,sizeof(Context));
	context->hostname = (char *) (context+1);
	strcpy(context->hostname,hostname);
	context->service = context->hostname + strlen(hostname) + 1;
	strcpy(context->service,service);
	
	// Set disconnect handler
	context->parent.disconnect = (void *) finalize;

	// TODO: Implement the resolver

	return (LIB3270_NET_CONTEXT *) context;
 }
