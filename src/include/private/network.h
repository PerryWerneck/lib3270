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

 #ifdef _WIN32
	#include <winsock2.h>
	#include <windows.h>
 #else
	 #include <sys/socket.h>
 #endif

 #include <time.h>
 #include <lib3270/defs.h>
 #include <lib3270/popup.h>

 #define NETWORK_BUFFER_LENGTH	16384

 typedef struct lib3270_ssl_message {
	LIB3270_POPUP_HEAD			///< @brief Standard popup fields.
	const char		* icon;		///< @brief Icon name from https://specifications.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html
 } LIB3270_SSL_MESSAGE;

 typedef struct _lib3270_net_context {

	/// @brief Disconnect context, doesnt cleanup.
	int (*disconnect)(H3270 *hSession, struct _lib3270_net_context *context);

	/// @brief Release context resources.
	int (*finalize)(H3270 *hSession, struct _lib3270_net_context *context);

 } LIB3270_NET_CONTEXT;

 /// @brief Close connection to host.
 /// @param hSession The TN3270 session
 /// @param code The error code (0 = ok) 
 /// @return 0 if ok or error code if not.
 LIB3270_INTERNAL int connection_close(H3270 *hSession,int code);

 #ifdef _WIN32

	 /// @brief Connection completed, set socket session.
	 /// @param hSession The tn3270 session.
	 /// @param sock The socket to use.
	 /// @return 
	 LIB3270_INTERNAL void set_connected_socket(H3270 *hSession, SOCKET sock);

	 /// @brief DNS resolver completed, wait for connection.
	 /// @param hSession TN3270 session.
	 /// @param sock Socket with pending connection.
	 LIB3270_INTERNAL void set_resolved(H3270 *hSession, SOCKET sock);

 #else

	 /// @brief Connection completed, set socket session.
	 /// @param hSession The tn3270 session.
	 /// @param sock The socket to use.
	 /// @return 
 	LIB3270_INTERNAL void set_connected_socket(H3270 *hSession, int sock);

	LIB3270_INTERNAL int connect_socket(H3270 *hSession, int sock, const struct sockaddr *addr, socklen_t addrlen);
 
 #endif

 LIB3270_INTERNAL void set_connected_initial(H3270 *hSession);
 LIB3270_INTERNAL void net_input(H3270 *hSession, const unsigned char *buffer, size_t len);

 LIB3270_INTERNAL LIB3270_NET_CONTEXT * resolv_hostname(H3270 *hSession, const char *hostname, const char *service, time_t timeout);	
 
 
 LIB3270_INTERNAL void setup_session(H3270 *hSession);
 LIB3270_INTERNAL int set_blocking_mode(H3270 *hSession, int sock, const unsigned char on);

 LIB3270_INTERNAL int connection_write_offline(H3270 *, const void * , size_t, LIB3270_NET_CONTEXT *);
 LIB3270_INTERNAL int connection_except_offline(H3270 *, LIB3270_NET_CONTEXT *);

 LIB3270_INTERNAL LIB3270_NET_CONTEXT * setup_non_tls_context(H3270 *hSession);
 
 ///
 /// @brief Start TLS/SSL
 /// @param hSession	Session handle.
 ///
 /// @return 0 if ok, non zero if failed.
 /// @retval ENOTSUP no SSL support.
 LIB3270_INTERNAL int start_tls(H3270 *hSession);

 /// @brief Set the popup body based on network error code.
 /// @param popup The popup to update.
 /// @param error The error code
 LIB3270_INTERNAL void set_popup_body(LIB3270_POPUP *popup, int error);
 