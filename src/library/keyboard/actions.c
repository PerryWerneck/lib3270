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
 * perry.werneck@gmail.com      (Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com     (Erico Mascarenhas Mendonça)
 *
 */

#pragma GCC diagnostic ignored "-Wsign-compare"


struct ta;

#define LIB3270_TA struct ta

#include <config.h>
#include <internals.h>
#include <lib3270/trace.h>
#include <lib3270/selection.h>
#include <lib3270/log.h>

#ifndef ANDROID
#include <stdlib.h>
#endif // !ANDROID

#if defined(X3270_DISPLAY) /*[*/
#include <X11/Xatom.h>
#endif
#define XK_3270
#if defined(X3270_APL) /*[*/
#define XK_APL
#endif /*]*/

#include <fcntl.h>
#include <private/3270ds.h>
//#include "resources.h"

#include <private/ansi.h>
#include <private/ctlr.h>
#include "ftc.h"
#include <private/host.h>
#include "kybdc.h"
#include <private/popup.h>
#include "screenc.h"
#include <private/screen.h>
#include <private/status.h>
#include "telnetc.h"
#include <private/toggle.h>
#include <private/trace.h>
//#include "utf8c.h"
#include <private/util.h>
#if defined(X3270_DBCS) /*[*/
#include "widec.h"
#endif /*]*/

#include <lib3270/actions.h>

LIB3270_EXPORT int lib3270_break(H3270 *hSession) {
	if (!IN_3270)
		return errno = ENOTCONN;

	net_break(hSession);

	return 0;
}

/***
 * @brief ATTN key, per RFC 2355.  Sends IP, regardless.
 */
LIB3270_EXPORT int lib3270_attn(H3270 *hSession) {
	if (!IN_3270)
		return errno = ENOTCONN;

	net_interrupt(hSession);

	return 0;
}

LIB3270_EXPORT int lib3270_nextfield(H3270 *hSession) {

	FAIL_IF_NOT_ONLINE(hSession);

	if (hSession->kybdlock) {
		if(KYBDLOCK_IS_OERR(hSession)) {
			lib3270_kybdlock_clear(hSession,KL_OERR_MASK);
			status_reset(hSession);
		} else {
			enq_action(hSession, lib3270_nextfield);
			return 0;
		}
	}
#if defined(X3270_ANSI) /*[*/
	if (IN_ANSI) {
		net_sendc(hSession,'\t');
		return 0;
	}
#endif /*]*/
	cursor_move(hSession,lib3270_get_next_unprotected(hSession,hSession->cursor_addr));
	return 0;
}

/**
 * @brief Tab backward to previous field.
 */
LIB3270_EXPORT int lib3270_previousfield(H3270 *hSession) {
	register int	baddr, nbaddr;
	int		sbaddr;

	FAIL_IF_NOT_ONLINE(hSession);

	if (hSession->kybdlock) {
		if (KYBDLOCK_IS_OERR(hSession)) {
			lib3270_kybdlock_clear(hSession,KL_OERR_MASK);
			status_reset(hSession);
		} else {
			enq_action(hSession, lib3270_previousfield);
			return 0;
		}
	}
	if (!IN_3270)
		return 0;
	baddr = hSession->cursor_addr;
	DEC_BA(baddr);
	if (hSession->ea_buf[baddr].fa)	/* at bof */
		DEC_BA(baddr);
	sbaddr = baddr;
	while (True) {
		nbaddr = baddr;
		INC_BA(nbaddr);
		if (hSession->ea_buf[baddr].fa &&
		        !FA_IS_PROTECTED(hSession->ea_buf[baddr].fa) &&
		        !hSession->ea_buf[nbaddr].fa)
			break;
		DEC_BA(baddr);
		if (baddr == sbaddr) {
			cursor_move(hSession,0);
			return 0;
		}
	}
	INC_BA(baddr);
	cursor_move(hSession,baddr);
	return 0;
}

/**
 * @brief Move to first unprotected field on screen.
 */
LIB3270_EXPORT int lib3270_firstfield(H3270 *hSession) {
	FAIL_IF_NOT_ONLINE(hSession);

	if (hSession->kybdlock) {
		enq_action(hSession, lib3270_firstfield);
		return 0;
	}
#if defined(X3270_ANSI) /*[*/
	if (IN_ANSI) {
		ansi_send_home(hSession);
		return 0;
	}
#endif /*]*/
	if (!hSession->formatted) {
		cursor_move(hSession,0);
		return 0;
	}

	cursor_move(hSession,lib3270_get_next_unprotected(hSession,hSession->view.rows * hSession->view.cols-1));

	return 0;
}

/**
 * @brief Cursor left 1 position.
 */
static void do_left(H3270 *hSession) {
	register int	baddr;

	baddr = hSession->cursor_addr;
	DEC_BA(baddr);
#ifdef X3270_DBCS
	enum dbcs_state d = ctlr_dbcs_state(baddr);
	if (IS_LEFT(d))
		DEC_BA(baddr);
#endif // X3270_DBCS
	cursor_move(hSession,baddr);
}

/**
 * @brief Delete char key.
 *
 * @param hSession	Session handle
 *
 * @Return "True" if succeeds, "False" otherwise.
 */
static Boolean do_delete(H3270 *hSession) {
	register int	baddr, end_baddr;
	int xaddr;
	register unsigned char	fa;
	int ndel;
	register int i;

	baddr = hSession->cursor_addr;

	/* Can't delete a field attribute. */
	fa = get_field_attribute(hSession,baddr);
	if (FA_IS_PROTECTED(fa) || hSession->ea_buf[baddr].fa) {
		operator_error(hSession,KL_OERR_PROTECTED);
		return False;
	}

	if (hSession->ea_buf[baddr].cc == EBC_so || hSession->ea_buf[baddr].cc == EBC_si) {
		/*
		 * Can't delete SO or SI, unless it's adjacent to its
		 * opposite.
		 */
		xaddr = baddr;
		INC_BA(xaddr);
		if (hSession->ea_buf[xaddr].cc == SOSI(hSession->ea_buf[baddr].cc)) {
			ndel = 2;
		} else {
			operator_error(hSession,KL_OERR_PROTECTED);
			return False;
		}
	} else if (IS_DBCS(hSession->ea_buf[baddr].db)) {
		if (IS_RIGHT(hSession->ea_buf[baddr].db))
			DEC_BA(baddr);
		ndel = 2;
	} else
		ndel = 1;

	/* find next fa */
	if (hSession->formatted) {
		end_baddr = baddr;
		do {
			INC_BA(end_baddr);
			if (hSession->ea_buf[end_baddr].fa)
				break;
		} while (end_baddr != baddr);
		DEC_BA(end_baddr);
	} else {
		if ((baddr % hSession->view.cols) == hSession->view.cols - ndel)
			return True;
		end_baddr = baddr + (hSession->view.cols - (baddr % hSession->view.cols)) - 1;
	}

	/* Shift the remainder of the field left. */
	if (end_baddr > baddr) {
		ctlr_bcopy(hSession,baddr + ndel, baddr, end_baddr - (baddr + ndel) + 1, 0);
	} else if (end_baddr != baddr) {
		/* XXX: Need to verify this. */
		ctlr_bcopy(hSession,baddr + ndel, baddr,((hSession->view.rows * hSession->view.cols) - 1) - (baddr + ndel) + 1, 0);
		ctlr_bcopy(hSession,0, (hSession->view.rows * hSession->view.cols) - ndel, ndel, 0);
		ctlr_bcopy(hSession,ndel, 0, end_baddr - ndel + 1, 0);
	}

	/* NULL fill at the end. */
	for (i = 0; i < ndel; i++)
		ctlr_add(hSession,end_baddr - i, EBC_null, 0);

	/* Set the MDT for this field. */
	mdt_set(hSession,hSession->cursor_addr);

#ifdef X3270_DBCS
	/* Patch up the DBCS state for display. */
	(void) ctlr_dbcs_postprocess(hSession);
#endif // X3270_DBCS

	return True;
}

LIB3270_EXPORT int lib3270_delete(H3270 *hSession) {
	FAIL_IF_NOT_ONLINE(hSession);

	if (hSession->kybdlock) {
		enq_action(hSession, lib3270_delete);
		return 0;
	}
#if defined(X3270_ANSI) /*[*/
	if (IN_ANSI) {
		net_sendc(hSession,'\177');
		return 0;
	}
#endif /*]*/
	if (!do_delete(hSession))
		return 0;
	if (hSession->reverse) {
		int baddr = hSession->cursor_addr;

		DEC_BA(baddr);
		if (!hSession->ea_buf[baddr].fa)
			cursor_move(hSession,baddr);
	}
	hSession->cbk.display(hSession);
	return 0;
}

/**
 * @brief 3270-style backspace.
 */
LIB3270_EXPORT int lib3270_backspace(H3270 *hSession) {
	FAIL_IF_NOT_ONLINE(hSession);

	if (hSession->kybdlock) {
		enq_action(hSession, lib3270_backspace );
		return 0;
	}
#if defined(X3270_ANSI) /*[*/
	if (IN_ANSI) {
		net_send_erase(hSession);
		return 0;
	}
#endif /*]*/
	if (hSession->reverse)
		(void) do_delete(hSession);
	else if (!hSession->flipped)
		do_left(hSession);
	else {
		register int	baddr;

		baddr = hSession->cursor_addr;
		DEC_BA(baddr);
		cursor_move(hSession,baddr);
	}
	hSession->cbk.display(hSession);
	return 0;
}

/**
 * @brief Destructive backspace, like Unix "erase".
 */
static void do_erase(H3270 *hSession) {
	int	baddr, faddr;

	baddr = hSession->cursor_addr;
	faddr = lib3270_field_addr(hSession,baddr);
	if (faddr == baddr || FA_IS_PROTECTED(hSession->ea_buf[baddr].fa)) {
		operator_error(hSession,KL_OERR_PROTECTED);
		return;
	}

	if (baddr && faddr == baddr - 1)
		return;

	do_left(hSession);

	/*
	 * If we are now on an SI, move left again.
	 */
	if (hSession->ea_buf[hSession->cursor_addr].cc == EBC_si) {
		baddr = hSession->cursor_addr;
		DEC_BA(baddr);
		cursor_move(hSession,baddr);
	}

	/*
	 * If we landed on the right-hand side of a DBCS character, move to the
	 * left-hand side.
	 * This ensures that if this is the end of a DBCS subfield, we will
	 * land on the SI, instead of on the character following.
	 */
#ifdef X3270_DBCS
	enum dbcs_state d = ctlr_dbcs_state(hSession->cursor_addr);
	if (IS_RIGHT(d)) {
		baddr = hSession->cursor_addr;
		DEC_BA(baddr);
		cursor_move(hSession,baddr);
	}
#endif // X3270_DBCS

	/*
	 * Try to delete this character.
	 */
	if (!do_delete(hSession))
		return;

	/*
	 * If we've just erased the last character of a DBCS subfield, erase
	 * the SO/SI pair as well.
	 */
	baddr = hSession->cursor_addr;
	DEC_BA(baddr);
	if (hSession->ea_buf[baddr].cc == EBC_so && hSession->ea_buf[hSession->cursor_addr].cc == EBC_si) {
		cursor_move(hSession,baddr);
		(void) do_delete(hSession);
	}
	hSession->cbk.display(hSession);
}

int lib3270_erase(H3270 *hSession) {
	FAIL_IF_NOT_ONLINE(hSession);

	if (hSession->kybdlock) {
		enq_action(hSession, lib3270_erase);
		return 0;
	}
#if defined(X3270_ANSI) /*[*/
	if (IN_ANSI) {
		net_send_erase(hSession);
		return 0;
	}
#endif /*]*/
	do_erase(hSession);
	return 0;
}

/**
 * @brief Cursor to previous word.
 */
LIB3270_EXPORT int lib3270_previousword(H3270 *hSession) {
	register int baddr;
	int baddr0;
	unsigned char  c;
	Boolean prot;

	FAIL_IF_NOT_ONLINE(hSession);

	if (hSession->kybdlock) {
		enq_action(hSession, lib3270_previousword);
//		enq_ta(PreviousWord_action, CN, CN);
		return 0;
	}
#if defined(X3270_ANSI) /*[*/
	if (IN_ANSI)
		return 0;
#endif /*]*/
	if (!hSession->formatted)
		return 0;

	baddr = hSession->cursor_addr;
	prot = FA_IS_PROTECTED(get_field_attribute(hSession,baddr));

	/* Skip to before this word, if in one now. */
	if (!prot) {
		c = hSession->ea_buf[baddr].cc;
		while (!hSession->ea_buf[baddr].fa && c != EBC_space && c != EBC_null) {
			DEC_BA(baddr);
			if (baddr == hSession->cursor_addr)
				return 0;
			c = hSession->ea_buf[baddr].cc;
		}
	}
	baddr0 = baddr;

	/* Find the end of the preceding word. */
	do {
		c = hSession->ea_buf[baddr].cc;
		if (hSession->ea_buf[baddr].fa) {
			DEC_BA(baddr);
			prot = FA_IS_PROTECTED(get_field_attribute(hSession,baddr));
			continue;
		}
		if (!prot && c != EBC_space && c != EBC_null)
			break;
		DEC_BA(baddr);
	} while (baddr != baddr0);

	if (baddr == baddr0)
		return 0;

	/* Go it its front. */
	for (;;) {
		DEC_BA(baddr);
		c = hSession->ea_buf[baddr].cc;
		if (hSession->ea_buf[baddr].fa || c == EBC_space || c == EBC_null) {
			break;
		}
	}
	INC_BA(baddr);
	cursor_move(hSession,baddr);
	return 0;
}

/**
 * @brief Delete word key.

 * Backspaces the cursor until it hits the front of a word,
 * deletes characters until it hits a blank or null, and deletes all of these
 * but the last.
 *
 * Which is to say, does a ^W.
 */
LIB3270_EXPORT int lib3270_deleteword(H3270 *hSession) {
	register int baddr;
	register unsigned char	fa;

//	reset_idle_timer();
	if (hSession->kybdlock) {
		enq_action(hSession, lib3270_deleteword);
//		enq_ta(DeleteWord_action, CN, CN);
		return 0;
	}
#if defined(X3270_ANSI) /*[*/
	if (IN_ANSI) {
		net_send_werase(hSession);
		return 0;
	}
#endif /*]*/
	if (!hSession->formatted)
		return 0;

	baddr = hSession->cursor_addr;
	fa = get_field_attribute(hSession,baddr);

	/* Make sure we're on a modifiable field. */
	if (FA_IS_PROTECTED(fa) || hSession->ea_buf[baddr].fa) {
		operator_error(hSession,KL_OERR_PROTECTED);
		return errno = EPERM;
	}

	/* Backspace over any spaces to the left of the cursor. */
	for (;;) {
		baddr = hSession->cursor_addr;
		DEC_BA(baddr);
		if (hSession->ea_buf[baddr].fa)
			return 0;
		if (hSession->ea_buf[baddr].cc == EBC_null ||
		        hSession->ea_buf[baddr].cc == EBC_space)
			do_erase(hSession);
		else
			break;
	}

	/* Backspace until the character to the left of the cursor is blank. */
	for (;;) {
		baddr = hSession->cursor_addr;
		DEC_BA(baddr);
		if (hSession->ea_buf[baddr].fa)
			return 0;
		if (hSession->ea_buf[baddr].cc == EBC_null ||
		        hSession->ea_buf[baddr].cc == EBC_space)
			break;
		else
			do_erase(hSession);
	}
	hSession->cbk.display(hSession);
	return 0;
}
