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

/**
 * @brief TN3270 WIN32 API definitions.
 *
 * @author perry.werneck@gmail.com
 *
 */

#ifndef LIB3270_WIN32_H_INCLUDED

#define LIB3270_WIN32_H_INCLUDED 1

#include <winsock2.h>
#include <windows.h>
#include <lib3270.h>

#ifdef __cplusplus
extern "C" {
#endif

LIB3270_EXPORT const char	* lib3270_win32_strerror(int e);
LIB3270_EXPORT const char	* lib3270_win32_local_charset(void);
LIB3270_EXPORT LSTATUS		  lib3270_win32_create_regkey(LPCSTR lpSubKey, REGSAM samDesired, PHKEY phkResult);
LIB3270_EXPORT DWORD		  lib3270_win32_get_dword(HKEY hKey, const char *name, DWORD def);
LIB3270_EXPORT LSTATUS		  lib3270_win32_set_registry(LPCSTR module, LPCSTR keyname, LPCSTR value);
LIB3270_EXPORT LSTATUS		  lib3270_win32_set_string(LPCSTR module, LPCSTR keyname, LPCSTR value);

/**
 * @brief Translate windows error code.
 *
 * @param lasterror	Windows error code (from GetLastError()).
 *
 * @return String with translated message (release it with lib3270_free).
 *
 */
LIB3270_EXPORT char 		* lib3270_win32_translate_error_code(int lasterror);

/**
 * @brief Get lib3270's installation path.
 *
 * @return Full path for the lib3270 installation path (release it with lib3270_free)
 *
 */
LIB3270_EXPORT char		* lib3270_get_installation_path();



#ifdef __cplusplus
}
#endif

#endif // LIB3270_WIN32_H_INCLUDED
