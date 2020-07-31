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
 * Este programa está nomeado como connect.c e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

#include <config.h>

// Compiling for WinXP or later: Expose getaddrinfo()/freeaddrinfo().
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include <internals.h>
#include <errno.h>
#include <lib3270/trace.h>
#include <lib3270/log.h>
#include <lib3270/toggle.h>

#ifdef HAVE_ICONV
	#include <iconv.h>
#endif // HAVE_ICONV

#define SOCK_CLOSE(s)	closesocket(s->connection.sock); s->connection.sock = -1;

#include "hostc.h"
#include "trace_dsc.h"
#include "telnetc.h"
#include "screen.h"

#include <lib3270/internals.h>

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
		lib3270_popup_dialog(	hSession,
								LIB3270_NOTIFY_ERROR,
								_( "Network error" ),
								_( "Unable to get connection state." ),
								"%s", lib3270_win32_strerror(WSAGetLastError())
							);
		return;
	}
	else if(err)
	{
		lib3270_autoptr(char) body = lib3270_strdup_printf(_("%s (rc=%d)"),strerror(err),err);

		connection_failed(hSession,body);
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

static void sockstart(H3270 *session)
{
	static int initted = 0;
	WORD wVersionRequested;
	WSADATA wsaData;

	if (initted)
		return;

	initted = 1;

	wVersionRequested = MAKEWORD(2, 2);

	if (WSAStartup(wVersionRequested, &wsaData) != 0)
	{
		lib3270_popup_dialog(	session,
								LIB3270_NOTIFY_CRITICAL,
								_( "Network startup error" ),
								_( "WSAStartup failed" ),
								"%s", lib3270_win32_strerror(GetLastError()) );

		_exit(1);
	}

	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		lib3270_popup_dialog(	session,
								LIB3270_NOTIFY_CRITICAL,
								_( "Network startup error" ),
								_( "Bad winsock version" ),
								_( "Can't use winsock version %d.%d" ), LOBYTE(wsaData.wVersion), HIBYTE(wsaData.wVersion));
		_exit(1);
	}
}

 struct resolver
 {
 	int			  rc;
 	int			  convert;
	const char 	* message;
 };

 static int background_connect(H3270 *hSession, void *host)
 {
	struct addrinfo	  hints;
 	struct addrinfo * result	= NULL;
	struct addrinfo * rp		= NULL;

	if(!(hSession->host.current && hSession->host.srvc))
		return errno = ENOENT;

	memset(&hints,0,sizeof(hints));
	hints.ai_family 	= AF_UNSPEC;	// Allow IPv4 or IPv6
	hints.ai_socktype	= SOCK_STREAM;	// Stream socket
	hints.ai_flags		= AI_PASSIVE;	// For wildcard IP address
	hints.ai_protocol	= 0;			// Any protocol

	debug("%s(%s,%s)",__FUNCTION__,hSession->host.current, hSession->host.srvc);

	status_resolving(hSession);

 	int rc = getaddrinfo(hSession->host.current, hSession->host.srvc, &hints, &result);
 	debug("getaddrinfo(%s,%s) returns %d",hSession->host.current,hSession->host.srvc,rc);

 	if(rc != 0)
	{
		((struct resolver *) host)->rc 		= rc;
		((struct resolver *) host)->message = gai_strerror(rc);
		((struct resolver *) host)->convert = 1;
		return -1;
	}

	status_connecting(hSession);

	for(rp = result; hSession->connection.sock < 0 && rp != NULL; rp = rp->ai_next)
	{
		hSession->connection.sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

		if(hSession->connection.sock < 0)
		{
			((struct resolver *) host)->rc = errno;
			((struct resolver *) host)->message = strerror(errno);
			debug("Socket error %d: %s",((struct resolver *) host)->rc,((struct resolver *) host)->message);
			continue;
		}

		// Connected!
		if(connect(hSession->connection.sock, rp->ai_addr, rp->ai_addrlen))
		{
			((struct resolver *) host)->rc = errno;
			((struct resolver *) host)->message = strerror(errno);
			debug("Connection error %d: %s",((struct resolver *) host)->rc,((struct resolver *) host)->message);
			SOCK_CLOSE(hSession);
			continue;
		}

	}

	freeaddrinfo(result);

	debug("%s: Connected using socket %d",__FUNCTION__,hSession->connection.sock);

	return 0;

}

