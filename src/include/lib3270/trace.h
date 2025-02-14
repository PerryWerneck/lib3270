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

 #pragma once

 #include <lib3270/defs.h>

 #ifdef __cplusplus
 extern "C" {
 #endif

 #ifdef _WIN32
 #define LIB3270_AS_PRINTF(a,b) /* __attribute__((format(printf, a, b))) */
 #else
 #define LIB3270_AS_PRINTF(a,b) __attribute__((format(printf, a, b)))
 #endif

 /// @brief Close trace for session.
 /// @param hSession 
 LIB3270_EXPORT void lib3270_trace_close(H3270 *hSession);

 LIB3270_EXPORT int lib3270_trace_open_file(H3270 *hSession, const char *filename);

 #ifdef __cplusplus
 }
 #endif
