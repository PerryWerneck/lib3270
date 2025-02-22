/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright 1995, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006 by Paul Mattes.
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
 #include <private/toggle.h>

 #if defined(X3270_ANSI)

	LIB3270_INTERNAL void ansi_process(H3270 *hSession, unsigned int c);
	LIB3270_INTERNAL void ansi_send_clear(H3270 *hSession);
	LIB3270_INTERNAL void ansi_send_down(H3270 *hSession);
	LIB3270_INTERNAL void ansi_send_home(H3270 *hSession);
	LIB3270_INTERNAL void ansi_send_left(H3270 *hSession);
	LIB3270_INTERNAL void ansi_send_pa(H3270 *hSession, int nn);
	LIB3270_INTERNAL void ansi_send_pf(H3270 *hSession, int nn);
	LIB3270_INTERNAL void ansi_send_right(H3270 *hSession);
	LIB3270_INTERNAL void ansi_send_up(H3270 *hSession);
	LIB3270_INTERNAL void ansi_in3270(H3270 *session, int in3270, void *dunno);

	LIB3270_INTERNAL void toggle_lineWrap(H3270 *hSession, struct lib3270_toggle *t, LIB3270_TOGGLE_TYPE type);

 #else

	#define ansi_process(n)
	#define ansi_send_clear()
	#define ansi_send_down()
	#define ansi_send_home()
	#define ansi_send_left()
	#define ansi_send_pa(n)
	#define ansi_send_pf(n)
	#define ansi_send_right()
	#define ansi_send_up()

 #endif /*]*/
