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
 * References:
 *
 * https://docs.microsoft.com/en-us/windows/win32/winhttp/winhttp-autoproxy-api
 *
 */

/**
 * @brief Implements CRL download using winhttp.
 *
 */

#include <config.h>
#include "private.h"

#if defined(HAVE_LIBSSL) && defined(SSL_ENABLE_CRL_CHECK)

#include <winhttp.h>
#include <utilc.h>

/*--[ Implement ]------------------------------------------------------------------------------------*/

X509_CRL * get_crl_using_http(H3270 *hSession, SSL_ERROR_MESSAGE * message, const char *consturl)
{
	size_t szResponse = 0;
	lib3270_autoptr(char) httpText = lib3270_get_from_url(hSession, consturl, &szResponse, &message->text);

	if(!httpText)
	{
		message->error = hSession->ssl.error = 0;
		message->title = _( "Security error" );
		trace_ssl(
			hSession,"Can't get %s: %s\n",
				consturl,
				message->description ? message->description : "Undefined message"
		);
		return NULL;
	}

	// Copy the pointer because d2i_X509_CRL changes the value!!!
	const unsigned char *crl_data = (const unsigned char *) httpText;

	X509_CRL * x509_crl = NULL;
	if(!d2i_X509_CRL(&x509_crl,&crl_data, (DWORD) szResponse))
	{
		message->error = hSession->ssl.error = ERR_get_error();
		message->title = _( "Security error" );
		message->text = _( "Can't decode certificate revocation list" );
		lib3270_write_log(hSession,"ssl","%s: %s",consturl, message->text);

		trace_ssl(
			hSession,"%s: %s\n",
				consturl,
				message->text
		);

		return NULL;
	}

	trace_ssl(hSession,"Got CRL from %s\n",consturl);

	return x509_crl;

}

#endif //  defined(HAVE_LIBSSL) && defined(SSL_ENABLE_CRL_CHECK)
