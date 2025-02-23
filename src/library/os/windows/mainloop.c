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

 //
 // References:
 //
 // https://learn.microsoft.com/en-us/previous-versions/windows/desktop/legacy/ms742267(v=vs.85)
 //


 #include <config.h>
 #include <lib3270/defs.h>
 #include <lib3270/trace.h>
 #include <lib3270/memory.h>
 #include <lib3270/log.h>
 #include <private/mainloop.h>
 #include <private/session.h>
 #include <private/intl.h>
 
 #include <winsock2.h>
 #include <windows.h>

 #define MILLION 1000000L

 static size_t instances = 0;
 static ATOM identifier;	

 #define IDT_CHECK_TIMERS 1

 static LRESULT WINAPI hwndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

 struct _lib3270_poll_context {
	LIB3270_LINKED_LIST
 };

 typedef struct timeout {
	LIB3270_LINKED_LIST_HEAD

	/// @brief The timestamp, in getCurrentTime() units, for next activation.
	unsigned long activation_time;

	/// @brief The timer callback.
	int (*call)(H3270 *, void *);

} timeout_t;

 struct _lib3270_timer_context {
	unsigned long interval;
	lib3270_linked_list timers;
 };

 static unsigned long getCurrentTime() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000) + (tv.tv_usec /1000);
}

 static void * win32_timer_add(H3270 *hSession, unsigned long interval_ms, int (*proc)(H3270 *session, void *userdata), void *userdata) {

	timeout_t *t_new = lib3270_new(timeout_t);

	t_new->call = proc;
	t_new->activation_time = getCurrentTime() + interval_ms;
	t_new->userdata = userdata;

	PostMessage(hSession->hwnd,WM_ADD_TIMER,0,(LPARAM) t_new);

	return t_new;

 }

 static void win32_timer_remove(H3270 *hSession, void *timer) {
	PostMessage(hSession->hwnd,WM_REMOVE_TIMER,0,(LPARAM) timer);
 }

 LIB3270_INTERNAL void win32_mainloop_new(H3270 *hSession) {

	if(instances++ == 0) {

		debug("%s","Initializing win32 mainloop");

		WNDCLASSEX wc;

		memset(&wc,0,sizeof(wc));

		wc.cbSize 			= sizeof(wc);
		wc.style  			= CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc  	= hwndProc;
		wc.hInstance  		= GetModuleHandle(NULL);
		wc.lpszClassName  	= PACKAGE_NAME;
		wc.cbWndExtra		= sizeof(LONG_PTR);

		identifier = RegisterClassEx(&wc);

	}

	// Create object window.
	hSession->hwnd = CreateWindow(
		PACKAGE_NAME,
		"MainLoop",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		0,
		CW_USEDEFAULT,
		0,
		NULL,
		NULL,
		GetModuleHandle(NULL),
		NULL
	);

	SetWindowLongPtr(hSession->hwnd, 0, (LONG_PTR) hSession);

	// Setup callbacks
	hSession->timer.add = win32_timer_add;
 	hSession->timer.remove = win32_timer_remove;
	hSession->timer.context = lib3270_new(struct _lib3270_timer_context);
	// hSession->timer.finalize = win32_timer_finalize;
	memset(hSession->timer.context,0,sizeof(struct _lib3270_timer_context));
	hSession->timer.context->interval = 100;

	/*
 	hSession->poll.add = win32_poll_add;
 	hSession->poll.remove = win32_poll_remove;
	hSession->poll.context = lib3270_new(struct _lib3270_poll_context);
	hSession->poll.finalize = win32_poll_finalize;
	memset(hSession->poll.context,0,sizeof(struct _lib3270_poll_context));

 	hSession->event_dispatcher = win32_event_dispatcher;
 	hSession->run = win32_run;
	hSession->post = win32_post;

 	hSession->wait = win32_wait;
	*/


 }

 LIB3270_INTERNAL void win32_mainloop_free(H3270 *hSession) {

	if(hSession->hwnd) {
		DestroyWindow(hSession->hwnd);
		hSession->hwnd = NULL;
	}

	if(--instances == 0) {
		debug("%s","Finalizing win32 mainloop");
		UnregisterClass(PACKAGE_NAME,GetModuleHandle(NULL));
	}

 }

 LRESULT WINAPI hwndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	H3270 *hSession = (H3270 *) GetWindowLongPtr(hwnd,0);

	switch(uMsg) {
	case WM_CREATE:
		if(hSession->timer.context->interval) {
			SetTimer(hwnd,IDT_CHECK_TIMERS,hSession->timer.context->interval,(TIMERPROC) NULL);
		}
		hSession->hwnd = hwnd;
		break;
	
	case WM_DESTROY:
		if(hSession->timer.context->interval) {
			KillTimer(hwnd, IDT_CHECK_TIMERS);
		}
		hSession->hwnd = NULL;
		break;

	case WM_TIMER:
		if(wParam == IDT_CHECK_TIMERS) {
			PostMessage(hwnd,WM_CHECK_TIMERS,0,0);
		}
		return 0;

	case WM_CHECK_TIMERS:
		{
			debug("%s","Checking timers");
			unsigned long now = getCurrentTime();
			unsigned long wait = 0;

			timeout_t *t;
			for (t = (timeout_t *) hSession->timer.context->timers.first; t != NULL; t = (timeout_t *) t->next) {
				if (t->activation_time <= now) {
					t->call(hSession,t->userdata);
				} else {
					wait = t->activation_time - now;
					break;
				}
			}

			if(!wait) {

				// Empty list.
				KillTimer(hwnd,IDT_CHECK_TIMERS);
				hSession->timer.context->interval = 0;

			} if(!hSession->timer.context->interval) {

				// New list.
				SetTimer(hwnd,IDT_CHECK_TIMERS,wait,(TIMERPROC) NULL);
				hSession->timer.context->interval = wait;

			} else if(wait != hSession->timer.context->interval) {

				// Update list.
				SetTimer(hwnd,IDT_CHECK_TIMERS,wait,(TIMERPROC) NULL);
				hSession->timer.context->interval = wait;

			}

		}
		return 0;

	case WM_ADD_TIMER:
		{
			timeout_t *t_new = (timeout_t *) lParam;
			timeout_t *t = NULL;
			timeout_t *prev = NULL;

			// Find where to insert this item.
			for (t = (timeout_t *) hSession->timer.context->timers.first; t != NULL; t = (timeout_t *) t->next) {
				if (t->activation_time > t_new->activation_time)
					break;
				prev = t;
			}

			// Insert it.
			if (prev == NULL) {
				// t_new is Front.
				t_new->next = hSession->timer.context->timers.first;
				hSession->timer.context->timers.first = (struct lib3270_linked_list_node *) t_new;
			} else if (t == NULL) {
				// t_new is Rear.
				t_new->next = NULL;
				prev->next = (struct lib3270_linked_list_node *) t_new;
				hSession->timer.context->timers.last = (struct lib3270_linked_list_node *) t_new;
			} else {
				// t_new is Middle.
				t_new->next = (struct lib3270_linked_list_node *) t;
				prev->next = (struct lib3270_linked_list_node *) t_new;
			}

			SendMessage(hwnd,WM_CHECK_TIMERS,0,0);

		}
		return 0;

	case WM_REMOVE_TIMER:
		{	
			timeout_t *timer = (timeout_t *) lParam;
			if(timer) {
				lib3270_linked_list_delete_node( 
					(lib3270_linked_list *) &hSession->timer.context->timers,
					timer
				);
			}			
			SendMessage(hwnd,WM_CHECK_TIMERS,0,0);
		}
		return 0;

	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);

 }

