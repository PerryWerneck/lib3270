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

/*
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

#include <config.h>
#include <lib3270.h>
#include <private/session.h>
#include <ctype.h>

/*--[ Implement ]------------------------------------------------------------------------------------*/

/**
 * Get field region
 *
 * @param hSession	Session handle.
 * @param baddr		Reference position to get the field start/stop offsets.
 * @param start		return location for start of selection, as a character offset.
 * @param end		return location for end of selection, as a character offset.
 *
 * @return Non zero if invalid or not connected (sets errno).
 *
 */
LIB3270_EXPORT int lib3270_get_field_bounds(H3270 *hSession, int baddr, int *start, int *end) {
	int first;

	first = lib3270_field_addr(hSession,baddr);

	if(first < 0)
		return -first;

	first++;

	if(start)
		*start = first;

	if(end) {
		int maxlen = (hSession->view.rows * hSession->view.cols)-1;
		*end	= first + lib3270_field_length(hSession,first);
		if(*end > maxlen)
			*end = maxlen;
	}

	return 0;
}

LIB3270_EXPORT int lib3270_get_word_bounds(H3270 *session, int baddr, int *start, int *end) {
	int pos;

	if(baddr < 0)
		baddr = lib3270_get_cursor_address(session);

	if(baddr > (int) lib3270_get_length(session)) {
		return errno = EINVAL;
	}

	if(!lib3270_is_connected(session))
		return errno = ENOTCONN;

	if(start) {
		for(pos = baddr; pos > 0 && !isspace(session->text[pos].chr); pos--);

		*start = pos > 0 ? pos+1 : 0;
	}

	if(end) {
		int maxlen = session->view.rows * session->view.cols;
		for(pos = baddr; pos < maxlen && !isspace(session->text[pos].chr); pos++);

		*end = pos < maxlen ? pos-1 : maxlen;
	}

	return 0;
}


