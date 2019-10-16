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
 *
 */


#include <config.h>
#include <lib3270-internals.h>
#include <lib3270/log.h>

/*--[ Implement ]------------------------------------------------------------------------------------*/

#if defined(HAVE_LIBSSL)

#include <openssl/err.h>

int popup_ssl_error(H3270 GNUC_UNUSED(*hSession), int rc, const char GNUC_UNUSED(*title), const char *summary, const char *body)
{
#ifdef _WIN32

	lib3270_autoptr(char) rcMessage = lib3270_strdup_printf("The error code was %d",rc);

	const char *outMsg[] = {
		title,
		summary,
		(body ? body : ""),
		rcMessage
	};

	ReportEvent(
		hEventLog,
		EVENTLOG_ERROR_TYPE,
		1,
		0,
		NULL,
		(sizeof(outMsg)/sizeof(outMsg[0])),
		0,
		outMsg,
		NULL
	);

#else

	lib3270_write_log(hSession, "SSL", "%s %s (rc=%d)", summary, (body ? body : ""), rc);

#endif // _WIN32

#ifdef SSL_ENABLE_NOTIFICATION_WHEN_FAILED

	return hSession->cbk.popup_ssl_error(hSession,rc,title,summary,body);

#else

	return 0;

#endif // SSL_ENABLE_NOTIFICATION_WHEN_FAILED
}

int notify_ssl_error(H3270 *hSession, int rc, const SSL_ERROR_MESSAGE *message)
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

#endif // defined(HAVE_LIBSSL)
