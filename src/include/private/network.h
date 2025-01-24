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
 #include <time.h>
 #include <lib3270/defs.h>

 typedef struct _lib3270_net_context {
	int sock;
	int (*disconnect)(H3270 *hSession, struct _lib3270_net_context *context);
 } LIB3270_NET_CONTEXT;

 /// @brief Close connection to host.
 /// @param hSession The TN3270 session
 /// @param code The error code (0 = ok) 
 /// @return 0 if ok or error code if not.
 LIB3270_INTERNAL int lib3270_connection_close(H3270 *hSession,int code);
 LIB3270_INTERNAL void lib3270_set_connected_socket(H3270 *hSession, int sock);
 LIB3270_INTERNAL void lib3270_set_connected_initial(H3270 *hSession);

 LIB3270_INTERNAL LIB3270_NET_CONTEXT * connect_insecure(H3270 *hSession, const char *hostname, const char *service, time_t timeout);
 LIB3270_INTERNAL LIB3270_NET_CONTEXT * connected_insecure(H3270 *hSession, int sock);
 
 LIB3270_INTERNAL void lib3270_setup_session(H3270 *hSession);

 LIB3270_INTERNAL int connection_write_offline(H3270 *hSession, const void *buffer, size_t length);
