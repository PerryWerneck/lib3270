/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270. Registro no INPI sob o nome G3270.
 *
 * Copyright (C) <2008> <Banco do Brasil S.A.>
 *
 * Este programa é software livre. Você pode redistribuí-lo e/ou modificá-lo sob
 * os termos da GPL v.2 - Licença Pública Geral  GNU,  conforme  publicado  pela
 * Free Software Foundation.
 *
 * Este programa é distribuído na expectativa de  ser  útil,  mas  SEM  QUALQUER
 * GARANTIA; sem mesmo a garantia implícita de COMERCIALIZAÇÃO ou  de  ADEQUAÇÃO
 * A QUALQUER PROPÓSITO EM PARTICULAR. Consulte a Licença Pública Geral GNU para
 * obter mais detalhes.
 *
 * Você deve ter recebido uma cópia da Licença Pública Geral GNU junto com este
 * programa; se não, escreva para a Free Software Foundation, Inc., 51 Franklin
 * St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Este programa está nomeado como - e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

/**
 *	@brief This module handles cursor moves.
 */


#include "private.h"
#include <lib3270/trace.h>
#include <lib3270/selection.h>

#include "kybdc.h"
#include "ctlrc.h"
#include "ansic.h"
#include "statusc.h"
#include "3270ds.h"

/*---[ Prototipes ]---------------------------------------------------------------------------------*/

static int cursor_left(H3270 *hSession);
static int cursor_right(H3270 *hSession);
static int cursor_up(H3270 *hSession);
static int cursor_down(H3270 *hSession);
static int cursor_end(H3270 *hSession);

/*---[ Globals ]------------------------------------------------------------------------------------*/

 static const struct {
	int (*exec)(H3270 *hSession);
 } calls[LIB3270_DIR_COUNT] = {
	{ cursor_up		},
	{ cursor_down	},
	{ cursor_left	},
	{ cursor_right	},
	{ cursor_end	}
 };


/*---[ Implement ]----------------------------------------------------------------------------------*/

/**
 * @brief Move cursor.
 *
 * @param hSession	Session handle.
 * @param dir		Where to move.
 * @param sel		Non zero if it's selecting.
 *
 * @return 0 if ok, non zero if not (sets errno).
 *
 */
LIB3270_EXPORT int lib3270_move_cursor(H3270 *hSession, LIB3270_DIRECTION dir, unsigned char sel)
{
	FAIL_IF_NOT_ONLINE(hSession);

	if(dir < 0 || dir >= LIB3270_DIR_COUNT)
	{
		return errno = EINVAL;
	}

	if (hSession->kybdlock)
	{
		if (KYBDLOCK_IS_OERR(hSession))
		{
			lib3270_kybdlock_clear(hSession,KL_OERR_MASK);
			status_reset(hSession);
		}
		else
		{
			struct ta *ta = new_ta(hSession, TA_TYPE_CURSOR_MOVE);

			ta->args.move.direction = dir;
			ta->args.move.fn = lib3270_move_cursor;
			ta->args.move.sel = sel;
			return 0;
		}
	}

	int rc = calls[dir].exec(hSession);
	if(rc)
		return rc;

	if(sel)
		lib3270_select_to(hSession,hSession->cursor_addr);
	else if(hSession->selected && !lib3270_get_toggle(hSession,LIB3270_TOGGLE_KEEP_SELECTED))
		lib3270_unselect(hSession);

	return 0;
}

LIB3270_EXPORT int lib3270_cursor_up(H3270 *hSession)
{
	return lib3270_move_cursor(hSession,LIB3270_DIR_UP,0);
}

LIB3270_EXPORT int lib3270_cursor_down(H3270 *hSession)
{
	return lib3270_move_cursor(hSession,LIB3270_DIR_DOWN,0);
}

LIB3270_EXPORT int lib3270_cursor_left(H3270 *hSession)
{
	return lib3270_move_cursor(hSession,LIB3270_DIR_LEFT,0);
}

LIB3270_EXPORT int lib3270_cursor_right(H3270 *hSession)
{
	return lib3270_move_cursor(hSession,LIB3270_DIR_RIGHT,0);
}

