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

#include <config.h>
#include <stdlib.h>
#include <internals.h>
#include <string.h>
#include <lib3270.h>
#include <lib3270/properties.h>
#include <lib3270/keyboard.h>
#include <lib3270/log.h>
#include <lib3270/ssl.h>
#include <lib3270/trace.h>

LIB3270_EXPORT const char * lib3270_get_termtype(const H3270 *hSession) {
	return hSession->termtype;
}

LIB3270_EXPORT const char * lib3270_get_termname(const H3270 *hSession) {
	return hSession->termname;
}

LIB3270_EXPORT const LIB3270_STRING_PROPERTY * lib3270_get_string_properties_list(void) {

	auto const char * get_version(const H3270 GNUC_UNUSED(*hSession));
	auto const char * get_revision(const H3270 GNUC_UNUSED(*hSession));

	static const LIB3270_STRING_PROPERTY properties[] = {

		{
			.name = "associated_lu",												//  Property name.
			.description = N_( "The name of the LU associated with the session" ),	//  Property description.
			.get = lib3270_get_associated_luname,									//  Get value.
			.set = NULL																//  Set value.
		},

		{
			.name = "url",															// Property name.
#ifdef LIB3270_DEFAULT_HOST
			.default_value = LIB3270_STRINGIZE_VALUE_OF(LIB3270_DEFAULT_HOST),		// Default value.
#endif // LIB3270_DEFAULT_HOST
			.group = LIB3270_ACTION_GROUP_OFFLINE,									// Property group.
			.icon = "network-server",												// Property icon.
			.description = N_( "URL of the current host" ),							// Property description.
			.get = lib3270_get_url,													// Get value.
			.set = lib3270_set_url													// Set value.
		},

		{
			.name = "model_name",													// Property name.
			.group = LIB3270_ACTION_GROUP_OFFLINE,									// Property group.
			.description = N_( "Model name" ),										// Property description.
			.get = lib3270_get_model_name,											// Get value.
			.set = lib3270_set_model_name											// Set value.
		},

		{
			.name = "host_type_name",												// Property name.
			.group = LIB3270_ACTION_GROUP_OFFLINE,									// Property group.
			.description = N_( "Host type name" ),									// Property description.
			.get = lib3270_get_host_type_name,										// Get value.
			.set = lib3270_set_host_type_by_name									// Set value.
		},

		{
			.name = "termtype",														//  Property name.
			.description = N_( "Terminal type" ),									//  Property description.
			.get = lib3270_get_termtype,											//  Get value.
			.set = NULL																//  Set value.
		},

		{
			.name = "termname",														//  Property name.
			.description = N_( "Terminal name" ),									//  Property description.
			.get = lib3270_get_termname,											//  Get value.
			.set = NULL																//  Set value.
		},

		{
			.name = "host_charset",													// Property name.
			.default_value = "bracket",												// Default charset.
			.group = LIB3270_ACTION_GROUP_OFFLINE,									// Property group.
			.description = N_( "Host charset" ),									// Property description.
			.get = lib3270_get_host_charset,										// Get value.
			.set = lib3270_set_host_charset											// Set value.
		},

		{
			.name = "display_charset",								//  Property name.
			.description = N_( "Display charset" ),					//  Property description.
			.get = lib3270_get_display_charset,						//  Get value.
			.set = NULL												//  Set value.
		},

		{
			.name = "version",										//  Property name.
			.description = N_( "Protocol library version" ),		//  Property description.
			.get = get_version,										//  Get value.
			.set = NULL												//  Set value.
		},

		{
			.name = "revision",										// Property name.
			.description = N_( "Protocol library revision" ),		// Property description.
			.get = get_revision,									// Get value.
			.set = NULL												// Set value.
		},

		{
			.name = "crl_preferred_protocol",									// Property name.
			.group = LIB3270_ACTION_GROUP_OFFLINE,								// Property group.
			.description = N_( "Preferred protocol for CRL download" ),			// Property description.
			.get = lib3270_crl_get_preferred_protocol,							// Get value.
			.set = lib3270_crl_set_preferred_protocol,							// Set value.
		},

		{
			.name = "default_host",												// Property name.
			.description = N_( "Default host URL" ),							// Property description.
			.get = lib3270_get_default_host,									// Get value.
			.set = NULL															// Set value.
		},

		{
			.name = "sslmessage",												//  Property name.
			.description = N_( "The security state" ),							//  Property description.
			.get = lib3270_get_ssl_state_message,								//  Get value.
			.set = NULL															//  Set value.
		},

		{
			.name = "ssldescription",											//  Property name.
			.description = N_( "Description of the current security state" ),	//  Property description.
			.get = lib3270_get_ssl_state_description,							//  Get value.
			.set = NULL															//  Set value.
		},

		{
			.name = "oversize",														//  Property name.
			.group = LIB3270_ACTION_GROUP_OFFLINE,									// Property group.
			.description = N_( "Screen oversize if larger than the chosen model"),	//  Property description.
			.get = lib3270_get_oversize,											//  Get value.
			.set = lib3270_set_oversize												//  Set value.
		},

		{
			.name = "logfile",														//  Property name.
			.group = LIB3270_ACTION_GROUP_NONE,										// Property group.
			.description = N_( "The log file name"),								//  Property description.
			.get = lib3270_get_log_filename,										//  Get value.
			.set = lib3270_set_log_filename											//  Set value.
		},

		{
			.name = "tracefile",													//  Property name.
			.group = LIB3270_ACTION_GROUP_NONE,										// Property group.
			.description = N_( "The trace file name"),								//  Property description.
			.get = lib3270_get_trace_filename,										//  Get value.
			.set = lib3270_set_trace_filename										//  Set value.
		},

		{
			.name = NULL,
			.description = NULL,
			.get = NULL,
			.set = NULL
		}

	};

	const char * get_version(const H3270 GNUC_UNUSED(*hSession)) {
		return lib3270_get_version();
	}

	const char * get_revision(const H3270 GNUC_UNUSED(*hSession)) {
		return lib3270_get_revision();
	}

	return properties;

}

