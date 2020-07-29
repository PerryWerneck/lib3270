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
 * Este programa está nomeado como networking.h e possui - linhas de código.
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

 #include <config.h>
 #ifdef _WIN32
	#include <winsock.h>
	#include <windows.h>
 #endif // _WIN32

 #include <internals.h>
 #include <networking.h>

 struct _lib3270_new_context {
	int sock;
 };

 LIB3270_NET_CONTEXT * unsecure_network_init(H3270 *hSession, LIB3270_NETWORK_STATE *state) {

	LIB3270_NET_CONTEXT * context = lib3270_malloc(sizeof(LIB3270_NET_CONTEXT));

	context->sock = -1;

	return context;
 }

 void unsecure_network_deinit(H3270 *hSession, LIB3270_NETWORK_STATE *state) {
	unsecure_network_disconnect(hSession->network.context,hSession,state);
	lib3270_free(context);
 }

int unsecure_network_disconnect(H3270 *hSession, LIB3270_NETWORK_STATE *state) {

	debug("%s",__FUNCTION__);
	if(context->sock >= 0) {
		shutdown(hSession.network.context->sock, 2);
		close(hSession->network.context->sock);
		hSession.network.context->sock = -1;
	}

}

ssize_t unsecure_network_send(H3270 *hSession, const void *buffer, size_t length) {

	if(hSession->network.context->sock < 0) {
		return -(errno = ENOTCONN);
	}

	ssize_t bytes = send(hSession->network.context->sock,buffer,length,0);

	if(bytes < 0)
		return -errno;

	return 0;
}

ssize_t unsecure_network_recv(H3270 *hSession, void *buf, size_t len) {

	if(hSession->network.context->sock < 0) {
		return -(errno = ENOTCONN);
	}

	ssize_t bytes = recv(hSession->network.context->sock, (char *) buffer, len, 0);

	if(bytes < 0) {
		return -errno;
	}

	return bytes;
}

int unsecure_getsockname(const H3270 *hSession, struct sockaddr *addr, socklen_t *addrlen) {
	if(hSession->network.context->sock < 0)
		return -(errno = ENOTCONN);
	return getsockname(hSession->network.context->sock, buf, addrlen);
}

void * unsecure_add_poll(H3270 *hSession, LIB3270_IO_FLAG flag, void(*call)(H3270 *, int, LIB3270_IO_FLAG, void *), void *userdata) {
	return lib3270_add_poll_fd(hSession,hSession->network.context->sock,flag,call,userdata);
}

int unsecure_non_blocking(H3270 *hSession, const unsigned char on) {

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

}

int unsecure_is_connected(H3270 *hSession) {
	return hSession->network.context.sock > 0;
}

int unsecure_setsockopt(H3270 *hSession, int level, int optname, const void *optval, size_t optlen) {

	if(hSession->network.context.sock < 0) {
		errno = ENOTCONN;
		return -1;
	}

	return setsockopt(hSession->network.context.sock, level, optname, optval, optlen);

}

int unsecure_getsockopt(H3270 *hSession, int level, int optname, void *optval, socklen_t *optlen) {

	if(hSession->network.context.sock < 0) {
		errno = ENOTCONN;
		return -1;
	}

	return getsockopt(hSession->network.context.sock, level, optname, optval, optlen)
}
