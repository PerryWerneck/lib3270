/*
 * "Software G3270, desenvolvido com base nos códigos fontes do WC3270  e  X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270.
 *
 * Copyright (C) <2008> <Banco do Brasil S.A.>
 *
 * Este programa é software livre. Você pode redistribuí-lo e/ou modificá-lo sob
 * os termos da GPL v.2 - Licença Pública Geral  ',  conforme  publicado  pela
 * Free Software Foundation.
 *
 * Este programa é distribuído na expectativa de  ser  útil,  mas  SEM  QUALQUER
 * GARANTIA; sem mesmo a garantia implícita de COMERCIALIZAÇÃO ou  de  ADEQUAÇÃO
 * A QUALQUER PROPÓSITO EM PARTICULAR. Consulte a Licença Pública Geral GNU para
 * obter mais detalhes.
 *
 * Você deve ter recebido uma cópia da Licença Pública Geral GNU junto com este
 * programa; se não, escreva para a Free Software Foundation, Inc., 51 Franklin
 * St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Este programa está nomeado como networking.h e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas de Mendonça)
 *
 */

#ifndef LIB3270_NETWORKING_H_INCLUDED

	#define LIB3270_NETWORKING_H_INCLUDED

	#include <lib3270/popup.h>
	#include <sys/socket.h>

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

		/// @brief get socket name.
		///
		/// @return On success, zero is returned.  On error, -1 is returned, and errno is set appropriately.
		///
		/// @retval 0	Success.
		/// @retval -1	Error (errno is set).
		int (*getsockname)(const H3270 *hSession, struct sockaddr *addr, socklen_t *addrlen);

		/// @brief Set socket options.
		int (*setsockopt)(H3270 *hSession, int level, int optname, const void *optval, size_t optlen);

		/// @brief Get socket options.
		int (*getsockopt)(H3270 *hSession, int level, int optname, void *optval, socklen_t *optlen);

	} LIB3270_NET_MODULE;

	/**
	 * @brief Activate the default (and insecure) network module.
	 *
	 */
	LIB3270_INTERNAL void lib3270_set_default_network_module(H3270 *hSession);

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
	LIB3270_INTERNAL int lib3270_network_recv_failed(H3270 *hSession);

	/**
	 * @brief Translate system socket send error codes, show popup if needed.
	 *
	 * @param hSession TN3270 Session handle.
	 *
	 * @return Translated error code.
	 *
	 */
	LIB3270_INTERNAL int lib3270_network_send_failed(H3270 *hSession);

	/**
	 * @brief Select the default (unsecure) network context.
	 *
	 * @param hSession	TN3270 Session handle.
	 *
	 */
	LIB3270_INTERNAL void	  lib3270_set_default_network_module(H3270 *hSession);

	LIB3270_INTERNAL int	  lib3270_activate_ssl_network_module(H3270 *hSession, int sock);

#endif // LIB3270_NETWORKING_H_INCLUDED

