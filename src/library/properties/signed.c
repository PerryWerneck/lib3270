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
 * @brief Signed int properties.
 */

#include <config.h>
#include <internals.h>
#include <string.h>
#include <lib3270.h>
#include <lib3270/properties.h>
#include <lib3270/keyboard.h>

LIB3270_EXPORT const char * lib3270_get_connection_state_as_string(const H3270 *hSession) {

	static const char * values[] = {
		N_("Disconnected"),
		N_("Connecting to host"),
		N_("Connection pending"),
		N_("Connected, no mode yet"),
		N_("Connected in NVT ANSI mode"),
		N_("Connected in old-style 3270 mode"),
		N_("Connected in TN3270E mode, no negotiated"),
		N_("Connected in TN3270E mode, NVT mode"),
		N_("Connected in TN3270E mode, SSCP-LU mode"),
		N_("Connected in TN3270E mode, 3270 mode")
	};

	size_t value = (size_t) lib3270_get_connection_state(hSession);
	if(value < (sizeof(value)/sizeof(values[0]))) {
		return dgettext(GETTEXT_PACKAGE,values[value]);
	}

	return _( "Unknown" );

}

LIB3270_EXPORT const char * lib3270_get_program_message_as_string(const H3270 *hSession) {

	// HTML Entities: &#10060; or &#x274C;

	static const struct {
		const char *standard;
		const char *utf;
		const char *html;
	} values[] = {
		{
			"",
			"",
			""
		},
		{
			N_( "X System" ),
			N_( "❌ System" ),
			N_( "&#10060; System" )
		},
		{
			N_( "X Wait" ),
			N_( "❌ Wait" ),
			N_( "&#10060; Wait" )
		},
		{
			N_( "X Connected" ),
			N_( "❌ Connected" ),
			N_( "&#10060; Connected" )
		},
		{
			N_( "X Not Connected" ),
			N_( "❌ Not Connected" ),
			N_( "&#10060; Not Connected" )
		},
		{
			N_( "X" ),
			N_( "❌" ),
			N_( "&#10060;" )
		},
		{
			N_( "X -f" ),
			N_( "❌ -f" ),
			N_( "&#10060; -f" )
		},
		{
			N_( "X Protected" ),
			N_( "❌ Protected" ),
			N_( "&#10060; Protected" )
		},
		{
			N_( "X Numeric" ),
			N_( "❌ Numeric" ),
			N_( "&#10060; Numeric" )
		},
		{
			N_( "X Overflow" ),
			N_( "❌ Overflow" ),
			N_( "&#10060; Overflow" )
		},
		{
			N_( "X Inhibit" ),
			N_( "❌ Inhibit" ),
			N_( "&#10060; Inhibit" )
		},
		{
			N_( "X" ),
			N_( "❌" ),
			N_( "&#10060;" )
		},
		{
			N_( "X" ),
			N_( "❌" ),
			N_( "&#10060;" )
		},
		{
			N_( "X Resolving" ),
			N_( "❌ Resolving" ),
			N_( "&#10060;" )
		},
		{
			N_( "X Connecting" )
			N_( "❌ Connecting" )
			N_( "&#10060; Connecting" )
		}
	};

	size_t message = (size_t) lib3270_get_program_message(hSession);
	if( message < (sizeof(values)/sizeof(values[0]))) {
		const char *msg = "";

		switch(hSession->charset.msgtype) {
		case 0:
			msg = values[message].standard;
			break;
		case 1:
			msg = values[message].utf;
			break;
		case 2:
			msg = values[message].html;
			break;

		default:
			msg = N_("Unexpected charset type");
		}

		return dgettext(GETTEXT_PACKAGE,msg);
	}

	return _("Unknown");
}

LIB3270_EXPORT const char * lib3270_get_ssl_state_as_string(const H3270 * hSession) {

	static const char * values[] = {
		N_("No secure connection"),
		N_("Connection secure with CA check"),
		N_("Connection secure, no CA, self-signed or expired CRL"),
		N_("Negotiating SSL"),
		N_("Verifying SSL (Getting CRL)"),
		N_("Undefined")
	};

	if(hSession->ssl.state < (sizeof(values)/sizeof(values[0]))) {
		return dgettext(GETTEXT_PACKAGE,values[hSession->ssl.state]);
	}

	return _( "Unknown" );
}

