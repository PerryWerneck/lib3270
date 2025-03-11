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

 #ifdef _WIN32
	#include <winsock2.h>
	#include <windows.h>
 #endif // _WIN32
 
 #include <lib3270/defs.h>
 #include <string.h>

 //
 // @brief Get a block of memory, fill it with zeros.
 //
 // @param len	Length of memory block to get.
 //
 // @return Pointer to new memory block.
 //
 //
 LIB3270_EXPORT void * lib3270_malloc(int len);

 #define lib3270_new(x) (x *) lib3270_malloc(sizeof(x))

 #define lib3270_replace(x,v) if(x) { lib3270_free(x); }; x = strdup(v)

 /**
 * @brief Release allocated memory.
 *
 * @param p	Memory block to release (can be NULL)
 *
 * @return NULL
 */
 LIB3270_EXPORT void  * lib3270_free(void *p);

 LIB3270_EXPORT void * lib3270_realloc(void *p, int len);
 LIB3270_EXPORT void * lib3270_strdup(const char *str);

 #ifdef _WIN32
 LIB3270_EXPORT void lib3270_autoptr_cleanup_HKEY(HKEY *hKey);
 #endif // _WIN32

 LIB3270_EXPORT void lib3270_autoptr_cleanup_char(char **ptr);
