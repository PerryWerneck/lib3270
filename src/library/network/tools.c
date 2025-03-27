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

/*
 * Contatos:
 *
 * perry.werneck@gmail.com      (Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com     (Erico Mascarenhas Mendonça)
 *
 */

 #include <config.h>
 #include <lib3270.h>
 #include <lib3270/log.h>
 #include <private/network.h>
 #include <private/intl.h>
 #include <private/trace.h>
 #include <private/mainloop.h>
 #include <private/session.h>
 #include <lib3270/memory.h>
 #include <fcntl.h>
 #include <private/intl.h>
 #include <string.h>

 #ifdef _WIN32
	#include <lib3270/win32.h>
 #endif // _WIN32

 LIB3270_INTERNAL void set_popup_body(LIB3270_POPUP *popup, int error) {

 	switch(error) {
	case ETIMEDOUT:
		popup->body = N_("The connection timed out. This usually indicates that the remote server is not responding. Please check the server status or network configuration and try reconnecting.");
		break;
		
	case EPIPE:
		popup->body = N_("The connection was closed by the host. This usually indicates that the remote server has terminated the connection unexpectedly. Please check the server status or network configuration and try reconnecting.");
		break;

	case ECONNRESET:
		popup->summary = N_("The connection was reset by the remote server. This typically happens when the server forcefully closes the connection, possibly due to a timeout, server restart, or network issues. Please verify the server status and network connectivity, then try reconnecting.");
		break;

	case ECONNREFUSED:
		popup->body = N_("The remote server refused the connection. This usually indicates that the server is not accepting connections on the specified port. Please verify the server status and network configuration, then try reconnecting.");
		break;

	default:
		popup->body = strerror(error);
		break;

	}

 }
 

/*--[ Implement ]------------------------------------------------------------------------------------*/

/*
int lib3270_socket_recv_failed(H3270 *hSession) {

#ifdef _WIN32

	int wsaError = WSAGetLastError();

	// EWOULDBLOCK & EAGAIN should return directly.
	if(wsaError == WSAEWOULDBLOCK)
		return -EWOULDBLOCK;

	if(wsaError == WSAEINPROGRESS)
		return -EAGAIN;

	int rc = -wsaError;

	// TODO: Translate WSA Error, update message body.

	lib3270_set_network_error(
		hSession,
		_("Error receiving data from host"),
		_("The windows error code was %u"),
		(unsigned int) wsaError
	);

#else

	// EWOULDBLOCK & EAGAIN should return directly.
	if(errno == EWOULDBLOCK || errno == EAGAIN)
		return -errno;

	// Network error, notify user
	int rc = -errno;

	lib3270_set_network_error(
		hSession,
		_("Error receiving data from host"),
		_("The system error code was %d (%s)"),
		errno,
		strerror(errno)
	);

#endif // _WIN32

	return rc;

}
*/

/*
int lib3270_socket_send_failed(H3270 *hSession) {

#ifdef _WIN32

	lib3270_set_network_error(
		hSession,
	    _("Erro sending data to host"),
		_("The windows error code was %u"),
		(unsigned int) WSAGetLastError()
	);

#else

	int rc = errno;

	switch(rc) {
	case EPIPE:
		lib3270_set_network_error(
			hSession,
		    _("Broken pipe"),
			_("The system error code was %d"),
			rc
		);
		break;

	case ECONNRESET:
		lib3270_set_network_error(
			hSession,
		    _("Connection reset by peer"),
			_("The system error code was %d"),
			rc
		);
		break;

	case EINTR:
		return 0;

	default:
		lib3270_set_network_error(
			hSession,
		    _("Unexpected error writing to network socket"),
		    _("The system error code was %d (%s)"),
		    rc, strerror(rc)
		);

	}

#endif // _WIN32

	return -1;

}
*/

#ifdef _WIN32
LIB3270_INTERNAL int set_blocking_mode(H3270 *hSession, SOCKET sock, const unsigned char on) {
#else
LIB3270_INTERNAL int set_blocking_mode(H3270 *hSession, int sock, const unsigned char on) {
#endif // _WIN32

	if(sock < 0) {
		return EINVAL;
	}

#ifdef WIN32

	WSASetLastError(0);

	// Got socket, set it to non blocking.
	// Set the socket I/O mode: In this case FIONBIO
	// enables or disables the blocking mode for the 
	// socket based on the numerical value of iMode.
	// If iMode = 0, blocking is enabled; 
	// If iMode != 0, non-blocking mode is enabled.	
	// https://learn.microsoft.com/pt-br/windows/win32/api/winsock/nf-winsock-ioctlsocket	
	u_long iMode= on ? 0 : 1;

	if(ioctlsocket(sock,FIONBIO,&iMode)) {

		int error = WSAGetLastError();

		static const LIB3270_POPUP popup = {
			.name		= "ioctl",
			.type		= LIB3270_NOTIFY_NETWORK_ERROR,
			.title		= N_("Network error"),
			.summary	= N_("Unable to set non-blocking mode."),
			.body		= "",
			.label		= N_("OK")
		};

		popup_wsa_error(hSession,error,&popup);

		return -1;

	}

#else

	int f;
	if((f = fcntl(sock, F_GETFL, 0))== -1) {

		int error = errno;
		connection_close(hSession,error);

		LIB3270_POPUP popup = {
			.name		= "socket-api-error",
			.type		= LIB3270_NOTIFY_CONNECTION_ERROR,
			.title		= _("System error"),
			.summary	= _( "fcntl() error when getting socket state." ),
			.body		= strerror(error),
			.label		= _("OK")
		};

		lib3270_popup_async(hSession, &popup);

		return error;
	}

	if(on) {
		f &= ~O_NONBLOCK;
	} else {
		f |= O_NONBLOCK;
	}

	if(fcntl(sock, F_SETFL, f) < 0) {

		int error = errno;
		connection_close(hSession,error);

		LIB3270_POPUP popup = {
			.name		= "socket-api-error",
			.type		= LIB3270_NOTIFY_CONNECTION_ERROR,
			.title		= _("System error"),
			.summary	= _( "fcntl() error when setting socket state." ),
			.body		= strerror(error),
			.label		= _("OK")
		};

		lib3270_popup_async(hSession, &popup);

		return error;

	}

#endif

	debug("Socket %d set to %s mode",sock,(on ? "blocking" : "non blocking"));
	trace_network(hSession,"Socket %d set to %s mode\n",sock,(on ? "blocking" : "non blocking"));

	return 0;

}
