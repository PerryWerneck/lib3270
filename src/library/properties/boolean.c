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
 * @brief Implements the boolean properties.
 *
 */

#include <config.h>
#include <internals.h>
#include <string.h>
#include <lib3270.h>
#include <lib3270/properties.h>
#include <lib3270/keyboard.h>
#include <lib3270/selection.h>
#include <lib3270/ssl.h>

int lib3270_is_starting(const H3270 *hSession) {
	return hSession->starting != 0;
}

LIB3270_EXPORT int lib3270_ssl_set_crl_download(H3270 *hSession, int enabled) {
	FAIL_IF_ONLINE(hSession);
	hSession->ssl.download_crl = (enabled ? 1 : 0);
	return 0;
}

LIB3270_EXPORT int lib3270_ssl_get_crl_download(const H3270 *hSession) {
	return hSession->ssl.download_crl;
}

const LIB3270_INT_PROPERTY * lib3270_get_boolean_properties_list(void) {

	static const LIB3270_INT_PROPERTY properties[] = {
		{
			.name = "ready",												//  Property name.
			.description = N_( "Is terminal ready" ),						//  Property description.
			.get = lib3270_is_ready,										//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "connected",											//  Property name.
			.description = N_( "Is terminal connected" ),					//  Property description.
			.get = lib3270_is_connected,									//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "secure",												//  Property name.
			.description = N_( "Is connection secure" ),					//  Property description.
			.get = lib3270_is_secure,										//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "tso",													//  Property name.
			.group = LIB3270_ACTION_GROUP_OFFLINE,							// Property group.
			.description = N_( "Non zero if the host is TSO." ),			//  Property description.
			.get = lib3270_is_tso,											//  Get value.
			.set = lib3270_set_tso											//  Set value.
		},

		{
			.name = "as400",												//  Property name.
			.group = LIB3270_ACTION_GROUP_OFFLINE,							// Property group.
			.description = N_( "Non zero if the host is AS400." ),			//  Property description.
			.get = lib3270_is_as400,										//  Get value.
			.set = lib3270_set_as400										//  Set value.
		},

		{
			.name = "pconnected",											//  Property name.
			.description = "",												//  Property description.
			.get = lib3270_pconnected,										//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "half_connected",										//  Property name.
			.description = "",												//  Property description.
			.get = lib3270_half_connected,									//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "neither",												//  Property name.
			.description = "",												//  Property description.
			.get = lib3270_in_neither,										//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "ansi",													//  Property name.
			.description = "",												//  Property description.
			.get = lib3270_in_ansi,											//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "tn3270",												//  Property name.
			.description = N_( "State is 3270, TN3270e or SSCP" ),			//  Property description.
			.get = lib3270_in_3270,											//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "sscp",													//  Property name.
			.description = "",												//  Property description.
			.get = lib3270_in_sscp,											//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "tn3270e",												//  Property name.
			.description = "",												//  Property description.
			.get = lib3270_in_tn3270e,										//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "e",													//  Property name.
			.description = N_( "Is terminal in the INITIAL_E state?" ),		//  Property description.
			.get = lib3270_in_e,											//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "has_selection",										//  Property name.
			.description = N_( "Has selected area" ),						//  Property description.
			.get = lib3270_get_has_selection,								//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "can_paste_next",										//  Property name.
			.description = N_( "Still have text to paste" ),				//  Property description.
			.get = lib3270_can_paste_next,									//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "starting",												//  Property name.
			.description = N_( "Is starting (no first screen)?" ),			//  Property description.
			.get = lib3270_is_starting,										//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "formatted",											//  Property name.
			.description = N_( "Formatted screen" ),						//  Property description.
			.get = lib3270_is_formatted,									//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "oerrlock",												//  Property name.
			.description = N_( "Lock keyboard on operator error" ),			//  Property description.
			.get = lib3270_get_lock_on_operator_error,						//  Get value.
			.set = lib3270_set_lock_on_operator_error						//  Set value.
		},

		{
			.name = "numericlock",											//  Property name.
			.description = N_( "numeric lock" ),							//  Property description.
			.get = lib3270_get_numeric_lock,								//  Get value.
			.set = lib3270_set_numeric_lock									//  Set value.
		},

		{
			.name = "crl_download",												//  Property name.
			.description = N_( "Non zero if the download of CRL is enabled" ),	//  Property description.
			.get = lib3270_ssl_get_crl_download,								//  Get value.
			.set = lib3270_ssl_set_crl_download,								//  Set value.
#if defined(SSL_ENABLE_CRL_CHECK)
			.default_value = 1
#else
			.default_value = 0
#endif
		},

		{
			.name = NULL,
			.description = NULL,
			.get = NULL,
			.set = NULL
		}

	};

	return properties;

}

int lib3270_set_boolean_property(H3270 *hSession, const char *name, int value, int seconds) {

	if(seconds) {
		lib3270_wait_for_ready(hSession, seconds);
	}

	size_t ix;
	const LIB3270_INT_PROPERTY * properties = lib3270_get_boolean_properties_list();
	for(ix = 0; properties[ix].name; ix++) {
		if(!strcasecmp(name,properties[ix].name)) {
			if(properties[ix].set) {
				lib3270_write_event_trace(hSession,"%s %s\n",(value ? "Enabling" : "Disabling"),properties[ix].name);
				return properties[ix].set(hSession, value);
			} else {
				return errno = EPERM;
			}
		}

	}

	return errno = ENOENT;

}

