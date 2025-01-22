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

#ifndef LIB3270_NETWORKING_H_INCLUDED

#define LIB3270_NETWORKING_H_INCLUDED

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>

typedef int socklen_t;

#else
#include <sys/socket.h>
#endif // _WIN32

#include <lib3270/popup.h>
#include <lib3270/mainloop.h>

typedef struct _lib3270_network_popup LIB3270_NETWORK_POPUP;
typedef struct _lib3270_net_context LIB3270_NET_CONTEXT;

typedef struct lib3270_ssl_message {
	LIB3270_POPUP_HEAD			///< @brief Standard popup fields.
	const char		* icon;		///< @brief Icon name from https://specifications.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html
} LIB3270_SSL_MESSAGE;

typedef struct lib3270_network_state {

	int syserror;				///< @brief System error (errno)
#ifdef _WIN32
	DWORD 	winerror;			///< @brief Win32 error got from GetLastError()
#endif // _WIN32

	const char * error_message;	/// @brief System error message.

	const LIB3270_POPUP *popup;	/// @brief Detailed info for popup.

} LIB3270_NETWORK_STATE;

typedef struct lib3270_net_module {

	/// @brief Protocol name for URL.
	const char *name;

	/// @brief Default service name.
	const char *service;

	/// @brief Prepare to connect.
	///
	/// @param hSession	TN3270 session.
	/// @param state	Pointer to state message.
	///
	int (*init)(H3270 *hSession);

	/// @brief Deinitialize network module.
	///
	/// @param hSession	TN3270 session.
	/// @param state	Pointer to state message.
	///
	void (*finalize)(H3270 *hSession);

	/// @brief Connect to host.
	///
	/// @param hSession	TN3270 session.
	/// @param seconds	Seconds for timeout.
	/// @param state	Pointer to state message.
	///
	int (*connect)(H3270 *hSession, LIB3270_NETWORK_STATE *state);

	/// @brief Disconnect from host.
	///
	/// @param hSession	TN3270 session.
	/// @param state	Pointer to state message.
	///
	int (*disconnect)(H3270 *hSession);

	/// @brief Start TLS/SSL.
	///
	/// @param hSession	TN3270 session.
	///
	/// @return 0 if ok, error code if not.
	///
	/// @retval 0		TLS/SSL was negotiated.
	/// @retval ENOTSUP	No TLS/SSL support in the network module.
	///
	int (*start_tls)(H3270 *hSession);

	/// @brief Send on network context.
	///
	/// @return Positive on data received, negative on error.
	///
	ssize_t (*send)(H3270 *hSession, const void *buffer, size_t length);

	/// @brief Receive on network context.
	///
	/// @return Positive on data received, negative on error.
	///
	/// @retval -ENOTCONN		Not connected to host.
	/// @retval -EWOULDBLOCK	Next request will block.
	/// @retval -EAGAIN			Try again.
	///
	ssize_t (*recv)(H3270 *hSession, void *buf, size_t len);

	/// @brief Add socket in poll list.
	void * (*add_poll)(H3270 *hSession, LIB3270_IO_FLAG flag, void(*call)(H3270 *, int, LIB3270_IO_FLAG, void *), void *userdata);

	/// @brief Set non blocking mode.
	///
	/// @retval 0	Not connected or Success.
	/// @retval -1	Failed (popup was sent).
	int (*non_blocking)(H3270 *hSession, const unsigned char on);

	/// @brief Check if the session is online.
	///
	/// @retval 0	The session is offline.
	int (*is_connected)(const H3270 *hSession);

	/// @brief Get socket name.
	///
	/// @return On success, zero is returned.  On error, -1 is returned, and errno is set appropriately.
	///
	/// @retval 0	Success.
	/// @retval -1	Error (errno is set).
	int (*getsockname)(const H3270 *hSession, struct sockaddr *addr, socklen_t *addrlen);

	/// @brief Get name of connected peer socket.
	///
	/// @return On success, zero is returned.  On error, -1 is returned, and errno is set appropriately.
	///
	/// @retval 0	Success.
	/// @retval -1	Error (errno is set).
	int (*getpeername)(const H3270 *hSession, struct sockaddr *addr, socklen_t *addrlen);

	/// @brief Set socket options.
	int (*setsockopt)(H3270 *hSession, int level, int optname, const void *optval, size_t optlen);

	/// @brief Get socket options.
	int (*getsockopt)(H3270 *hSession, int level, int optname, void *optval, socklen_t *optlen);

	/// @brief Get Peer certificate.
	///
	/// @return String with the peer certificate (release it with lib3270_free); NULL if not available.
	char * (*getcert)(const H3270 *hSession);

	/// @brief Get CRL.
	///
	/// @return String with the CRL certificate (release it with lib3270_free); NULL if not available.
	char * (*getcrl)(const H3270 *hSession);

	/// @brief Reset.
	///
	/// Clear network module state (used when URL changes).
	void (*reset)(H3270 *hSession);

} LIB3270_NET_MODULE;

/**
 * @brief Activate the default (and insecure) network module.
 *
 */
LIB3270_INTERNAL void lib3270_set_default_network_module(H3270 *hSession);

/**
 * @brief Set network error message.
 *
 */
LIB3270_INTERNAL void lib3270_set_network_error(H3270 *hSession,const char *summary, const char *fmt, ...);

/**
 * @brief Connect to host, returns a connected socket.
 *
 * @param hSession	Disconnected TN3270 session.
 * @param state		Pointer to network state context.
 *
 * @return The Socket number or -1 in case of failure.
 *
 */
LIB3270_INTERNAL int	  lib3270_network_connect(H3270 *hSession, LIB3270_NETWORK_STATE *state);

/**
 * @brief Translate system socket receive error codes, show popup if needed.
 *
 * @param hSession TN3270 Session handle.
 *
 * @return Translated error code.
 *
 * @retval -EWOULDBLOCK	Next call would block.
 * @retval -EAGAIN		Try again.
 *
 */
LIB3270_INTERNAL int lib3270_socket_recv_failed(H3270 *hSession);

/**
 * @brief Translate system socket send error codes, show popup if needed.
 *
 * @param hSession TN3270 Session handle.
 *
 * @return Translated error code.
 *
 */
LIB3270_INTERNAL int lib3270_socket_send_failed(H3270 *hSession);

LIB3270_INTERNAL int lib3270_socket_set_non_blocking(H3270 *hSession, int sock, const unsigned char on);

/**
 * @breif Select the network context from URL.
 *
 * @return Pointer to the hostname or NULL if failed (sets errno).
 *
 */
LIB3270_INTERNAL char * lib3270_set_network_module_from_url(H3270 *hSession, char *url);


/**
 * @brief Select the default (unsecure) network context.
 *
 * @param hSession	TN3270 Session handle.
 *
 */
LIB3270_INTERNAL void	  lib3270_set_default_network_module(H3270 *hSession);

#ifdef HAVE_LIBSSL
LIB3270_INTERNAL void	  lib3270_set_libssl_network_module(H3270 *hSession);
#endif // HAVE_LIBSSL

LIB3270_INTERNAL int	  lib3270_activate_ssl_network_module(H3270 *hSession, int sock);


#endif // LIB3270_NETWORKING_H_INCLUDED

