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
#include "private.h"
#include "telnetc.h"
#include <errno.h>
#include <lib3270/trace.h>

#if defined(HAVE_LIBSSL)
	#include <openssl/err.h>
#endif

/*---[ Implement ]-------------------------------------------------------------------------------*/

 LIB3270_EXPORT int lib3270_connect_url(H3270 *hSession, const char *url, int wait)
 {
	CHECK_SESSION_HANDLE(hSession);

	if(url && *url)
	{
		lib3270_set_url(hSession,url);
	}

	return lib3270_reconnect(hSession, wait);

 }

#ifdef SSL_ENABLE_CRL_CHECK
static int background_ssl_crl_check(H3270 *hSession, void *ssl_error)
{
	return lib3270_check_X509_crl(hSession, (SSL_ERROR_MESSAGE *) ssl_error);
}
#endif // SSL_ENABLE_CRL_CHECK

 int lib3270_reconnect(H3270 *hSession, int seconds)
 {
 	debug("%s",__FUNCTION__);

 	FAIL_IF_ONLINE(hSession);

	//
	// Can't reconnect if already reconnecting *OR* there's an open popup
	// (to avoid open more than one connect error popup.
	//
	if(hSession->auto_reconnect_inprogress || hSession->popups)
		return errno = EAGAIN;

	if(hSession->sock > 0)
		return errno = EISCONN;

	if(!(hSession->host.current && hSession->host.srvc))
	{
		// No host info, try the default one.
        if(lib3270_set_url(hSession,NULL))
		{
			int err = errno;
			lib3270_trace_event(hSession,"Can't set default URL (%s)\n",strerror(err));
			return errno = err;
		}

		if(!(hSession->host.current && hSession->host.srvc))
		{
			return errno = EINVAL;
		}
	}

#ifdef SSL_ENABLE_CRL_CHECK
	SSL_ERROR_MESSAGE ssl_error;
	memset(&ssl_error,0,sizeof(ssl_error));

	set_ssl_state(hSession,LIB3270_SSL_NEGOTIATING);
	int rc = lib3270_run_task(hSession, background_ssl_crl_check, &ssl_error);
	if(rc)
	{
		if(ssl_error.description)
			lib3270_popup_dialog(hSession, LIB3270_NOTIFY_ERROR, ssl_error.title, ssl_error.text, "%s", ssl_error.description);
		else
			lib3270_popup_dialog(hSession, LIB3270_NOTIFY_ERROR, ssl_error.title, ssl_error.text, "%s", ERR_reason_error_string(ssl_error.error));

		return errno = rc;
	}
#endif // SSL_ENABLE_CRL_CHECK

#if defined(HAVE_LIBSSL)
	set_ssl_state(hSession,LIB3270_SSL_UNDEFINED);
#endif // HAVE_LIBSSL

	snprintf(hSession->full_model_name,LIB3270_FULL_MODEL_NAME_LENGTH,"IBM-327%c-%d",hSession->m3279 ? '9' : '8', hSession->model_num);

	lib3270_trace_event(hSession,"Reconnecting to %s\n",lib3270_get_url(hSession));

	hSession->ever_3270	= False;
	hSession->ssl.host  = 0;

	return net_reconnect(hSession,seconds);

 }

