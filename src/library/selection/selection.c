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

#include <internals.h>
#include <lib3270.h>
#include <lib3270/actions.h>
#include <lib3270/session.h>
#include <lib3270/selection.h>
#include <lib3270/log.h>
#include <lib3270/memory.h>
#include <private/3270ds.h>
#include <private/screen.h>
#include <private/util.h>
#include "kybdc.h"

/*--[ Implement ]------------------------------------------------------------------------------------*/

static void get_selected_addr(H3270 *session, int *start, int *end) {
	if(session->select.start > session->select.end) {
		*end   = session->select.start;
		*start = session->select.end;
	} else {
		*start = session->select.start;
		*end   = session->select.end;
	}
}

static void update_selected_rectangle(H3270 *session) {
	struct {
		int row;
		int col;
	} p[2];

	int begin, end, row, col, baddr;

	get_selected_addr(session,&begin,&end);

	// Get start & end posision
	p[0].row = (begin/session->view.cols);
	p[0].col = (begin%session->view.cols);
	p[1].row = (end/session->view.cols);
	p[1].col = (end%session->view.cols);

	if(p[0].row > p[1].row) {
		int swp = p[0].row;
		p[0].row = p[1].row;
		p[1].row = swp;
	}

	if(p[0].col > p[1].col) {
		int swp = p[0].col;
		p[0].col = p[1].col;
		p[1].col = swp;
	}

	// First remove unselected areas
	baddr = 0;
	for(row=0; row < ((int) session->view.rows); row++) {
		for(col = 0; col < ((int) session->view.cols); col++) {
			if(!(row >= p[0].row && row <= p[1].row && col >= p[0].col && col <= p[1].col) && (session->text[baddr].attr & LIB3270_ATTR_SELECTED)) {
				session->text[baddr].attr &= ~LIB3270_ATTR_SELECTED;
				screen_update_addr(session,baddr);
			}
			baddr++;
		}
	}

	// Then, draw selected ones
	baddr = 0;
	for(row=0; row < ((int) session->view.rows); row++) {
		for(col = 0; col < ((int) session->view.cols); col++) {
			if((row >= p[0].row && row <= p[1].row && col >= p[0].col && col <= p[1].col) && !(session->text[baddr].attr & LIB3270_ATTR_SELECTED)) {
				session->text[baddr].attr |= LIB3270_ATTR_SELECTED;
				screen_update_addr(session,baddr);
			}
			baddr++;
		}
	}

}

static void update_selected_region(H3270 *session) {
	int baddr,begin,end;
	int len = session->view.rows * session->view.cols;

	get_selected_addr(session,&begin,&end);

	// First remove unselected areas
	for(baddr = 0; baddr < begin; baddr++) {
		if(session->text[baddr].attr & LIB3270_ATTR_SELECTED) {
			session->text[baddr].attr &= ~LIB3270_ATTR_SELECTED;
			screen_update_addr(session,baddr);
		}
	}

	for(baddr = end+1; baddr < len; baddr++) {
		if(session->text[baddr].attr & LIB3270_ATTR_SELECTED) {
			session->text[baddr].attr &= ~LIB3270_ATTR_SELECTED;
			screen_update_addr(session,baddr);
		}
	}

	// Then draw the selected ones
	for(baddr = begin; baddr <= end; baddr++) {
		if(!(session->text[baddr].attr & LIB3270_ATTR_SELECTED)) {
			session->text[baddr].attr |= LIB3270_ATTR_SELECTED;
			screen_update_addr(session,baddr);
		}
	}

}

void toggle_rectselect(H3270 *hSession, const struct lib3270_toggle *t, LIB3270_TOGGLE_TYPE GNUC_UNUSED(tt)) {
	if(!hSession->selected)
		return;

	hSession->rectsel = (t->value != 0);

	if(hSession->rectsel)
		update_selected_rectangle(hSession);
	else
		update_selected_region(hSession);
}

