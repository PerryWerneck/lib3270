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
  * @brief Common methods for send/recv errors.
  *
  */

 #include <config.h>
 #include <lib3270.h>
 #include <lib3270/log.h>
 #include <internals.h>
 #include <networking.h>
 #include <fcntl.h>
 #ifdef _WIN32
	#include <lib3270/win32.h>
 #endif // _WIN32

/*--[ Implement ]------------------------------------------------------------------------------------*/

 int lib3270_socket_recv_failed(H3270 *hSession) {

#ifdef _WIN32

	int wsaError = WSAGetLastError();

	// EWOULDBLOCK & EAGAIN should return directly.
	if(wsaError == WSAEWOULDBLOCK)
		return -EWOULDBLOCK;

	if(wsaError == WSAEINPROGRESS)
		return -EAGAIN;

	int rc = -wsaError;

	LIB3270_POPUP popup = {
		.name = "RecvFailed",
		.type = LIB3270_NOTIFY_ERROR,
		.summary = _("Error receiving data from host"),
	};

	// TODO: Translate WSA Error, update message body.

	lib3270_popup(hSession,&popup,0);

#else

	// EWOULDBLOCK & EAGAIN should return directly.
	if(errno == EWOULDBLOCK || errno == EAGAIN)
		return -errno;

	// Network error, notify user
	int rc = -errno;

	lib3270_autoptr(char) body = lib3270_strdup_printf(
										_("The system error code was %d (%s)"),
										errno,
										strerror(errno)
								);

	LIB3270_POPUP popup = {
		.name = "RecvFailed",
		.type = LIB3270_NOTIFY_ERROR,
		.summary = _("Error receiving data from host"),
		.body = body
	};

	lib3270_popup(hSession,&popup,0);

#endif // _WIN32

	return rc;

 }

 int lib3270_socket_send_failed(H3270 *hSession) {

 #ifdef _WIN32

	int rc = WSAGetLastError();

	lib3270_popup_dialog(
		hSession,
		LIB3270_NOTIFY_ERROR,
		NULL,
		_("Erro sending data to host"),
		_( "The system error was %s (%d)" ),
		lib3270_win32_strerror(rc),
		rc
	);

 #else

	int rc = errno;

	switch(rc) {
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


 #endif // _WIN32

	return -1;

 }

int lib3270_socket_set_non_blocking(H3270 *hSession, int sock, const unsigned char on) {

	if(sock < 0)
		return 0;

#ifdef WIN32

		WSASetLastError(0);
		u_long iMode= on ? 1 : 0;

		if(ioctlsocket(sock,FIONBIO,&iMode))
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

	if ((f = fcntl(sock, F_GETFL, 0)) == -1)
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

	if (fcntl(sock, F_SETFL, f) < 0)
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

	debug("Socket %d is now %s",sock,(on ? "Non Blocking" : "Blocking"));

	return 0;

}

 static const char * crl_download_protocols[] = {
 	NULL,
	"http",
	"https",
#ifdef HAVE_LDAP
	"ldap",
	"ldaps"
#endif // HAVE_LDAP
 };

 const char * lib3270_crl_get_preferred_protocol(const H3270 *hSession)
 {
 	debug("%s: selected: %d",__FUNCTION__,(int) hSession->ssl.crl_preferred_protocol);
 	if(hSession->ssl.crl_preferred_protocol < (sizeof(crl_download_protocols)/sizeof(crl_download_protocols[0])))
		return crl_download_protocols[hSession->ssl.crl_preferred_protocol];

	errno = EINVAL;
	return NULL;
 }

 int lib3270_crl_set_preferred_protocol(H3270 *hSession, const char *protocol)
 {
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
