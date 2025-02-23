/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2025 Perry Werneck <perry.werneck@gmail.com>
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

 #pragma once

 #include <config.h>
 #include <lib3270/defs.h>
 #include <lib3270.h>

#ifdef _WIN32

	#include <winsock2.h>
	#include <windows.h>

	enum MessageTypes {
		WM_ADD_TIMER				= WM_USER+102,
		WM_REMOVE_TIMER				= WM_USER+103,
	};

 	LIB3270_INTERNAL void win32_mainloop_new(H3270 *hSession);
	LIB3270_INTERNAL void win32_mainloop_free(H3270 *hSession);

#else
	/// @brief Use internal mainloop methods for the session.
	/// @param hSession The session to be set.
	/// @param gui Non-zero if the session is running in GUI mode.
	LIB3270_INTERNAL void setup_default_mainloop(H3270 *hSession);

	/// @brief Try to set the glib mainloop methods for the session.
	/// @return 0 if ok, error code if not
	LIB3270_INTERNAL int setup_glib_mainloop(H3270 *hSession);
#endif // _WIN32

