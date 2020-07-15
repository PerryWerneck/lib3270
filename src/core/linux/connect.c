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

#define SOCK_CLOSE(s)	close(s->connection.sock); s->connection.sock = -1;

#include <stdlib.h>

#include "hostc.h"
#include "trace_dsc.h"
#include "telnetc.h"
#include "screen.h"

#include <lib3270/internals.h>
#include <lib3270/log.h>
#include <lib3270/trace.h>

/*---[ Implement ]-------------------------------------------------------------------------------*/


static void net_connected(H3270 *hSession, int GNUC_UNUSED(fd), LIB3270_IO_FLAG GNUC_UNUSED(flag), void GNUC_UNUSED(*dunno))
{
	int 		err;
	socklen_t	len		= sizeof(err);

	if(hSession->xio.write) {
		trace("%s write=%p",__FUNCTION__,hSession->xio.write);
		lib3270_remove_poll(hSession, hSession->xio.write);
		hSession->xio.write = NULL;
	}

	if(getsockopt(hSession->connection.sock, SOL_SOCKET, SO_ERROR, (char *) &err, &len) < 0)
	{
		lib3270_disconnect(hSession);
		lib3270_popup_dialog(
			hSession,
			LIB3270_NOTIFY_ERROR,
			_( "Network error" ),
			_( "Unable to get connection state." ),
			_( "%s" ), strerror(errno)
		);
		return;
	}
	else if(err)
	{
		char buffer[4096];

		snprintf(buffer,4095,_( "Can't connect to %s" ), lib3270_get_url(hSession) );

		lib3270_disconnect(hSession);
		lib3270_popup_dialog(
			hSession,
			LIB3270_NOTIFY_ERROR,
			_( "Connection failed" ),
			buffer,
			_( "%s" ), strerror(err)
		);
		return;
	}

	hSession->xio.except	= lib3270_add_poll_fd(hSession,hSession->connection.sock,LIB3270_IO_FLAG_EXCEPTION,net_exception,0);
	hSession->xio.read		= lib3270_add_poll_fd(hSession,hSession->connection.sock,LIB3270_IO_FLAG_READ,net_input,0);

#if defined(HAVE_LIBSSL)
	if(hSession->ssl.con && hSession->ssl.state == LIB3270_SSL_UNDEFINED)
	{
		if(ssl_negotiate(hSession))
			return;
	}
#endif

	lib3270_setup_session(hSession);
	lib3270_set_connected_initial(hSession);

}

 struct resolver
 {
	const char 			* message;
 };

 static int background_connect(H3270 *hSession, void *host)
 {

	struct addrinfo	  hints;
 	struct addrinfo * result	= NULL;
	struct addrinfo * rp		= NULL;

	memset(&hints,0,sizeof(hints));
	hints.ai_family 	= AF_UNSPEC;	// Allow IPv4 or IPv6
	hints.ai_socktype	= SOCK_STREAM;	// Stream socket
	hints.ai_flags		= AI_PASSIVE;	// For wildcard IP address
	hints.ai_protocol	= 0;			// Any protocol

	status_resolving(hSession);

 	int rc = getaddrinfo(hSession->host.current, hSession->host.srvc, &hints, &result);
 	if(rc != 0)
	{
		((struct resolver *) host)->message = gai_strerror(rc);
		return -1;
	}

	status_connecting(hSession);

	for(rp = result; hSession->connection.sock < 0 && rp != NULL; rp = rp->ai_next)
	{
		hSession->connection.sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(hSession->connection.sock < 0)
		{
			((struct resolver *) host)->message = strerror(errno);
			continue;
		}

		// Connected!
		if(connect(hSession->connection.sock, rp->ai_addr, rp->ai_addrlen))
		{
			SOCK_CLOSE(hSession);
			((struct resolver *) host)->message = strerror(errno);
			continue;
		}

	}

	freeaddrinfo(result);

	return 0;

 }

 int net_reconnect(H3270 *hSession, int seconds)
 {
	struct resolver		  host;
	memset(&host,0,sizeof(host));

	// Connect to host
	if(lib3270_run_task(hSession, background_connect, &host) || hSession->connection.sock < 0)
	{
		char buffer[4096];
		snprintf(buffer,4095,_( "Can't connect to %s:%s"), hSession->host.current, hSession->host.srvc);

		lib3270_popup_dialog(	hSession,
								LIB3270_NOTIFY_ERROR,
								_( "Connection error" ),
								buffer,
								"%s",
								host.message);

		lib3270_set_disconnected(hSession);
		return errno = ENOTCONN;
	}

	/* don't share the socket with our children */
	(void) fcntl(hSession->connection.sock, F_SETFD, 1);

	hSession->ever_3270 = False;

#if defined(HAVE_LIBSSL)
	if(hSession->ssl.enabled)
	{
		hSession->ssl.host = 1;
		if(ssl_init(hSession))
			return errno = ENOTCONN;

	}
#endif // HAVE_LIBSSL

	// set options for inline out-of-band data and keepalives
	int optval = 1;
	if (setsockopt(hSession->connection.sock, SOL_SOCKET, SO_OOBINLINE, (char *)&optval,sizeof(optval)) < 0)
	{
		int rc = errno;
		lib3270_popup_dialog(	hSession,
								LIB3270_NOTIFY_ERROR,
								_( "Connection error" ),
								_( "setsockopt(SO_OOBINLINE) has failed" ),
								"%s",
								strerror(rc));
		SOCK_CLOSE(hSession);
		return rc;
	}

	optval = lib3270_get_toggle(hSession,LIB3270_TOGGLE_KEEP_ALIVE) ? 1 : 0;
	if (setsockopt(hSession->connection.sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&optval, sizeof(optval)) < 0)
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
		SOCK_CLOSE(hSession);
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

	hSession->xio.write = lib3270_add_poll_fd(hSession,hSession->connection.sock,LIB3270_IO_FLAG_WRITE,net_connected,0);
	// hSession->ns_write_id = AddOutput(hSession->sock, hSession, net_connected);

	trace("%s: Connection in progress",__FUNCTION__);

	if(seconds)
	{
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
			case LIB3270_RESOLVING:
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
	}

	return 0;

 }

