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

 // References:
 //		https://learn.microsoft.com/en-us/windows/win32/api/windns/nf-windns-dnsqueryex [Not available on MinGW]

 #include <config.h>
 #include <winsock2.h>
 #include <windows.h>

 #include <private/intl.h>
 #include <lib3270/log.h>
 #include <lib3270/popup.h>
 #include <lib3270/memory.h>
 #include <private/session.h>

 typedef struct {

	LIB3270_NET_CONTEXT parent;

	int enabled;
	HANDLE thread;

	const char *hostname;
	const char *service;


 } Context;

 static int cancel(H3270 *hSession, Context *context) {	

	debug("%s: Cleaning resolver context %p",__FUNCTION__,context);

	

	return 0;
 }

 static void failed(H3270 *hSession, Context *context) {
 }

 static void finalize(H3270 *hSession, Context *context) {

	if(context->thread) {
		context->enabled = 0;
		lib3270_log_write(hSession,"win32","Resolver thread still active, delaying context cleanup");
	} else {
		lib3270_free(context);
	}

 }

 static DWORD __stdcall resolver_thread(LPVOID lpParam) {

	Context *context = (Context *) lpParam;
	debug("%s: Resolving hostname %s",__FUNCTION__,context->hostname);





	debug("%s: Resolver thread finished",__FUNCTION__);
	context->thread = NULL;
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
	context->parent.disconnect = (void *) cancel;
	context->parent.finalize = (void *) finalize;

	// Call resolver in background
	context->enabled = 1;
	context->thread = CreateThread(NULL,0,resolver_thread,context,0,NULL);

	return (LIB3270_NET_CONTEXT *) context;
 }
