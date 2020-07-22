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
#include "telnetc.h"
#include <errno.h>
#include <lib3270/log.h>
#include <lib3270/trace.h>
#include <lib3270/toggle.h>
#include <trace_dsc.h>

#include "../ssl/crl.h"
#include "utilc.h"

/*---[ Implement ]-------------------------------------------------------------------------------*/

 LIB3270_EXPORT int lib3270_connect_url(H3270 *hSession, const char *url, int seconds)
 {
	CHECK_SESSION_HANDLE(hSession);

	if(url && *url)
	{
		lib3270_set_url(hSession,url);
	}

	return lib3270_reconnect(hSession, seconds);

 }


#if defined(HAVE_LIBSSL)

 static int background_ssl_init(H3270 *hSession, void *ssl_error)
 {
	if(ssl_ctx_init(hSession, (SSL_ERROR_MESSAGE *) ssl_error))
		return -1;

#if defined(SSL_ENABLE_CRL_CHECK)
	lib3270_crl_free_if_expired(hSession);
#endif // defined(SSL_ENABLE_CRL_CHECK)

	return 0;
 }

#endif // HAVE_LIBSSL

 void connection_failed(H3270 *hSession, const char *message)
 {
	lib3270_disconnect(hSession);

	lib3270_autoptr(char) summary = lib3270_strdup_printf(
										_( "Can't connect to %s:%s"),
										hSession->host.current,
										hSession->host.srvc
									);

	LIB3270_POPUP popup = {
		.name = "CantConnect",
		.title = _( "Connection failed" ),
		.type = LIB3270_NOTIFY_INFO,
		.summary = summary,
		.body = message
	};

	if(hSession->cbk.popup_show(hSession,&popup,lib3270_get_toggle(hSession,LIB3270_TOGGLE_RECONNECT) && !hSession->auto_reconnect_inprogress) == 0) {
		// Schedule an automatic reconnection.
		hSession->auto_reconnect_inprogress = 1;
		(void) AddTimer(RECONNECT_ERR_MS, hSession, lib3270_check_for_auto_reconnect);
	}

 }

 int lib3270_allow_reconnect(const H3270 *hSession)
 {
	//
	// Can't reconnect if already reconnecting *OR* there's an open popup
	// (to avoid open more than one connect error popup).
	//
	if(hSession->auto_reconnect_inprogress)
	{
		errno = EBUSY;
		return 0;
	}

 	// Is the session disconnected?
	if(!lib3270_is_disconnected(hSession))
	{
		errno = EISCONN;
		return 0;
	}

	// Do I have a defined host?
	if(!(hSession->host.current && hSession->host.srvc && *hSession->host.current && *hSession->host.srvc))
	{
		errno = EINVAL;
		return 0;
	}

	if(hSession->connection.sock > 0)
	{
		errno = EISCONN;
		return 0;
	}

	return 1;
 }

 int lib3270_reconnect(H3270 *hSession, int seconds)
 {
 	debug("%s",__FUNCTION__);

 	if(!lib3270_allow_reconnect(hSession))
	{
		return errno == 0 ? -1 : errno;
	}

#if defined(HAVE_LIBSSL)
	debug("%s: TLS/SSL is %s",__FUNCTION__,hSession->ssl.enabled ? "ENABLED" : "DISABLED")
	trace_dsn(hSession,"TLS/SSL is %s\n", hSession->ssl.enabled ? "enabled" : "disabled" );

	if(hSession->ssl.enabled)
	{
		SSL_ERROR_MESSAGE ssl_error;
		memset(&ssl_error,0,sizeof(ssl_error));

		set_ssl_state(hSession,LIB3270_SSL_NEGOTIATING);
		int rc = lib3270_run_task(hSession, background_ssl_init, &ssl_error);

		if(rc && popup_ssl_error(hSession, rc, &ssl_error))
			return errno = rc;

		set_ssl_state(hSession,LIB3270_SSL_UNDEFINED);
		hSession->ssl.host  = 0;
	}
#endif // HAVE_LIBSSL

	snprintf(hSession->full_model_name,LIB3270_FULL_MODEL_NAME_LENGTH,"IBM-327%c-%d",hSession->m3279 ? '9' : '8', hSession->model_num);

	lib3270_write_event_trace(hSession,"Reconnecting to %s\n",lib3270_get_url(hSession));

	hSession->ever_3270	= False;

	return net_reconnect(hSession,seconds);

 }

