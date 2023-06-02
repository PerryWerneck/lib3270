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

#include <config.h>
#include <internals.h>
#include <string.h>
#include <lib3270.h>
#include <lib3270/properties.h>
#include <lib3270/keyboard.h>

static int lib3270_get_connection_state_as_int(const H3270 *hSession) {
	return (int) lib3270_get_connection_state(hSession);
}

const char * lib3270_get_connection_state_as_string(const H3270 *hSession) {

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

static int lib3270_get_program_message_as_int(const H3270 *hSession) {
	return (int) lib3270_get_program_message(hSession);
}

const char * lib3270_get_program_message_as_string(const H3270 *hSession) {

	static const char * values[] = {
		"",
		N_( "X System" ),
		N_( "X Wait" ),
		N_( "X Connected" ),
		N_( "X Not Connected" ),
		N_( "X" ),
		N_( "X -f" ),
		N_( "X Protected" ),
		N_( "X Numeric" ),
		N_( "X Overflow" ),
		N_( "X Inhibit" ),
		N_( "X" ),
		N_( "X" ),
		N_( "X Resolving" ),
		N_( "X Connecting" )
	};

	size_t value = (size_t) lib3270_get_program_message(hSession);
	if(value < (sizeof(value)/sizeof(values[0]))) {
		return dgettext(GETTEXT_PACKAGE,values[value]);
	}

	return _( "Unknown" );

}

static int lib3270_get_ssl_state_as_int(const H3270 * hSession) {
	return (int) lib3270_get_ssl_state(hSession);
}

const char * lib3270_get_ssl_state_as_string(const H3270 * hSession) {

	static const char * values[] = {
		N_("No secure connection"),
		N_("Connection secure with CA check"),
		N_("Connection secure, no CA, self-signed or expired CRL"),
		N_("Negotiating SSL"),
		N_("Verifying SSL (Getting CRL)"),
		N_("Undefined")
	};

	size_t value = (size_t) lib3270_get_ssl_state(hSession);
	if(value < (sizeof(value)/sizeof(values[0]))) {
		return dgettext(GETTEXT_PACKAGE,values[value]);
	}

	return _( "Unknown" );
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

