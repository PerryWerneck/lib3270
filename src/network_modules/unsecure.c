/*
 * "Software PW3270, desenvolvido com base nos códigos fontes do WC3270  e  X3270
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
 * Este programa está nomeado como unsecure.c e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas de Mendonça)
 *
 */

 /**
  * @brief Default networking methods.
  *
  */

 #include "private.h"

 #include <sys/types.h>
 #include <sys/socket.h>
 #include <netdb.h>

 struct _lib3270_net_context {
	int sock;
 };

 static void unsecure_network_finalize(H3270 *hSession) {

	debug("%s",__FUNCTION__);

	if(hSession->network.context) {
		lib3270_free(hSession->network.context);
		hSession->network.context = NULL;
	}

 }

 static int unsecure_network_disconnect(H3270 *hSession) {

	debug("%s",__FUNCTION__);

	if(hSession->network.context->sock >= 0) {
		shutdown(hSession->network.context->sock, 2);
		close(hSession->network.context->sock);
		hSession->network.context->sock = -1;
	}

	return 0;
 }

 ssize_t unsecure_network_send(H3270 *hSession, const void *buffer, size_t length) {

	if(hSession->network.context->sock < 0) {
		return -(errno = ENOTCONN);
	}

	ssize_t bytes = send(hSession->network.context->sock,buffer,length,0);

	debug("%s bytes=%d",__FUNCTION__,(int) bytes);

	if(bytes >= 0)
		return bytes;

	int rc = errno;

	debug("%s: %s",__FUNCTION__,strerror(rc));

	switch(rc)
	{
	case EPIPE:
		lib3270_popup_dialog(
			hSession,
			LIB3270_NOTIFY_ERROR,
			NULL,
			_("Broken pipe"),
			_("The system error code was %d"),
			rc
		);
		break;

	case ECONNRESET:
		lib3270_popup_dialog(
			hSession,
			LIB3270_NOTIFY_ERROR,
			NULL,
			_("Connection reset by peer"),
			_("The system error code was %d"),
			rc
		);
		break;

	case EINTR:
		return 0;

	default:
		lib3270_popup_dialog(
			hSession,
			LIB3270_NOTIFY_ERROR,
			NULL,
			_("Unexpected error writing to network socket"),
			_("The system error code was %d (%s)"),
			rc, strerror(rc)
		);

	}

	return -rc;
 }

 static ssize_t unsecure_network_recv(H3270 *hSession, void *buf, size_t len) {

 	debug("%s",__FUNCTION__);

	if(hSession->network.context->sock < 0) {
		return -(errno = ENOTCONN);
	}

	ssize_t bytes = recv(hSession->network.context->sock, (char *) buf, len, 0);

	debug("%s bytes=%d",__FUNCTION__,(int) bytes);

	if(bytes < 0) {
		return -errno;
	}

	return bytes;
}

static int unsecure_network_getsockname(const H3270 *hSession, struct sockaddr *addr, socklen_t *addrlen) {
	if(hSession->network.context->sock < 0)
		return -(errno = ENOTCONN);
	return getsockname(hSession->network.context->sock, addr, addrlen);
}

static void * unsecure_network_add_poll(H3270 *hSession, LIB3270_IO_FLAG flag, void(*call)(H3270 *, int, LIB3270_IO_FLAG, void *), void *userdata) {
	return lib3270_add_poll_fd(hSession,hSession->network.context->sock,flag,call,userdata);
}

