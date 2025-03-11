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

 /// @brief Get trace filename.
 LIB3270_EXPORT const char * lib3270_trace_get_filename(const H3270 *hSession);

 /// @brief Open trace to file
 /// @param hSession The session to trace.
 /// @param filename The filename to write the trace.
 /// @return 0 if success, error code otherwise.
 LIB3270_EXPORT int lib3270_trace_open_file(H3270 *hSession, const char *filename);

/// @brief Open trace to console.
 /// @param hSession The session.
 /// @param option The log target (0 = stdout, 1 = stderr)
 LIB3270_EXPORT int lib3270_trace_open_console(H3270 *hSession, int option);

 /// @brief Close trace for session.
 /// @param hSession The session to close the trace.
 LIB3270_EXPORT void lib3270_trace_close(H3270 *hSession);

 #ifdef __cplusplus
 }
 #endif
