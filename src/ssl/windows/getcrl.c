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

#if defined(HAVE_LIBSSL) && defined(SSL_ENABLE_CRL_CHECK)

#include "private.h"

/*--[ Implement ]------------------------------------------------------------------------------------*/

static inline void lib3270_autoptr_cleanup_FILE(FILE **file)
{
	if(*file)
		fclose(*file);
}

X509_CRL * lib3270_download_crl(H3270 *hSession, SSL_ERROR_MESSAGE * message, const char *consturl)
{
	X509_CRL * x509_crl = NULL;

	if(!(consturl && *consturl))
	{
		message->error = hSession->ssl.error = 0;
		message->title = _( "Security error" );
		message->text = _( "Can't open CRL File" );
		message->description = _("The URL for the CRL is undefined or empty");
		errno = ENOENT;
		return NULL;
	}

	if(strncasecmp(consturl,"file://",7) == 0)
	{
		lib3270_autoptr(FILE) hCRL = fopen(consturl+7,"r");

		if(!hCRL)
		{
			// Can't open CRL File.
			int err = errno;

			message->error = hSession->ssl.error = 0;
			message->title = _( "Security error" );
			message->text = _( "Can't open CRL File" );
			message->description = strerror(err);
			trace_ssl(hSession,"Can't open %s: %s\n",consturl,message->description);
			return NULL;

		}

		trace_ssl(hSession,"Loading CRL from %s\n",consturl+7);
		if(d2i_X509_CRL_fp(hCRL, &x509_crl))
		{
			message->error = hSession->ssl.error = ERR_get_error();
			message->title = _( "Security error" );
			message->text = _( "Can't decode CRL" );
			lib3270_write_log(hSession,"ssl","%s: %s",consturl, message->text);
			return NULL;
		}



	}
#ifdef HAVE_LDAP
	else if(strncasecmp(consturl,"ldap://",7) == 0 && strlen(consturl) > 8)
	{
		return get_crl_using_ldap(hSession, message, consturl);

	}
#endif // HAVE_LDAP
	else if(strncasecmp(consturl,"http://",7) == 0 && strlen(consturl) > 8)
	{
		return get_crl_using_http(hSession, message, consturl);
	}
	else
	{
#ifdef HAVE_LIBCURL

		return get_crl_using_curl(hSession, message, consturl);

#else
		// Can't get CRL.

		message->error = hSession->ssl.error = 0;
		message->title = _( "Security error" );
		message->text = _( "Unexpected or invalid CRL URL" );
		message->description = _("The URL scheme is unknown");
		lib3270_write_log(hSession,"ssl","%s: %s",consturl, message->description);
		errno = EINVAL;
		return NULL;

#endif // HAVE_LIBCURL
	}

	return x509_crl;

}

#endif // HAVE_LIBSSL && SSL_ENABLE_CRL_CHECK
