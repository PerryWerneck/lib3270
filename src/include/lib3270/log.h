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
 #include <time.h>
 #include <stdarg.h>

 #ifdef __cplusplus
 extern "C" {
 #endif

 #ifdef DEBUG
	#include <stdio.h>
	#define debug( fmt, ... )	fprintf(stderr, "%s(%d) " fmt "\n", __FILE__, __LINE__, __VA_ARGS__ ); fflush(stderr);
 #else
	#define debug( fmt, ... )	// fprintf(stderr, "%s(%d) " fmt "\n", __FILE__, __LINE__, __VA_ARGS__ ); fflush(stderr);
 #endif

 /// @brief Open log to file
 /// @param hSession The session.
 /// @param filename The filename to write the log (strftime formattted).
 /// @param maxage The maximum age of the log file.
 /// @return 0 if success, error code otherwise.
 LIB3270_EXPORT int lib3270_log_open_file(H3270 *hSession, const char *filename, time_t maxage);

 /// @brief Open log to syslog
 /// @param hSession The session.
 LIB3270_EXPORT int lib3270_log_open_syslog(H3270 *hSession);

 /// @brief Close log for session.
 /// @param hSession The session to close the log.
 LIB3270_EXPORT void lib3270_log_close(H3270 *hSession);

 LIB3270_EXPORT int lib3270_log_write(const H3270 *session, const char *module, const char *fmt, ...) LIB3270_GNUC_FORMAT(3,4);
 LIB3270_EXPORT int lib3270_log_write_rc(const H3270 *session, const char *module, int rc, const char *fmt, ...) LIB3270_GNUC_FORMAT(4,5);
 LIB3270_EXPORT void lib3270_log_write_va(const H3270 *session, const char *module, const char *fmt, va_list arg);

#ifdef __cplusplus
}
#endif
