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

#include <config.h>
#include <internals.h>
#include <errno.h>
#include <lib3270.h>
#include <lib3270/popup.h>
#include <lib3270/trace.h>
#include <private/trace.h>
#include <lib3270/log.h>
#include <lib3270/trace.h>
#include <private/network.h>

#ifdef HAVE_LIBSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif // HAVE_LIBSSL

/*--[ Implement ]------------------------------------------------------------------------------------*/

LIB3270_EXPORT int lib3270_is_secure(const H3270 *hSession) {
	return lib3270_get_ssl_state(hSession) == LIB3270_SSL_SECURE;
}

LIB3270_EXPORT LIB3270_SSL_STATE lib3270_get_ssl_state(const H3270 *hSession) {
	return hSession->ssl.state;
}

void set_ssl_state(H3270 *hSession, LIB3270_SSL_STATE state) {
	if(state == hSession->ssl.state)
		return;

	hSession->ssl.state = state;
	trace_dsn(hSession,"SSL state changes to %d\n",(int) state);
	debug("SSL state changes to %d\n",(int) state);

	hSession->cbk.update_ssl(hSession,hSession->ssl.state);
}

LIB3270_EXPORT const char * lib3270_get_ssl_state_message(const H3270 *hSession) {
	if(hSession->ssl.message.summary && *hSession->ssl.message.summary) {
		return hSession->ssl.message.summary;
	}
	return _( "The connection is insecure" );
}

LIB3270_EXPORT const char * lib3270_get_ssl_state_icon_name(const H3270 *hSession) {
	if(hSession->ssl.message.icon && *hSession->ssl.message.icon) {
		return hSession->ssl.message.icon;
	}
	return "dialog-error";
}

LIB3270_EXPORT const char * lib3270_get_ssl_state_description(const H3270 *hSession) {
	if(hSession->ssl.message.body && *hSession->ssl.message.body) {
		return hSession->ssl.message.body;
	}
	return "";
}

LIB3270_EXPORT char * lib3270_get_ssl_crl_text(const H3270 *hSession) {

//	if(hSession->network.module && hSession->network.module->getcrl)
//		return hSession->network.module->getcrl(hSession);

	errno = ENOTSUP;
	return NULL;
}

LIB3270_EXPORT char * lib3270_get_ssl_peer_certificate_text(const H3270 *hSession) {

//	if(hSession->network.module && hSession->network.module->getcert)
//		return hSession->network.module->getcert(hSession);

	errno = ENOTSUP;
	return NULL;
}
