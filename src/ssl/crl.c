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
#include <lib3270.h>
#include <lib3270/log.h>
#include <trace_dsc.h>
#include <lib3270-internals.h>
#include <array.h>

#ifdef HAVE_LIBSSL
	#include <openssl/ssl.h>
	#include <openssl/err.h>
#endif // HAVE_LIBSSL

/*--[ Implement ]------------------------------------------------------------------------------------*/

#ifdef SSL_ENABLE_CRL_CHECK
int lib3270_get_crl_from_url(H3270 *hSession, void *ssl_error, const char *url)
{

	if(!(url && *url))
		return -1;

	// Invalidate current certificate.
	if(hSession->ssl.crl.cert)
	{
		trace_ssl(hSession,"%s\n","Discarding current CRL");
		X509_CRL_free(hSession->ssl.crl.cert);
		hSession->ssl.crl.cert = NULL;
	}

	//
	// Get the new CRL
	//
	// https://stackoverflow.com/questions/10510850/how-to-verify-the-certificate-for-the-ongoing-ssl-session
	//
	trace_ssl(hSession,"Getting new CRL from %s\n",url);

	hSession->ssl.crl.cert = lib3270_get_crl(hSession,(SSL_ERROR_MESSAGE *) ssl_error,url);

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
#endif // SSL_ENABLE_CRL_CHECK

#if !defined(SSL_DEFAULT_CRL_URL) && defined(SSL_ENABLE_CRL_CHECK)
int lib3270_get_crl_from_dist_points(H3270 *hSession, CRL_DIST_POINTS * dist_points, void *ssl_error)
{
	size_t ix;
	int i, gtype;
	lib3270_autoptr(LIB3270_STRING_ARRAY) uris = lib3270_string_array_new();

	// https://nougat.cablelabs.com/DLNA-RUI/openssl/commit/57912ed329f870b237f2fd9f2de8dec3477d1729

	for(ix = 0; ix < (size_t) sk_DIST_POINT_num(dist_points); ix++) {

		DIST_POINT *dp = sk_DIST_POINT_value(dist_points, ix);

		if(!dp->distpoint || dp->distpoint->type != 0)
			continue;

		GENERAL_NAMES *gens = dp->distpoint->name.fullname;

		for (i = 0; i < sk_GENERAL_NAME_num(gens); i++)
		{
			GENERAL_NAME *gen = sk_GENERAL_NAME_value(gens, i);
			ASN1_STRING *uri = GENERAL_NAME_get0_value(gen, &gtype);
			if(uri)
			{
				const unsigned char * data = ASN1_STRING_get0_data(uri);
				if(data)
				{
					lib3270_string_array_append(uris,(char *) data);
				}
			}

		}

	}

#ifdef DEBUG
	{
		for(ix = 0; ix < uris->length; ix++)
		{
			debug("%u: %s", (unsigned int) ix, uris->str[ix]);
		}
	}
#endif // DEBUG

	if(hSession->ssl.crl.url)
	{
		// Check if we already have the URL.
		if(!strcmp(hSession->ssl.crl.url,uris->str[ix]))
		{
			trace_ssl(hSession,"Keeping CRL from %s\n",hSession->ssl.crl.url);
			return 0;
		}

		// The URL is invalid or not to this cert, remove it!
		lib3270_free(hSession->ssl.crl.url);
		hSession->ssl.crl.url = NULL;
	}

	if(hSession->ssl.crl.prefer && *hSession->ssl.crl.prefer)
	{
		size_t length = strlen(hSession->ssl.crl.prefer);

		for(ix = 0; ix < uris->length; ix++)
		{
			if(!strncmp(uris->str[ix],hSession->ssl.crl.prefer,length))
			{
				trace_ssl(hSession,"Trying preferred URL %s\n",uris->str[ix]);
				if(lib3270_get_crl_from_url(hSession, ssl_error, uris->str[ix]) == 0)
					return 0;
			}

		}

	}

	// Can't load, try all of them.
	for(ix = 0; ix < uris->length; ix++)
	{
		trace_ssl(hSession,"Trying CRL from %s\n",uris->str[ix]);
		if(lib3270_get_crl_from_url(hSession, ssl_error, uris->str[ix]) == 0)
			return 0;
	}

	return -1;
}
#endif // !SSL_DEFAULT_CRL_URL && SSL_ENABLE_CRL_CHECK
