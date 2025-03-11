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

#include <internals.h>

/*---[ Implement ]------------------------------------------------------------------------------------------------------------*/

LIB3270_EXPORT LIB3270_CSTATE lib3270_get_connection_state(const H3270 *h) {
	return h->connection.state;
}

LIB3270_EXPORT int lib3270_pconnected(const H3270 *h) {
	return (((int) h->connection.state) >= (int)LIB3270_CONNECTING);
}

LIB3270_EXPORT int lib3270_half_connected(const H3270 *h) {
	return (h->connection.state == LIB3270_CONNECTING || h->connection.state == LIB3270_PENDING);
}

LIB3270_EXPORT int lib3270_is_disconnected(const H3270 *h) {
	return ((int) h->connection.state == (int)LIB3270_NOT_CONNECTED);
}

LIB3270_EXPORT int lib3270_in_neither(const H3270 *h) {
	return (h->connection.state == LIB3270_CONNECTED_INITIAL);
}

LIB3270_EXPORT int lib3270_in_ansi(const H3270 *h) {
	return (h->connection.state == LIB3270_CONNECTED_ANSI || h->connection.state == LIB3270_CONNECTED_NVT);
}

LIB3270_EXPORT int lib3270_in_3270(const H3270 *h) {
	return (h->connection.state == LIB3270_CONNECTED_3270 || h->connection.state == LIB3270_CONNECTED_TN3270E || h->connection.state == LIB3270_CONNECTED_SSCP);
}

LIB3270_EXPORT int lib3270_in_sscp(const H3270 *h) {
	return (h->connection.state == LIB3270_CONNECTED_SSCP);
}

LIB3270_EXPORT int lib3270_in_tn3270e(const H3270 *h) {
	return (h->connection.state == LIB3270_CONNECTED_TN3270E);
}

LIB3270_EXPORT int lib3270_is_connected(const H3270 *h) {
	return ((int) h->connection.state >= (int)LIB3270_CONNECTED_INITIAL);
}

LIB3270_EXPORT int lib3270_in_e(const H3270 *h) {
	return (h->connection.state >= LIB3270_CONNECTED_INITIAL_E);
}

LIB3270_EXPORT int lib3270_is_unlocked(const H3270 *h) {
	return !(h->kybdlock) && lib3270_is_connected(h);
}


