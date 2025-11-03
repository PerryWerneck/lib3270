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
#include <lib3270.h>
#include <lib3270/session.h>
#include <lib3270/selection.h>
#include <lib3270/trace.h>
#include <lib3270/log.h>
#include <lib3270/memory.h>
#include <private/3270ds.h>

// TODO: Refactor

/*--[ Implement ]------------------------------------------------------------------------------------*/

void clear_chr(H3270 *hSession, int baddr) {

	hSession->text[baddr].chr = " ";
	hSession->ea_buf[baddr].cc = EBC_null;
	hSession->ea_buf[baddr].cs = 0;

	screen_update_addr(hSession,baddr);

}

/*
LIB3270_EXPORT char * lib3270_get_selected_text(H3270 *hSession, char tok, LIB3270_SELECTION_OPTIONS options) {
	int				  row, col, baddr;
	char 			* ret;
	size_t			  buflen	= (hSession->view.rows * (hSession->view.cols+1))+1;
	size_t			  sz		= 0;
	unsigned short	  attr		= 0xFFFF;
	char			  cut		= (options & LIB3270_SELECTION_CUT) != 0;
	char			  all		= (options & LIB3270_SELECTION_ALL) != 0;

	if(check_online_session(hSession))
		return NULL;

	if(!hSession->selected || hSession->select.start == hSession->select.end) {
		errno = ENOENT;
		return NULL;
	}

	ret = lib3270_malloc(buflen);

	baddr = 0;
	unsigned char fa = 0;

	for(row=0; row < ((int) hSession->view.rows); row++) {
		int cr = 0;

		for(col = 0; col < ((int) hSession->view.cols); col++) {
			if(hSession->ea_buf[baddr].fa) {
				fa = hSession->ea_buf[baddr].fa;
			}

			if(all || hSession->text[baddr].attr & LIB3270_ATTR_SELECTED) {
				if(tok && attr != hSession->text[baddr].attr) {
					attr = hSession->text[baddr].attr;
					ret[sz++] = tok;
					ret[sz++] = (attr & 0x0F);
					ret[sz++] = ((attr & 0xF0) >> 4);

				}

				cr++;
				ret[sz++] = hSession->text[baddr].chr;

				if(cut && !FA_IS_PROTECTED(fa)) {
					clear_chr(hSession,baddr);
				}

			}
			baddr++;
		}

		if(cr)
			ret[sz++] = '\n';

		if((sz+10) > buflen) {
			buflen += 100;
			ret = lib3270_realloc(ret,buflen);
		}
	}

	if(!sz) {
		lib3270_free(ret);
		errno = ENOENT;
		return NULL;
	} else if(sz > 1 && ret[sz-1] == '\n') { // Remove ending \n
		ret[sz-1] = 0;
	}

	ret[sz++] = 0;

	if(sz != buflen)
		ret = lib3270_realloc(ret,sz);

	return ret;
}

LIB3270_EXPORT char * lib3270_get_selected(H3270 *hSession) {
	return lib3270_get_selected_text(hSession,0,0);
}

LIB3270_EXPORT char * lib3270_cut_selected(H3270 *hSession) {
	return lib3270_get_selected_text(hSession,0,LIB3270_SELECTION_CUT);
}

static size_t get_selection_length(unsigned int width, unsigned int height) {
	return sizeof(lib3270_selection) + (sizeof(lib3270_selection_element) * ((width*height)+1));
}

LIB3270_EXPORT size_t lib3270_selection_get_length(const lib3270_selection *selection) {
	return get_selection_length(selection->bounds.width,selection->bounds.height);
}

LIB3270_EXPORT lib3270_selection * lib3270_get_selection(H3270 *hSession, int cut, int all) {
	return lib3270_selection_new(hSession,cut,all);
}

LIB3270_EXPORT lib3270_selection * lib3270_selection_new(H3270 *hSession, int cut, int all) {
	if(check_online_session(hSession))
		return NULL;

	// Get block size
	unsigned int row, col, width, height;

	if(all) {
		row = col = 0;
		width = lib3270_get_width(hSession);
		height = lib3270_get_height(hSession);
	} else if(lib3270_get_selection_rectangle(hSession, &row, &col, &width, &height)) {
		return NULL;
	}

	// Get output buffer.
	size_t length = get_selection_length(width, height);
	lib3270_selection * selection = lib3270_malloc(length);

	memset(selection,0,length);

	selection->bounds.col		= col;
	selection->bounds.row		= row;
	selection->bounds.height	= height;
	selection->bounds.width		= width;
	selection->cursor_address	= lib3270_get_cursor_address(hSession);

	debug(
	    "width=%u height=%u length=%u (sz=%u szHeader=%u szElement=%u)",
	    selection->bounds.width,
	    selection->bounds.height,
	    ((selection->bounds.width * selection->bounds.height) + 1),
	    (unsigned int) length,
	    (unsigned int) sizeof(lib3270_selection),
	    (unsigned int) sizeof(lib3270_selection_element)
	);

	unsigned int dstaddr = 0;

	for(row=0; row < selection->bounds.height; row++) {
		// Get starting address.
		int baddr			= lib3270_translate_to_address(hSession, selection->bounds.row+row+1, selection->bounds.col+1);
		unsigned char fa	= get_field_attribute(hSession,baddr);

		if(baddr < 0) {
			lib3270_free(selection);
			return NULL;
		}

		for(col=0; col < selection->bounds.width; col++) {
			if(hSession->ea_buf[baddr].fa) {
				fa = hSession->ea_buf[baddr].fa;
			}

			selection->contents[dstaddr].chr				= (hSession->text[baddr].chr ? hSession->text[baddr].chr : ' ');
			selection->contents[dstaddr].attribute.visual	= hSession->text[baddr].attr;
			selection->contents[dstaddr].attribute.field	= fa;

			if(cut && !FA_IS_PROTECTED(fa)) {
				clear_chr(hSession,baddr);
			}

			dstaddr++;
			baddr++;
		}

	}

	debug("dstaddr=%u length=%u",(unsigned int) dstaddr, (unsigned int) length);

	return selection;
}
*/
