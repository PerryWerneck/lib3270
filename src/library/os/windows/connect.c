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

 #include <private/network.h>
 #include <private/intl.h>
 #include <private/mainloop.h>
 #include <private/session.h>

 typedef struct {
	LIB3270_NET_CONTEXT parent;
	void *timer;
	void *connected;
	void *except;
 } Context;

 static void net_except(H3270 *hSession, int sock, LIB3270_IO_FLAG flag, Context *context) {


 }

 static void net_connected(H3270 *hSession, int sock, LIB3270_IO_FLAG flag, Context *context) {



 }

 static int net_timeout(H3270 *hSession, Context *context) {


	return 0;
 }

static int net_disconnect(H3270 *hSession, Context *context) {


	return 0;
 }

 LIB3270_INTERNAL int set_resolved(H3270 *hSession, SOCKET sock) {

	hSession->ever_3270 = 0;


	return 0;

 }
