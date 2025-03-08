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
 #include <lib3270/win32.h>
 #include <private/mainloop.h>
 #include <private/session.h>
 #include <private/intl.h>
 #include <private/popup.h>
 #include <private/win32_poll.h>
 
 #include <winsock2.h>
 #include <windows.h>
 #include <wininet.h>

 static size_t instances = 0;
 static LPARAM timer_id = 0;
 static ATOM identifier;	

 static LRESULT WINAPI hwndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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

	debug("%s: Posting WM_ADD_TIMER",__FUNCTION__);

	if(!PostMessage(hSession->hwnd,WM_ADD_TIMER,0,(LPARAM) t_new)) {
		lib3270_autoptr(char) windows_error = lib3270_win32_strerror(GetLastError());
		lib3270_log_write(hSession,"win32","Error adding timer: %s",windows_error);
	}

	return t_new;

 }

 static void win32_timer_remove(H3270 *hSession, void *timer) {
	if(!PostMessage(hSession->hwnd,WM_REMOVE_TIMER,0,(LPARAM) timer)) {
		lib3270_autoptr(char) windows_error = lib3270_win32_strerror(GetLastError());
		lib3270_log_write(hSession,"win32","Error removing timer: %s",windows_error);
	}
 }

 static void win32_timer_finalize(H3270 *session, LIB3270_TIMER_CONTEXT * context) {
	lib3270_linked_list_free((lib3270_linked_list *) context);
	lib3270_free(context);
 }

 static void win32_post(H3270 *hSession, void(*callback)(void *), void *parm, size_t parmlen) {
	PostData *pd = (PostData *) lib3270_malloc(sizeof(PostData)+parmlen+1);
	pd->callback = callback;
	memcpy((pd+1),parm,parmlen);
	if(!PostMessage(hSession->hwnd,WM_POST_CALLBACK,0,(LPARAM) pd)) {
		lib3270_autoptr(char) windows_error = lib3270_win32_strerror(GetLastError());
		lib3270_log_write(hSession,"win32","Error posting callback: %s",windows_error);
	}
 }

 LIB3270_EXPORT int lib3270_mainloop_run(H3270 *hSession, int wait) {

	// https://learn.microsoft.com/pt-br/windows/win32/api/winuser/nf-winuser-getmessage
	// https://learn.microsoft.com/pt-br/windows/win32/api/winuser/nf-winuser-peekmessagea

	MSG msg;
	BOOL bRet;
	
	if(wait) {
		bRet = GetMessage(&msg, NULL, 0, 0 );		
	} else {
		bRet = PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);	
	}

	if(bRet == -1) {
		return -GetLastError();
	}

	if(bRet) {
		TranslateMessage(&msg); 
		DispatchMessage(&msg); 
		return 1;
	}

	return 0;

 }

 typedef struct {
	int running;
	HANDLE thread;
	H3270 *hSession;
	int(*callback)(H3270 *, void *);
	void *parm;
 } ThreadParms;

 static DWORD __stdcall background_thread(LPVOID lpParam) {
	ThreadParms *tp = (ThreadParms *) lpParam;
	tp->callback(tp->hSession,tp->parm);
	return 0;
 }

 static int win32_run(H3270 *hSession, const char *name, int(*callback)(H3270 *, void *), void *parm) {

	ThreadParms tp;

	tp.hSession = hSession;
	tp.callback = callback;
	tp.parm = parm;

	tp.thread = CreateThread(NULL,0,background_thread,&tp,0,NULL);

	DWORD rc = STILL_ACTIVE;
	while(rc == STILL_ACTIVE) {
		if(lib3270_mainloop_run(hSession,1) < 0) {
			Sleep(100);
		}
		GetExitCodeThread(tp.thread,&rc);
	}

	CloseHandle(tp.thread);

	return rc;

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

	hSession->post = (void *) win32_post;
	hSession->run = (void *) win32_run;

	win32_poll_init(hSession);

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
			debug("Got timer %d first=%p",(int) wParam, hSession->timer.context->first);

			timeout_t *t;
			for (t = (timeout_t *) hSession->timer.context->first; t != NULL; t = (timeout_t *) t->next) {
				debug("t-id=%d wParam=%d",t->id,wParam);
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

			debug("Adding timer %lu with %lu ms",t->id,t->interval_ms);

			if(!hSession->timer.context->first) {
				hSession->timer.context->first = (struct lib3270_linked_list_node *) t;
			} else {
				t->prev = hSession->timer.context->last;
			}

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

	case WM_POPUP_MESSAGE:
		{
			LIB3270_POPUP popup = *((const LIB3270_POPUP *) lParam);

			popup.label = dgettext(GETTEXT_PACKAGE,((const LIB3270_POPUP *) lParam)->label);
			popup.summary = dgettext(GETTEXT_PACKAGE,((const LIB3270_POPUP *) lParam)->summary);
			popup.title = dgettext(GETTEXT_PACKAGE,((const LIB3270_POPUP *) lParam)->title);
			popup.body = dgettext(GETTEXT_PACKAGE,((const LIB3270_POPUP *) lParam)->body);

			hSession->cbk.popup(hSession,&popup,0);
			
		}
		return 0;

	case WM_POPUP_WSA_ERROR:
	case WM_POPUP_LAST_ERROR:
		{
			LIB3270_POPUP popup = *((const LIB3270_POPUP *) lParam);
			lib3270_autoptr(char) body = lib3270_win32_strerror((int) wParam);
			lib3270_autoptr(char) name = lib3270_strdup_printf("%s-%d",((const LIB3270_POPUP *) lParam)->name,(int) wParam);
				
			popup.name = name;
			popup.body = body;

			popup.label = dgettext(GETTEXT_PACKAGE,((const LIB3270_POPUP *) lParam)->label);
			popup.summary = dgettext(GETTEXT_PACKAGE,((const LIB3270_POPUP *) lParam)->summary);
			popup.title = dgettext(GETTEXT_PACKAGE,((const LIB3270_POPUP *) lParam)->title);

			hSession->cbk.popup(hSession,&popup,0);

		}
		return 0;

	case WM_CONNECTION_FAILED:
		{
			debug("%s: WM_CONNECTION_FAILED",__FUNCTION__);
			
			lib3270_autoptr(char) summary = lib3270_strdup_printf(
				_( "Failed to establish connection to %s" ),lib3270_get_url(hSession)
			);

			lib3270_autoptr(char) body = lib3270_win32_strerror((int) wParam);
			lib3270_autoptr(char) name = lib3270_strdup_printf("connection-%d",(int) wParam);

			LIB3270_POPUP popup = {
				.name		= name,
				.type		= LIB3270_NOTIFY_CONNECTION_ERROR,
				.title		= _("Network connection failed"),
				.summary	= summary,
				.body		= body,
				.label		= _("OK")
			};

			hSession->cbk.popup(hSession,&popup,0);

		}
		return 0;

	case WM_RESOLV_FAILED:
		{
			debug("%s: WM_RESOLV_FAILED",__FUNCTION__);
			
			lib3270_autoptr(char) summary = lib3270_strdup_printf(
				_( "Failed to establish connection to %s"),lib3270_get_url(hSession)
			);

			lib3270_autoptr(char) body = lib3270_win32_strerror((int) wParam);
			lib3270_autoptr(char) name = lib3270_strdup_printf("resolv-%d",(int) wParam);

			LIB3270_POPUP popup = {
				.name		= name,
				.type		= LIB3270_NOTIFY_CONNECTION_ERROR,
				.title		= _("Name resolution failed"),
				.summary	= summary,
				.body		= body,
				.label		= _("OK")
			};

			connection_close(hSession,(int) wParam);
			hSession->cbk.popup(hSession,&popup,0);

		}
		return 0;

	case WM_RECV_FAILED:
		{
			lib3270_autoptr(char) body = lib3270_win32_strerror((int) wParam);
			lib3270_autoptr(char) name = lib3270_strdup_printf("recv-%d",(int) wParam);

			LIB3270_POPUP popup = {
				.name		= name,
				.type		= LIB3270_NOTIFY_NETWORK_ERROR,
				.title		= _("Network I/O error"),
				.summary	= _("Failed to receive data from the host"),
				.body		= body,
				.label		= _("OK")
			};
	
			hSession->cbk.popup(hSession,&popup,0);
			
		}
		return 0;		

	case WM_SEND_FAILED:
		{
			lib3270_autoptr(char) body = lib3270_win32_strerror((int) wParam);
			lib3270_autoptr(char) name = lib3270_strdup_printf("send-%d",(int) wParam);

			LIB3270_POPUP popup = {
				.name		= name,
				.type		= LIB3270_NOTIFY_NETWORK_ERROR,
				.title		= _("Network I/O error"),
				.summary	= _("Failed to send data to the host"),
				.body		= body,
				.label		= _("OK")
			};
			hSession->cbk.popup(hSession,&popup,0);
			
		}
		return 0;		

	case WM_RESOLV_SUCCESS:
		debug("%s: WM_RESOLV_SUCCESS socket=%llu",__FUNCTION__,(SOCKET) lParam);
		set_resolved(hSession,(SOCKET) lParam);
		return 0;

	case WM_CONNECTION_SUCCESS:
		debug("%s: WM_CONNECTION_SUCCESS socket=%llu",__FUNCTION__,(SOCKET) lParam);
		set_connected_socket(hSession,(SOCKET) lParam);
		return 0;

	case WM_POST_CALLBACK:
		{
			PostData *pd = (PostData *) lParam;
			pd->callback((void *) (pd+1));
			lib3270_free(pd);
		}
		return 0;

	case WM_CLOSE_THREAD:
		{
			debug("%s:","WM_CLOSE_THREAD");

			HANDLE thread = (HANDLE) lParam;
			debug("Waiting for thread %p",thread);	
			WaitForSingleObject(thread,INFINITE);
			debug("Closing thread %p",thread);	
			CloseHandle(thread);
		}
		return 0;

	case WM_SOCKET_EVENT:
		{
			handler_t *handler = (handler_t *) lParam;
			handler->proc(handler->hSession,handler->sock,handler->userdata);
			handler->disabled = 0;
			win32_poll_wake_up();
		}
		return 0;

	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);

 }

