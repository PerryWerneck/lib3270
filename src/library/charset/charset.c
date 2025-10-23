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

#include <config.h>
#include <string.h>

#include <internals.h>
#include <private/charset.h>
#include <X11keysym.h>
#include <lib3270/charset.h>
#include <lib3270/log.h>
#include <lib3270/trace.h>
#include <lib3270/memory.h>

const remap internal_remaps[] = {
	{
		"us",
		LIB3270_DEFAULT_CGEN | LIB3270_DEFAULT_CSET,
		(const unsigned short []) {
			0x00, 0x00
		}
	},

	{
		"bracket",
		LIB3270_DEFAULT_CGEN|LIB3270_DEFAULT_CSET,
		(const unsigned short []) {
			0xad, '[',
			0xba, XK_Yacute,
			0xbd, ']',
			0xbb, XK_diaeresis,
			0x00, 0x00
		}
	},

	{
		"cp500",
		LIB3270_DEFAULT_CGEN|0x000001F4,
		(const unsigned short []) {
			0x4a, '[',
			0x4f, '!',
			0x5a, ']',
			0x5f, '^',
			0xb0, XK_percent,
			0xba, XK_notsign,
			0xbb, XK_bar
		}
	},

	// Terminate list
	{
		NULL
	}

};


/*
LIB3270_EXPORT void lib3270_reset_charset(H3270 *hSession, const char * host, const char * display, unsigned long cgcsgid) {

	if(host && *host) {
		lib3270_replace(hSession->charset.host, host);
	} else {
		lib3270_replace(hSession->charset.host, "us");
	}

	hSession->charset.cgcsgid = cgcsgid;
	set_iso_8859_1_charset(hSession);

}

LIB3270_EXPORT int lib3270_set_host_charset(H3270 *hSession, const char *name) {
	int f;

	debug("%s(%s)",__FUNCTION__,name);

	if(name && hSession->charset.host && !strcasecmp(name,hSession->charset.host)) {
		debug("Charset is \"%s\", returning",hSession->charset.host);
		return 0;
	}

	if(!name) {
		name = hSession->charset.host;
		debug("Resetting to charset \"%s\"",name);
	}

	if(!name) {
		lib3270_reset_charset(hSession, NULL, NULL, LIB3270_DEFAULT_CGEN | LIB3270_DEFAULT_CSET);
		return 0;
	}

	for(f=0; charset[f].name != NULL; f++) {
		if(!strcasecmp(name,charset[f].name)) {
			// Found required charset
			int c;

			debug("%s: %s -> %s",__FUNCTION__,hSession->charset.host,charset[f].name);

			lib3270_reset_charset(hSession,charset[f].name,"ISO-8859-1", charset[f].cgcsgid);

			for(c=0; charset[f].chr[c]; c+=2)
				lib3270_remap_char(hSession,charset[f].chr[c],charset[f].chr[c+1], BOTH, 0);

			debug("Charset is now \"%s\"",charset[f].name);
			return 0;
		}
	}

	debug("%s: %s",__FUNCTION__,strerror(EINVAL));
	return errno = EINVAL;

}
*/