int do_select(H3270 *hSession, unsigned int start, unsigned int end, unsigned int rect) {
	unsigned int length = (hSession->view.rows * hSession->view.cols);

	if(end > length || start > length)
		return errno = EINVAL;

	// Do we really need to change selection?
	if( ((int) start) == hSession->select.start && ((int) end) == hSession->select.end && hSession->selected)
		return 0;

	// Start address is inside the screen?
	hSession->select.start		= start;
	hSession->select.end 		= end;

	if(rect) {
		hSession->rectsel = 1;
		update_selected_rectangle(hSession);
	} else {
		hSession->rectsel = 0;
		update_selected_region(hSession);
	}

	if(!hSession->selected) {
		hSession->selected = 1;
		hSession->cbk.set_selection(hSession,1);
		lib3270_action_group_notify(hSession,LIB3270_ACTION_GROUP_SELECTION);
	}

	hSession->cbk.update_selection(hSession,start,end);

	return 0;
}

LIB3270_EXPORT unsigned char lib3270_get_selection_flags(H3270 *hSession, int baddr) {
	int row,col;
	unsigned char rc = 0;

	if(!(lib3270_is_connected(hSession) && (hSession->text[baddr].attr & LIB3270_ATTR_SELECTED)))
		return rc;

	row = baddr / hSession->view.cols;
	col = baddr % hSession->view.cols;
	rc |= SELECTION_ACTIVE;

	if( (hSession->select.start % hSession->view.cols) == (hSession->select.end % hSession->view.cols) ) {
		rc |= SELECTION_SINGLE_COL;
	} else {
		if( (col == 0) || !(hSession->text[baddr-1].attr & LIB3270_ATTR_SELECTED) )
			rc |= SELECTION_LEFT;

		/// FIXME: It should test if baddr is the last element before the +1.

		if( (col == ((int) hSession->view.cols)) || !(hSession->text[baddr+1].attr & LIB3270_ATTR_SELECTED) )
			rc |= SELECTION_RIGHT;
	}

	if( (hSession->select.start / hSession->view.cols) == (hSession->select.end / hSession->view.cols) ) {
		rc |= SELECTION_SINGLE_ROW;
	} else {
		if( (row == 0) || !(hSession->text[baddr-hSession->view.cols].attr & LIB3270_ATTR_SELECTED) )
			rc |= SELECTION_TOP;

		if( (row == ((int) hSession->view.rows)) || !(hSession->text[baddr+hSession->view.cols].attr & LIB3270_ATTR_SELECTED) )
			rc |= SELECTION_BOTTOM;
	}

	return rc;
}

LIB3270_EXPORT char * lib3270_get_region(H3270 *h, int start_pos, int end_pos, unsigned char all) {

	char *	text = NULL;
	int 	maxlen;
	int		baddr;

	if(check_online_session(h))
		return NULL;

	maxlen = h->view.rows * (h->view.cols+1);

	if(start_pos < 0 || start_pos > maxlen || end_pos < 0 || end_pos > maxlen || end_pos < start_pos)
		return NULL;

	for(baddr=start_pos; baddr<end_pos; baddr++) {

		if(all || h->text[baddr].attr & LIB3270_ATTR_SELECTED)
			text = append_string(text,h->text[baddr].chr);

		if((baddr%h->view.cols) == 0 && text && text[0])
			text = append_string(text,"\n");
	}

	return text;

}

LIB3270_EXPORT char * lib3270_get_string_at_address(H3270 *h, int offset, int len, const char *lf) {

#ifndef DEBUG
	if(!lib3270_is_connected(h)) {
		errno = ENOTCONN;
		return NULL;
	}
#endif // !DEBUG

	if(offset < 0)
		offset = lib3270_get_cursor_address(h);

	if(offset > (h->view.rows * h->view.cols)) {
		errno = EOVERFLOW;
		return NULL;
	}

	string_buffer sb = { NULL, 0 };

	while(len > 0) {
		string_buffer_append(&sb,h->text[offset].chr);
		offset++;
		len--;

		if(lf && (offset%h->view.cols) == 0 && len > 0) {
			string_buffer_append(&sb,lf);
			len--;
		}
	}

	return sb.buf;
}

LIB3270_EXPORT char * lib3270_get_string_at(H3270 *h, unsigned int row, unsigned int col, int len, const char *lf) {

	int baddr = lib3270_translate_to_address(h,row,col);
	if(baddr < 0)
		return NULL;

	return lib3270_get_string_at_address(h, baddr, len, lf);
}

