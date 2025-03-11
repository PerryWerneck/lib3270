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
#include <lib3270/defs.h>
#include <private/defs.h>
#include <lib3270.h>
#include <string.h>
#include <private/intl.h>
#include <private/session.h>

#include <internals.h>

/*---[ Globals ]--------------------------------------------------------------------------------------------------------------*/

/*---[ Statics ]--------------------------------------------------------------------------------------------------------------*/

static const LIB3270_HOST_TYPE_ENTRY host_type[] = {
	{
		.type = LIB3270_HOST_S390,
		.name = "S390",
		.description = N_( "IBM S/390" ),
		.tooltip = NULL
	},
	{
		.type = LIB3270_HOST_AS400,
		.name = "AS400",
		.description = N_( "IBM AS/400" ),
		.tooltip = NULL
	},
	{
		.type = LIB3270_HOST_TSO,
		.name = "TSO",
		.description = N_( "Other (TSO)" ),
		.tooltip = NULL
	},
	{
		.type = LIB3270_HOST_OTHER,
		.name = "VM/CMS",
		.description = N_( "Other (VM/CMS)"	),
		.tooltip = NULL
	},

	{
		.type = LIB3270_HOST_OTHER,
		.name = NULL,
		.description = NULL,
		.tooltip = NULL
	}
};


/*---[ Implement ]------------------------------------------------------------------------------------------------------------*/

LIB3270_EXPORT LIB3270_HOST_TYPE lib3270_get_host_type(const H3270 *hSession) {
	return hSession->host_type;
}

LIB3270_EXPORT int lib3270_set_host_type(H3270 *hSession, LIB3270_HOST_TYPE opt) {
	FAIL_IF_ONLINE(hSession);
	hSession->host_type = opt;
	return 0;
}

LIB3270_EXPORT unsigned int lib3270_get_color_type(const H3270 *hSession) {
	return (unsigned int) (hSession->mono ? 2 : hSession->colors);
}

LIB3270_EXPORT int lib3270_set_color_type(H3270 *hSession, unsigned int colortype) {
	FAIL_IF_ONLINE(hSession);

	switch(colortype) {
	case 0:
	case 16:
		hSession->colors 	= 16;
		hSession->mono		= 0;
		hSession->m3279		= 1;
		break;

	case 8:
		hSession->colors	= 8;
		hSession->mono		= 0;
		hSession->m3279		= 1;
		break;

	case 2:
		hSession->colors 	= 16;
		hSession->mono		= 1;
		hSession->m3279		= 0;
		break;

	default:
		return errno = EINVAL;
	}

	return 0;
}


LIB3270_EXPORT const LIB3270_HOST_TYPE_ENTRY * lib3270_get_option_list(void) {
	return host_type;
}

LIB3270_EXPORT int lib3270_is_tso(const H3270 *hSession) {
	return (hSession->host_type & LIB3270_HOST_TSO) != 0;
}

LIB3270_EXPORT int lib3270_set_tso(H3270 *hSession, int on) {
	FAIL_IF_ONLINE(hSession);

	if(on)
		hSession->host_type = LIB3270_HOST_TSO;
	else
		hSession->host_type &= ~LIB3270_HOST_TSO;

	return 0;
}

LIB3270_EXPORT int lib3270_is_as400(const H3270 *hSession) {
	return (hSession->host_type & LIB3270_HOST_AS400) != 0;
}

LIB3270_EXPORT int lib3270_set_as400(H3270 *hSession, int on) {
	FAIL_IF_ONLINE(hSession);

	if(on)
		hSession->host_type |= LIB3270_HOST_AS400;
	else
		hSession->host_type &= ~LIB3270_HOST_AS400;

	return 0;
}

LIB3270_EXPORT LIB3270_HOST_TYPE lib3270_parse_host_type(const char *name) {

	int f;

	for(f=0; host_type[f].name; f++) {
		if(!strcasecmp(host_type[f].name,name))
			return host_type[f].type;
	}

	errno = ENOENT;
	return 0;
}

LIB3270_EXPORT int lib3270_set_host_type_by_name(H3270 *hSession, const char *name) {
	FAIL_IF_ONLINE(hSession);

	size_t f;
	for(f=0; f<(sizeof(host_type)/sizeof(host_type[0])); f++) {
		if(host_type[f].name && !strcasecmp(host_type[f].name,name)) {
			hSession->host_type = host_type[f].type;
			return 0;
		}
	}

	return errno = EINVAL;
}

LIB3270_EXPORT const char * lib3270_get_host_type_name(const H3270 *hSession) {
	size_t f;

	for(f=0; f<(sizeof(host_type)/sizeof(host_type[0])); f++) {
		if(hSession->host_type == host_type[f].type) {
			return host_type[f].name;
		}
	}

	errno = EINVAL;
	return "";

}
