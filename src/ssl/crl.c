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
#include <lib3270/log.h>
#include <lib3270/trace.h>
#include <lib3270/toggle.h>
#include <trace_dsc.h>

#include "crl.h"

/*--[ Implement ]------------------------------------------------------------------------------------*/

#if defined(SSL_ENABLE_CRL_CHECK) && defined(HAVE_LIBSSL)

void lib3270_crl_free(H3270 *hSession)
{
	if(hSession->ssl.crl.cert)
	{
		X509_CRL_free(hSession->ssl.crl.cert);
		hSession->ssl.crl.cert = NULL;
	}

}

void lib3270_crl_free_if_expired(H3270 *hSession)
{
	if(!hSession->ssl.crl.cert)
		return;

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
			trace_ssl(hSession,"CRL is valid for %d day(s) and %d second(s)\n",day,sec);
			return;
		}

		trace_ssl(hSession,"Can't get CRL next update, discarding it\n");

	}
	else
	{
		trace_ssl(hSession,"CRL is no longer valid\n");
	}

	// Certificate is no longer valid, release it.
	X509_CRL_free(hSession->ssl.crl.cert);
	hSession->ssl.crl.cert = NULL;

}

int lib3270_crl_new_from_url(H3270 *hSession, void *ssl_error, const char *url)
{
	if(!(url && *url))
		return -1;

	lib3270_crl_free(hSession);	// Just in case!

	//
	// Get the new CRL
	//
	// https://stackoverflow.com/questions/10510850/how-to-verify-the-certificate-for-the-ongoing-ssl-session
	//
	trace_ssl(hSession,"Getting CRL from %s\n",url);

	hSession->ssl.crl.cert = lib3270_download_crl(hSession,(SSL_ERROR_MESSAGE *) ssl_error, url);

	if(hSession->ssl.crl.cert)
	{
		// Got CRL!

		// Update URL
		if(hSession->ssl.crl.url)
			lib3270_free(hSession->ssl.crl.url);

		hSession->ssl.crl.url = lib3270_strdup(url);

		// Add it to ssl store
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
			trace_ssl(hSession,"CRL was added to context cert store\n");
		}
		else
		{
			trace_ssl(hSession,"CRL was not added to context cert store\n");
		}

		return 0;
	}

	return -1;

}

#endif // SSL_ENABLE_CRL_CHECK && HAVE_LIBSSL
