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

// ISO-8859-1 Character Set tables

#include <config.h>
#include <string.h>
#include <lib3270/memory.h>
#include <lib3270/charset.h>
#include <private/session.h>
#include <private/charset.h>
#include <private/trace.h>
#include <X11keysym.h>
#include <stdint.h>

struct _lib3270_charset_context {
	size_t ebc2asc[256];			///< Offset of remmapped EBCDIC to ASCII strings.
	unsigned char asc2ebc[256];

	// Allways the last members
	size_t buflen;
	char *buffer;
};

const char * ebc2iso[256] = {
	"\x20", "\x20", "\x20", "\x20", "\x20", "\x20", "\x20", "\x20",	// 00
	"\x20", "\x20", "\x20", "\x20", "\x20", "\x20", "\x20", "\x20",	// 08
	"\x20", "\x20", "\x20", "\x20", "\x20", "\x20", "\x20", "\x20",	// 10
	"\x20", "\x20", "\x20", "\x20", "\x2a", "\x20", "\x3b", "\x20",	// 18
	"\x20", "\x20", "\x20", "\x20", "\x20", "\x20", "\x20", "\x20",	// 20
	"\x20", "\x20", "\x20", "\x20", "\x20", "\x20", "\x20", "\x20",	// 28
	"\x20", "\x20", "\x20", "\x20", "\x20", "\x20", "\x20", "\x20",	// 30
	"\x20", "\x20", "\x20", "\x20", "\x20", "\x20", "\x20", "\x20",	// 38
	"\x20", "\x20", "\xe2", "\xe4", "\xe0", "\xe1", "\xe3", "\xe5",	// 40
	"\xe7", "\xf1", "\xa2", "\x2e", "\x3c", "\x28", "\x2b", "\x7c",	// 48
	"\x26", "\xe9", "\xea", "\xeb", "\xe8", "\xed", "\xee", "\xef",	// 50
	"\xec", "\xdf", "\x21", "\x24", "\x2a", "\x29", "\x3b", "\xac",	// 58
	"\x2d", "\x2f", "\xc2", "\xc4", "\xc0", "\xc1", "\xc3", "\xc5",	// 60
	"\xc7", "\xd1", "\xa6", "\x2c", "\x25", "\x5f", "\x3e", "\x3f",	// 68
	"\xf8", "\xc9", "\xca", "\xcb", "\xc8", "\xcd", "\xce", "\xcf",	// 70
	"\xcc", "\x60", "\x3a", "\x23", "\x40", "\x27", "\x3d", "\x22",	// 78
	"\xd8", "\x61", "\x62", "\x63", "\x64", "\x65", "\x66", "\x67",	// 80
	"\x68", "\x69", "\xab", "\xbb", "\xf0", "\xfd", "\xfe", "\xb1",	// 88
	"\xb0", "\x6a", "\x6b", "\x6c", "\x6d", "\x6e", "\x6f", "\x70",	// 90
	"\x71", "\x72", "\xaa", "\xba", "\xe6", "\xb8", "\xc6", "\xa4",	// 98
	"\xb5", "\x7e", "\x73", "\x74", "\x75", "\x76", "\x77", "\x78",	// a0
	"\x79", "\x7a", "\xa1", "\xbf", "\xd0", "\xdd", "\xde", "\xae",	// a8
	"\x5e", "\xa3", "\xa5", "\xb7", "\xa9", "\xa7", "\xb6", "\xbc",	// b0
	"\xbd", "\xbe", "\x5b", "\x5d", "\xaf", "\xa8", "\xb4", "\xd7",	// b8
	"\x7b", "\x41", "\x42", "\x43", "\x44", "\x45", "\x46", "\x47",	// c0
	"\x48", "\x49", "\xad", "\xf4", "\xf6", "\xf2", "\xf3", "\xf5",	// c8
	"\x7d", "\x4a", "\x4b", "\x4c", "\x4d", "\x4e", "\x4f", "\x50",	// d0
	"\x51", "\x52", "\xb9", "\xfb", "\xfc", "\xf9", "\xfa", "\xff",	// d8
	"\x5c", "\xf7", "\x53", "\x54", "\x55", "\x56", "\x57", "\x58",	// e0
	"\x59", "\x5a", "\xb2", "\xd4", "\xd6", "\xd2", "\xd3", "\xd5",	// e8
	"\x30", "\x31", "\x32", "\x33", "\x34", "\x35", "\x36", "\x37",	// f0
	"\x38", "\x39", "\xb3", "\xdb", "\xdc", "\xd9", "\xda", "\x20",	// f8
};

