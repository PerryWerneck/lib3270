/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270. Registro no INPI sob o nome G3270.
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

/**
 *	@file popup.c
 *
 *	@brief A callback based popup dialog engine.
 *
 */

#include <config.h>
#include <internals.h>
#include <lib3270.h>
#include <lib3270/log.h>

/*--[ Implement ]------------------------------------------------------------------------------------*/

/// @brief Pop up an error dialog.
void popup_an_error(H3270 *hSession, const char *fmt, ...)
{

	lib3270_autoptr(char) summary = NULL;

	if(fmt)
	{
		va_list	args;
		va_start(args, fmt);
		summary = lib3270_vsprintf(fmt,args);
		va_end(args);
	}

	LIB3270_POPUP popup = {
		.type = LIB3270_NOTIFY_ERROR,
		.summary = summary
	};

	hSession->cbk.popup_show(hSession,&popup,0);

}

void popup_system_error(H3270 *hSession, const char *title, const char *summary, const char *fmt, ...)
{

	lib3270_autoptr(char) body = NULL;

	if(fmt)
	{
		va_list	args;
		va_start(args, fmt);
		body = lib3270_vsprintf(fmt,args);
		va_end(args);
	}

	LIB3270_POPUP popup = {
		.type = LIB3270_NOTIFY_ERROR,
		.title = title,
		.summary = summary,
		.body = body
	};

	hSession->cbk.popup_show(hSession,&popup,0);

}

LIB3270_EXPORT void lib3270_popup_dialog(H3270 *session, LIB3270_NOTIFY id , const char *title, const char *message, const char *fmt, ...)
{
	va_list	args;
	va_start(args, fmt);
    lib3270_popup_va(session, id, title, message, fmt, args);
	va_end(args);
}

LIB3270_EXPORT void lib3270_popup_va(H3270 *hSession, LIB3270_NOTIFY id , const char *title, const char *message, const char *fmt, va_list args)
{
	CHECK_SESSION_HANDLE(hSession);

	lib3270_autoptr(char) body = NULL;

	if(fmt) {
		body = lib3270_vsprintf(fmt,args);
	}

	LIB3270_POPUP popup = {
		.type = id,
		.title = title,
		.summary = message,
		.body = body
	};

	hSession->cbk.popup_show(hSession,&popup,0);

}
