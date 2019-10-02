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
#include <lib3270-internals.h>
#include "telnetc.h"
#include <errno.h>
#include <lib3270/log.h>
#include <lib3270/trace.h>
#include <lib3270/toggle.h>
#include <trace_dsc.h>

#if defined(HAVE_LIBSSL)
	#include <openssl/err.h>
#endif

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

#ifdef SSL_ENABLE_CRL_CHECK
static int background_ssl_crl_get(H3270 *hSession, void *ssl_error)
{
	if(ssl_ctx_init(hSession, (SSL_ERROR_MESSAGE *) ssl_error)) {
		return -1;
	}

	// Do I have X509 CRL?
	if(hSession->ssl.crl.cert)
	{
		// Ok, have it. Is it valid?

		// https://stackoverflow.com/questions/23407376/testing-x509-certificate-expiry-date-with-c
		// X509_CRL_get_nextUpdate is deprecated in openssl 1.1.0
		#if OPENSSL_VERSION_NUMBER < 0x10100000L
			const ASN1_TIME * next_update = X509_CRL_get_nextUpdate(hSession->ssl.crl.cert);
		#else
			const ASN1_TIME * next_update = X509_CRL_get0_nextUpdate(hSession->ssl.crl.cert);
		#endif

		if(X509_cmp_current_time(next_update) == 1)
		{
			int day, sec;
			if(ASN1_TIME_diff(&day, &sec, NULL, next_update))
			{
				trace_ssl(hSession,"CRL Certificate is valid for %d day(s) and %d second(s)\n",day,sec);
				return 0;
			}
			else
			{
				trace_ssl(hSession,"Can't get CRL next update, releasing it\n");
			}

		}
		else
		{
			trace_ssl(hSession,"CRL Certificate is no longer valid\n");
		}

		// Certificate is no longer valid, release it.
		X509_CRL_free(hSession->ssl.crl.cert);
		hSession->ssl.crl.cert = NULL;

	}

	//
	// Get CRL
	//
	// https://stackoverflow.com/questions/10510850/how-to-verify-the-certificate-for-the-ongoing-ssl-session
	//
	trace_ssl(hSession,"Getting CRL from %s\n",lib3270_get_crl_url(hSession));

	hSession->ssl.crl.cert = lib3270_get_crl(hSession,(SSL_ERROR_MESSAGE *) ssl_error,lib3270_get_crl_url(hSession));
	if(hSession->ssl.crl.cert)
	{
		// Got CRL, add it to ssl store
		if(lib3270_get_toggle(hSession,LIB3270_TOGGLE_SSL_TRACE))
		{
			lib3270_autoptr(char) text = lib3270_get_ssl_crl_text(hSession);

			if(text)
				trace_ssl(hSession,"\n%s\n",text);

		}

		// Add CRL in the store.
		X509_STORE *store = SSL_CTX_get_cert_store(ssl_ctx);
		if(X509_STORE_add_crl(store, hSession->ssl.crl.cert))
		{
			trace_ssl(hSession,"CRL was added to cert store\n");
		}
		else
		{
			trace_ssl(hSession,"CRL was not added to cert store\n");
		}


	}

	return 0;

}

static int notify_crl_error(H3270 *hSession, int rc, const SSL_ERROR_MESSAGE *message)
{
	lib3270_write_log(
		hSession,
		"SSL-CRL-GET",
		"CRL GET error: %s (rc=%d ssl_error=%d)",
			message->title,
			rc,
			message->error
	);

	if(message->description)
	{
		if(popup_ssl_error(hSession,rc,message->title,message->text,message->description))
			return rc;
	}
#ifdef _WIN32
	else if(message->lasterror)
	{
		lib3270_autoptr(char) windows_error = lib3270_win32_translate_error_code(message->lasterror);
		lib3270_autoptr(char) formatted_error = lib3270_strdup_printf(_( "Windows error was \"%s\" (%u)" ), windows_error,(unsigned int) message->lasterror);

		if(popup_ssl_error(hSession,rc,message->title,message->text,formatted_error))
			return rc;

	}
#endif // WIN32
	else if(message->error)
	{
		lib3270_autoptr(char) formatted_error = lib3270_strdup_printf(_( "%s (SSL error %d)" ),ERR_reason_error_string(message->error),message->error);
		lib3270_write_log(hSession,"SSL-CRL-GET","%s",formatted_error);

		if(popup_ssl_error(hSession,rc,message->title,message->text,formatted_error))
			return rc;
	}
	else
	{
		if(popup_ssl_error(hSession,rc,message->title,message->text,""))
			return rc;
	}

	return 0;
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
	int rc = lib3270_run_task(hSession, background_ssl_crl_get, &ssl_error);

	debug("CRL check returns %d",rc);

	if(rc && notify_crl_error(hSession, rc,&ssl_error))
		return errno = rc;

#endif // SSL_ENABLE_CRL_CHECK

#if defined(HAVE_LIBSSL)
	set_ssl_state(hSession,LIB3270_SSL_UNDEFINED);
	hSession->ssl.host  = 0;
#endif // HAVE_LIBSSL

	snprintf(hSession->full_model_name,LIB3270_FULL_MODEL_NAME_LENGTH,"IBM-327%c-%d",hSession->m3279 ? '9' : '8', hSession->model_num);

	lib3270_trace_event(hSession,"Reconnecting to %s\n",lib3270_get_url(hSession));

	hSession->ever_3270	= False;

	return net_reconnect(hSession,seconds);

 }