int lib3270_set_string_property(H3270 *hSession, const char *name, const char * value, int seconds) {
	size_t ix;

	if(seconds) {
		lib3270_wait_for_ready(hSession, seconds);
	}

	//
	// Check for string property
	//
	{
		const LIB3270_STRING_PROPERTY * properties = lib3270_get_string_properties_list();
		for(ix = 0; properties[ix].name; ix++) {
			if(!strcasecmp(name,properties[ix].name)) {
				if(properties[ix].set) {
					return properties[ix].set(hSession, value);
				} else {
					return errno = EPERM;
				}
			}

		}
	}

	//
	// Check for signed int property
	//
	{
		const LIB3270_INT_PROPERTY * properties = lib3270_get_int_properties_list();
		for(ix = 0; properties[ix].name; ix++) {
			if(!strcasecmp(name,properties[ix].name)) {
				if(properties[ix].set) {
					return properties[ix].set(hSession, atoi(value));
				} else {
					return errno = EPERM;
				}
			}

		}
	}

	//
	// Check for unsigned int property
	//
	{
		const LIB3270_UINT_PROPERTY * properties = lib3270_get_unsigned_properties_list();
		for(ix = 0; properties[ix].name; ix++) {
			if(!strcasecmp(name,properties[ix].name)) {
				if(properties[ix].set) {
					return properties[ix].set(hSession, strtoul(value,NULL,0));
				} else {
					return errno = EPERM;
				}
			}

		}
	}

	//
	// Check for boolean property
	//
	{
		const LIB3270_INT_PROPERTY * properties = lib3270_get_int_properties_list();
		for(ix = 0; properties[ix].name; ix++) {
			if(!strcasecmp(name,properties[ix].name)) {
				if(properties[ix].set) {
					return properties[ix].set(hSession, atoi(value));
				} else {
					return errno = EPERM;
				}
			}

		}
	}

	return errno = ENOENT;

}

LIB3270_EXPORT int lib3270_set_lunames(H3270 *hSession, const char *lunames) {
	FAIL_IF_ONLINE(hSession);

	if(hSession->lu.names) {
		lib3270_free(hSession->lu.names);
		hSession->lu.names = NULL;
	}

	// Do I have lunames to set? If not just return.
	if(!lunames)
		return 0;

	//
	// Count the commas in the LU names.  That plus one is the
	// number of LUs to try.
	//
	char *comma;
	char *lu;
	int n_lus = 1;

	lu = (char *) lunames;
	while ((comma = strchr(lu, ',')) != CN) {
		n_lus++;
		lu++;
	}

	//
	// Allocate enough memory to construct an argv[] array for
	// the LUs.
	//
	Replace(hSession->lu.names,(char **)lib3270_malloc((n_lus+1) * sizeof(char *) + strlen(lunames) + 1));

	// Copy each LU into the array.
	lu = (char *)(hSession->lu.names + n_lus + 1);
	(void) strcpy(lu, lunames);

	size_t i = 0;
	do {
		hSession->lu.names[i++] = lu;
		comma = strchr(lu, ',');
		if (comma != CN) {
			*comma = '\0';
			lu = comma + 1;
		}
	} while (comma != CN);

	hSession->lu.names[i]	= CN;

	return 0;
}

LIB3270_EXPORT const char ** lib3270_get_lunames(H3270 *hSession) {
	return (const char **) hSession->lu.names;
}

LIB3270_EXPORT const char * lib3270_host_get_name(const H3270 *h) {
	return h->host.current;
}

LIB3270_EXPORT const char * lib3270_service_get_name(const H3270 *h) {
	return h->host.srvc;
}

