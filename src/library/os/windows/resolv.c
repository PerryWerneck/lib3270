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
 #include <lib3270/win32.h>

 typedef struct {

	LIB3270_NET_CONTEXT parent;

	int enabled;
	int finalized;
	int running;

	H3270 *hSession;
	HANDLE thread;

	void *timer;

	char *hostname;
	char *service;

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

 static void net_timeout(H3270 *hSession, Context *context) {

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

	debug("%s: Finalizing resolver context %p and thread %p",__FUNCTION__,context,context->thread);
	
	if(context->timer) {
		hSession->timer.remove(hSession,context->timer);
		context->timer = NULL;
	}

	if(context->running) {
		context->enabled = 0;
		context->finalized = 1;
		trace_network(hSession,"Resolver thread still active, delaying context finalization\n");
	} else {
		debug("%s: Cleaning resolver context %p",__FUNCTION__,context);
		lib3270_free(context);
	}

 }

 static DWORD __stdcall resolver_thread(LPVOID lpParam) {

	Context *context = (Context *) lpParam;
	HWND hwnd = context->hSession->hwnd;
	HANDLE thread = context->thread;

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
		debug("Name resolution failed with error %d",rc);
		
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

	debug("%s: Processing responses enabled=%d result=%p",__FUNCTION__,context->enabled,result);

	for(ptr=result; ptr != NULL && context->enabled; ptr=ptr->ai_next) {

		sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if(sock == INVALID_SOCKET) {
			lib3270_autoptr(char) error = lib3270_win32_strerror(WSAGetLastError());
			debug("---> %s",error);
			trace_network(
				context->hSession,
				"Cant get socket for '%s:%s': %s\n",
				context->hostname,
				context->service,
				error
			);
			continue;
		}

		debug("-----------------------> %s: Got socket",__FUNCTION__);

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

		debug("%s: Connecting",__FUNCTION__);
		if(connect(sock, ptr->ai_addr, ptr->ai_addrlen) == 0) {
			// Connection established.
			trace_network(context->hSession,"Connected to %s\n",host);
			debug("%s: Connected!",__FUNCTION__);
			break;
		}

		error = WSAGetLastError();
		if(error == WSAEINPROGRESS) {
			debug("Connected to %s:%s",context->hostname,context->service);
			break;
		}

		{
			lib3270_autoptr(char) message = lib3270_win32_strerror(error);
			trace_network(context->hSession,"Failed to connect socket to %s: %s\n",host,message);
			closesocket(sock);
			sock = INVALID_SOCKET;
		}
		error = 0; // Reset error for next address.

	}

	// Release results.
	freeaddrinfo(result);

	if(!(context->enabled && context->hSession && context->hSession->hwnd)) {
		// Context was canceled.
		trace_network(context->hSession,"Hostname resolution to %s:%s was cancelled\n",context->hostname,context->service);
	} else if(sock == INVALID_SOCKET) {
		PostMessage(hwnd,WM_RESOLV_FAILED,error ? error : WSAECONNREFUSED,0);
	} else {
		PostMessage(hwnd,WM_RESOLV_SUCCESS,0,(LPARAM) socket);
	}

	debug("%s: Resolver thread finished",__FUNCTION__);

	context->running = 0;
	if(context->finalized) {
		lib3270_free(context);
	}

	PostMessage(hwnd,WM_CLOSE_THREAD,0,(LPARAM) thread);
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
	context->running = 1;
	context->hSession = hSession;
	context->timer = hSession->timer.add(hSession,timeout*1000,(void *) net_timeout,context);
	context->thread = CreateThread(NULL,0,resolver_thread,context,0,NULL);

	debug("Started resolver thread %p for %s:%s",context->thread,hostname,service);

	return (LIB3270_NET_CONTEXT *) context;
 }
