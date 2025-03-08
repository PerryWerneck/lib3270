/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) <2008> <Banco do Brasil S.A.>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
#include <lib3270/memory.h>
#include <lib3270/log.h>
#include <private/trace.h>
#include <private/popup.h>

/*--[ Implement ]------------------------------------------------------------------------------------*/

LIB3270_EXPORT int lib3270_popup(H3270 *hSession, const LIB3270_POPUP *popup, unsigned char wait) {

	if(hSession->trace && lib3270_get_toggle(hSession,LIB3270_TOGGLE_EVENT_TRACE)) {

		trace_event(hSession,"%s",popup->name);

		if(popup->title && *popup->title) {
			trace_event(hSession," - %s\n",popup->title);
		} else {
			trace_event(hSession,"\n");
		}

		if(popup->summary && *popup->summary) {
			trace_event(hSession,"\t%s\n",popup->summary);
		}

		if(popup->body && *popup->body) {
			trace_event(hSession,"\t%s\n",popup->body);
		}

	}

	if(hSession->log) {
		lib3270_log_write(hSession,"popup","%s - %s",popup->name,popup->summary ? popup->summary : "");
	}

	return hSession->cbk.popup(hSession,popup,wait);
}


/// @brief Pop up an error dialog.
void popup_an_error(H3270 *hSession, const char *fmt, ...) {

	lib3270_autoptr(char) summary = NULL;

	if(fmt) {
		va_list	args;
		va_start(args, fmt);
		summary = lib3270_vsprintf(fmt,args);
		va_end(args);
	}

	LIB3270_POPUP popup = {
		.type = LIB3270_NOTIFY_ERROR,
		.summary = summary
	};

	lib3270_popup(hSession,&popup,0);

}

void popup_system_error(H3270 *hSession, const char *title, const char *summary, const char *fmt, ...) {

	lib3270_autoptr(char) body = NULL;

	if(fmt) {
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

	lib3270_popup(hSession,&popup,0);

}

LIB3270_EXPORT void lib3270_popup_dialog(H3270 *session, LIB3270_NOTIFY id, const char *title, const char *message, const char *fmt, ...) {
	va_list	args;
	va_start(args, fmt);
	lib3270_popup_va(session, id, title, message, fmt, args);
	va_end(args);
}

LIB3270_EXPORT void lib3270_popup_va(H3270 *hSession, LIB3270_NOTIFY id, const char *title, const char *message, const char *fmt, va_list args) {

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

	lib3270_popup(hSession,&popup,0);

}

LIB3270_POPUP * lib3270_popup_clone_printf(const LIB3270_POPUP *origin, const char *fmt, ...) {
	va_list args;

	// Create body
	lib3270_autoptr(char) body = NULL;

	va_start(args, fmt);
	body = lib3270_vsprintf(fmt, args);
	va_end(args);

	// Alocate new struct
	LIB3270_POPUP * popup = lib3270_malloc(sizeof(LIB3270_POPUP)+strlen(body)+1);

	if(origin) {
		*popup = *origin;
	} else {
		memset(popup,0,sizeof(LIB3270_POPUP));
	}

	strcpy((char *)(popup+1),body);
	popup->body = (char *)(popup+1);
	return popup;
}

static int def_popup(H3270 *hSession, const LIB3270_POPUP *popup, unsigned char GNUC_UNUSED wait) {
#ifdef DEBUG 
	const char * text[] = {
		popup->name,
		popup->title,
		popup->summary,
		popup->body
	};

	size_t ix;

	for(ix = 0; ix < (sizeof(text)/sizeof(text[0])); ix++) {
		if(text[ix] && *text[ix]) {
			debug("---> %s",text[ix]);
		}
	}
#endif // DEBUG

	return ENOTSUP;
}


LIB3270_EXPORT void lib3270_set_popup_handler(H3270 *hSession, int (*handler)(H3270 *, const LIB3270_POPUP *, unsigned char wait)) {

	if(handler)
		hSession->cbk.popup = handler;
	else
		hSession->cbk.popup = def_popup;

}


