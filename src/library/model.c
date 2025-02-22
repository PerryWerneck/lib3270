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

/*
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

#include <config.h>
#include <lib3270/defs.h>
#include <private/defs.h>

#include <string.h>
#include <stdlib.h>
#include <private/screen.h>
#include <private/ctlr.h>
#include <private/popup.h>
#include <lib3270/trace.h>
#include <lib3270/log.h>
#include <lib3270/properties.h>
#include <lib3270/malloc.h>
#include <private/session.h>

const char * lib3270_get_oversize(const H3270 *hSession) {
	return hSession->oversize.str;
}

int lib3270_set_oversize(H3270 *hSession, const char *value) {
	if(hSession->connection.state != LIB3270_NOT_CONNECTED)
		return errno = EISCONN;

	if(!hSession->extended)
		return errno = ENOTSUP;

	if(hSession->oversize.str) {
		// Do nothing if it's the same value!
		if(value && !strcasecmp(hSession->oversize.str,value))
			return 0;

		lib3270_free(hSession->oversize.str);
		hSession->oversize.str = NULL;
	}

	int ovc = 0, ovr = 0;

	if(value) {
		char junk;

		if(sscanf(value, "%dx%d%c", &ovc, &ovr, &junk) != 2)
			return errno = EINVAL;

		hSession->oversize.str = lib3270_strdup(value);

	}

	ctlr_set_rows_cols(hSession, hSession->model_num, ovc, ovr);
	ctlr_model_changed(hSession);
	screen_update(hSession,0,hSession->view.rows*hSession->view.cols);

	return 0;

}

/**
 * @brief Get current 3270 model.
 *
 * @param hSession selected 3270 session.
 * @return Current model number.
 */
unsigned int lib3270_get_model_number(const H3270 *hSession) {
	return hSession->model_num;
}

const char * lib3270_get_model(const H3270 *hSession) {
	return hSession->model_name;
}

const char * lib3270_get_model_name(const H3270 *hSession) {
	return hSession->model_name;
}

/**
 * @brief Parse the model number.
 *
 * @param session	Session Handle.
 * @param m		Model number (NULL for "2").
 *
 * @return -1 (error), 0 (default), or the specified number.
 */
static int parse_model_number(H3270 *session, const char *m) {
	int sl;
	int n;

	if(!m)
		m = "2";

	sl = strlen(m);

	// An empty model number is no good.
	if (!sl)
		return 0;

	if (sl > 1) {

		// If it's longer than one character, it needs to start with
		// '327[89]', and it sets the m3279 resource.

		if (!strncmp(m, "3278", 4)) {
			session->m3279 = 0;
		} else if (!strncmp(m, "3279", 4)) {
			session->m3279 = 1;
		} else {
			return -1;
		}
		m += 4;
		sl -= 4;

		// Check more syntax.  -E is allowed, but ignored.
		switch (m[0]) {
		case '\0':
			// Use default model number.
			return 0;
		case '-':
			// Model number specified.
			m++;
			sl--;
			break;
		default:
			return -1;
		}
		switch (sl) {
		case 1: // n
			break;
		case 3:	// n-E
			if (strcasecmp(m + 1, "-E")) {
				return -1;
			}
			break;
		default:
			return -1;
		}
	}

	// Check the numeric model number.
	n = atoi(m);
	if (n >= 2 && n <= 5) {
		return n;
	} else {
		return -1;
	}

}

int lib3270_set_model_name(H3270 *hSession, const char *model_name) {
	return lib3270_set_model_number(hSession,parse_model_number(hSession, model_name));
}

int lib3270_set_model(H3270 *hSession, const char *model_name) {
	return lib3270_set_model_number(hSession,parse_model_number(hSession, model_name));
}

int lib3270_set_model_number(H3270 *hSession, unsigned int model_number) {
	if(hSession->connection.state != LIB3270_NOT_CONNECTED)
		return errno = EISCONN;

	strncpy(hSession->full_model_name,"IBM-",LIB3270_FULL_MODEL_NAME_LENGTH);
	hSession->model_name = &hSession->full_model_name[4];

	if (!model_number) {
#if defined(RESTRICT_3279)
		model_number = 3;
#else
		model_number = 4;
#endif
	}

	if(hSession->mono)
		hSession->m3279 = 0;
	else
		hSession->m3279 = 1;

	if(!hSession->extended) {
		if(hSession->oversize.str)
			lib3270_free(hSession->oversize.str);
		hSession->oversize.str = CN;
	}

#if defined(RESTRICT_3279)
	if (hSession->m3279 && model_number == 4)
		model_number = 3;
#endif

	debug("Model_number: %d",model_number);

	// Check for oversize
	char junk;
	int ovc, ovr;

	if (!hSession->extended || hSession->oversize.str == CN || sscanf(hSession->oversize.str, "%dx%d%c", &ovc, &ovr, &junk) != 2) {
		ovc = 0;
		ovr = 0;
	}

	ctlr_set_rows_cols(hSession, model_number, ovc, ovr);
	ctlr_model_changed(hSession);
	screen_update(hSession,0,hSession->view.rows*hSession->view.cols);

	return 0;
}

