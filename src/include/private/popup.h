/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2008 Banco do Brasil S.A.
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
#include <lib3270/popup.h>

#ifdef _WIN32
	#include <private/mainloop.h>
	#include <private/session.h>
#endif // _WIN32

#define popup_an_errno(hSession, errn, fmt, ...) lib3270_popup_an_errno(hSession, errn, fmt, __VA_ARGS__)

LIB3270_INTERNAL void popup_an_error(H3270 *session, const char *fmt, ...) LIB3270_GNUC_FORMAT(2,3);
LIB3270_INTERNAL void popup_system_error(H3270 *session, const char *title, const char *message, const char *fmt, ...) LIB3270_GNUC_FORMAT(4,5);
LIB3270_INTERNAL void popup_a_sockerr(H3270 *session, char *fmt, ...) LIB3270_GNUC_FORMAT(2,3);

LIB3270_INTERNAL void Error(H3270 *session, const char *fmt, ...);
LIB3270_INTERNAL void Warning(H3270 *session, const char *fmt, ...);

#ifdef _WIN32

	/// @brief Popup message from thread.
	/// @param hSession TN3270 session handle.
	/// @param code Error code form WSAGetLastError().
	/// @param popup Popup description.
	inline void popup_message(H3270 *hSession, const LIB3270_POPUP *popup) {
		PostMessage(hSession->hwnd,WM_POPUP_MESSAGE, 0, (LPARAM) popup);
	}

	/// @brief Popup a Windows error.
	/// @param hSession TN3270 session handle.
	/// @param code Error code form WSAGetLastError().
	/// @param popup Popup description.
	inline void popup_last_error(H3270 *hSession, int code, const LIB3270_POPUP *popup) {
		PostMessage(hSession->hwnd,WM_POPUP_LAST_ERROR,(WPARAM) code, (LPARAM) popup);
	}

	/// @brief Popup a WSA error.
	/// @param hSession TN3270 session handle.
	/// @param code Error code form WSAGetLastError().
	/// @param popup Popup description.
	void popup_wsa_error(H3270 *hSession, int code, const LIB3270_POPUP *popup);
#endif
