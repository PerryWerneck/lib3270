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

/*
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
//		.title = _( "Connection failed" ),
		.type = LIB3270_NOTIFY_INFO,
		.summary = summary,
		.body = message,
		.label = _("Try again")
	};

	if(hSession->cbk.popup(hSession,&popup,!hSession->auto_reconnect_inprogress) == 0)
		lib3270_activate_auto_reconnect(hSession,1000);

 }
*/

 int lib3270_allow_reconnect(const H3270 *hSession)
 {
	//
	// Can't reconnect if already reconnecting *OR* there's an open popup
	// (to avoid open more than one connect error popup).
	//
	if(hSession->auto_reconnect_inprogress)
	{
		debug("%s: auto_reconnect_inprogress",__FUNCTION__);
		errno = EBUSY;
		return 0;
	}

 	// Is the session disconnected?
	if(!lib3270_is_disconnected(hSession))
	{
		debug("%s: is_disconnected=FALSE",__FUNCTION__);
		errno = EISCONN;
		return 0;
	}

	// Do I have a defined host?
	if(!(hSession->host.current && hSession->host.srvc && *hSession->host.current && *hSession->host.srvc))
	{
		errno = EINVAL;
		return 0;
	}

	if(hSession->network.module->is_connected(hSession))
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

	debug("%s: TLS/SSL is %s",__FUNCTION__,hSession->ssl.host ? "ENABLED" : "DISABLED")
	trace_dsn(hSession,"TLS/SSL is %s\n", hSession->ssl.host ? "enabled" : "disabled" );

	set_ssl_state(hSession,LIB3270_SSL_UNDEFINED);

	snprintf(hSession->full_model_name,LIB3270_FULL_MODEL_NAME_LENGTH,"IBM-327%c-%d",hSession->m3279 ? '9' : '8', hSession->model_num);
	lib3270_write_event_trace(hSession,"Reconnecting to %s\n",lib3270_get_url(hSession));

	hSession->ever_3270	= False;

	return net_reconnect(hSession,seconds);

 }

 void lib3270_notify_tls(H3270 *hSession) {

	// Negotiation complete is the connection secure?
	if(hSession->ssl.message->type != LIB3270_NOTIFY_INFO) {

		// Ask user what I can do!
		if(lib3270_popup_translated(hSession,(const LIB3270_POPUP *) hSession->ssl.message,1) == ECANCELED) {
			lib3270_disconnect(hSession);
		}

	}

 }

 int lib3270_start_tls(H3270 *hSession)
 {
	hSession->ssl.message = NULL;	// Reset message.
	set_ssl_state(hSession,LIB3270_SSL_NEGOTIATING);

	non_blocking(hSession,False);

	int rc = lib3270_run_task(
			hSession,
			(int(*)(H3270 *h, void *)) hSession->network.module->start_tls,
			NULL
		);

	if(rc == ENOTSUP) {

		// No support for TLS/SSL in the active network module, the connection is insecure
		set_ssl_state(hSession,LIB3270_SSL_UNSECURE);
		return 0;

	}

	// The network module SHOULD set the status message.
	if(!hSession->ssl.message) {

		static const LIB3270_POPUP message = {
			.type = LIB3270_NOTIFY_CRITICAL,
			.summary = N_( "Can't determine the TLS/SSL state"),
			.body = N_("The network module didn't set the TLS/SSL state message, this is not supposed to happen and can be a coding error")
		};

		set_ssl_state(hSession,LIB3270_SSL_UNSECURE);
		lib3270_popup_translated(hSession,&message,0);
		return EINVAL;

	}

	if(rc) {

		// Negotiation has failed. Will disconnect
		set_ssl_state(hSession,LIB3270_SSL_UNSECURE);

		if(hSession->ssl.message) {
			lib3270_popup_translated(hSession,(const LIB3270_POPUP *) hSession->ssl.message,0);
		}

		return rc;
	}

	set_ssl_state(hSession,(hSession->ssl.message->type == LIB3270_NOTIFY_INFO ? LIB3270_SSL_SECURE : LIB3270_SSL_NEGOTIATED));
	non_blocking(hSession,True);

	return 0;
 }


