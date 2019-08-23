/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270.
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

 #include <lib3270-internals.h>
 #include <lib3270.h>
 #include <lib3270/actions.h>
 #include <lib3270/session.h>
 #include <lib3270/selection.h>
 #include <lib3270/log.h>
 #include <lib3270/trace.h>
 #include <lib3270/toggle.h>
 #include "3270ds.h"

/*--[ Implement ]------------------------------------------------------------------------------------*/

LIB3270_EXPORT int lib3270_unselect(H3270 *hSession)
{
	int a;

	CHECK_SESSION_HANDLE(hSession);

	trace("%s",__FUNCTION__);

	if(hSession->selected)
	{
		hSession->selected = 0;

		for(a = 0; a < ((int) (hSession->view.rows * hSession->view.cols)); a++)
		{
			if(hSession->text[a].attr & LIB3270_ATTR_SELECTED)
			{
				hSession->text[a].attr &= ~LIB3270_ATTR_SELECTED;
				if(hSession->cbk.update)
					hSession->cbk.update(hSession,a,hSession->text[a].chr,hSession->text[a].attr,a == hSession->cursor_addr);
			}
		}

		hSession->cbk.set_selection(hSession,0);
		hSession->cbk.update_selection(hSession,-1,-1);
	}

	return 0;
}

LIB3270_EXPORT void lib3270_select_to(H3270 *session, int baddr)
{
	int start, end;

	CHECK_SESSION_HANDLE(session);

	if(!lib3270_connected(session))
		return;

	start = session->selected ? session->select.start : session->cursor_addr;

	cursor_move(session,end = baddr);

	do_select(session,start,end,lib3270_get_toggle(session,LIB3270_TOGGLE_RECTANGLE_SELECT));

}

LIB3270_EXPORT int lib3270_select_region(H3270 *h, int start, int end)
{
	int maxlen;

	CHECK_SESSION_HANDLE(h);

	if(!lib3270_connected(h))
		return ENOTCONN;

	maxlen = (h->view.rows * h->view.cols);

	// Check bounds
	if(start < 0 || start > maxlen || end < 0 || end > maxlen || start > end)
		return EINVAL;

	do_select(h,start,end,lib3270_get_toggle(h,LIB3270_TOGGLE_RECTANGLE_SELECT));
	cursor_move(h,h->select.end);

	return 0;
}

LIB3270_EXPORT int lib3270_select_word_at(H3270 *session, int baddr)
{
	int start, end;

	if(lib3270_get_word_bounds(session,baddr,&start,&end))
		return -1;

	trace("%s: baddr=%d start=%d end=%d",__FUNCTION__,baddr,start,end);

	do_select(session,start,end,0);

	return 0;
}

LIB3270_EXPORT int lib3270_select_field_at(H3270 *session, int baddr)
{
	int start, end;

	if(lib3270_get_field_bounds(session,baddr,&start,&end))
		return -1;

	do_select(session,start,end,0);

	return 0;
}

LIB3270_EXPORT int lib3270_select_field(H3270 *hSession)
{
	CHECK_SESSION_HANDLE(hSession);
	lib3270_select_field_at(hSession,hSession->cursor_addr);
	return 0;
}

LIB3270_EXPORT int lib3270_select_all(H3270 * hSession)
{
	FAIL_IF_NOT_ONLINE(hSession);

	do_select(hSession,0,(hSession->view.rows * hSession->view.cols)-1,0);

	return 0;
}

LIB3270_EXPORT int lib3270_reselect(H3270 *hSession)
{
	FAIL_IF_NOT_ONLINE(hSession);

	if(hSession->select.start == hSession->select.end || hSession->selected)
		return 0;

	do_select(hSession, hSession->select.start,hSession->select.end,lib3270_get_toggle(hSession,LIB3270_TOGGLE_RECTANGLE_SELECT));

	return 0;
}

LIB3270_EXPORT int lib3270_get_selection_bounds(H3270 *hSession, int *start, int *end)
{
	int first, last;

	CHECK_SESSION_HANDLE(hSession);

	if(!hSession->selected || hSession->select.start == hSession->select.end)
		return 0;

	if(hSession->select.end > hSession->select.start)
	{
		first = hSession->select.start;
		last  = hSession->select.end;
	}
	else
	{
		first = hSession->select.end;
		last  = hSession->select.start;
	}

	if(start)
		*start = first;

	if(end)
		*end = last;

	return 1;
}

