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
 #include <private/mainloop.h>
 #include <private/session.h>
 #include <private/intl.h>
 
 #include <winsock2.h>
 #include <windows.h>

 static size_t instances = 0;
 static ATOM identifier;	

 static LRESULT WINAPI hwndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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

 LRESULT WINAPI hwndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	H3270 *hSession = (H3270 *) GetWindowLongPtr(hWnd,0);

	/*
	switch(uMsg) {


	}
	*/

	return DefWindowProc(hWnd, uMsg, wParam, lParam);

 }

