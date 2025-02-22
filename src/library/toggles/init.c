/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2008 Banco do Brasil S.A.
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

/*
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */


/**
 *	@file	toggles/init.c
 *	@brief	Initialize toggles.
 */

#include <errno.h>
#include <sys/types.h>

#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#endif // !WIN32

#include <config.h>
#include <lib3270/toggle.h>
#include <internals.h>

#include <private/ansi.h>
#include <private/ctlr.h>
#include <private/popup.h>
#include "screenc.h"
#include <private/trace.h>
#include <private/session.h>
#include <private/toggle.h>
#include <private/util.h>
#include <lib3270/log.h>

/*---[ Implement ]------------------------------------------------------------------------------------------------------------*/

static void toggle_altscreen(H3270 *session, const struct lib3270_toggle *t, LIB3270_TOGGLE_TYPE GNUC_UNUSED(tt)) {
	if(!session->screen_alt)
		set_viewsize(session,t->value ? 24 : session->max.rows,80);
}

static void toggle_redraw(H3270 *session, const struct lib3270_toggle GNUC_UNUSED(*t), LIB3270_TOGGLE_TYPE GNUC_UNUSED(tt)) {
	session->cbk.display(session);
}

/**
 * @brief No-op toggle.
 */
static void toggle_nop(H3270 GNUC_UNUSED(*session), const struct lib3270_toggle GNUC_UNUSED(*t), LIB3270_TOGGLE_TYPE GNUC_UNUSED(tt)) {
}

static void toggle_keepalive(H3270 *hSession, const struct lib3270_toggle GNUC_UNUSED(*t), LIB3270_TOGGLE_TYPE GNUC_UNUSED(tt)) {
/*
	if(hSession->network.module->is_connected(hSession)) {
		// Has network connection, update keep-alive option
		int optval = t->value ? 1 : 0;

		if(hSession->network.module->setsockopt(hSession, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) < 0) {
			popup_a_sockerr(hSession, _( "Can't %s network keep-alive" ), optval ? _( "enable" ) : _( "disable" ));
		} else {
			trace_dsn(hSession,"Network keep-alive is %s\n",optval ? "enabled" : "disabled" );
		}
	}
*/
}

static void toggle_connect(H3270 *hSession, const struct lib3270_toggle *toggle, LIB3270_TOGGLE_TYPE tt) {
	if(toggle->value && tt != LIB3270_TOGGLE_TYPE_INITIAL && lib3270_is_disconnected(hSession)) {
		if(lib3270_activate_auto_reconnect(hSession, 100)) {
			lib3270_log_write(hSession,"3270","Auto-connect fails: %s",strerror(errno));
		}
	}
}

static void toggle_ssl_trace(H3270 *hSession, const struct lib3270_toggle *toggle, LIB3270_TOGGLE_TYPE tt) {

	if(tt != LIB3270_TOGGLE_TYPE_INITIAL && toggle->value) {

		trace_ssl(hSession,
					"SSL build options:\n" \
					"  self signed cert check is %s" \
					"\n  CRL check is %s" \
					"\n  CRL expiration check is %s" \
					"\n  Notify when failed is %s\n",

#ifdef SSL_ENABLE_SELF_SIGNED_CERT_CHECK
					"on",
#else
					"off",
#endif

#ifdef SSL_ENABLE_CRL_CHECK
					"on",
#else
					"off",
#endif

#ifdef SSL_ENABLE_CRL_EXPIRATION_CHECK
					"on",
#else
					"off",
#endif

#ifdef SSL_ENABLE_NOTIFICATION_WHEN_FAILED
					"on"
#else
					"off"
#endif
		);

	}

}


/**
 * @brief Called from system initialization code to handle initial toggle settings.
 */
void initialize_toggles(H3270 *session) {
	static const struct _upcalls {
		LIB3270_TOGGLE_ID	id;
		void (*upcall)(H3270 *session, const struct lib3270_toggle *t, LIB3270_TOGGLE_TYPE tt);
	}
	upcalls[LIB3270_TOGGLE_COUNT] = {
		{
			LIB3270_TOGGLE_RECTANGLE_SELECT,
			toggle_rectselect

		},
		{
			LIB3270_TOGGLE_MONOCASE,
			toggle_redraw
		},
		{
			LIB3270_TOGGLE_UNDERLINE,
			toggle_redraw

		},
		{
			LIB3270_TOGGLE_ALTSCREEN,
			toggle_altscreen

		},
		{
			LIB3270_TOGGLE_KEEP_ALIVE,
			toggle_keepalive
		},
		{
			LIB3270_TOGGLE_CONNECT_ON_STARTUP,
			toggle_connect
		},
		{
			LIB3270_TOGGLE_SSL_TRACE,
			toggle_ssl_trace
		}

	};

	unsigned int f;

	// Set defaults
	for(f=0; f<LIB3270_TOGGLE_COUNT; f++) {
		session->toggle[f].upcall = toggle_nop;
		session->toggle[f].value = toggle_descriptor[f].def;
	}

	// Load upcalls
	for(f=0; f<(sizeof(upcalls)/sizeof(upcalls[0])); f++)
		session->toggle[upcalls[f].id].upcall = upcalls[f].upcall;

#ifdef _WIN32
	{
		lib3270_auto_cleanup(HKEY) hKey = 0;
		DWORD disp = 0;
		LSTATUS	rc = RegCreateKeyEx(
		                 HKEY_LOCAL_MACHINE,
		                 "Software\\" LIB3270_STRINGIZE_VALUE_OF(PRODUCT_NAME) "\\toggles",
		                 0,
		                 NULL,
		                 REG_OPTION_NON_VOLATILE,
		                 KEY_QUERY_VALUE|KEY_READ,
		                 NULL,
		                 &hKey,
		                 &disp);

		if(rc == ERROR_SUCCESS) {
			debug("%s: Loading toggles from registry",__FUNCTION__);
			for(f=0; f<LIB3270_TOGGLE_COUNT; f++) {
				DWORD val 		= 0;
				DWORD cbData    = sizeof(DWORD);

				DWORD dwRet = RegQueryValueEx(
				                  hKey,
				                  lib3270_toggle_get_from_id(f)->name,
				                  NULL,
				                  NULL,
				                  (LPBYTE) &val,
				                  &cbData
				              );

				debug("get(%s)=%d",lib3270_toggle_get_from_id(f)->name,(int) dwRet);
				if(dwRet == ERROR_SUCCESS) {
					debug("toggle.%s=%s",lib3270_toggle_get_from_id(f)->name,val ? "True" : "False");
					session->toggle[f].value = (val ? True : False);
				}

			}
		}

	}
#endif // _WIN32

	// Initialize upcalls.
	for(f=0; f<LIB3270_TOGGLE_COUNT; f++) {
		if(session->toggle[f].value)
			session->toggle[f].upcall(session,&session->toggle[f],LIB3270_TOGGLE_TYPE_INITIAL);
	}

}

/**
 * @brief Called from system exit code to handle toggles.
 */
void shutdown_toggles(H3270 *session) {
#if defined(X3270_TRACE)
	static const LIB3270_TOGGLE_ID disable_on_shutdown[] = {LIB3270_TOGGLE_DS_TRACE, LIB3270_TOGGLE_EVENT_TRACE, LIB3270_TOGGLE_SCREEN_TRACE};

	size_t f;

	for(f=0; f< (sizeof(disable_on_shutdown)/sizeof(disable_on_shutdown[0])); f++)
		lib3270_set_toggle(session,disable_on_shutdown[f],0);

#endif
}
