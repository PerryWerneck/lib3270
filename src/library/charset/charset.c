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
#include <private/trace.h>

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

 LIB3270_INTERNAL int reload_charset_tables(H3270 *hSession) {

	// Finalize previous charset
	if(hSession->charset.context && hSession->charset.finalize) {
		hSession->charset.finalize(hSession, hSession->charset.context);
		hSession->charset.context = NULL;
	}

	debug("%s -----------------------------------------> %s",__FUNCTION__,hSession->charset.host);

	// Set ISO-8859-1 charset
	// TODO: support other charsets
	int rc = set_iso_8859_1_charset(hSession);
	debug("%s: set_iso_8859_1_charset returned %d",__FUNCTION__,rc);

	if(rc) {
		return rc;
	}

	// Apply remap if any
	for(size_t f=0; internal_remaps[f].name != NULL; f++) {

		if(!strcasecmp(hSession->charset.host,internal_remaps[f].name)) {

			// Found remap
			if(!hSession->charset.remap) {
				return errno = EINVAL;
			}

			int c;

			debug("%s: %s -> %s",__FUNCTION__,hSession->charset.host,internal_remaps[f].name);

			hSession->charset.cgcsgid = internal_remaps[f].cgcsgid;

			for(c=0; internal_remaps[f].chr[c]; c+=2) {
				char iso[2] = { (char) internal_remaps[f].chr[c+1], 0x0 };
				hSession->charset.remap(hSession,internal_remaps[f].chr[c], iso, BOTH, 0);
			}

			trace_ds(hSession,
				"Host charset=%s display charset=%s remap=%s", 
					hSession->charset.host, 
					hSession->charset.display,
					internal_remaps[f].name
			);

			break;

		}
	}

	return 0;
 }
