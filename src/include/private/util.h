/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright 1995, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006 by Paul Mattes.
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

/***
 * @brief Global declarations for util.c.
 */

LIB3270_INTERNAL char 	* ctl_see(int c);

/// @brief "unescape" text (Replaces %value for corresponding character).
/// @param text	Text to convert.
/// @return Converted string (release it with g_free).
LIB3270_INTERNAL char * unescape(const char *text);

/// @brief Compare strings ignoring non alphanumeric chars.
/// @param s1	First string.
/// @param s2	Second string.
/// @return 0 if equal, non zero if not.
LIB3270_INTERNAL int compare_alnum(const char *s1, const char *s2);

/// @brief Append string, reallocating if necessary.
/// @param dst	Destination string (can be NULL).
/// @param src	Source string.
/// @return Reallocated destination string.
LIB3270_INTERNAL char * append_string(char * dst, const char * src);

typedef struct _string_buffer {
	char *buf;
	size_t len;
} string_buffer;

LIB3270_INTERNAL void string_buffer_append(string_buffer *sb, const char *text);
