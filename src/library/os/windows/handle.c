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

 /*
 #include <config.h>
 #include <winsock2.h>
 #include <windows.h>

 #include <lib3270/defs.h>
 #include <lib3270/trace.h>
 #include <lib3270/memory.h>
 #include <lib3270/log.h>
 #include <lib3270/win32.h>
 #include <private/session.h>
 #include <private/intl.h>
 #include <private/linkedlist.h>
 #include <private/popup.h>
 
 static HANDLE thread = NULL;
 static lib3270_linked_list handles = { NULL, NULL };
 
 typedef struct {
	LIB3270_LINKED_LIST_HEAD
 	HANDLE handle;
 	H3270 *hSession;
	void (*callback)(H3270 *, HANDLE, void *);
 } HandleData;
 
 static DWORD __stdcall controller(LPVOID lpParam) {
 
	while(handles.first) {

		DWORD wait = INFINITE;
		HANDLE objects[MAXIMUM_WAIT_OBJECTS];
		size_t count = 0;

		for(HandleData *hd = (HandleData *) handles.first; hd; hd = (HandleData *) hd->next) {
			objects[count++] = hd->handle;
			if(count == MAXIMUM_WAIT_OBJECTS) {
				break;
			}
		}

		DWORD result = WaitForMultipleObjects(count,objects,FALSE,wait);

		if(result == WAIT_FAILED) {
			lib3270_autoptr(char) error = lib3270_win32_strerror(GetLastError());
			lib3270_log_write(NULL,"win32","Error waiting for handles: %s",error);
			break;
		}

		if(result == WAIT_TIMEOUT) {
			continue;
		}

		for(HandleData *hd = (HandleData *) handles.first; hd; hd = (HandleData *) hd->next) {
			if(hd->handle == objects[result]) {
				hd->callback(hd->hSession,hd->handle,hd->userdata);
				break;
			}
		}

	}

	return 0;
 }

 LIB3270_EXPORT void lib3270_add_handle(H3270 *hSession, HANDLE handle, void (*callback)(H3270 *, HANDLE, void *), void *userdata) {

	HandleData *hd = (HandleData *) lib3270_linked_list_append_node(&handles,sizeof(HandleData),userdata);
	hd->handle = handle;
	hd->hSession = hSession;
	hd->callback = callback;

	if(!thread) {
		DWORD threadId;
		thread = CreateThread(NULL,0,controller,NULL,0,&threadId);
	}

 }

 LIB3270_EXPORT void lib3270_remove_handle(H3270 *hSession, HANDLE handle) {

	for(HandleData *hd = (HandleData *) handles.first; hd; hd = (HandleData *) hd->next) {
		if(hd->hSession == hSession && hd->handle == handle) {
			lib3270_linked_list_delete_node(&handles,hd);
			break;
		}
	}

	if(!handles.first && thread) {
		WaitForSingleObject(thread,INFINITE);
		CloseHandle(thread);
		thread = NULL;
	}

 } 
 */