static int lib3270_get_connection_state_as_int(const H3270 *hSession) {
	return (int) lib3270_get_connection_state(hSession);
}

static int lib3270_get_ssl_state_as_int(const H3270 * hSession) {
	return (int) lib3270_get_ssl_state(hSession);
}

static int lib3270_get_program_message_as_int(const H3270 *hSession) {
	return (int) lib3270_get_program_message(hSession);
}

const LIB3270_INT_PROPERTY * lib3270_get_int_properties_list(void) {

	static const LIB3270_INT_PROPERTY properties[] = {

		{
			.name = "cstate",									//  Property name.
			.description = N_( "Connection state" ),			//  Property description.
			.get = lib3270_get_connection_state_as_int,			//  Get value.
			.set = NULL,										//  Set value.
			.describe = lib3270_get_connection_state_as_string
		},

		{
			.name = "cursor_address",								// Property name.
			.group = LIB3270_ACTION_GROUP_ONLINE,					// Property group.
			.description = N_( "Cursor address" ),					// Property description.
			.get = lib3270_get_cursor_address,						// Get value.
			.set = lib3270_set_cursor_address,						// Set value.
			.describe = NULL
		},

		{
			.name = "program_message",									//  Property name.
			.description = N_( "Latest program message" ),				//  Property description.
			.get = lib3270_get_program_message_as_int,					//  Get value.
			.set = NULL,												//  Set value.
			.describe = lib3270_get_program_message_as_string
		},

		{
			.name = "ssl_state",										//  Property name.
			.description = N_( "ID of the session security state" ),	//  Property description.
			.get = lib3270_get_ssl_state_as_int,						//  Get value.
			.set = NULL,												//  Set value.
			.describe = lib3270_get_ssl_state_as_string
		},

		{
			.name = NULL,
			.description = NULL,
			.get = NULL,
			.set = NULL,										//  Set value.
			.describe = NULL
		}
	};

	return properties;
}


int lib3270_get_int_property(H3270 *hSession, const char *name, int seconds) {
	size_t ix;
	const LIB3270_INT_PROPERTY * properties;

	if(seconds) {
		lib3270_wait_for_ready(hSession, seconds);
	}

	// Check for boolean properties
	properties = lib3270_get_boolean_properties_list();
	for(ix = 0; properties[ix].name; ix++) {
		if(!strcasecmp(name,properties[ix].name)) {
			if(properties[ix].get) {
				return properties[ix].get(hSession);
			} else {
				errno = EPERM;
				return -1;
			}
		}


	}

	// Check for int properties
	properties = lib3270_get_int_properties_list();
	for(ix = 0; properties[ix].name; ix++) {
		if(!strcasecmp(name,properties[ix].name)) {
			if(properties[ix].get) {
				return properties[ix].get(hSession);
			} else {
				errno = EPERM;
				return -1;
			}
		}


	}

	errno = ENOENT;
	return -1;
}

int lib3270_set_int_property(H3270 *hSession, const char *name, int value, int seconds) {
	size_t ix;
	const LIB3270_INT_PROPERTY * properties;

	if(seconds)
		lib3270_wait_for_ready(hSession, seconds);

	// Check for INT Properties
	properties = lib3270_get_int_properties_list();
	for(ix = 0; properties[ix].name; ix++) {
		if(!strcasecmp(name,properties[ix].name)) {
			if(properties[ix].set)
				return properties[ix].set(hSession, value);
			else
				return errno = EPERM;
		}

	}

	// Check for boolean properties
	properties = lib3270_get_boolean_properties_list();
	for(ix = 0; properties[ix].name; ix++) {
		if(!strcasecmp(name,properties[ix].name)) {
			if(properties[ix].set)
				return properties[ix].set(hSession, value);
			else
				return errno = EPERM;
		}

	}

	return errno = ENOENT;

}

