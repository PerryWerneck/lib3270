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

LIB3270_EXPORT int lib3270_popup(H3270 *hSession, const LIB3270_POPUP *popup, unsigned char wait) {
	return hSession->cbk.popup(hSession,popup,wait);
}

LIB3270_EXPORT int lib3270_popup_translated(H3270 *hSession, const LIB3270_POPUP *popup, unsigned char wait) {

	LIB3270_POPUP translated = *popup;

	if(popup->title) {
		translated.title = dgettext(GETTEXT_PACKAGE,popup->title);
	}

	if(popup->label) {
		translated.label = dgettext(GETTEXT_PACKAGE,popup->label);
	}

	if(popup->summary) {
		translated.summary = dgettext(GETTEXT_PACKAGE,popup->summary);
	}

	if(popup->body) {
		translated.body = dgettext(GETTEXT_PACKAGE,popup->body);
	}

	return hSession->cbk.popup(hSession,&translated,wait);
}

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

	hSession->cbk.popup(hSession,&popup,0);

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

	hSession->cbk.popup(hSession,&popup,0);

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

	hSession->cbk.popup(hSession,&popup,0);

}

LIB3270_POPUP * lib3270_popup_clone_printf(const LIB3270_POPUP *origin, const char *fmt, ...)
{
	va_list args;

	// Create body
	lib3270_autoptr(char) body = NULL;

	va_start(args, fmt);
	body = lib3270_vsprintf(fmt, args);
	va_end(args);

	// Alocate new struct
	LIB3270_POPUP * popup = lib3270_malloc(sizeof(LIB3270_POPUP)+strlen(body)+1);

	if(origin)
	{
		*popup = *origin;
	}
	else
	{
		memset(popup,0,sizeof(LIB3270_POPUP));
	}

	strcpy((char *)(popup+1),body);
	popup->body = (char *)(popup+1);
	return popup;
}

static int def_popup(H3270 *hSession, const LIB3270_POPUP *popup, unsigned char GNUC_UNUSED wait)
{
	const char * text[] = {
		popup->title,
		popup->summary,
		popup->body
	};

	size_t ix;

	for(ix = 0; ix < (sizeof(text)/sizeof(text[0])); ix++)
	{
		if(text[ix])
			lib3270_write_log(hSession,"popup","%s",text[ix]);
	}

	return ENOTSUP;
}


LIB3270_EXPORT void lib3270_set_popup_handler(H3270 *hSession, int (*handler)(H3270 *, const LIB3270_POPUP *, unsigned char wait)) {

	if(handler)
		hSession->cbk.popup = handler;
	else
		hSession->cbk.popup = def_popup;


}