static const unsigned char iso2ebc[256] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 00
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 08
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 10
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 18
	0x40, 0x5a, 0x7f, 0x7b, 0x5b, 0x6c, 0x50, 0x7d,	// 20
	0x4d, 0x5d, 0x5c, 0x4e, 0x6b, 0x60, 0x4b, 0x61,	// 28
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,	// 30
	0xf8, 0xf9, 0x7a, 0x5e, 0x4c, 0x7e, 0x6e, 0x6f,	// 38
	0x7c, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,	// 40
	0xc8, 0xc9, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6,	// 48
	0xd7, 0xd8, 0xd9, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6,	// 50
	0xe7, 0xe8, 0xe9, 0xba, 0xe0, 0xbb, 0xb0, 0x6d,	// 58
	0x79, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,	// 60
	0x88, 0x89, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96,	// 68
	0x97, 0x98, 0x99, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6,	// 70
	0xa7, 0xa8, 0xa9, 0xc0, 0x4f, 0xd0, 0xa1, 0x00,	// 78
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 80
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 88
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 90
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 98
	0x41, 0xaa, 0x4a, 0xb1, 0x9f, 0xb2, 0x6a, 0xb5,	// a0
	0xbd, 0xb4, 0x9a, 0x8a, 0x5f, 0xca, 0xaf, 0xbc,	// a8
	0x90, 0x8f, 0xea, 0xfa, 0xbe, 0xa0, 0xb6, 0xb3,	// b0
	0x9d, 0xda, 0x9b, 0x8b, 0xb7, 0xb8, 0xb9, 0xab,	// b8
	0x64, 0x65, 0x62, 0x66, 0x63, 0x67, 0x9e, 0x68,	// c0
	0x74, 0x71, 0x72, 0x73, 0x78, 0x75, 0x76, 0x77,	// c8
	0xac, 0x69, 0xed, 0xee, 0xeb, 0xef, 0xec, 0xbf,	// d0
	0x80, 0xfd, 0xfe, 0xfb, 0xfc, 0xad, 0xae, 0x59,	// d8
	0x44, 0x45, 0x42, 0x46, 0x43, 0x47, 0x9c, 0x48,	// e0
	0x54, 0x51, 0x52, 0x53, 0x58, 0x55, 0x56, 0x57,	// e8
	0x8c, 0x49, 0xcd, 0xce, 0xcb, 0xcf, 0xcc, 0xe1,	// f0
	0x70, 0xdd, 0xde, 0xdb, 0xdc, 0x8d, 0x8e, 0xdf	// f8
};

/*
static const unsigned char iso2uc[UT_SIZE] = {
		  0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,	// 40
	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,	// 48
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,	// 50
	0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,	// 58
	0x60, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,	// 60
	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,	// 68
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,	// 70
	0x58, 0x59, 0x5a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,	// 78
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,	// 80
	0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,	// 88
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,	// 90
	0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,	// 98
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,	// a0
	0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,	// a8
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,	// b0
	0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,	// b8
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,	// c0
	0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,	// c8
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,	// d0
	0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,	// d8
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,	// e0
	0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,	// e8
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xf7,	// f0
	0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde		// f8
};
*/

/*
static inline void copy_charset(const unsigned char *from, unsigned char *to) {
	int f;
	for(f=0; f < UT_SIZE; f++)
		to[f+UT_OFFSET] = from[f];
}
*/

static void finalize(H3270 *session, LIB3270_CHARSET_CONTEXT * context) {
	if(context->buffer) {
		lib3270_free(context->buffer);
	}
	lib3270_free(context);
	session->charset.context = NULL;
}

static const unsigned char to_ebc(const LIB3270_CHARSET_CONTEXT *context, const char *asc) {
	return context->asc2ebc[(size_t) (*asc & 0xFF)];
}

static const char * to_asc(const LIB3270_CHARSET_CONTEXT *context, unsigned short ebc) {

	size_t offset = (size_t) (ebc & 0xFF); 

	// Check for remapped characters first
	if(context->ebc2asc[offset] != (size_t) -1) {
		return context->buffer + context->ebc2asc[offset];
	}

	// No remap, return default mapping
	return ebc2iso[offset];

}

static const char * to_uc(const LIB3270_CHARSET_CONTEXT *context, unsigned short ebc) {

	// FIXME: Detect if ebc in the UT range and use proper mapping.

	return to_asc(context, ebc);
}