/**
 * @brief Cursor left 1 position.
 */
static void do_left(H3270 *hSession)
{
	register int	baddr;
	enum dbcs_state d;

	baddr = hSession->cursor_addr;
	DEC_BA(baddr);
	d = ctlr_dbcs_state(baddr);
	if (IS_LEFT(d))
		DEC_BA(baddr);
	cursor_move(hSession,baddr);
}

static int cursor_left(H3270 *hSession)
{
	if (hSession->kybdlock)
	{
		if(KYBDLOCK_IS_OERR(hSession))
		{
			lib3270_kybdlock_clear(hSession,KL_OERR_MASK);
			status_reset(hSession);
		}
		else
		{
			enq_action(hSession, cursor_left);
			return 0;
		}
	}
#if defined(X3270_ANSI) /*[*/
	if (IN_ANSI)
	{
		ansi_send_left(hSession);
		return 0;
	}
#endif /*]*/

	if (!hSession->flipped)
	{
		do_left(hSession);
	}
	else
	{
		register int	baddr;

		baddr = hSession->cursor_addr;
		INC_BA(baddr);
		/* XXX: DBCS? */
		cursor_move(hSession,baddr);
	}
	return 0;
}

static int cursor_right(H3270 *hSession)
{
	register int	baddr;
	enum dbcs_state d;

	if (hSession->kybdlock)
	{
		if (KYBDLOCK_IS_OERR(hSession))
		{
			lib3270_kybdlock_clear(hSession,KL_OERR_MASK);
			status_reset(hSession);
		}
		else
		{
			enq_action(hSession, cursor_right);
			return 0;
		}
	}
#if defined(X3270_ANSI) /*[*/
	if (IN_ANSI) {
		ansi_send_right(hSession);
		return 0;
	}
#endif /*]*/
	if (!hSession->flipped)
	{
		baddr = hSession->cursor_addr;
		INC_BA(baddr);
		d = ctlr_dbcs_state(baddr);
		if (IS_RIGHT(d))
			INC_BA(baddr);
		cursor_move(hSession,baddr);
	}
	else
	{
		do_left(hSession);
	}
	return 0;
}

static int cursor_up(H3270 *hSession)
{
	register int	baddr;

	trace("kybdlock=%d OERR=%s",(int) hSession->kybdlock, (KYBDLOCK_IS_OERR(hSession) ? "yes" : "no"));
	if (hSession->kybdlock)
	{
		if (KYBDLOCK_IS_OERR(hSession))
		{
			lib3270_kybdlock_clear(hSession,KL_OERR_MASK);
			status_reset(hSession);
		}
		else
		{
			enq_action(hSession, cursor_up);
			return 0;
		}
	}

#if defined(X3270_ANSI) /*[*/
	if (IN_ANSI) {
		ansi_send_up(hSession);
		return 0;
	}
#endif /*]*/

	baddr = hSession->cursor_addr - hSession->cols;
	if (baddr < 0)
		baddr = (hSession->cursor_addr + (hSession->rows * hSession->cols)) - hSession->cols;
	cursor_move(hSession,baddr);
	return 0;
}

static int cursor_down(H3270 *hSession)
{
	register int baddr;

	if (hSession->kybdlock)
	{
		if (KYBDLOCK_IS_OERR(hSession))
		{
			lib3270_kybdlock_clear(hSession,KL_OERR_MASK);
			status_reset(hSession);
		} else
		{
			enq_action(hSession, cursor_down);
//			enq_ta(Down_action, CN, CN);
			return 0;
		}
	}
#if defined(X3270_ANSI) /*[*/
	if (IN_ANSI)
	{
		ansi_send_down(hSession);
		return 0;
	}
#endif /*]*/
	baddr = (hSession->cursor_addr + hSession->cols) % (hSession->cols * hSession->rows);
	cursor_move(hSession,baddr);
	return 0;
}

static int cursor_end(H3270 *hSession)
{
	cursor_move(hSession,lib3270_get_field_end(hSession,hSession->cursor_addr));
	return 0;
}

