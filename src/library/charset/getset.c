/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2025 Perry Werneck <perry.werneck@gmail.com>
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
 *	@file charset/getset.c
 *
 *	@brief This module handles get/set the 3270 display/host character set.
 */

#include <config.h>
#include <private/charset.h>
#include <lib3270/memory.h>
#include <internals.h>
#include <X11keysym.h>
#include <lib3270/charset.h>
#include <lib3270/log.h>
#include <lib3270/trace.h>

/*---[ Implement ]------------------------------------------------------------------------------------------------------------*/

LIB3270_EXPORT const char * lib3270_get_default_charset(void) {
	return "ISO-8859-1";
}

LIB3270_EXPORT const char * lib3270_get_host_charset(const H3270 *hSession) {
	return hSession->charset.host;
}

LIB3270_EXPORT const char * lib3270_get_display_charset(const H3270 *hSession) {
	return hSession->charset.display;
}

LIB3270_EXPORT int lib3270_set_host_charset(H3270 *hSession, const char *name) {

	if(name && *name && hSession->charset.host && *hSession->charset.host && !strcasecmp(name,hSession->charset.host)) {
		debug("Host charset is already \"%s\", returning",hSession->charset.host);
		return 0;
	}

	if(name && *name) {
		lib3270_replace(hSession->charset.host, name);
	} else {
		lib3270_replace(hSession->charset.host, "us");
	}

	return reload_charset_tables(hSession);

}

LIB3270_EXPORT int lib3270_set_display_charset(H3270 *hSession, const char *name) {

	if(!(name && *name)) {
		name = "ISO-8859-1";
	}

	if(hSession->charset.display && *hSession->charset.display && !strcasecmp(name,hSession->charset.display)) {
		debug("Display charset is already \"%s\", returning",hSession->charset.display);
		return 0;
	}

	if(strcasecmp(name,"ISO-8859-1")) {
		return errno = ENOENT;
	}

	return reload_charset_tables(hSession);

}