int net_reconnect(H3270 *hSession, int seconds)
{
	struct resolver host;

	memset(&host,0,sizeof(host));

	sockstart(hSession);

	if(lib3270_run_task(hSession, background_connect, &host) || hSession->connection.sock < 0)
	{
		if(host.message)
		{
			// Have windows message, convert charset.

			char msg[4096];
			snprintf(msg,sizeof(msg),"%s (RC=%d)",host.message,host.rc);

			if(hEventLog)
			{
				// Register on event log
				lib3270_autoptr(char) username = lib3270_get_user_name();

				const char *outMsg[] = {
					username,
					"networking",
					msg
				};

				ReportEvent(
					hEventLog,
					EVENTLOG_ERROR_TYPE,
					1,
					0,
					NULL,
					(sizeof(outMsg)/sizeof(outMsg[0])),
					0,
					outMsg,
					NULL
				);

			}


#ifdef HAVE_ICONV
			if(host.convert)
			{
				char	* ptr = msg;
				size_t	  out = 4096;
				size_t	  in	= strlen(host.message);

				iconv_t hConv = iconv_open("UTF-8",lib3270_win32_local_charset());
				if(iconv(
						hConv,
						(ICONV_CONST char *) host.message,
						&in,
						&ptr,&out
					) == ((size_t) -1))
				{
					strncpy(msg,host.message,4095);
				}
				iconv_close(hConv);

			}
#endif // HAVE_ICONV

			connection_failed(hSession,msg);

		}
		else
		{
			connection_failed(hSession,NULL);
		}

		lib3270_set_disconnected(hSession);
		return errno = ENOTCONN;
	}

	hSession->ever_3270 = False;

#if defined(HAVE_LIBSSL)
	hSession->ssl.host  = 0;

	if(hSession->ssl.enabled)
	{
		hSession->ssl.host = 1;
		if(ssl_init(hSession))
			return errno = ENOTCONN;

	}
#endif // HAVE_LIBSSL

	/* connect */

	WSASetLastError(0);

	int optval = lib3270_get_toggle(hSession,LIB3270_TOGGLE_KEEP_ALIVE) ? 1 : 0;
	if (setsockopt(hSession->connection.sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&optval, sizeof(optval)) < 0)
	{
		char buffer[4096];
		snprintf(buffer,4095,_( "Can't %s network keep-alive" ), optval ? _( "enable" ) : _( "disable" ));

		lib3270_popup_dialog(	hSession,
								LIB3270_NOTIFY_ERROR,
								_( "Connection error" ),
								buffer,
								"%s", lib3270_win32_strerror(WSAGetLastError()));
		SOCK_CLOSE(hSession);
		return errno = ENOTCONN;
	}
	else
	{
		trace_dsn(hSession,"Network keep-alive is %s\n",optval ? "enabled" : "disabled" );
	}

	optval = 1;
	if (setsockopt(hSession->connection.sock, SOL_SOCKET, SO_OOBINLINE, (char *)&optval,sizeof(optval)) < 0)
	{
		lib3270_popup_dialog(	hSession,
								LIB3270_NOTIFY_ERROR,
								_( "Connection error" ),
								_( "setsockopt(SO_OOBINLINE) has failed" ),
								"%s", lib3270_win32_strerror(WSAGetLastError()));
		SOCK_CLOSE(hSession);
		return errno = ENOTCONN;
	}

	// Connecting, set callbacks, wait for connection
	lib3270_set_cstate(hSession, LIB3270_PENDING);
	lib3270_st_changed(hSession, LIB3270_STATE_HALF_CONNECT, True);

	hSession->xio.write = lib3270_add_poll_fd(hSession,hSession->connection.sock,LIB3270_IO_FLAG_WRITE,net_connected,0);

	trace("%s: Connection in progress",__FUNCTION__);

	if(seconds)
	{
		int rc = lib3270_wait_for_cstate(hSession,LIB3270_CONNECTED_TN3270E,seconds);
		if(rc)
		{
			lib3270_disconnect(hSession);
			lib3270_write_log(hSession,"connect", "%s: %s",__FUNCTION__,strerror(rc));
			return errno = rc;
		}
	}

	/*
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
				return 0;

			default:
				lib3270_write_log(hSession,"connect", "%s: State changed to unexpected state %d",__FUNCTION__,hSession->connection.state);
				return -1;
			}

		}

		lib3270_disconnect(hSession);
		lib3270_write_log(hSession,"connect", "%s: %s",__FUNCTION__,strerror(ETIMEDOUT));
		return errno = ETIMEDOUT;
	}
	*/

	return 0;

 }

