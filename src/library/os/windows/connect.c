/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270.
 *
 * Copyright (C) <2008> <Banco do Brasil S.A.>
 *
 * Este programa é software livre. Você pode redistribuí-lo e/ou modificá-lo sob
 * os termos da GPL v.2 - Licença Pública Geral  GNU,  conforme  publicado  pela
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
 * Este programa está nomeado como - e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

#include <config.h>

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <lib3270/win32.h>

#include <internals.h>
#include <lib3270/log.h>
#include <lib3270/internals.h>

#include "telnetc.h"
#include <private/host.h>
#include <private/trace.h>
#include "screen.h"

/*---[ Implement ]-------------------------------------------------------------------------------*/

static int sock_connect(H3270 *hSession, int sock, const struct sockaddr *address, socklen_t address_len) {

	lib3270_socket_set_non_blocking(hSession, sock, 1);

	WSASetLastError(0);

	if(!connect(sock,address,address_len))
		return 0;

	/*
	if(WSAGetLastError() != WSAEINPROGRESS) {
	debug("Can't connect WSAGetLastError=%d",(int) WSAGetLastError());
	errno = ENOTCONN;
	return -1;
	}
	*/

	unsigned int timer;
	for(timer = 0; timer < hSession->connection.timeout; timer += 10) {

		if(lib3270_get_connection_state(hSession) != LIB3270_CONNECTING)
			return errno = ECANCELED;

		struct timeval tm;

		tm.tv_sec 	= 0;
		tm.tv_usec	= 10000;

		fd_set wfds;
		FD_ZERO(&wfds);
		FD_SET(sock, &wfds);

		int ns = select(sock+1, NULL, &wfds, NULL, &tm);

		if (ns < 0 && errno != EINTR) {
			return errno;
		}

		if(FD_ISSET(sock, &wfds))
			return 0;

	}

	debug("Timeout! WSAGetLastError=%d",(int) WSAGetLastError());
	return errno = ETIMEDOUT;

}


int lib3270_network_connect(H3270 *hSession, LIB3270_NETWORK_STATE *state) {

	// Do we need to start wsa?
	static unsigned char wsa_is_started = 0;
	if(!wsa_is_started) {

		WORD wVersionRequested;
		WSADATA wsaData;

		wVersionRequested = MAKEWORD(2, 2);

		if (WSAStartup(wVersionRequested, &wsaData) != 0) {
			static const LIB3270_POPUP popup = {
				.type = LIB3270_NOTIFY_CRITICAL,
				.summary = N_("WSAStartup failed")
			};

			state->popup = &popup;
			state->winerror = GetLastError();

			return -1;
		}

		if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
			static const LIB3270_POPUP popup = {
				.type = LIB3270_NOTIFY_CRITICAL,
				.summary = N_("Bad winsock version"),
				.body = N_("Can't use this system winsock")
			};

			state->popup = &popup;
			state->winerror = GetLastError();

			return -1;
		}

		wsa_is_started = 1;
	}

	// Reset state
	set_ssl_state(hSession,LIB3270_SSL_UNDEFINED);

	//
	// Resolve hostname
	//
	struct addrinfo	  hints;
	struct addrinfo * result	= NULL;
	memset(&hints,0,sizeof(hints));
	hints.ai_family 	= AF_UNSPEC;	// Allow IPv4 or IPv6
	hints.ai_socktype	= SOCK_STREAM;	// Stream socket
	hints.ai_flags		= AI_PASSIVE;	// For wildcard IP address
	hints.ai_protocol	= 0;			// Any protocol

	status_resolving(hSession);

	int rc = getaddrinfo(hSession->host.current, hSession->host.srvc, &hints, &result);
	if(rc) {
		state->winerror = rc;
		return -1;
	}

	//
	// Try connecting to hosts.
	//
	int sock = -1;
	struct addrinfo * rp = NULL;

	status_connecting(hSession);

	for(rp = result; sock < 0 && rp != NULL && state->syserror != ECANCELED; rp = rp->ai_next) {
		// Got socket from host definition.
		sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(sock < 0) {
			// Can't get socket.
			state->syserror = errno;
			continue;
		}

		// Try connect.
		if(sock_connect(hSession, sock, rp->ai_addr, rp->ai_addrlen)) {
			// Can't connect to host
			state->winerror = WSAGetLastError();
			if(state->winerror == WSAEWOULDBLOCK) {
				state->winerror = 0;
			}
			state->syserror = errno;
			closesocket(sock);
			sock = -1;
			continue;
		}

		lib3270_socket_set_non_blocking(hSession,sock,0);
	}

	freeaddrinfo(result);

	if(sock < 0) {
		static const LIB3270_POPUP popup = {
			.name = "CantConnect",
			.type = LIB3270_NOTIFY_CONNECTION_ERROR,
			.summary = N_("Can't connect to host"),
			.label = N_("Try again")
		};

		state->popup = &popup;
		return sock;
	}

	return sock;
}

