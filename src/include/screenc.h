/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright 1995, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006 by Paul Mattes.
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

#ifndef SCREENC_H_INCLUDED

#define SCREENC_H_INCLUDED 1
/* c3270 version of screenc.h */

// #define blink_start()
#define display_heightMM()	100
#define display_height()	1
#define display_widthMM()	100
#define display_width()		1

LIB3270_INTERNAL int		screen_init(H3270 *session);
// LIB3270_INTERNAL void		mcursor_set(H3270 *session,LIB3270_POINTER m);

LIB3270_INTERNAL void notify_toggle_changed(H3270 *session, LIB3270_TOGGLE_ID ix, unsigned char value, LIB3270_TOGGLE_TYPE reason);
LIB3270_INTERNAL void set_viewsize(H3270 *session, unsigned int rows, unsigned int cols);

// LIB3270_INTERNAL Boolean escaped;


// LIB3270_INTERNAL void screen_title(char *text);

#endif // SCREENC_H_INCLUDED
