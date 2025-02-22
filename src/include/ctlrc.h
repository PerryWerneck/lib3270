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

/*
 *	ctlrc.h
 *		Global declarations for ctlr.c.
 */

#pragma once

#include <config.h>
#include <private/defs.h>

enum pds {
	PDS_OKAY_NO_OUTPUT = 0,	/* command accepted, produced no output */
	PDS_OKAY_OUTPUT = 1,	/* command accepted, produced output */
	PDS_BAD_CMD = -1,	/* command rejected */
	PDS_BAD_ADDR = -2	/* command contained a bad address */
};

LIB3270_INTERNAL void ctlr_aclear(H3270 *session, int baddr, int count, int clear_ea);
LIB3270_INTERNAL void ctlr_add(H3270 *hSession, int baddr, unsigned char c, unsigned char cs);
LIB3270_INTERNAL void ctlr_add_bg(H3270 *hSession, int baddr, unsigned char color);
LIB3270_INTERNAL void ctlr_add_cs(H3270 *hSession, int baddr, unsigned char cs);
LIB3270_INTERNAL void ctlr_add_fa(H3270 *hSession, int baddr, unsigned char fa, unsigned char cs);
LIB3270_INTERNAL void ctlr_add_fg(H3270 *hSession, int baddr, unsigned char color);
LIB3270_INTERNAL void ctlr_add_gr(H3270 *hSession, int baddr, unsigned char gr);
LIB3270_INTERNAL void ctlr_altbuffer(H3270 *session, int alt);
LIB3270_INTERNAL int  ctlr_any_data(H3270 *session);
LIB3270_INTERNAL void ctlr_bcopy(H3270 *hSession, int baddr_from, int baddr_to, int count, int move_ea);
LIB3270_INTERNAL void ctlr_clear(H3270 *hSession, Boolean can_snap);
LIB3270_INTERNAL void ctlr_erase_all_unprotected(H3270 *hSession);
LIB3270_INTERNAL void ctlr_init(H3270 *session, unsigned cmask);
LIB3270_INTERNAL void ctlr_read_buffer(H3270 *session, unsigned char aid_byte);
LIB3270_INTERNAL void ctlr_read_modified(H3270 *hSession, unsigned char aid_byte, Boolean all);
LIB3270_INTERNAL void ctlr_model_changed(H3270 *session);
LIB3270_INTERNAL void ctlr_scroll(H3270 *hSession);
LIB3270_INTERNAL void ctlr_wrapping_memmove(H3270 *session, int baddr_to, int baddr_from, int count);
LIB3270_INTERNAL enum pds ctlr_write(H3270 *hSession, unsigned char buf[], int buflen, Boolean erase);
LIB3270_INTERNAL void ctlr_write_sscp_lu(H3270 *session, unsigned char buf[], int buflen);
LIB3270_INTERNAL void mdt_clear(H3270 *hSession, int baddr);
LIB3270_INTERNAL void mdt_set(H3270 *hSession, int baddr);

// #define next_unprotected(session, baddr0) lib3270_get_next_unprotected(session, baddr0)

LIB3270_INTERNAL enum pds process_ds(H3270 *hSession, unsigned char *buf, int buflen);
LIB3270_INTERNAL void ps_process(H3270 *hSession);

LIB3270_INTERNAL void update_model_info(H3270 *session, unsigned int model, unsigned int cols, unsigned int rows);
LIB3270_INTERNAL void ctlr_set_rows_cols(H3270 *session, int mn, int ovc, int ovr);
LIB3270_INTERNAL void ctlr_erase(H3270 *session, int alt);

// LIB3270_INTERNAL void ticking_start(H3270 *session, Boolean anyway);

enum dbcs_state {
	DBCS_NONE = 0,		///< @brief position is not DBCS
	DBCS_LEFT,			///< @brief position is left half of DBCS character
	DBCS_RIGHT,			///< @brief position is right half of DBCS character
	DBCS_SI,			///< @brief position is SI terminating DBCS subfield
	DBCS_SB,			///< @brief position is SBCS character after the SI
	DBCS_LEFT_WRAP,		///< @brief position is left half of split DBCS
	DBCS_RIGHT_WRAP,	///< @brief position is right half of split DBCS
	DBCS_DEAD			///< @brief position is dead left-half DBCS
};
#define IS_LEFT(d)	((d) == DBCS_LEFT || (d) == DBCS_LEFT_WRAP)
#define IS_RIGHT(d)	((d) == DBCS_RIGHT || (d) == DBCS_RIGHT_WRAP)
#define IS_DBCS(d)	(IS_LEFT(d) || IS_RIGHT(d))
#define SOSI(c)	(((c) == EBC_so)? EBC_si: EBC_so)

enum dbcs_why { DBCS_FIELD, DBCS_SUBFIELD, DBCS_ATTRIBUTE };

#if defined(X3270_DBCS)

	LIB3270_INTERNAL enum dbcs_state ctlr_lookleft_state(int baddr, enum dbcs_why *why);
	LIB3270_INTERNAL int ctlr_dbcs_postprocess(H3270 *hSession);

#else

	inline enum dbcs_state ctlr_lookleft_state(int baddr, enum dbcs_why *why) {
		return DBCS_NONE;
	}

	inline int ctlr_dbcs_postprocess(H3270 *hSession) {
		return 0;
	}

#endif /*]*/

/**
 * @brief DBCS state query.
 *
 * Takes line-wrapping into account, which probably isn't done all that well.
 *
 * @return DBCS state
 *
 * @retval DBCS_NONE	Buffer position is SBCS.
 * @retval DBCS_LEFT	Buffer position is left half of a DBCS character.
 * @retval DBCS_RIGHT:	Buffer position is right half of a DBCS character.
 * @retval DBCS_SI    	Buffer position is the SI terminating a DBCS subfield (treated as DBCS_LEFT for wide cursor tests)
 * @retval DBCS_SB		Buffer position is an SBCS character after an SI (treated as DBCS_RIGHT for wide cursor tests)
 *
 */
inline enum dbcs_state ctlr_dbcs_state(int baddr) {
#if defined(X3270_DBCS) /*[*/
	return dbcs? ea_buf[baddr].db: DBCS_NONE;
#else
	return DBCS_NONE;
#endif // X3270_DBCS
}
