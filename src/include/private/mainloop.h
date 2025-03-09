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

	/**
	 * @brief I/O Controller.
	 *
	 * GUI unblocking I/O calls, used to replace the lib3270´s internal ones.
	 *
	 */ /*
	typedef struct lib3270_io_controller {

		void	* (*AddTimer)(H3270 *session, unsigned long interval_ms, int (*proc)(H3270 *session, void *userdata), void *userdata);
		void	  (*RemoveTimer)(H3270 *session, void *timer);

		void	* (*add_poll)(H3270 *session, int fd, LIB3270_IO_FLAG flag, void(*proc)(H3270 *, int, LIB3270_IO_FLAG, void *), void *userdata);
		void	  (*remove_poll)(H3270 *session, void *id);
		void	  (*set_poll_state)(H3270 *session, void *id, int enabled);

		int		  (*Wait)(H3270 *hSession, int seconds);
		int		  (*event_dispatcher)(H3270 *session, int wait);
		void	  (*ring_bell)(H3270 *session);
		int		  (*run_task)(H3270 *session, const char *name, int(*callback)(H3270 *, void *), void *parm);

		/// @brief Async popup dialog.
		/// @param hSession	Session handle.
		/// @param popup		Popup descriptor.
		void	  (*popup)(H3270 *hSession, const LIB3270_POPUP *popup);
		
	} LIB3270_IO_CONTROLLER;
	 */


#ifdef _WIN32

	#include <winsock2.h>
	#include <windows.h>

	enum MessageTypes {
		WM_ADD_TIMER				= WM_USER+102,
		WM_REMOVE_TIMER				= WM_USER+103,
		WM_ADD_SOCKET				= WM_USER+104,
		WM_REMOVE_SOCKET			= WM_USER+105,
		WM_RESOLV_FAILED			= WM_USER+106,
		WM_CONNECTION_FAILED		= WM_USER+107,
		WM_RESOLV_SUCCESS			= WM_USER+108,
		WM_POST_CALLBACK			= WM_USER+109,
		WM_CLOSE_THREAD				= WM_USER+110,
		WM_POPUP_MESSAGE			= WM_USER+111,
		WM_POPUP_WSA_ERROR			= WM_USER+112,
		WM_POPUP_LAST_ERROR			= WM_USER+113,
		WM_SOCKET_EVENT				= WM_USER+114,
		WM_RECV_FAILED				= WM_USER+115,
		WM_SEND_FAILED				= WM_USER+116,
		WM_CONNECTION_SUCCESS		= WM_USER+117,
		WM_ASYNC_POPUP				= WM_USER+118,
	};

 	/// @brief Create win32 object window for the session
 	/// @param hSession The session to be associated with the object window.
 	LIB3270_INTERNAL void win32_mainloop_new(H3270 *hSession);

	/// @brief Destroy win32 object window for the session
	/// @param hSession The session to be dissociated from the object window.
	LIB3270_INTERNAL void win32_mainloop_free(H3270 *hSession);

#else

	#include <poll.h>

	typedef enum {
		LIB3270_IO_FLAG_READ		= POLLIN,
		LIB3270_IO_FLAG_WRITE		= POLLOUT,
		LIB3270_IO_FLAG_EXCEPTION	= POLLPRI,
		LIB3270_IO_FLAG_ERROR		= POLLERR,
		LIB3270_IO_FLAG_HUP			= POLLHUP,
		LIB3270_IO_FLAG_NVAL		= POLLNVAL,
	} LIB3270_IO_FLAG;

	/// @brief Use internal mainloop methods for the session.
	/// @param hSession The session to be set.
	/// @param gui Non-zero if the session is running in GUI mode.
	LIB3270_INTERNAL void setup_default_mainloop(H3270 *hSession);

	/// @brief Try to set the glib mainloop methods for the session.
	/// @return 0 if ok, error code if not
	LIB3270_INTERNAL int setup_glib_mainloop(H3270 *hSession);

#endif // _WIN32

