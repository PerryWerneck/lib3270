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

 typedef struct _handler {
	LIB3270_LINKED_LIST_HEAD
	H3270 *hSession;
	SOCKET sock;
	WSAEVENT event;
	char disabled;
	long events;
	void (*proc)(H3270 *hSession, SOCKET sock, void *userdata);
 } handler_t;

 /// @brief Initialize poll context for session.
 /// @param hSession The session asking for the poll context.
 /// @return The new poll context.
 LIB3270_INTERNAL void win32_poll_init(H3270 *hSession);

 /// @brief Finalize poll context for session.
 /// @param hSession The session asking for the poll context.
 /// @param context The context to be finalized.
 LIB3270_INTERNAL void win32_poll_finalize(H3270 *hSession);

 /// @brief Add pool to the watch list.
 /// @param hSession The session for the poll.
 /// @param sock The socket to watch.
 /// @param events A bitmask that specifies the combination of FD_XXX network events in which the application has interest.
 /// @param call The method to call.
 /// @return The poll handler.
 LIB3270_INTERNAL void * win32_poll_add(H3270 *hSession, SOCKET sock, long events, void (*call)(H3270 *hSession, SOCKET sock, void *userdata), void *userdata);

 LIB3270_INTERNAL void * win32_poll_remove(void *handler);

 LIB3270_INTERNAL int win32_poll_enabled();

 