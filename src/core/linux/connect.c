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
#include <internals.h>
#include <errno.h>
#include <lib3270/trace.h>
#include <lib3270/toggle.h>
#include "kybdc.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdlib.h>

#include "hostc.h"
#include "trace_dsc.h"
#include "telnetc.h"
#include "screen.h"
#include "utilc.h"

#include <lib3270/internals.h>
#include <lib3270/log.h>
#include <lib3270/trace.h>
#include <networking.h>
//#include <fcntl.h>
#include <poll.h>

/*---[ Implement ]-------------------------------------------------------------------------------*/

 static int sock_connect(H3270 *hSession, int sock, const struct sockaddr *address, socklen_t address_len) {

	lib3270_socket_set_non_blocking(hSession, sock, 1);

	if(!connect(sock,address,address_len))
		return 0;

	if(errno != EINPROGRESS)
		return errno;

	unsigned int timer;
	for(timer = 0; timer < hSession->connection.timeout; timer += 10) {

		if(lib3270_get_connection_state(hSession) != LIB3270_CONNECTING)
			return errno = ECANCELED;

		struct pollfd pfd = {
			.fd = sock,
			.events = POLLOUT
		};

		switch(poll(&pfd,1,10)) {
		case -1:	// Poll error
			return errno;

		case 0:
			break;

		case 1:
			// Got response.
			if(pfd.revents && POLLOUT) {
				debug("%s: Connection complete",__FUNCTION__);
				return 0;
			}
			break;
		}

	}

	return errno = ETIMEDOUT;

 }


 int lib3270_network_connect(H3270 *hSession, LIB3270_NETWORK_STATE *state) {

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
 	if(rc)
	{
		state->error_message = gai_strerror(rc);
		return -1;
	}

	//
	// Try connecting to hosts.
	//
	int sock = -1;
	struct addrinfo * rp = NULL;

	status_connecting(hSession);

	for(rp = result; sock < 0 && rp != NULL && state->syserror != ECANCELED; rp = rp->ai_next)
	{
		// Got socket from host definition.
		sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(sock < 0)
		{
			// Can't get socket.
			state->syserror = errno;
			continue;
		}

		// Try connect.
		if(sock_connect(hSession, sock, rp->ai_addr, rp->ai_addrlen))
		{
			// Can't connect to host
			state->syserror = errno;
			close(sock);
			sock = -1;
			continue;
		}

		lib3270_socket_set_non_blocking(hSession,sock,0);
	}

	freeaddrinfo(result);

	if(sock < 0)
	{
		static const LIB3270_POPUP popup = {
			.name = "CantConnect",
			.type = LIB3270_NOTIFY_ERROR,
			.summary = N_("Can't connect to host"),
			.label = N_("Try again")
		};

		state->popup = &popup;
		return sock;
	}

	// don't share the socket with our children
	(void) fcntl(sock, F_SETFD, 1);

	return sock;
 }

 static void net_connected(H3270 *hSession, int GNUC_UNUSED(fd), LIB3270_IO_FLAG GNUC_UNUSED(flag), void GNUC_UNUSED(*dunno))
 {
	int 		err;
	socklen_t	len		= sizeof(err);

	if(hSession->xio.write) {
		trace("%s write=%p",__FUNCTION__,hSession->xio.write);
		lib3270_remove_poll(hSession, hSession->xio.write);
		hSession->xio.write = NULL;
	}

	if(hSession->network.module->getsockopt(hSession, SOL_SOCKET, SO_ERROR, (char *) &err, &len) < 0)
	{
		int err = errno;
		lib3270_disconnect(hSession);
		lib3270_popup_dialog(
			hSession,
			LIB3270_NOTIFY_ERROR,
			_( "Network error" ),
			_( "Unable to get connection state." ),
			_( "The system error was %s" ), strerror(err)
		);
		return;
	}
	else if(err)
	{
		lib3270_autoptr(LIB3270_POPUP) popup =
			lib3270_popup_clone_printf(
				NULL,
				_( "Can't connect to %s:%s"),
				hSession->host.current,
				hSession->host.srvc
			);

		lib3270_autoptr(char) syserror =
						lib3270_strdup_printf(
							_("The system error was \"%s\" (rc=%d)"),
							strerror(err),
							err
						);

		if(hSession->cbk.popup(hSession,popup,!hSession->auto_reconnect_inprogress) == 0)
			lib3270_activate_auto_reconnect(hSession,1000);

		return;
	}

	if(lib3270_start_tls(hSession)) {
		lib3270_disconnect(hSession);
		return;
	}

	hSession->xio.except = hSession->network.module->add_poll(hSession,LIB3270_IO_FLAG_EXCEPTION,net_exception,0);
	hSession->xio.read = hSession->network.module->add_poll(hSession,LIB3270_IO_FLAG_READ,net_input,0);

	lib3270_setup_session(hSession);
	lib3270_set_connected_initial(hSession);

	lib3270_notify_tls(hSession);

 }

 int net_reconnect(H3270 *hSession, int seconds)
 {
	LIB3270_NETWORK_STATE state;
	memset(&state,0,sizeof(state));

 	// Initialize and connect to host
	set_ssl_state(hSession,LIB3270_SSL_UNDEFINED);
	lib3270_set_cstate(hSession,LIB3270_CONNECTING);

	if(lib3270_run_task(hSession, (int(*)(H3270 *, void *)) hSession->network.module->connect, &state))
	{
		lib3270_autoptr(LIB3270_POPUP) popup =
			lib3270_popup_clone_printf(
				NULL,
				_( "Can't connect to %s:%s"),
				hSession->host.current,
				hSession->host.srvc
			);

		if(!popup->summary)
		{
			popup->summary = popup->body;
			popup->body = NULL;
		}

		lib3270_autoptr(char) syserror = NULL;
		if(state.syserror)
		{
			syserror = lib3270_strdup_printf(
							_("The system error was \"%s\" (rc=%d)"),
							strerror(state.syserror),
							state.syserror
						);
		}
#ifdef _WIN32
		else if(state.winerror)
		{
			#error TODO
		}
#endif // _WIN32

		if(!popup->body)
		{
			if(state.error_message)
				popup->body = state.error_message;
			else
				popup->body = syserror;
		}

		lib3270_disconnect(hSession);	// To cleanup states.

		popup->label = _("_Retry");
		if(lib3270_popup(hSession,popup,!hSession->auto_reconnect_inprogress) == 0)
			lib3270_activate_auto_reconnect(hSession,1000);

		return errno = ENOTCONN;
	}


	//
	// Connected
	//
	hSession->ever_3270 = False;

#if defined(HAVE_LIBSSLx)
	if(hSession->ssl.enabled)
	{
		hSession->ssl.host = 1;
		if(ssl_init(hSession))
			return errno = ENOTCONN;

	}
#endif // HAVE_LIBSSL

	// set options for inline out-of-band data and keepalives
	int optval = 1;
	if(hSession->network.module->setsockopt(hSession, SOL_SOCKET, SO_OOBINLINE, &optval, sizeof(optval)) < 0)
	{
		int rc = errno;
		lib3270_popup_dialog(	hSession,
								LIB3270_NOTIFY_ERROR,
								_( "Connection error" ),
								_( "setsockopt(SO_OOBINLINE) has failed" ),
								"%s",
								strerror(rc));
		hSession->network.module->disconnect(hSession);
		return rc;
	}

	optval = lib3270_get_toggle(hSession,LIB3270_TOGGLE_KEEP_ALIVE) ? 1 : 0;
	if (hSession->network.module->setsockopt(hSession, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) < 0)
	{
		int rc = errno;

		char buffer[4096];
		snprintf(buffer,4095,_( "Can't %s network keep-alive" ), optval ? _( "enable" ) : _( "disable" ));

		lib3270_popup_dialog(	hSession,
								LIB3270_NOTIFY_ERROR,
								_( "Connection error" ),
								buffer,
								"%s",
								strerror(rc));

		hSession->network.module->disconnect(hSession);
		return rc;
	}
	else
	{
		trace_dsn(hSession,"Network keep-alive is %s\n",optval ? "enabled" : "disabled" );
	}


	/*
#if defined(OMTU)
	else if (setsockopt(hSession->sock, SOL_SOCKET, SO_SNDBUF, (char *)&mtu,sizeof(mtu)) < 0)
	{
		popup_a_sockerr(hSession, _( "setsockopt(%s)" ), "SO_SNDBUF");
		SOCK_CLOSE(hSession);
	}
#endif

	*/

	// Connecting, set callbacks, wait for connection
	lib3270_set_cstate(hSession, LIB3270_PENDING);
	lib3270_st_changed(hSession, LIB3270_STATE_HALF_CONNECT, True);

	hSession->xio.write = hSession->network.module->add_poll(hSession,LIB3270_IO_FLAG_WRITE,net_connected,0);

	trace("%s: Connection in progress",__FUNCTION__);

	if(seconds)
	{
		int rc = lib3270_wait_for_cstate(hSession,LIB3270_CONNECTED_TN3270E,seconds);
		if(rc)
		{
			lib3270_disconnect(hSession);
			lib3270_write_log(hSession,"connect", "%s: %s",__FUNCTION__,strerror(ETIMEDOUT));
			return errno = rc;
		}

		/*
		time_t end = time(0)+seconds;

		while(time(0) < end)
		{
			lib3270_main_iterate(hSession,1);

			switch(hSession->connection.state)
			{
			case LIB3270_PENDING:
			case LIB3270_CONNECTED_INITIAL:
			case LIB3270_CONNECTED_ANSI:
			case LIB3270_CONNECTED_3270:
			case LIB3270_CONNECTED_INITIAL_E:
			case LIB3270_CONNECTED_NVT:
			case LIB3270_CONNECTED_SSCP:
			case LIB3270_CONNECTING:
				break;

			case LIB3270_NOT_CONNECTED:
				return errno = ENOTCONN;

			case LIB3270_CONNECTED_TN3270E:
				if(!hSession->starting)
					return 0;
				break;

			default:
				lib3270_write_log(hSession,"connect", "%s: State changed to unexpected state %d",__FUNCTION__,hSession->connection.state);
				return errno = EINVAL;
			}

		}

		lib3270_disconnect(hSession);
		lib3270_write_log(hSession,"connect", "%s: %s",__FUNCTION__,strerror(ETIMEDOUT));

		return errno = ETIMEDOUT;
		*/
	}

	return 0;

 }

