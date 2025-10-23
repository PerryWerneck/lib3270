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

#ifdef __cplusplus
extern "C" {
#endif

#define LIB3270_DEFAULT_CGEN			0x02b90000
#define LIB3270_DEFAULT_CSET			0x00000025

struct lib3270_charset {
	char			* host;
	char			* display;
	unsigned long	  cgcsgid;

	// Translation tables
	unsigned short		  ebc2asc[256];
	unsigned short 		  asc2ebc[256];
	unsigned short		  asc2uc[256];

};

typedef enum {
	CS_ONLY,
	FT_ONLY,
	BOTH
} lib3270_remap_scope;

/// @brief Set host charset.
/// @param hSession	Session Handle.
/// @param name		Charset name (us, bracket, cp500) or NULL to lib3270's default.
/// @return 0 if ok, error code if not.
/// @retval EINVAL	Invalid charset name.
LIB3270_EXPORT int			  lib3270_set_host_charset(H3270 *hSession, const char *name);

LIB3270_EXPORT const char	* lib3270_get_host_charset(const H3270 *hSession);

/// @brief Set display charset.
/// @param hSession The session handle.
/// @param name The charset name (ISO-8859-1) or NULL to lib3270's default.
/// @return 0 if ok, error code if not.
/// @retval ENOENT Invalid charset name.
LIB3270_EXPORT int			  lib3270_set_display_charset(H3270 *hSession, const char *name);

LIB3270_EXPORT const char	* lib3270_get_display_charset(const H3270 *hSession);

LIB3270_EXPORT void			  lib3270_remap_char(H3270 *hSession, unsigned short ebc, unsigned short iso, lib3270_remap_scope scope, unsigned char one_way);
LIB3270_EXPORT const char	* lib3270_ebc2asc(H3270 *hSession, unsigned char *buffer, int sz);
LIB3270_EXPORT const char	* lib3270_asc2ebc(H3270 *hSession, unsigned char *buffer, int sz);

/**
 * @brief Get character code from string definition.
 *
 * @param id	The character definition (id or 0x[code]).
 *
 * @return Character code if ok, 0 if not (sets errno).
 *
 * @retval EINVAL	Invalid character id.
 */
LIB3270_EXPORT unsigned short lib3270_translate_char(const char *id);

typedef struct _lib3270_iconv LIB3270_ICONV;

LIB3270_EXPORT void lib3270_autoptr_cleanup_LIB3270_ICONV(LIB3270_ICONV **iconv);
 
///
/// @brief Create a new ICONV wrapper.
///
LIB3270_EXPORT LIB3270_ICONV * lib3270_iconv_new(const char *remote, const char *local);

///
/// @brief Release the ICONV Wrapper.
///
LIB3270_EXPORT void lib3270_iconv_free(LIB3270_ICONV *conv);

///
/// @brief Convert from host to local.
///
LIB3270_EXPORT char * lib3270_iconv_from_host(LIB3270_ICONV *conv, const char *str, int len);

///
/// @brief Convert from local to host.
///
LIB3270_EXPORT char * lib3270_iconv_to_host(LIB3270_ICONV *conv, const char *str, int len);

#ifdef __cplusplus
}
#endif
