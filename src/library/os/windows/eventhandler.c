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
 #include <windows/lib3270/win32.h>
 #include <private/linkedlist.h>
 #include <private/win32_poll.h>
 #include <private/mainloop.h>
 #include <private/popup.h>
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

	debug("---> %s %p %llu %lu %p",__FUNCTION__,hSession,sock,events,userdata);
#ifdef DEBUG 
	if(events & FD_READ) {
		debug("%s: FD_READ",__FUNCTION__);
	}
	if(events & FD_WRITE) {
		debug("%s: FD_WRITE",__FUNCTION__);
	}
	if(events & FD_CONNECT) {
		debug("%s: FD_CONNECT",__FUNCTION__);
	}
#endif // DEBUG

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

	if(WSAEventSelect(sock, handler->event, events) == SOCKET_ERROR) {

		static const LIB3270_POPUP popup = {
			.name		= "network-error",
			.type		= LIB3270_NOTIFY_NETWORK_ERROR,
			.title		= N_("Unexpected network error"),
			.summary	= N_("Unable to watch socket, WSAEventSelect has failed."),
			.body		= "",
			.label		= N_("OK")
		};

		int err = WSAGetLastError(); 
		debug("%s: WSAEventSelect failed with error %d (%s)",__FUNCTION__,err,popup.summary);

		popup_wsa_error(hSession,err,&popup);
	
		WSACloseEvent(handler->event);
		lib3270_linked_list_delete_node((lib3270_linked_list *) &controller,handler);
		handler = NULL;

	} else if(controller.thread) {

		debug("%s: Waking up network thread %llu",__FUNCTION__,controller.thread);
		WSASetEvent(controller.event);	// Force the thread to wake up

	} else {

		controller.thread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE) win32_poll_thread,NULL,0,NULL);
		debug("%s: Thread %llu started",__FUNCTION__,controller.thread);

	}

	ReleaseMutex(controller.mutex);

	return handler;

 }

 LIB3270_INTERNAL void win32_poll_remove(void *handler) {

	assert(WaitForSingleObject(controller.mutex, INFINITE ) == WAIT_OBJECT_0);
	
	debug(
		"---> %s %p %llu %lu %p",
			__FUNCTION__,
			((handler_t *) handler)->hSession,
			((handler_t *) handler)->sock,
			((handler_t *) handler)->events,
			((handler_t *) handler)->userdata
	);

	((handler_t *) handler)->sock = INVALID_SOCKET;
	
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
		WSASetEvent(controller.event);	// Force the thread to wake up
		debug("%s: Waiting for network thread",__FUNCTION__);
		WaitForSingleObject(controller.thread,INFINITE);
		debug("%s: Closing network thread",__FUNCTION__);
		CloseHandle(controller.thread);			
		controller.thread = 0;
	}

}

 static DWORD __stdcall win32_poll_thread(LPVOID lpParam) {

	debug("------------------> Network thread %s has started",__FUNCTION__);

	size_t buflen = 1;
	HANDLE *events = lib3270_malloc((buflen+1) * sizeof(HANDLE));
	handler_t **workers = lib3270_malloc((buflen+1) * sizeof(handler_t *));

	while(win32_poll_enabled()) {

		debug("Network thread %s is waiting for events",__FUNCTION__);
		
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

				if(cEvents >= buflen) {
					buflen++;
					events = lib3270_realloc(events,(buflen+1) * sizeof(HANDLE));
					workers = lib3270_realloc(workers,(buflen+1) * sizeof(handler_t *));
				}

				events[cEvents] = handler->event;
				workers[cEvents] = handler;

				debug("Event %d: handler=%p sock=%llu event=%llu",cEvents,handler,handler->sock,events[cEvents]);

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

		debug("Network thread %s is waiting for %d events",__FUNCTION__,(int) cEvents);

		// Wait for events
		// https://stackoverflow.com/questions/41743043/windows-wait-on-event-and-socket-simulatenously
		int rc = WSAWaitForMultipleEvents(cEvents,events,FALSE,INFINITE,FALSE);
		
		debug("------------------> %s rc=%d",__FUNCTION__,rc);

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

					debug("---------------> Event on socket %llu: %lu",workers[event]->sock,networkEvents.lNetworkEvents);

					if(workers[event]->hSession && workers[event]->hSession->hwnd) {
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

	debug("------------------> Network thread %s has stopped",__FUNCTION__);

	return 0;
 }