static const char * to_cg(const LIB3270_CHARSET_CONTEXT *context, unsigned short ebc) {

	// https://www.charset.org/charsets/iso-8859-1

	static const struct {
		unsigned short ebc;
		const char *chr;
	} chars[] = {

		{ 0x8c, " " }, // CG 0xf7, less or equal "≤"
		{ 0xae, " " }, // CG 0xd9, greater or equal "≥"
		{ 0xbe, " " }, // CG 0x3e, not equal "≠"
		{ 0xad, "[" }, // "["
		{ 0xbd, "]" }, // "]"
		{ 0xc6, " " }, // CG 0xa5, left tee
		{ 0xd3, " " }, // CG 0xab, plus
		{ 0xa2, " " }, // CG 0x92, horizontal line
		{ 0xc7, " " }, // CG 0xa6, bottom tee
		{ 0xd7, " " }, // CG 0xaf, top tee
		{ 0xd6, " " }, // CG 0xae, right tee
		{ 0xc5, " " }, // CG 0xa4, UL corner
		{ 0xd5, " " }, // CG 0xad, UR corner
		{ 0x85, " " }, // CG 0x184, vertical line
		{ 0xc4, " " }, // CG 0xa3, LL corner
		{ 0xd4, " " }, // CG 0xac, LR corner

		{ 0xf0, " " }, // 0-9 Superscript
		{ 0xf1, " " },
		{ 0xf2, " " },
		{ 0xf3, " " },
		{ 0xf4, " " },
		{ 0xf5, " " },
		{ 0xf6, " " },
		{ 0xf7, " " },
		{ 0xf8, " " },
		{ 0xf9, " " },

		{ 0xe1, " " }, // 1-3 subscript
		{ 0xe2, " " },
		{ 0xe3, " " },

		{ 0xb8, "\xf7" }, // Division Sign ÷

	};

	for(size_t ix = 0; ix < sizeof(chars)/sizeof(chars[0]); ix++) {
		if(chars[ix].ebc == ebc) {
			return chars[ix].chr;
		}
	}

	return " ";
}

static size_t append(struct _lib3270_charset_context *context, const char *str) {

	size_t offset = context->buflen;
	context->buflen = context->buflen + strlen(str)+1;
	
	if(context->buffer) {
		context->buffer = lib3270_realloc(context->buffer, context->buflen);
	} else {
		context->buffer = lib3270_malloc(context->buflen);
	}

	strcpy(context->buffer + offset, str);

	return offset;
}

static int do_remap(H3270 *session, unsigned short ebc, const char *iso, int scope, unsigned char one_way) {

	// Ignore mappings of EBCDIC control codes and the space character.
	if (ebc <= 0x40) {
		return EINVAL;
	}

	// If they want to map to a NULL or a blank, make it a one-way blank.
	if (iso[0] == 0x0)
		iso = "\x20";

	if (iso[0] == 0x20)
		one_way = 1;

	if (iso[0] <= 0xff) {
		if (scope == BOTH || scope == CS_ONLY) {
			if (ebc > 0x40) {
				session->charset.context->ebc2asc[ebc] = append(session->charset.context,iso);
				if(!one_way && iso[1] == 0x0) {
					session->charset.context->asc2ebc[(int) (iso[0] & 0xFF) ] = ebc;
				}
			}
		}
	}

	return 0;

}

LIB3270_INTERNAL int set_iso_8859_1_charset(H3270 *hSession) {

	debug("%s: Initializing ISO-8859-1",__FUNCTION__);
	
	hSession->charset.context = lib3270_new(struct _lib3270_charset_context);
	hSession->charset.context->buflen = 0;

	hSession->charset.asc2ebc = to_ebc;
	hSession->charset.ebc2uc = to_uc;
	hSession->charset.ebc2asc = to_asc;
	hSession->charset.ebc2cg = to_cg;
	hSession->charset.remap = do_remap;
	hSession->charset.finalize = finalize;

	if(!(hSession->charset.host && *hSession->charset.host)) {
		lib3270_replace(hSession->charset.host,"us");
	}

	lib3270_replace(hSession->charset.display,"ISO-8859-1");

	memcpy(hSession->charset.context->asc2ebc, iso2ebc, sizeof(hSession->charset.context->asc2ebc));

	// Clear remap table.
	for(size_t ix = 0; ix < 256; ix++) {
		hSession->charset.context->ebc2asc[ix] = (size_t) -1;
	}

	/*
	for(f=0; f<UT_OFFSET; f++)
		hSession->charset.context->asc2uc[f] = f;
	copy_charset(iso2uc,hSession->charset.context->asc2uc);
	*/

	return 0;
}
