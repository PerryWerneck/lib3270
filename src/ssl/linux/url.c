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
 *
 * References:
 *
 * http://www.openssl.org/docs/ssl/
 * https://stackoverflow.com/questions/4389954/does-openssl-automatically-handle-crls-certificate-revocation-lists-now
 *
 */

#include <config.h>

#if defined(HAVE_LIBSSL) && defined(SSL_ENABLE_CRL_CHECK) && defined(HAVE_LIBCURL)

#include "private.h"
#include <curl/curl.h>
#include <lib3270/toggle.h>

#define CRL_DATA_LENGTH 2048

/*--[ Implement ]------------------------------------------------------------------------------------*/

static inline void lib3270_autoptr_cleanup_BIO(BIO **ptr)
{
	debug("%s(%p)",__FUNCTION__,*ptr);
	if(*ptr)
		BIO_free_all(*ptr);
	*ptr = NULL;
}

LIB3270_INTERNAL X509_CRL * get_crl_using_url(H3270 *hSession, SSL_ERROR_MESSAGE * message, const char *consturl)
{
	X509_CRL * x509_crl = NULL;

	size_t szText = 0;
	const char * error_message = NULL;
	lib3270_autoptr(char) httpText = lib3270_get_from_url(hSession, consturl, &szText, &error_message);

	if(!httpText)
	{
		LIB3270_POPUP_DESCRIPTOR popup = {
			.type = LIB3270_NOTIFY_SECURE,
			.name = "SSL-CantGetCRL",
			.summary = N_( "Error getting certificate revocation list" ),
			.body = error_message
		};
		message->popup = &popup;
		return NULL;
	}

	if(lib3270_get_toggle(hSession,LIB3270_TOGGLE_SSL_TRACE))
		lib3270_trace_data(hSession,"CRL Data",(const unsigned char *) httpText, (unsigned int) szText);

	if(strncasecmp(consturl,"ldap://",7) == 0)
	{
		// It's an LDAP query, assumes a base64 data.
		char * data = strstr((char *) httpText,":: ");
		if(!data)
		{
			static const LIB3270_POPUP_DESCRIPTOR popup = {
				.type = LIB3270_NOTIFY_SECURE,
				.summary = N_( "Got a bad formatted certificate revocation list from LDAP server" )
			};

			message->code = hSession->ssl.error = ERR_get_error();
			message->popup = &popup;
			lib3270_write_log(hSession,"ssl","%s: invalid format:\n%s\n", consturl, httpText);
			errno = EINVAL;
			return NULL;
		}
		data += 3;

		lib3270_autoptr(BIO) bio = BIO_new_mem_buf(httpText,-1);

		BIO * b64 = BIO_new(BIO_f_base64());
		bio = BIO_push(b64, bio);

		BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

		if(!d2i_X509_CRL_bio(bio, &x509_crl))
		{
			static const LIB3270_POPUP_DESCRIPTOR popup = {
				.type = LIB3270_NOTIFY_SECURE,
				.summary = N_( "Can't decode certificate revocation list got from LDAP server" )
			};

			message->code = hSession->ssl.error = ERR_get_error();
			message->popup = &popup;

			lib3270_write_log(hSession,"ssl","%s: %s",consturl, popup.summary);
			errno = EINVAL;
			return NULL;
		}

	}
	else
	{
		// CRL File, convert it
		// Copy the pointer because d2i_X509_CRL changes the value!!!
		const unsigned char *crl_data = (const unsigned char *) httpText;

		if(!d2i_X509_CRL(&x509_crl, &crl_data, szText))
		{
			static const LIB3270_POPUP_DESCRIPTOR popup = {
				.type = LIB3270_NOTIFY_SECURE,
				.summary = N_( "Can't decode certificate revocation list" )
			};

			message->code = hSession->ssl.error = ERR_get_error();
			message->popup = &popup;
			lib3270_write_log(hSession,"ssl","%s: %s",consturl, popup.summary);
			return NULL;
		}

	}

	return x509_crl;

}

#endif // HAVE_LIBSSL && SSL_ENABLE_CRL_CHECK && HAVE_LIBCURL
