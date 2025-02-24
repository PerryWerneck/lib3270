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
 #include <private/popup.h>
 
 #include <winsock2.h>
 #include <windows.h>

 static size_t instances = 0;
 static LPARAM timer_id = 0;
 static ATOM identifier;	

 static LRESULT WINAPI hwndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

 struct _lib3270_poll_context {
	LIB3270_LINKED_LIST
 };

 typedef struct timeout {

	LIB3270_LINKED_LIST_HEAD

	LPARAM id;
	unsigned long interval_ms;

	/// @brief The timer callback.
	int (*call)(H3270 *, void *);

} timeout_t;

 struct _lib3270_timer_context {
	LIB3270_LINKED_LIST
 };

 typedef struct {
	void (*callback)(void *);
 } PostData;

 static void * win32_timer_add(H3270 *hSession, unsigned long interval_ms, int (*proc)(H3270 *session, void *userdata), void *userdata) {

	timeout_t *t_new = lib3270_new(timeout_t);

	t_new->call = proc;
	t_new->id = ++timer_id;
	t_new->interval_ms = interval_ms;
	t_new->userdata = userdata;

	PostMessage(hSession->hwnd,WM_ADD_TIMER,0,(LPARAM) t_new);

	return t_new;

 }

 static void win32_timer_remove(H3270 *hSession, void *timer) {
	PostMessage(hSession->hwnd,WM_REMOVE_TIMER,0,(LPARAM) timer);
 }

 static void win32_timer_finalize(H3270 *session, LIB3270_TIMER_CONTEXT * context) {
	lib3270_linked_list_free((lib3270_linked_list *) context);
	lib3270_free(context);
 }

 static void win32_post(H3270 *hSession, void(*callback)(void *), void *parm, size_t parmlen) {
	PostData *pd = (PostData *) lib3270_malloc(sizeof(PostData)+parmlen+1);
	pd->callback = callback;
	memcpy((pd+1),parm,parmlen);
	PostMessage(hSession->hwnd,WM_POST_CALLBACK,0,pd);
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
	hSession->timer.context = lib3270_new(struct _lib3270_timer_context);
	memset(hSession->timer.context,0,sizeof(struct _lib3270_timer_context));

	hSession->timer.add = win32_timer_add;
 	hSession->timer.remove = win32_timer_remove;
	hSession->timer.finalize = win32_timer_finalize;

	/*
	hSession->poll.context = lib3270_new(struct _lib3270_poll_context);
	memset(hSession->poll.context,0,sizeof(struct _lib3270_poll_context));

	hSession->poll.add = win32_poll_add;
 	hSession->poll.remove = win32_poll_remove;
	hSession->poll.finalize = win32_poll_finalize;
 	hSession->event_dispatcher = win32_event_dispatcher;
 	hSession->run = win32_run;

 	hSession->wait = win32_wait;
	*/

	hSession->post = win32_post;

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
		hSession->hwnd = hwnd;
		break;
	
	case WM_DESTROY:
		{
			timeout_t *t;
			for (t = (timeout_t *) hSession->timer.context->first; t != NULL; t = (timeout_t *) t->next) {
				KillTimer(hwnd, t->id);
			}
			hSession->hwnd = NULL;
		}
		break;

	case WM_TIMER:
		{
			timeout_t *t;
			for (t = (timeout_t *) hSession->timer.context->first; t != NULL; t = (timeout_t *) t->next) {
				if(t->id == wParam) {
					t->call(hSession,t->userdata);
					SendMessage(hwnd,WM_REMOVE_TIMER,0,(LPARAM) t);
					break;
				}
			}
		}
		return 0;

	case WM_ADD_TIMER:
		{
			timeout_t *t = (timeout_t *) lParam;

			t->prev = hSession->timer.context->last;
			t->next = NULL;
			hSession->timer.context->last = (struct lib3270_linked_list_node *) t;	
			
			SetTimer(hwnd,t->id,t->interval_ms,(TIMERPROC) NULL);

		}
		return 0;

	case WM_REMOVE_TIMER:
		{	
			timeout_t *timer = (timeout_t *) lParam;
			if(timer) {
				KillTimer(hwnd,timer->id);
				lib3270_linked_list_delete_node( 
					(lib3270_linked_list *) hSession->timer.context,
					timer
				);
			}			
			if(!hSession->timer.context->first) {
				timer_id = 0;
			}
		}
		return 0;

	case WM_RESOLV_FAILED:
		{
			debug("%s: WM_RESOLV_FAILED",__FUNCTION__);
			lib3270_autoptr(char) summary = lib3270_strdup_printf(
				_( "Can't connect to %s"),lib3270_get_url(hSession)
			);

			LIB3270_POPUP popup = {
				.name		= "dns-error",
				.type		= LIB3270_NOTIFY_CONNECTION_ERROR,
				.title		= _("DNS error"),
				.summary	= summary,
				.body		= "",
				.label		= _("OK")
			};

			connection_close(hSession,-1);
			popup_win32_error(hSession,wParam,&popup,0);

		}
		return 0;

	case WM_RESOLV_TIMEOUT:
		{
			LIB3270_POPUP popup = {
				.name		= "dns-timeout",
				.type		= LIB3270_NOTIFY_CONNECTION_ERROR,
				.title		= _("DNS error"),
				.summary	= _("Unable to resolve host name"),
				.body		= strerror(ETIMEDOUT),
				.label		= _("OK")
			};
	
			connection_close(hSession,ETIMEDOUT);
			lib3270_popup(hSession, &popup, 0);

		}
		return 0;

	case WM_RESOLV_SUCCESS:
		debug("%s: WM_RESOLV_SUCCESS",__FUNCTION__);
		set_resolved(hSession,(SOCKET) lParam);
		return 0;

	case WM_POST_CALLBACK:
		{
			PostData *pd = (PostData *) lParam;
			pd->callback((void *) (pd+1));
			lib3270_free(pd);
		}
		return 0;
		

	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);

 }

