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

#include "../private.h"
#include <errno.h>

#include <ws2tcpip.h>

#ifdef HAVE_ICONV
	#include <iconv.h>
#endif // HAVE_ICONV

#define SOCK_CLOSE(s)	closesocket(s->sock); s->sock = -1;

//#include "statusc.h"
#include "hostc.h"
#include "trace_dsc.h"
//#include "utilc.h"
#include "telnetc.h"
#include "screen.h"

#include <lib3270/internals.h>

/*---[ Implement ]-------------------------------------------------------------------------------*/


//static void net_connected(H3270 *hSession)
static void net_connected(H3270 *hSession, int fd unused, LIB3270_IO_FLAG flag unused, void *dunno unused)
{
	int 		err;
	socklen_t	len		= sizeof(err);

	if(hSession->xio.write) {
		trace("%s write=%p",__FUNCTION__,hSession->xio.write);
		lib3270_remove_poll(hSession, hSession->xio.write);
		hSession->xio.write = NULL;
	}

	if(getsockopt(hSession->sock, SOL_SOCKET, SO_ERROR, (char *) &err, &len) < 0)
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
		char buffer[4096];
		snprintf(buffer,4095,_( "Can't connect to %s" ), hSession->host.current );

		lib3270_disconnect(hSession);
		lib3270_popup_dialog(	hSession,
								LIB3270_NOTIFY_ERROR,
								_( "Connection failed" ),
								buffer,
								_( "%s"), lib3270_win32_strerror(err)
							);
		trace("%s",__FUNCTION__);
		return;
	}

	hSession->xio.except	= lib3270_add_poll_fd(hSession,hSession->sock,LIB3270_IO_FLAG_EXCEPTION,net_exception,0);
	hSession->xio.read		= lib3270_add_poll_fd(hSession,hSession->sock,LIB3270_IO_FLAG_READ,net_input,0);

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
								N_( "Network startup error" ),
								N_( "WSAStartup failed" ),
								"%s", lib3270_win32_strerror(GetLastError()) );

		_exit(1);
	}

	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		lib3270_popup_dialog(	session,
								LIB3270_NOTIFY_CRITICAL,
								N_( "Network startup error" ),
								N_( "Bad winsock version" ),
								N_( "Can't use winsock version %d.%d" ), LOBYTE(wsaData.wVersion), HIBYTE(wsaData.wVersion));
		_exit(1);
	}
}

LIB3270_EXPORT int lib3270_connect_url(H3270 *hSession, const char *url, int wait)
{
	CHECK_SESSION_HANDLE(hSession);

	if(url && *url)
	{
		lib3270_set_url(hSession,url);
	}

	return lib3270_reconnect(hSession, wait);

}

 struct resolver
 {
 	int			  convert;
	const char 	* message;
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

 	int rc = getaddrinfo(hSession->host.current, hSession->host.srvc, &hints, &result);
 	if(rc != 0)
	{
		((struct resolver *) host)->message = gai_strerror(rc);
		((struct resolver *) host)->convert = 1;
		return -1;
	}

	status_connecting(hSession,1);

	for(rp = result; hSession->sock < 0 && rp != NULL; rp = rp->ai_next)
	{
		hSession->sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(hSession->sock < 0)
		{
			((struct resolver *) host)->message = strerror(errno);
			continue;
		}

		// Connected!
		if(connect(hSession->sock, rp->ai_addr, rp->ai_addrlen))
		{
			SOCK_CLOSE(hSession);
			((struct resolver *) host)->message = strerror(errno);
			continue;
		}

	}

	freeaddrinfo(result);

	return 0;

}

