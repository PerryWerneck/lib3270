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
 #include <private/intl.h>
 
 #include <winsock2.h>
 #include <windows.h>
 #include <wininet.h>
 #include <assert.h>

 static DWORD __stdcall win32_poll_thread(LPVOID lpParam);

 static struct {
	LIB3270_LINKED_LIST	
	HANDLE mutex;
	HANDLE thread;
	WSAEVENT event;
 } controller = {
	.first = NULL,
	.last = NULL,
	.mutex = 0,
	.event = 0
 };

 LIB3270_INTERNAL void win32_poll_init(H3270 *hSession) {

	if(!controller.mutex) {
		controller.mutex = CreateMutex(NULL,FALSE,NULL);
	}

	assert(WaitForSingleObject( controller.mutex, INFINITE ) == WAIT_OBJECT_0);

	if(!controller.event) {
		controller.event = WSACreateEvent();
	}

	ReleaseMutex(controller.mutex);

 }

 LIB3270_INTERNAL void win32_poll_finalize(H3270 *hSession) {

	assert(WaitForSingleObject( controller.mutex, INFINITE ) == WAIT_OBJECT_0);

	// TODO: Remove all session handlers from list.

	if(!controller.first) {

		// No more instances, cleanup	

		if(controller.thread) {
			WSASetEvent(controller.event);	// Force the thread to wake up
			lib3270_log_write(NULL,"win32","Waiting for network thread to close");
			WaitForSingleObject(controller.thread,INFINITE);
			CloseHandle(controller.thread);			
			controller.thread = 0;
		}

		WSACloseEvent(controller.event);
		controller.event = 0;
		return;
	}

	ReleaseMutex(controller.mutex);

 }

 LIB3270_INTERNAL int win32_poll_enabled() {
	assert(WaitForSingleObject(controller.mutex, INFINITE ) == WAIT_OBJECT_0);
	int rc = controller.first != NULL;
	ReleaseMutex(controller.mutex);
	return rc;
 }

 LIB3270_INTERNAL void win32_poll_wake_up() {
	if(controller.thread && controller.event) {
		WSASetEvent(controller.event);	// Force the thread to wake up
	}
 }


 LIB3270_INTERNAL void * win32_poll_add(H3270 *hSession, SOCKET sock, long events, void (*call)(H3270 *hSession, SOCKET sock, void *userdata), void *userdata) {

	assert(WaitForSingleObject(controller.mutex, INFINITE ) == WAIT_OBJECT_0);

	handler_t *handler;
	
	// Just in case.
	for(handler = (handler_t *) controller.first;handler;handler = (handler_t *) handler->next) {
		if(handler->sock == sock) {

			static const LIB3270_POPUP popup = {
				.name		= "internal",
				.type		= LIB3270_NOTIFY_ERROR,
				.title		= N_("Internal error"),
				.summary	= N_("Requested connection is already on the watch list."),
				.body		= "",
				.label		= N_("OK")
			};
	
			PostMessage(hSession->hwnd,WM_POPUP_MESSAGE,-1,(LPARAM) &popup);
			connection_close(hSession,-1);
	
			ReleaseMutex(controller.mutex);
			return NULL;
		}
	}

	handler =	
		(handler_t *) lib3270_linked_list_append_node(
			(lib3270_linked_list *) &controller, 
			sizeof(handler_t), 
			userdata
		);

	handler->hSession = hSession;
	handler->sock = sock;
	handler->events = events;
	handler->proc = call;
	handler->event = WSACreateEvent();

	WSAEventSelect(sock, &handler->event, events);

	if(controller.thread) {
		WSASetEvent(controller.event);	// Force the thread to wake up
	} else {
		controller.thread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE) win32_poll_thread,NULL,0,NULL);
	}
	ReleaseMutex(controller.mutex);

 }

 LIB3270_INTERNAL void * win32_poll_remove(void *handler) {

	assert(WaitForSingleObject(controller.mutex, INFINITE ) == WAIT_OBJECT_0);
	
	if(controller.thread) {

		WSASetEvent(controller.event);	// Force the thread to wake up

	} else { // Just in case!

		WSACloseEvent(((handler_t *) handler)->event);
		lib3270_linked_list_delete_node(
			(lib3270_linked_list *) &controller, 
			handler
		);
		
	}

	ReleaseMutex(controller.mutex);

	if(controller.thread && !win32_poll_enabled()) {
		debug("%s: Waiting for network thread",__FUNCTION__);
		WaitForSingleObject(controller.thread,INFINITE);
		CloseHandle(controller.thread);			
		controller.thread = 0;
	}

}

 static DWORD __stdcall win32_poll_thread(LPVOID lpParam) {

	size_t buflen = 1;
	HANDLE *events = lib3270_malloc((buflen+1) * sizeof(HANDLE));
	handler_t **workers = lib3270_malloc((buflen+1) * sizeof(handler_t *));

	while(win32_poll_enabled()) {

		ULONG cEvents = 0;

		// Load FDs
		{
			assert(WaitForSingleObject( controller.mutex, INFINITE ) == WAIT_OBJECT_0);
			handler_t *handler = (handler_t *) controller.first;
			while(handler) {

				if(handler->sock == INVALID_SOCKET) {
					// Remove handler
					debug("Destroying handler %p",handler);

					WSACloseEvent(handler->event);
					handler_t *next = (handler_t *) handler->next;
					lib3270_linked_list_delete_node((lib3270_linked_list *) &controller,handler);
					handler = next;
					continue;
				}

				if(handler->disabled) {
					debug("Handler %p is disabled",handler);
					handler = (handler_t *) handler->next;
					continue;
				}
#ifdef DEBUG
				else {
					debug("Handler %p is enabled on socket %llu",handler,handler->sock);
				}
#endif // DEBUG

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
			WSAResetEvent(controller.event);	// List was updated, reset signal for next change.
			ReleaseMutex(controller.mutex);
		}

		// Store the global event.
		events[cEvents] = controller.event;
		workers[cEvents] = NULL;
		cEvents++;

		// Wait for events
		// https://stackoverflow.com/questions/41743043/windows-wait-on-event-and-socket-simulatenously
		int rc = WSAWaitForMultipleEvents(cEvents,events,FALSE,INFINITE,FALSE);
		if(rc == WSA_WAIT_FAILED) {

			debug("%s -----> WSA_WAIT_FAILED",__FUNCTION__);
			lib3270_autoptr(char) message = lib3270_win32_strerror(WSAGetLastError());

			// TODO: Write to system event log.

			debug("%s: %s",__FUNCTION__,message);
			assert(WSAGetLastError());
		}

		{
			ULONG event;
			for(event = 0; event < cEvents; event++) {
	
				if(!workers[event] || workers[event]->sock == INVALID_SOCKET) {
					WSAResetEvent(events[event]);
					continue;
				}
	
				WSANETWORKEVENTS networkEvents;	
				if(WSAEnumNetworkEvents(workers[event]->sock,events[event],&networkEvents) == 0) {
					if(workers[event]->hSession && workers[event]->hSession->hwnd) {
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

	}

	lib3270_free(events);
	lib3270_free(workers);

	return 0;
 }