LIB3270_EXPORT int lib3270_move_selected_area(H3270 *hSession, int from, int to)
{
	int pos[2];
	int rows, cols, f, step;

	if(from == to)
		return from;

	if(!lib3270_get_selection_bounds(hSession,&pos[0],&pos[1]))
		return from;

	rows = (to / hSession->view.cols) - (from / hSession->view.cols);
	cols = (to % hSession->view.cols) - (from % hSession->view.cols);

	for(f=0;f<2;f++)
	{
		int row  = (pos[f] / hSession->view.cols) + rows;
		int col  = (pos[f] % hSession->view.cols) + cols;

		if(row < 0)
			rows = - (pos[f] / hSession->view.cols);

		if(col < 0)
			cols = - (pos[f] % hSession->view.cols);

		if(row >= ((int) hSession->view.rows))
			rows = hSession->view.rows - ((pos[f] / hSession->view.cols)+1);

		if(col >= ((int) hSession->view.cols))
			cols = hSession->view.cols - ((pos[f] % hSession->view.cols)+1);
	}

	step = (rows * hSession->view.cols) + cols;

	do_select(hSession,hSession->select.start + step,hSession->select.end + step,hSession->rectsel);
	cursor_move(hSession,hSession->select.end);

	return from+step;
}

LIB3270_EXPORT int lib3270_drag_selection(H3270 *h, unsigned char flag, int origin, int baddr)
{
	int first, last, row, col;

	if(!lib3270_get_selection_bounds(h,&first,&last))
		return origin;

/*
	trace("%s: flag=%04x %s %s %s %s",__FUNCTION__,
				flag,
				flag & SELECTION_LEFT	? "Left"	: "-",
				flag & SELECTION_TOP	? "Top"		: "-",
				flag & SELECTION_RIGHT	? "Right"	: "-",
				flag & SELECTION_BOTTOM	? "Bottom"	: "-"
				);
*/

	if(!flag)
		return origin;
	else if((flag&0x8F) == SELECTION_ACTIVE)
		return lib3270_move_selected_area(h,origin,baddr);

	row = baddr/h->view.cols;
	col = baddr%h->view.cols;

	if(flag & SELECTION_LEFT)		// Update left margin
		origin = first = ((first/h->view.cols)*h->view.cols) + col;

	if(flag & SELECTION_TOP)		// Update top margin
		origin = first = (row*h->view.cols) + (first%h->view.cols);

	if(flag & SELECTION_RIGHT) 		// Update right margin
		origin = last = ((last/h->view.cols)*h->view.cols) + col;

	if(flag & SELECTION_BOTTOM)		// Update bottom margin
		origin = last = (row*h->view.cols) + (last%h->view.cols);

	trace("origin=%d first=%d last=%d",origin,first,last);

	if(first < last)
		do_select(h,first,last,h->rectsel);
	else
		do_select(h,last,first,h->rectsel);

	cursor_move(h,h->select.end);

	return origin;
}

LIB3270_EXPORT int lib3270_move_selection(H3270 *hSession, LIB3270_DIRECTION dir)
{
	int start, end;

	if(!hSession->selected || hSession->select.start == hSession->select.end)
		return ENOENT;

	start = hSession->select.start;
	end   = hSession->select.end;

	switch(dir)
	{
	case LIB3270_DIR_UP:
		if(start <= ((int) hSession->view.cols))
			return EINVAL;
		start -= hSession->view.cols;
		end   -= hSession->view.cols;
		break;

	case LIB3270_DIR_DOWN:
		if(end >= ((int) (hSession->view.cols * (hSession->view.rows-1))))
			return EINVAL;
		start += hSession->view.cols;
		end   += hSession->view.cols;
		break;

	case LIB3270_DIR_LEFT:
		if( (start % hSession->view.cols) < 1)
			return EINVAL;
		start--;
		end--;
		break;

	case LIB3270_DIR_RIGHT:
		if( (end % hSession->view.cols) >= (hSession->view.cols-1))
			return EINVAL;
		start++;
		end++;
		break;

	default:
		return -1;
	}

	do_select(hSession,start,end,hSession->rectsel);
	cursor_move(hSession,hSession->select.end);

	return 0;
}


