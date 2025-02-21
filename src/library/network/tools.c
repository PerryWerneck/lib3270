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
 #include <networking.h>
 #include <private/network.h>
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

int set_blocking_mode(H3270 *hSession, int sock, const unsigned char on) {

	if(sock < 0) {
		return EINVAL;
	}

#ifdef WIN32

	WSASetLastError(0);
	u_long iMode= on ? 1 : 0;

	if(ioctlsocket(sock,FIONBIO,&iMode)) {
		lib3270_popup_dialog(	hSession,
		                        LIB3270_NOTIFY_CONNECTION_ERROR,
		                        _( "Connection error" ),
		                        _( "ioctlsocket(FIONBIO) failed." ),
		                        "%s", lib3270_win32_strerror(GetLastError()));
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

		lib3270_popup(hSession, &popup, 0);

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

		lib3270_popup(hSession, &popup, 0);

		return error;

	}

#endif

	debug("Socket %d is now %s",sock,(on ? "Non Blocking" : "Blocking"));

	return 0;

}

/*
static const char * crl_download_protocols[] = {
	NULL,
	"http",
	"https",
#ifdef HAVE_LDAP
	"ldap",
	"ldaps"
#endif // HAVE_LDAP
};

const char * lib3270_crl_get_preferred_protocol(const H3270 *hSession) {
	debug("%s: selected: %d",__FUNCTION__,(int) hSession->ssl.crl_preferred_protocol);
	if(hSession->ssl.crl_preferred_protocol < (sizeof(crl_download_protocols)/sizeof(crl_download_protocols[0])))
		return crl_download_protocols[hSession->ssl.crl_preferred_protocol];

	errno = EINVAL;
	return NULL;
}

int lib3270_crl_set_preferred_protocol(H3270 *hSession, const char *protocol) {
	FAIL_IF_ONLINE(hSession);

	debug("%s(%s)",__FUNCTION__,protocol);
	size_t ix;
	for(ix = 0; ix < (sizeof(crl_download_protocols)/sizeof(crl_download_protocols[0])); ix++) {

		debug("[%s] [%s]",protocol,crl_download_protocols[ix]);
		if(crl_download_protocols[ix] && !strcasecmp(protocol,crl_download_protocols[ix])) {
			hSession->ssl.crl_preferred_protocol = (unsigned short) ix;
			return 0;
		}
	}

	debug("Unsupported protocol: %s",protocol);

	return EINVAL;
}

LIB3270_EXPORT int lib3270_getpeername(H3270 *hSession, struct sockaddr *addr, socklen_t *addrlen) {
	FAIL_IF_NOT_ONLINE(hSession);
	return hSession->network.module->getpeername(hSession, addr, addrlen);
}

LIB3270_EXPORT int lib3270_getsockname(H3270 *hSession, struct sockaddr *addr, socklen_t *addrlen) {
	FAIL_IF_NOT_ONLINE(hSession);
	return hSession->network.module->getsockname(hSession, addr, addrlen);
}

*/
