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
 //		https://learn.microsoft.com/pt-br/windows/win32/api/winsock2/nf-winsock2-wsaasyncgethostbyaddr [Doesn't support IPv6]
 //		https://learn.microsoft.com/pt-br/windows/win32/api/ws2tcpip/nf-ws2tcpip-getnameinfo

 #include <config.h>
 #include <winsock2.h>
 #include <windows.h>

 #include <private/intl.h>
 #include <lib3270/log.h>
 #include <lib3270/popup.h>
 #include <lib3270/memory.h>
 #include <private/trace.h>
 #include <private/session.h>
 #include <private/mainloop.h>

 typedef struct {

	LIB3270_NET_CONTEXT parent;

	int enabled;
	int finalized;

	H3270 *hSession;
	HANDLE thread;

	void *timer;

	const char *hostname;
	const char *service;


 } Context;

 static int cancel(H3270 *hSession, Context *context) {	

	if(context->timer) {
		hSession->timer.remove(hSession,context->timer);
		context->timer = NULL;
	}

	debug("%s: Cleaning resolver context %p",__FUNCTION__,context);
	context->enabled = 0;
	return 0;
 }

 static void timeout(H3270 *hSession, Context *context) {

	trace_network(
		hSession,
		"Hostname resolution to %s:%s has timed out, cancelling\n",
		context->hostname,
		context->service
	);
	
	context->enabled = 0;
	PostMessage(context->hSession->hwnd,WM_RESOLV_TIMEOUT,ETIMEDOUT,0);

 }

 static void finalize(H3270 *hSession, Context *context) {

	if(context->timer) {
		hSession->timer.remove(hSession,context->timer);
		context->timer = NULL;
	}

	if(context->thread) {
		context->enabled = 0;
		context->finalized = 1;
		trace_network(hSession,"Resolver thread still active, delaying context finalization\n");
	} else {
		lib3270_free(context);
	}

 }

 static DWORD __stdcall resolver_thread(LPVOID lpParam) {

	Context *context = (Context *) lpParam;
	trace_network(context->hSession,"Resolving %s:%s\n",context->hostname,context->service);

	// https://learn.microsoft.com/en-us/windows/win32/api/ws2tcpip/nf-ws2tcpip-getaddrinfo
	struct addrinfo hints;
	struct addrinfo *result = NULL;
	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

	int rc = getaddrinfo(
		context->hostname,
		context->service,
		&hints,
		&result
	);

	if(rc) {

		// Name resolution has failed.
		
		if(context->enabled) {
			// Context was not canceled.
			PostMessage(context->hSession->hwnd,WM_RESOLV_FAILED,rc,0);
		} else {
			// Context was canceled.
			trace_network(context->hSession,"Hostname resolution to %s:%s was cancelled\n",context->hostname,context->service);
		}
		context->thread = NULL;
		if(context->finalized) {
			// Context was finalized.
			lib3270_free(context);
		}
		return 0;

	}

	// Got response.
	SOCKET sock = INVALID_SOCKET;
	struct addrinfo *ptr = NULL;
	int error = 0;

	for(ptr=result; ptr != NULL && sock == INVALID_SOCKET && context->enabled; ptr=ptr->ai_next) {

		sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if(sock == INVALID_SOCKET) {
			continue;
		}

		char host[NI_MAXHOST];
		if (getnameinfo(ptr->ai_addr, ptr->ai_addrlen, host, sizeof(host), NULL, 0, NI_NUMERICHOST) == 0) {
			trace_network(
				context->hSession,
				"Trying %s for '%s:%s'\n",
				host,
				context->hostname,
				context->service
			);
		} else {
			error = WSAGetLastError();
			trace_network(context->hSession,"Failed to get connection state for socket\n");
			closesocket(sock);
			sock = INVALID_SOCKET;	
			continue;
		}

		// Got socket, set it to non blocking.
		WSASetLastError(0);
		u_long iMode= 0;			
		if(ioctlsocket(sock,FIONBIO,&iMode)) {
			// Failed to set non-blocking mode.
			error = WSAGetLastError();
			trace_network(context->hSession,"Failed to set non-blocking mode on socket\n");
			closesocket(sock);
			sock = INVALID_SOCKET;
			continue;
		}

		if(connect(sock, ptr->ai_addr, ptr->ai_addrlen)) {
			// Connection established.
			break;
		}

		error = WSAGetLastError();
		if(error != WSAEINPROGRESS) {
			trace_network(context->hSession,"Failed to connect socket to %s\n",host);
			closesocket(sock);
			sock = INVALID_SOCKET;
			continue;
		}

		error = 0; // Reset error for next address.

	}

	// Release results.
	freeaddrinfo(result);

	if(!(context->enabled && context->hSession && context->hSession->hwnd)) {
		// Context was canceled.
		trace_network(context->hSession,"Hostname resolution to %s:%s was cancelled\n",context->hostname,context->service);
	} else if(sock == INVALID_SOCKET) {
		PostMessage(context->hSession->hwnd,WM_RESOLV_FAILED,error ? error : WSAECONNREFUSED,0);
	} else {
		PostMessage(context->hSession->hwnd,WM_RESOLV_SUCCESS,0,socket);
	}

	debug("%s: Resolver thread finished",__FUNCTION__);
	context->thread = NULL;

	if(context->finalized) {
		lib3270_free(context);
	}

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
	context->enabled = 0;
	context->hSession = hSession;
	context->timer = hSession->timer.add(hSession,timeout*1000,(void *) timeout,context);
	context->thread = CreateThread(NULL,0,resolver_thread,context,0,NULL);

	return (LIB3270_NET_CONTEXT *) context;
 }
