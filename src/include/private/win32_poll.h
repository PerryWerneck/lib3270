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
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

 #pragma once

 #include <winsock2.h>
 #include <windows.h>
 #include <wininet.h>

 #include <lib3270/defs.h>
 #include <private/session.h>
 #include <private/linkedlist.h>

 struct _lib3270_poll_context {
	int dunno;
 };

 typedef struct _handler {
	LIB3270_LINKED_LIST_HEAD
	HANDLE event;			// Event to signal the thread.
	H3270 *hSession;
	SOCKET sock;
	char disabled;
	LIB3270_IO_FLAG flag;
	void(*proc)(H3270 *, SOCKET, LIB3270_IO_FLAG, void *);
 } handler_t;

 /// @brief Initialize poll context for session.
 /// @param hSession The session asking for the poll context.
 /// @return The new poll context.
 LIB3270_INTERNAL LIB3270_POLL_CONTEXT * win32_poll_init(H3270 *hSession);

 /// @brief Finalize poll context for session.
 /// @param hSession The session asking for the poll context.
 /// @param context The context to be finalized.
 LIB3270_INTERNAL void win32_poll_finalize(H3270 *hSession, LIB3270_POLL_CONTEXT * context);

 /// @brief Remove a socket from the poll list.
 /// @param hSession The session asking for removal.
 /// @param id The id of the event to be removed.
 LIB3270_INTERNAL void win32_poll_remove(H3270 *hSession, void *id);

 /// @brief Add socket to the poll list.
 /// @param hSession The session asking for the poll.
 /// @param sock The socket to be added.
 /// @param flag The flags to be monitored.
 /// @param proc The callback to be called when the event is triggered.
 /// @param userdata The userdata to be passed to the callback.
 /// @return The id of the inserted event.
 LIB3270_INTERNAL void * win32_poll_add(H3270 *hSession, SOCKET sock, LIB3270_IO_FLAG flag, void(*proc)(H3270 *, SOCKET, LIB3270_IO_FLAG, void *), void *userdata );

 /// @brief Force the network thread to wake up.
 /// @param hSession The session asking for the wake up.
 LIB3270_INTERNAL void win32_poll_wake_up(H3270 *hSession);

