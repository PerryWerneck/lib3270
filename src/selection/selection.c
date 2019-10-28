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
 * Este programa está nomeado como selection.c e possui - linhas de código.
 *
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
 #include "3270ds.h"
 #include "kybdc.h"

 /*--[ Implement ]------------------------------------------------------------------------------------*/

static void get_selected_addr(H3270 *session, int *start, int *end)
{
	if(session->select.start > session->select.end)
	{
		*end   = session->select.start;
		*start = session->select.end;
	}
	else
	{
		*start = session->select.start;
		*end   = session->select.end;
	}
}

static void update_selected_rectangle(H3270 *session)
{
	struct
	{
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

	if(p[0].row > p[1].row)
	{
		int swp = p[0].row;
		p[0].row = p[1].row;
		p[1].row = swp;
	}

	if(p[0].col > p[1].col)
	{
		int swp = p[0].col;
		p[0].col = p[1].col;
		p[1].col = swp;
	}

	// First remove unselected areas
	baddr = 0;
	for(row=0;row < ((int) session->view.rows);row++)
	{
		for(col = 0; col < ((int) session->view.cols);col++)
		{
			if(!(row >= p[0].row && row <= p[1].row && col >= p[0].col && col <= p[1].col) && (session->text[baddr].attr & LIB3270_ATTR_SELECTED))
			{
				session->text[baddr].attr &= ~LIB3270_ATTR_SELECTED;
				session->cbk.update(session,baddr,session->text[baddr].chr,session->text[baddr].attr,baddr == session->cursor_addr);
			}
			baddr++;
		}
	}

	// Then, draw selected ones
	baddr = 0;
	for(row=0;row < ((int) session->view.rows);row++)
	{
		for(col = 0; col < ((int) session->view.cols);col++)
		{
			if((row >= p[0].row && row <= p[1].row && col >= p[0].col && col <= p[1].col) && !(session->text[baddr].attr & LIB3270_ATTR_SELECTED))
			{
				session->text[baddr].attr |= LIB3270_ATTR_SELECTED;
				session->cbk.update(session,baddr,session->text[baddr].chr,session->text[baddr].attr,baddr == session->cursor_addr);
			}
			baddr++;
		}
	}

}

static void update_selected_region(H3270 *session)
{
	int baddr,begin,end;
	int len = session->view.rows * session->view.cols;

	get_selected_addr(session,&begin,&end);

	// First remove unselected areas
	for(baddr = 0; baddr < begin; baddr++)
	{
		if(session->text[baddr].attr & LIB3270_ATTR_SELECTED)
		{
			session->text[baddr].attr &= ~LIB3270_ATTR_SELECTED;
			session->cbk.update(session,baddr,session->text[baddr].chr,session->text[baddr].attr,baddr == session->cursor_addr);
		}
	}

	for(baddr = end+1; baddr < len; baddr++)
	{
		if(session->text[baddr].attr & LIB3270_ATTR_SELECTED)
		{
			session->text[baddr].attr &= ~LIB3270_ATTR_SELECTED;
			session->cbk.update(session,baddr,session->text[baddr].chr,session->text[baddr].attr,baddr == session->cursor_addr);
		}
	}

	// Then draw the selected ones
	for(baddr = begin; baddr <= end; baddr++)
	{
		if(!(session->text[baddr].attr & LIB3270_ATTR_SELECTED))
		{
			session->text[baddr].attr |= LIB3270_ATTR_SELECTED;
			session->cbk.update(session,baddr,session->text[baddr].chr,session->text[baddr].attr,baddr == session->cursor_addr);
		}
	}

}

void toggle_rectselect(H3270 *session, struct lib3270_toggle GNUC_UNUSED(*t), LIB3270_TOGGLE_TYPE GNUC_UNUSED(tt))
{
	if(!session->selected)
		return;

	if(t->value)
		update_selected_rectangle(session);
	else
		update_selected_region(session);
}

void do_select(H3270 *hSession, unsigned int start, unsigned int end, unsigned int rect)
{
	if(end > (hSession->view.rows * hSession->view.cols))
		return;

	// Do we really need to change selection?
	if( ((int) start) == hSession->select.start && ((int) end) == hSession->select.end && hSession->selected)
		return;

	// Start address is inside the screen?
	hSession->select.start		= start;
	hSession->select.end 		= end;

	if(rect)
	{
		hSession->rectsel = 1;
		update_selected_rectangle(hSession);
	}
	else
	{
		hSession->rectsel = 0;
		update_selected_region(hSession);
	}

	if(!hSession->selected)
	{
		hSession->selected = 1;
		hSession->cbk.set_selection(hSession,1);
	}

	hSession->cbk.update_selection(hSession,start,end);
	lib3270_notify_actions(hSession,LIB3270_ACTION_GROUP_SELECTION);

}

LIB3270_EXPORT unsigned char lib3270_get_selection_flags(H3270 *hSession, int baddr)
{
	int row,col;
	unsigned char rc = 0;

	CHECK_SESSION_HANDLE(hSession);

	if(!(lib3270_is_connected(hSession) && (hSession->text[baddr].attr & LIB3270_ATTR_SELECTED)))
		return rc;

	row = baddr / hSession->view.cols;
	col = baddr % hSession->view.cols;
	rc |= SELECTION_ACTIVE;

	if( (hSession->select.start % hSession->view.cols) == (hSession->select.end % hSession->view.cols) )
	{
		rc |= SELECTION_SINGLE_COL;
	}
	else
	{
		if( (col == 0) || !(hSession->text[baddr-1].attr & LIB3270_ATTR_SELECTED) )
			rc |= SELECTION_LEFT;

		/// FIXME: It should test if baddr is the last element before the +1.

		if( (col == ((int) hSession->view.cols)) || !(hSession->text[baddr+1].attr & LIB3270_ATTR_SELECTED) )
			rc |= SELECTION_RIGHT;
	}

	if( (hSession->select.start / hSession->view.cols) == (hSession->select.end / hSession->view.cols) )
	{
		rc |= SELECTION_SINGLE_ROW;
	}
	else
	{
		if( (row == 0) || !(hSession->text[baddr-hSession->view.cols].attr & LIB3270_ATTR_SELECTED) )
			rc |= SELECTION_TOP;

		if( (row == ((int) hSession->view.rows)) || !(hSession->text[baddr+hSession->view.cols].attr & LIB3270_ATTR_SELECTED) )
			rc |= SELECTION_BOTTOM;
	}

	return rc;
}

LIB3270_EXPORT char * lib3270_get_region(H3270 *h, int start_pos, int end_pos, unsigned char all)
{
	char *	text;
	int 	maxlen;
	int		sz = 0;
	int		baddr;

	if(check_online_session(h))
		return NULL;

	maxlen = h->view.rows * (h->view.cols+1);

	if(start_pos < 0 || start_pos > maxlen || end_pos < 0 || end_pos > maxlen || end_pos < start_pos)
		return NULL;

	text = lib3270_malloc(maxlen);

	for(baddr=start_pos;baddr<end_pos;baddr++)
	{
		if(all || h->text[baddr].attr & LIB3270_ATTR_SELECTED)
			text[sz++] = (h->text[baddr].attr & LIB3270_ATTR_CG) ? ' ' : h->text[baddr].chr;

		if((baddr%h->view.cols) == 0 && sz > 0)
			text[sz++] = '\n';
	}
	text[sz++] = 0;

	return lib3270_realloc(text,sz);
}

LIB3270_EXPORT char * lib3270_get_string_at_address(H3270 *h, int offset, int len, char lf)
{
	char * buffer;
	int    maxlen;
	char * ptr;

	CHECK_SESSION_HANDLE(h);

	if(!lib3270_is_connected(h))
	{
		errno = ENOTCONN;
		return NULL;
	}

	if(offset < 0)
		offset = lib3270_get_cursor_address(h);

	maxlen = (h->view.rows * (h->view.cols+ (lf ? 1 : 0) )) - offset;
	if(maxlen <= 0 || offset < 0)
	{
		errno = EOVERFLOW;
		return NULL;
	}

	if(len < 0 || len > maxlen)
		len = maxlen;

	buffer	= lib3270_malloc(len+1);
	ptr		= buffer;

	memset(buffer,0,len+1);

	trace("len=%d buffer=%p",len,buffer);

	while(len > 0)
	{
		if(h->text[offset].attr & LIB3270_ATTR_CG)
			*ptr = ' ';
		else if(h->text[offset].chr)
			*ptr = h->text[offset].chr;
		else
			*ptr = ' ';

		ptr++;
		offset++;
		len--;

		if(lf && (offset%h->view.cols) == 0 && len > 0)
		{
			*(ptr++) = lf;
			len--;
		}
	}
//	trace("len=%d buffer=%p pos=%d",len,buffer,ptr-buffer);

	*ptr = 0;

	return buffer;
}

LIB3270_EXPORT char * lib3270_get_string_at(H3270 *h, unsigned int row, unsigned int col, int len, char lf)
{
	CHECK_SESSION_HANDLE(h);

	int baddr = lib3270_translate_to_address(h,row,col);
	if(baddr < 0)
		return NULL;

	return lib3270_get_string_at_address(h, baddr, len, lf);
}

LIB3270_EXPORT int lib3270_cmp_string_at(H3270 *h, unsigned int row, unsigned int col, const char *text, char lf)
{
	int baddr = lib3270_translate_to_address(h,row,col);
	if(baddr < 0)
		return -1;

	return lib3270_cmp_string_at_address(h,baddr,text,lf);
}

 LIB3270_EXPORT int lib3270_cmp_string_at_address(H3270 *h, int baddr, const char *text, char lf)
 {
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


/**
 * Get field contents
 *
 * @param session	Session handle
 * @param baddr		Field addr
 *
 * @return String with the field contents (release it with lib3270_free()
 */
LIB3270_EXPORT char * lib3270_get_field_string_at(H3270 *session, int baddr)
{
	int first = lib3270_field_addr(session,baddr);

	if(first < 0)
		return NULL;

	return lib3270_get_string_at_address(session,first,lib3270_field_length(session,first)+1,0);
}

LIB3270_EXPORT int lib3270_has_selection(const H3270 *hSession)
{
	errno = 0;
	if(check_online_session(hSession))
		return 0;

	return hSession->selected;
}

LIB3270_EXPORT int lib3270_get_selection_rectangle(H3270 *hSession, unsigned int *row, unsigned int *col, unsigned int *width, unsigned int *height)
{
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

	for(r=0;r < hSession->view.rows;r++)
	{
		for(c = 0; c < hSession->view.cols;c++)
		{
			if(hSession->text[baddr].attr & LIB3270_ATTR_SELECTED)
			{
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

LIB3270_EXPORT int lib3270_erase_selected(H3270 *hSession)
{
	FAIL_IF_NOT_ONLINE(hSession);

	if (hSession->kybdlock)
	{
		enq_action(hSession, lib3270_erase_selected);
		return 0;
	}

	unsigned int baddr = 0;
	unsigned char fa = 0;

	for(baddr = 0; baddr < lib3270_get_length(hSession); baddr++)
	{
		if(hSession->ea_buf[baddr].fa) {
			fa = hSession->ea_buf[baddr].fa;
		}

		if( (hSession->text[baddr].attr & LIB3270_ATTR_SELECTED) && !FA_IS_PROTECTED(fa))
		{
			clear_chr(hSession,baddr);
		}
	}

    return -1;
}