static void net_connected(H3270 *hSession, int GNUC_UNUSED(fd), LIB3270_IO_FLAG GNUC_UNUSED(flag), void GNUC_UNUSED(*dunno)) {
	int 		err;
	socklen_t	len		= sizeof(err);

	if(hSession->xio.write) {
		debug"%s write=%p",__FUNCTION__,hSession->xio.write);
		hSession->poll.remove(hSession, hSession->xio.write);
		hSession->xio.write = NULL;
	}

	if(hSession->network.module->getsockopt(hSession, SOL_SOCKET, SO_ERROR, (char *) &err, &len) < 0) {
		connection_close(hSession,err);
		lib3270_popup_dialog(
		    hSession,
		    LIB3270_NOTIFY_ERROR,
		    _( "Network error" ),
		    _( "Unable to get connection state." ),
		    _( "The system error was %s" ), lib3270_win32_strerror(WSAGetLastError())
		);
		return;
	} else if(err) {
		lib3270_autoptr(LIB3270_POPUP) popup =
		    lib3270_popup_clone_printf(
		        NULL,
		        _( "Can't connect to %s"),
		        hSession->connection.url,
		    );

		lib3270_autoptr(char) syserror =
		    lib3270_strdup_printf(
		        _("The system error was \"%s\""),
		        lib3270_win32_strerror(WSAGetLastError())
		    );

		if(lib3270_popup(hSession,popup,!hSession->auto_reconnect_inprogress) == 0)
			lib3270_activate_auto_reconnect(hSession,1000);

		return;
	}

	if(lib3270_start_tls(hSession)) {
		connection_close(hSession,-1);
		return;
	}

	hSession->xio.except = hSession->network.module->add_poll(hSession,LIB3270_IO_FLAG_EXCEPTION,net_exception,0);
	hSession->xio.read = hSession->network.module->add_poll(hSession,LIB3270_IO_FLAG_READ,net_input,0);

	setup_session(hSession);
	set_connected_initial(hSession);

	lib3270_notify_tls(hSession);

}

