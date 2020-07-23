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
#include <lib3270/popup.h>

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

	printf("\n\nMSG-CODE=%d\n\n",msg->code);

	if(msg->code)
	{
		if(msg->popup->body)
		{
			popup = lib3270_popup_clone_printf(
						msg->popup,
						_( "%s\nThe SSL error message was \"%s\"(%d)" ),
						dgettext(GETTEXT_PACKAGE,msg->popup->body),
						ERR_reason_error_string(msg->code),
						msg->code
					);
		}
		else
		{
			popup = lib3270_popup_clone_printf(
						msg->popup,
						_( "The SSL error message was \"%s\" (%d)" ),
						ERR_reason_error_string(msg->code),
						msg->code
					);
		}

	}
#ifdef _WIN32
	else if(msg->lasterror)
	{
		lib3270_autoptr(char) windows_error = lib3270_win32_translate_error_code(msg->lasterror);

		if(msg->popup->body)
		{
			popup = lib3270_popup_clone_printf(
						msg->popup,
						_( "%s\nThe windows error was \"%s\" (%u)" ),
						dgettext(GETTEXT_PACKAGE,msg->popup->body),
						windows_error,
						(unsigned int) msg->lasterror
					);
		}
		else
		{
			popup = lib3270_popup_clone_printf(
						msg->popup,
						_( "Windows error was \"%s\" (%u)" ),
						windows_error,
						(unsigned int) msg->lasterror
					);
		}

	}
#endif // _WIN32
	else if(rc)
	{
		if(msg->popup->body)
		{
			popup = lib3270_popup_clone_printf(
						msg->popup,
						_( "%s\nThe operating system error was \"%s\" (%u)" ),
						dgettext(GETTEXT_PACKAGE,msg->popup->body),
						strerror(rc),
						rc
					);
		}
		else
		{
			popup = lib3270_popup_clone_printf(
						msg->popup,
						_( "The operating system error was \"%s\" (%u)" ),
						strerror(rc),
						rc
					);
		}

	}
	else
	{
		popup = lib3270_malloc(sizeof(LIB3270_POPUP));
		*popup = *msg->popup;

		if(msg->popup->body)
			popup->body = dgettext(GETTEXT_PACKAGE,msg->popup->body);

	}

	popup->summary = dgettext(GETTEXT_PACKAGE,msg->popup->summary);

	if(popup->title)
		popup->title = dgettext(GETTEXT_PACKAGE,popup->title);
	else
		popup->title = _("Your connection is not safe");

	popup->label = _("Continue");
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

	LIB3270_POPUP * popup = translate_ssl_error_message(msg,0);
	hSession->cbk.popup_show(hSession,popup,0);
	lib3270_free(popup);

}

#endif // defined(HAVE_LIBSSL)
