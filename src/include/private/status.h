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

LIB3270_INTERNAL void 		status_compose(int on, unsigned char c, enum keytype keytype);
LIB3270_INTERNAL void 		status_ctlr_done(H3270 *session);

LIB3270_INTERNAL void 		status_oerr(H3270 *session, int error_type);
LIB3270_INTERNAL void 		status_reset(H3270 *session);
LIB3270_INTERNAL void 		status_twait(H3270 *session);

LIB3270_INTERNAL void 		message_changed(H3270 *session, LIB3270_MESSAGE id);
LIB3270_INTERNAL void 		set_status(H3270 *session, LIB3270_FLAG id, Boolean on);


#define status_typeahead(h,on)	set_status(h,LIB3270_FLAG_TYPEAHEAD,on)