int net_reconnect(H3270 *hSession, int seconds) {
	LIB3270_NETWORK_STATE state;
	memset(&state,0,sizeof(state));

	// Initialize and connect to host
	set_ssl_state(hSession,LIB3270_SSL_UNDEFINED);
	set_cstate(hSession,LIB3270_CONNECTING);

	if(lib3270_run_task(hSession, "reconnect", (int(*)(H3270 *, void *)) hSession->network.module->connect, &state)) {
		lib3270_autoptr(LIB3270_POPUP) popup =
		    lib3270_popup_clone_printf(
		        NULL,
		        _( "Can't connect to %s:%s"),
		        hSession->host.current,
		        hSession->host.srvc
		    );

		if(!popup->summary) {
			popup->summary = popup->body;
			popup->body = NULL;
		}

		lib3270_autoptr(char) syserror = NULL;

#ifdef _WIN32
		if(state.winerror) {
			syserror = lib3270_strdup_printf(
			               "%s (Windows error %u)",
			               lib3270_win32_strerror(state.winerror),
			               (unsigned int) state.winerror
			           );
		} else if(state.syserror == ETIMEDOUT) {

			syserror = lib3270_strdup_printf(
			               _("The system error was \"%s\" (rc=%d)"),
			               _("Timeout conneting to host"),
			               state.syserror
			           );

		} else if(state.syserror == ENOTCONN) {

			syserror = lib3270_strdup_printf(
			               _("The system error was \"%s\" (rc=%d)"),
			               _("Not connected to host"),
			               state.syserror
			           );

		} else if(state.syserror) {

			syserror = lib3270_strdup_printf(
			               _("The system error was \"%s\" (rc=%d)"),
			               strerror(state.syserror),
			               state.syserror
			           );

		}

#else
		if(state.syserror) {
			syserror = lib3270_strdup_printf(
			               _("The system error was \"%s\" (rc=%d)"),
			               strerror(state.syserror),
			               state.syserror
			           );
		}
#endif // _WIN32

		if(!popup->body) {
			if(state.error_message)
				popup->body = state.error_message;
			else
				popup->body = syserror;
		}

		connection_close(hSession,-1);	// To cleanup states.

		popup->label = _("_Retry");
		if(lib3270_popup(hSession,popup,!hSession->auto_reconnect_inprogress) == 0)
			lib3270_activate_auto_reconnect(hSession,1000);

		return errno = ENOTCONN;
	}

	//
	// Connected
	//
	hSession->ever_3270 = False;

	// set options for inline out-of-band data and keepalives
	int optval = 1;
	if(hSession->network.module->setsockopt(hSession, SOL_SOCKET, SO_OOBINLINE, &optval, sizeof(optval)) < 0) {
		int rc = errno;
		lib3270_popup_dialog(	hSession,
		                        LIB3270_NOTIFY_CONNECTION_ERROR,
		                        _( "Connection error" ),
		                        _( "setsockopt(SO_OOBINLINE) has failed" ),
		                        _( "The system error was %s" ),
		                        lib3270_win32_strerror(WSAGetLastError())
		                    );
		hSession->network.module->disconnect(hSession);
		return rc;
	}

	optval = lib3270_get_toggle(hSession,LIB3270_TOGGLE_KEEP_ALIVE) ? 1 : 0;
	if (hSession->network.module->setsockopt(hSession, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) < 0) {
		int rc = errno;

		char buffer[4096];
		snprintf(buffer,4095,_( "Can't %s network keep-alive" ), optval ? _( "enable" ) : _( "disable" ));

		lib3270_popup_dialog(	hSession,
		                        LIB3270_NOTIFY_CONNECTION_ERROR,
		                        _( "Connection error" ),
		                        buffer,
		                        _( "The system error was %s" ),
		                        lib3270_win32_strerror(WSAGetLastError())
		                    );

		hSession->network.module->disconnect(hSession);
		return rc;
	} else {
		trace_dsn(hSession,"Network keep-alive is %s\n",optval ? "enabled" : "disabled" );
	}

	// Connecting, set callbacks, wait for connection
	set_cstate(hSession, LIB3270_PENDING);
	notify_new_state(hSession, LIB3270_STATE_HALF_CONNECT, True);

	hSession->xio.write = hSession->network.module->add_poll(hSession,LIB3270_IO_FLAG_WRITE,net_connected,0);

	debug("%s: Connection in progress",__FUNCTION__);

	if(seconds) {
		int rc = lib3270_wait_for_cstate(hSession,LIB3270_CONNECTED_TN3270E,seconds);
		if(rc) {
			connection_close(hSession,rc);
			lib3270_log_write(hSession,"connect", "%s: %s",__FUNCTION__,strerror(ETIMEDOUT));
			return errno = rc;
		}

	}

	return 0;

}

