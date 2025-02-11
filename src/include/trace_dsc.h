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

/**
 *	@brief Global declarations for trace_ds.c.
 */

#if defined(X3270_TRACE)

const char *rcba(H3270 *session, int baddr);

void trace_ansi_disc(H3270 *hSession);
void trace_char(H3270 *hSession, char c);
void trace_ds(H3270 *hSession, const char *fmt, ...) LIB3270_GNUC_FORMAT(2, 3);
void trace_ds_nb(H3270 *hSession, const char *fmt, ...) LIB3270_GNUC_FORMAT(2, 3);
void trace_dsn(H3270 *hSession, const char *fmt, ...) LIB3270_GNUC_FORMAT(2, 3);
void trace_ssl(H3270 *hSession, const char *fmt, ...) LIB3270_GNUC_FORMAT(2, 3);
void trace_screen(H3270 *session);

#elif defined(__GNUC__)

#define trace_ds(session, format, args...)
#define trace_dsn(session, format, args...)
#define trace_ssl(session, format, args...)
#define trace_ds_nb(session, format, args...)

#else

#define trace_ds 0 &&
#define trace_ds_nb 0 &&
#define trace_dsn 0 &&
#define trace_ssl 0 &&
#define rcba 0 &&

#endif