LIB3270_EXPORT int lib3270_cmp_string_at(H3270 *h, unsigned int row, unsigned int col, const char *text, const char *lf) {
	int baddr = lib3270_translate_to_address(h,row,col);
	if(baddr < 0)
		return -1;

	return lib3270_cmp_string_at_address(h,baddr,text,lf);
}

LIB3270_EXPORT int lib3270_cmp_string_at_address(H3270 *h, int baddr, const char *text, const char *lf) {
	int		  rc;
	size_t	  sz		= strlen(text);
	char	* contents;

	contents = lib3270_get_string_at_address(h,baddr,sz,lf);
	if(!contents)
		return -1;

	rc = strncmp(contents,text,sz);

	lib3270_free(contents);

	return rc;
}

LIB3270_EXPORT char * lib3270_get_field_string_at(H3270 *session, int baddr) {
	int first = lib3270_field_addr(session,baddr);

	if(first < 0)
		return NULL;

	return lib3270_get_string_at_address(session,first,lib3270_field_length(session,first)+1,0);
}

LIB3270_EXPORT int lib3270_get_has_selection(const H3270 *hSession) {
	errno = 0;
	if(check_online_session(hSession))
		return 0;

	return hSession->selected ? 1 : 0;
}

//LIB3270_EXPORT int lib3270_has_selection(const H3270 *hSession) {
//	return lib3270_get_has_selection(hSession);
//}

LIB3270_EXPORT int lib3270_get_has_copy(const H3270 *hSession) {
	errno = 0;
	if(check_online_session(hSession))
		return 0;

	return hSession->has_copy ? 1 : 0;
}

LIB3270_EXPORT void lib3270_set_has_copy(H3270 *hSession, int has_copy) {
	hSession->has_copy = has_copy ? 1 : 0;
	lib3270_action_group_notify(hSession,LIB3270_ACTION_GROUP_COPY);
}

LIB3270_EXPORT int lib3270_get_selection_rectangle(H3270 *hSession, unsigned int *row, unsigned int *col, unsigned int *width, unsigned int *height) {
	unsigned int r, c, minRow, minCol, maxRow, maxCol, baddr, count;

	if(check_online_session(hSession))
		return errno;

	if(!hSession->selected || hSession->select.start == hSession->select.end)
		return errno = ENOENT;

	minRow = hSession->view.rows;
	minCol = hSession->view.cols;
	maxRow = 0;
	maxCol = 0;
	baddr  = 0;
	count  = 0;

	for(r=0; r < hSession->view.rows; r++) {
		for(c = 0; c < hSession->view.cols; c++) {
			if(hSession->text[baddr].attr & LIB3270_ATTR_SELECTED) {
				count++;

				if(c < minCol)
					minCol = c;

				if(r < minRow)
					minRow = r;

				if(c > maxCol)
					maxCol = c;

				if(r > maxRow)
					maxRow = r;
			}
			baddr++;
		}
	}

	if(!count)
		return errno = ENOENT;

	debug("Selection from %u,%u and to %u,%u",minCol,minRow,maxCol,maxRow);

	*col	= minCol;
	*row	= minRow;
	*width	= (maxCol - minCol)+1;
	*height = (maxRow - minRow)+1;

	return 0;
}

LIB3270_EXPORT int lib3270_erase_selected(H3270 *hSession) {
	FAIL_IF_NOT_ONLINE(hSession);

	if (hSession->kybdlock) {
		enq_action(hSession, lib3270_erase_selected);
		return 0;
	}

	unsigned int baddr = 0;
	unsigned char fa = 0;

	for(baddr = 0; baddr < lib3270_get_length(hSession); baddr++) {
		if(hSession->ea_buf[baddr].fa) {
			fa = hSession->ea_buf[baddr].fa;
		}

		if( (hSession->text[baddr].attr & LIB3270_ATTR_SELECTED) && !FA_IS_PROTECTED(fa)) {
			clear_chr(hSession,baddr);
		}
	}

	return -1;
}