int lib3270_reconnect(H3270 *hSession, int seconds)
{
 	int					  optval;
	struct resolver		  host;

	CHECK_SESSION_HANDLE(hSession);

	memset(&host,0,sizeof(host));

	lib3270_main_iterate(hSession,0);

	if(hSession->auto_reconnect_inprogress)
		return errno = EAGAIN;

	if(hSession->sock > 0)
		return errno = EBUSY;

	sockstart(hSession);

#if defined(HAVE_LIBSSL)
	set_ssl_state(hSession,LIB3270_SSL_UNSECURE);
#endif // HAVE_LIBSSL

	snprintf(hSession->full_model_name,LIB3270_FULL_MODEL_NAME_LENGTH,"IBM-327%c-%d",hSession->m3279 ? '9' : '8', hSession->model_num);

	hSession->ever_3270	= False;
	hSession->cstate = LIB3270_RESOLVING;

	lib3270_st_changed(hSession, LIB3270_STATE_RESOLVING, True);

	// s = getaddrinfo(hSession->host.current, hSession->host.srvc, &hints, &result);
	if(lib3270_run_task(hSession, background_connect, &host) || hSession->sock < 0)
	{
		char buffer[4096];
		char msg[4096];

		snprintf(buffer,4095,_( "Can't connect to %s:%s"), hSession->host.current, hSession->host.srvc);

		strncpy(msg,host.message,4095);

#ifdef HAVE_ICONV
		if(host.convert)
		{
			char	* ptr = msg;
			size_t	  out = 4096;
			size_t	  in	= strlen(host.message);

			iconv_t hConv = iconv_open(lib3270_win32_local_charset(),"UTF-8");
			if(iconv(
					hConv,
					&host.message,&in,
					&ptr,&out
				) == ((size_t) -1))
			{
				strncpy(msg,host.message,4095);
			}
			iconv_close(hConv);

		}
#endif // HAVE_ICONV

		lib3270_popup_dialog(	hSession,
								LIB3270_NOTIFY_ERROR,
								_( "Connection error" ),
								buffer,
								"%s",
								msg);

		lib3270_set_disconnected(hSession);
		return errno = ENOTCONN;
	}

	hSession->ever_3270 = False;
	hSession->ssl.host  = 0;

#if defined(HAVE_LIBSSL)
	if(hSession->ssl.enabled)
	{
		hSession->ssl.host = 1;
		ssl_init(hSession);
	}
#endif // HAVE_LIBSSL

	/* connect */

	WSASetLastError(0);
	// u_long iMode=1;

	optval = lib3270_get_toggle(hSession,LIB3270_TOGGLE_KEEP_ALIVE) ? 1 : 0;
	if (setsockopt(hSession->sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&optval, sizeof(optval)) < 0)
	{
		char buffer[4096];
		snprintf(buffer,4095,N_( "Can't %s network keep-alive" ), optval ? _( "enable" ) : _( "disable" ));

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
	if (setsockopt(hSession->sock, SOL_SOCKET, SO_OOBINLINE, (char *)&optval,sizeof(optval)) < 0)
	{
		lib3270_popup_dialog(	hSession,
								LIB3270_NOTIFY_ERROR,
								_( "Connection error" ),
								_( "setsockopt(SO_OOBINLINE) has failed" ),
								"%s", lib3270_win32_strerror(WSAGetLastError()));
		SOCK_CLOSE(hSession);
		return errno = ENOTCONN;
	}

	// set options for inline out-of-band data and keepalives

	/*
#if defined(OMTU)
	else if (setsockopt(hSession->sock, SOL_SOCKET, SO_SNDBUF, (char *)&mtu,sizeof(mtu)) < 0)
	{
		popup_a_sockerr(hSession, N_( "setsockopt(%s)" ), "SO_SNDBUF");
		SOCK_CLOSE(hSession);
	}
#endif

	*/

	// Connecting, set callbacks, wait for connection
	hSession->cstate = LIB3270_PENDING;
	lib3270_st_changed(hSession, LIB3270_STATE_HALF_CONNECT, True);

	hSession->xio.write = lib3270_add_poll_fd(hSession,hSession->sock,LIB3270_IO_FLAG_WRITE,net_connected,0);
	// hSession->ns_write_id = AddOutput(hSession->sock, hSession, net_connected);

	trace("%s: Connection in progress",__FUNCTION__);

	if(seconds)
	{
		time_t end = time(0)+seconds;

		while(time(0) < end)
		{
			lib3270_main_iterate(hSession,1);

			switch(hSession->cstate)
			{
			case LIB3270_PENDING:
			case LIB3270_CONNECTED_INITIAL:
			case LIB3270_CONNECTED_ANSI:
			case LIB3270_CONNECTED_3270:
			case LIB3270_CONNECTED_INITIAL_E:
			case LIB3270_CONNECTED_NVT:
			case LIB3270_CONNECTED_SSCP:
				break;

			case LIB3270_NOT_CONNECTED:
				return errno = ENOTCONN;

			case LIB3270_CONNECTED_TN3270E:
				return 0;

			default:
				lib3270_write_log(hSession,"connect", "%s: State changed to unexpected state %d",__FUNCTION__,hSession->cstate);
				return -1;
			}

		}

		lib3270_disconnect(hSession);
		lib3270_write_log(hSession,"connect", "%s: %s",__FUNCTION__,strerror(ETIMEDOUT));
		return errno = ETIMEDOUT;
	}

	return 0;

 }