static int unsecure_network_non_blocking(H3270 *hSession, const unsigned char on) {

	if(hSession->network.context->sock < 0)
		return 0;

#ifdef WIN32

		WSASetLastError(0);
		u_long iMode= on ? 1 : 0;

		if(ioctlsocket(hSession->network.context->sock,FIONBIO,&iMode))
		{
			lib3270_popup_dialog(	hSession,
									LIB3270_NOTIFY_ERROR,
									_( "Connection error" ),
									_( "ioctlsocket(FIONBIO) failed." ),
									"%s", lib3270_win32_strerror(GetLastError()));
			return -1;
		}

#else

	int f;

	if ((f = fcntl(hSession->network.context->sock, F_GETFL, 0)) == -1)
	{
		lib3270_popup_dialog(	hSession,
								LIB3270_NOTIFY_ERROR,
								_( "Socket error" ),
								_( "fcntl() error when getting socket state." ),
								_( "%s" ), strerror(errno)
							);

		return -1;
	}

	if (on)
		f |= O_NDELAY;
	else
		f &= ~O_NDELAY;

	if (fcntl(hSession->network.context->sock, F_SETFL, f) < 0)
	{
		lib3270_popup_dialog(	hSession,
								LIB3270_NOTIFY_ERROR,
								_( "Socket error" ),
								on ? _( "Can't set socket to blocking mode." ) : _( "Can't set socket to non blocking mode" ),
								_( "%s" ), strerror(errno)
							);
		return -1;
	}

#endif

	debug("Socket %d is now %s",hSession->network.context->sock,(on ? "Non Blocking" : "Blocking"));

	return 0;
}

static int unsecure_network_is_connected(const H3270 *hSession) {
	return hSession->network.context->sock > 0;
}

static int unsecure_network_setsockopt(H3270 *hSession, int level, int optname, const void *optval, size_t optlen) {

	if(hSession->network.context->sock < 0) {
		errno = ENOTCONN;
		return -1;
	}

	return setsockopt(hSession->network.context->sock, level, optname, optval, optlen);

}

static int unsecure_network_getsockopt(H3270 *hSession, int level, int optname, void *optval, socklen_t *optlen) {

	if(hSession->network.context->sock < 0) {
		errno = ENOTCONN;
		return -1;
	}

	return getsockopt(hSession->network.context->sock, level, optname, optval, optlen);
}

static int unsecure_network_connect(H3270 *hSession, LIB3270_NETWORK_STATE *state) {

	hSession->network.context->sock = lib3270_network_connect(hSession, state);
	if(hSession->network.context->sock < 0)
		return hSession->network.context->sock;

	return 0;
}

static int unsecure_network_start_tls(H3270 GNUC_UNUSED(*hSession), LIB3270_NETWORK_STATE *msg) {

	if(hSession->ssl.required) {

		// TODO: Replace network module with the openssl version, initialize and execute start_tls on it.

		static const LIB3270_POPUP popup = {
			.type = LIB3270_NOTIFY_ERROR,
			.summary = N_("Can't activate SSL/TLS"),
			.body = N_("The protocol library was build without SSL/TLS support")
		};

		msg->popup = &popup;

		return ENOTSUP;

	}

	return 0;
}

void lib3270_set_default_network_module(H3270 *hSession) {

	static const LIB3270_NET_MODULE module = {
		.finalize = unsecure_network_finalize,
		.connect = unsecure_network_connect,
		.disconnect = unsecure_network_disconnect,
		.start_tls = unsecure_network_start_tls,
		.send = unsecure_network_send,
		.recv = unsecure_network_recv,
		.add_poll = unsecure_network_add_poll,
		.non_blocking = unsecure_network_non_blocking,
		.is_connected = unsecure_network_is_connected,
		.getsockname = unsecure_network_getsockname,
		.setsockopt = unsecure_network_setsockopt,
		.getsockopt = unsecure_network_getsockopt
	};

 	debug("%s",__FUNCTION__);

	if(hSession->network.context) {
		// Has context, finalize it.
		hSession->network.module->finalize(hSession);
	}

	hSession->network.context = lib3270_malloc(sizeof(LIB3270_NET_CONTEXT));
	memset(hSession->network.context,0,sizeof(LIB3270_NET_CONTEXT));
	hSession->network.context->sock = -1;

	hSession->network.module = &module;

}

