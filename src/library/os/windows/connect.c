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
 #include <winsock2.h>
 #include <windows.h>

 #include <lib3270/defs.h>
 #include <lib3270/memory.h>

 #include <private/network.h>
 #include <private/intl.h>
 #include <private/mainloop.h>
 #include <private/session.h>

 typedef struct {
	LIB3270_NET_CONTEXT parent;
 } Context;

 /// @brief Connnection was started, wait for it to be available.
 /// @param hSession The session handle.
 /// @param sock The socket
 LIB3270_INTERNAL void set_resolved(H3270 *hSession, SOCKET sock) {

	debug("%s socket=%lu",__FUNCTION__,(unsigned long) sock);

	if(hSession->connection.context) {
		hSession->connection.context->finalize(hSession,hSession->connection.context);
		hSession->connection.context = NULL;
	}

	hSession->ever_3270 = 0;
	set_cstate(hSession, LIB3270_PENDING);
	notify_new_state(hSession, LIB3270_STATE_HALF_CONNECT, 1);

	Context *context = lib3270_new(Context);
	memset(context,0,sizeof(Context));

 }

 LIB3270_INTERNAL void win32_poll_finalize(H3270 *hSession, LIB3270_POLL_CONTEXT * context) {
	
 }
