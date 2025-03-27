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

 LIB3270_INTERNAL const char *rcba(const H3270 *session, int baddr);

 LIB3270_INTERNAL char * trace_filename(const H3270 *session, const char *template);

 LIB3270_INTERNAL void trace_ansi_disc(H3270 *hSession);
 LIB3270_INTERNAL void trace_char(const H3270 *hSession, char c);

 /// @brief Trace a data stream message.
 /// @return 1 if trace is enabled, 0 otherwise.
 LIB3270_INTERNAL int trace_ds(const H3270 *hSession, const char *fmt, ...) LIB3270_GNUC_FORMAT(2, 3);
 
 LIB3270_INTERNAL void trace_ds_nb(const H3270 *hSession, const char *fmt, ...) LIB3270_GNUC_FORMAT(2, 3);
 LIB3270_INTERNAL void trace_dsn(const H3270 *hSession, const char *fmt, ...) LIB3270_GNUC_FORMAT(2, 3);
 LIB3270_INTERNAL int trace_ssl(const H3270 *hSession, const char *fmt, ...) LIB3270_GNUC_FORMAT(2, 3);
 LIB3270_INTERNAL void trace_screen(H3270 *hSession);
 LIB3270_INTERNAL void trace_data(const H3270 *hSession, const char *msg, const unsigned char *data, size_t datalen);
 LIB3270_INTERNAL void trace_event(const H3270 *hSession, const char *fmt, ...);
 LIB3270_INTERNAL void trace_network(const H3270 *hSession, const char *fmt, ...);

 #define trace(hSession, fmt, ... )	trace_event(hSession, fmt "\n", __VA_ARGS__ );
 