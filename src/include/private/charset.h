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

 #pragma once
 
 #include <config.h>
 #include <lib3270/defs.h>

 typedef struct _remap {
	const char				* name;
	unsigned long			  cgcsgid;
	const unsigned short	* chr;
 } remap;

 const LIB3270_INTERNAL remap internal_remaps[];
 const LIB3270_INTERNAL char * ebc2iso8859[256];

 // EBCDIC-to-Unicode translation tables.
 // Each table maps EBCDIC codes X'41' through X'FE' to UCS-2.
 // Other codes are mapped programmatically.
 #define UT_SIZE	190
 #define UT_OFFSET	0x41

 /// @brief Set the ISO-8859-1 character set for the given session.
 /// @param hSession The session handle.
 /// @return 0 on success, error code on failure (sets errno).
 /// @retval EINVAL Unable to set iso-8859-1 charset.
 LIB3270_INTERNAL int set_iso_8859_1_charset(H3270 *hSession);

