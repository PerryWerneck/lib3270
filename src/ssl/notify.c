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
#include <internals.h>
#include <lib3270/log.h>

/*--[ Implement ]------------------------------------------------------------------------------------*/

#if defined(HAVE_LIBSSL)

#include <openssl/err.h>

/**
 * @brief Translate strings from ssl error message.
 *
 * @param msg	SSL error message descriptor.
 * @param rc	Value of errno.
 *
 * @return Dynamically allocated popup description.
 *
 */
static LIB3270_POPUP * translate_ssl_error_message(const SSL_ERROR_MESSAGE *msg, int rc)
{
	LIB3270_POPUP * popup;

	if(msg->popup->body)
	{
		popup = lib3270_malloc(sizeof(LIB3270_POPUP));
		memcpy(popup,msg->popup,sizeof(LIB3270_POPUP));
		popup->body = dgettext(GETTEXT_PACKAGE,msg->popup->body);
	}
	else
	{
		lib3270_autoptr(char) body = NULL;
		if(msg->code)
		{
			body = lib3270_strdup_printf(_( "%s (SSL error %d)" ),ERR_reason_error_string(msg->code),msg->code);
		}
#ifdef _WIN32
		else if(msg->lasterror)
		{
			lib3270_autoptr(char) windows_error = lib3270_win32_translate_error_code(msg->lasterror);
			body = lib3270_strdup_printf(_( "Windows error was \"%s\" (%u)" ), windows_error,(unsigned int) msg->lasterror);
		}
#endif
		else if(rc) {
			body = lib3270_strdup_printf(_( "%s (rc=%d)" ),strerror(rc),rc);
		}

		popup = lib3270_malloc(sizeof(LIB3270_POPUP)+strlen(body)+1);
		memcpy(popup,msg->popup,sizeof(LIB3270_POPUP));
		popup->body = (char *) (popup+1);
		strcpy((char *) (popup+1),body);

	}

	if(popup->summary)
		popup->summary = dgettext(GETTEXT_PACKAGE,popup->summary);

	if(popup->title)
		popup->title = dgettext(GETTEXT_PACKAGE,popup->title);
	else
		popup->title = _("Security alert");

	return popup;
}


int popup_ssl_error(H3270 GNUC_UNUSED(*hSession), int rc, const SSL_ERROR_MESSAGE *msg)
{
	int response = 0;

	LIB3270_POPUP * popup = translate_ssl_error_message(msg,0);

#ifdef _WIN32

	lib3270_autoptr(char) rcMessage = lib3270_strdup_printf("The error code was %d",rc);

	const char *outMsg[] = {
		popup->title,
		popup->summary,
		(popup->body ? popup->body : ""),
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

	lib3270_write_log(hSession, "SSL", "%s %s (rc=%d)", popup->summary, (popup->body ? popup->body : ""), rc);

#endif // _WIN32

#ifdef SSL_ENABLE_NOTIFICATION_WHEN_FAILED

	response = hSession->cbk.popup_ssl_error(
							hSession,
							rc,
							popup->title,
							popup->summary,
							popup->body
						);


#endif // SSL_ENABLE_NOTIFICATION_WHEN_FAILED

	lib3270_free(popup);
	return response;

}

void ssl_popup_message(H3270 *hSession, const SSL_ERROR_MESSAGE *msg) {

	lib3270_autoptr(LIB3270_POPUP) * popup = translate_ssl_error_message(msg,0);
	hSession->cbk.popup_show(hSession,popup,0);

}

#endif // defined(HAVE_LIBSSL)
