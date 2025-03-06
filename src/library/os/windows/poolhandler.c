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
 #include <lib3270/defs.h>
 #include <lib3270/trace.h>
 #include <lib3270/memory.h>
 #include <private/linkedlist.h>
 #include <private/win32_poll.h>
 #include <private/mainloop.h>
 
 #include <winsock2.h>
 #include <windows.h>
 #include <wininet.h>
 #include <assert.h>

 static size_t instances	= 0;
 static HANDLE mutex		= 0;
 static HANDLE thread		= 0;
 static WSAEVENT event		= 0;

 static struct {
	LIB3270_LINKED_LIST	
 } handlers;

 LIB3270_INTERNAL LIB3270_POLL_CONTEXT * win32_poll_init(H3270 *hSession) {

	if(!mutex) {
		mutex = CreateMutex(NULL,FALSE,NULL);
	}

	assert(WaitForSingleObject( mutex, INFINITE ) == WAIT_OBJECT_0);

	if(!event) {
		event = WSACreateEvent();
	}

	LIB3270_POLL_CONTEXT *context = lib3270_new(LIB3270_POLL_CONTEXT);
	memset(context,0,sizeof(LIB3270_POLL_CONTEXT));
	instances++;


	ReleaseMutex(mutex);
	return context;

 }

 LIB3270_INTERNAL void win32_poll_finalize(H3270 *hSession, LIB3270_POLL_CONTEXT * context) {

	lib3270_free(context);

	// Remove all session handlers.
	handler_t *handler;

	// Remove session handlers.
	assert(WaitForSingleObject( mutex, INFINITE ) == WAIT_OBJECT_0);
	for(handler = (handler_t *) handlers.first;handler;handler = (handler_t *) handler->next) {
		if(handler->hSession == hSession && handler->sock != INVALID_SOCKET) {
			lib3270_log_write(handler->hSession,"win32","Cleaning lost socket %llu",handler->sock);
			closesocket(handler->sock);
			handler->sock = INVALID_SOCKET;
		}
	}	
	ReleaseMutex(mutex);

	// Check for remaining instances
	if(--instances == 0) {

		// No more instances, cleanup
		assert(WaitForSingleObject( mutex, INFINITE ) == WAIT_OBJECT_0);
		for(handler = (handler_t *) handlers.first;handler;handler = (handler_t *) handler->next) {
			if(handler->sock != INVALID_SOCKET) {
				lib3270_log_write(handler->hSession,"win32","Cleaning orphaned socket %llu",handler->sock);
				closesocket(handler->sock);
				handler->sock = INVALID_SOCKET;
			}
		}	
		ReleaseMutex(mutex);
	
		if(thread) {
			WSASetEvent(event);	// Force the thread to wake up
			lib3270_log_write(NULL,"win32","Waiting for network thread to close");
			WaitForSingleObject(thread,INFINITE);
			CloseHandle(thread);			
			thread = 0;
		}

		WSACloseEvent(event);
		CloseHandle(mutex);

		event = 0;
		mutex = 0;
		return;
	}

 }

 static DWORD __stdcall win32_poll_thread(LPVOID lpParam) {

	size_t buflen = 1;
	HANDLE *events = lib3270_malloc((buflen+1) * sizeof(HANDLE));
	handler_t **workers = lib3270_malloc((buflen+1) * sizeof(handler_t *));

	while(handlers.first) {
	
		ULONG cEvents = 0;

		// Load FDs
		{
			assert(WaitForSingleObject( mutex, INFINITE ) == WAIT_OBJECT_0);
			handler_t *handler = (handler_t *) handlers.first;
			while(handler) {

				if(handler->sock == INVALID_SOCKET) {
					// Remove handler
					WSACloseEvent(handler->event);
					handler_t *next = (handler_t *) handler->next;
					lib3270_linked_list_delete_node((lib3270_linked_list *) &handlers,handler);
					handler = next;
					continue;
				}

				if(handler->disabled) {
					handler = (handler_t *) handler->next;
					continue;
				}

				if(cEvents >= buflen) {
					buflen++;
					events = lib3270_realloc(events,(buflen+1) * sizeof(HANDLE));
					workers = lib3270_realloc(workers,(buflen+1) * sizeof(handler_t *));
				}

				events[cEvents] = handler->event;
				workers[cEvents] = handler;

				cEvents++;
				handler = (handler_t *) handler->next;

			}
			WSAResetEvent(event);	// Clear global event.
			ReleaseMutex(mutex);
		}

		// Store the global event.
		events[cEvents] = event;
		workers[cEvents] = NULL;
		cEvents++;

		// Wait for events
		// https://stackoverflow.com/questions/41743043/windows-wait-on-event-and-socket-simulatenously
		WSAWaitForMultipleEvents(cEvents,events,FALSE,INFINITE,FALSE);

		ULONG event;
		for(event = 0; event < cEvents; event++) {
			WSANETWORKEVENTS networkEvents;
			if(WSAEnumNetworkEvents(workers[event]->sock,events[event],&networkEvents) == 0) {
				if(workers[event] && workers[event]->hSession && workers[event]->hSession->hwnd) {
					workers[event]->disabled = 1;
					PostMessage(
						workers[event]->hSession->hwnd,
						WM_SOCKET_EVENT,
						(WPARAM) networkEvents.lNetworkEvents,
						(LPARAM) workers[event]
					);
				}
			}
		}
		
	}

	lib3270_free(workers);
	lib3270_free(events);

	// Thread cleanup
	{
		assert(WaitForSingleObject( mutex, INFINITE ) == WAIT_OBJECT_0);
		HANDLE hThread = thread;
		thread = 0;
		ReleaseMutex(mutex);
		CloseHandle(hThread);		
	}

	return 0;

 }

 LIB3270_INTERNAL void win32_poll_wake_up(H3270 *) {
	assert(WaitForSingleObject( mutex, INFINITE ) == WAIT_OBJECT_0);
	if(event) {
		WSASetEvent(event);	// Force the thread to wake up
	}
	ReleaseMutex(mutex);
 }

 LIB3270_INTERNAL void * win32_poll_add(H3270 *hSession, SOCKET sock, LIB3270_IO_FLAG flag, void(*proc)(H3270 *, SOCKET, LIB3270_IO_FLAG, void *), void *userdata ) {

	// TODO: if flag == FD_READ start a wsaasyncrecv instead of thread, same for FD_WRITE. 

	// Add socket to the list
	assert(WaitForSingleObject( mutex, INFINITE ) == WAIT_OBJECT_0);

	handler_t *handler = 
		(handler_t *) lib3270_linked_list_append_node(
			(lib3270_linked_list *) &handlers,
			sizeof(handler_t),
			userdata
		);

	handler->hSession = hSession;
	handler->sock = sock;
	handler->proc = proc;
	handler->flag = flag;
	handler->userdata = userdata;
		
	long events = 0;

	if(flag & LIB3270_IO_FLAG_READ) events |= FD_READ;
	if(flag & LIB3270_IO_FLAG_WRITE) events |= FD_WRITE;	
	if(flag & LIB3270_IO_FLAG_EXCEPTION) events |= FD_READ;

	WSAEventSelect(sock, &handler->event, events);


	if(!thread) {
		// No thread, start one
		thread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE) win32_poll_thread,NULL,0,NULL);
	} else {
		// Has thread, wake it up
		WSASetEvent(event);	
	}

	ReleaseMutex(mutex);

	return (void *) handler;
 }

 LIB3270_INTERNAL void win32_poll_remove(H3270 *hSession, void *id) {
	assert(WaitForSingleObject( mutex, INFINITE ) == WAIT_OBJECT_0);
	((handler_t *) id)->sock = INVALID_SOCKET;
	WSASetEvent(event);	// Force the thread to wake up
	ReleaseMutex(mutex);
 }
