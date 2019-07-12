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

 #include "../private.h"
 #include <lib3270.h>
 #include <lib3270/actions.h>
 #include <lib3270/session.h>
 #include <lib3270/selection.h>
 #include "3270ds.h"

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
	p[0].row = (begin/session->cols);
	p[0].col = (begin%session->cols);
	p[1].row = (end/session->cols);
	p[1].col = (end%session->cols);

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
	for(row=0;row < session->rows;row++)
	{
		for(col = 0; col < session->cols;col++)
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
	for(row=0;row < session->rows;row++)
	{
		for(col = 0; col < session->cols;col++)
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
	int len = session->rows*session->cols;

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

void do_select(H3270 *h, int start, int end, int rect)
{
	if(start < 0 || end > (h->rows * h->cols))
		return;

	// Do we really need to change selection?
	if(start == h->select.start && end == h->select.end && h->selected)
		return;

	// Start address is inside the screen?
	h->select.start		= start;
	h->select.end 		= end;

	if(rect)
	{
		h->rectsel = 1;
		update_selected_rectangle(h);
	}
	else
	{
		h->rectsel = 0;
		update_selected_region(h);
	}

	if(!h->selected)
	{
		h->selected = 1;
		h->cbk.set_selection(h,1);
	}

	h->cbk.update_selection(h,start,end);

}

LIB3270_EXPORT unsigned char lib3270_get_selection_flags(H3270 *hSession, int baddr)
{
	int row,col;
	unsigned char rc = 0;

	CHECK_SESSION_HANDLE(hSession);

	if(!(lib3270_connected(hSession) && (hSession->text[baddr].attr & LIB3270_ATTR_SELECTED)))
		return rc;

	row = baddr / hSession->cols;
	col = baddr % hSession->cols;
	rc |= SELECTION_ACTIVE;

	if( (hSession->select.start % hSession->cols) == (hSession->select.end % hSession->cols) )
	{
		rc |= SELECTION_SINGLE_COL;
	}
	else
	{
		if( (col == 0) || !(hSession->text[baddr-1].attr & LIB3270_ATTR_SELECTED) )
			rc |= SELECTION_LEFT;

		if( (col == hSession->cols) || !(hSession->text[baddr+1].attr & LIB3270_ATTR_SELECTED) )
			rc |= SELECTION_RIGHT;
	}

	if( (hSession->select.start / hSession->cols) == (hSession->select.end / hSession->cols) )
	{
		rc |= SELECTION_SINGLE_ROW;
	}
	else
	{
		if( (row == 0) || !(hSession->text[baddr-hSession->cols].attr & LIB3270_ATTR_SELECTED) )
			rc |= SELECTION_TOP;

		if( (row == hSession->rows) || !(hSession->text[baddr+hSession->cols].attr & LIB3270_ATTR_SELECTED) )
			rc |= SELECTION_BOTTOM;
	}

	return rc;
}

static void clear_chr(H3270 *hSession, int baddr)
{
	hSession->text[baddr].chr = ' ';

	hSession->ea_buf[baddr].cc = EBC_null;
	hSession->ea_buf[baddr].cs = 0;

	hSession->cbk.update(	hSession,
							baddr,
							hSession->text[baddr].chr,
							hSession->text[baddr].attr,
							baddr == hSession->cursor_addr );
}

static char * get_text(H3270 *hSession,unsigned char all, unsigned char tok, Boolean cut)
{
	int				  row, col, baddr;
	char 			* ret;
	size_t			  buflen	= (hSession->rows * (hSession->cols+1))+1;
	size_t			  sz		= 0;
    unsigned short	  attr		= 0xFFFF;

	if(check_online_session(hSession))
		return NULL;

	if(!hSession->selected || hSession->select.start == hSession->select.end)
		return NULL;

	ret = lib3270_malloc(buflen);

	baddr = 0;
	unsigned char fa = 0;

	for(row=0;row < hSession->rows;row++)
	{
		int cr = 0;

		for(col = 0; col < hSession->cols;col++)
		{
			if(hSession->ea_buf[baddr].fa) {
				fa = hSession->ea_buf[baddr].fa;
			}

			if(all || hSession->text[baddr].attr & LIB3270_ATTR_SELECTED)
			{
				if(tok && attr != hSession->text[baddr].attr)
				{
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

        if((sz+10) > buflen)
        {
            buflen += 100;
       		ret = lib3270_realloc(ret,buflen);
        }
	}

	if(!sz)
	{
		lib3270_free(ret);
		errno = ENOENT;
		return NULL;
	}
	else if(sz > 1 && ret[sz-1] == '\n') // Remove ending \n
	{
		ret[sz-1] = 0;
	}

	ret[sz++] = 0;

	if(sz != buflen)
		ret = lib3270_realloc(ret,sz);

	return ret;
}

LIB3270_EXPORT char * lib3270_get_region(H3270 *h, int start_pos, int end_pos, unsigned char all)
{
	char *	text;
	int 	maxlen;
	int		sz = 0;
	int		baddr;

	CHECK_SESSION_HANDLE(h);

	if(!lib3270_connected(h))
		return NULL;

	maxlen = h->rows * (h->cols+1);

	if(start_pos < 0 || start_pos > maxlen || end_pos < 0 || end_pos > maxlen || end_pos < start_pos)
		return NULL;

	text = lib3270_malloc(maxlen);

	for(baddr=start_pos;baddr<end_pos;baddr++)
	{
		if(all || h->text[baddr].attr & LIB3270_ATTR_SELECTED)
			text[sz++] = (h->text[baddr].attr & LIB3270_ATTR_CG) ? ' ' : h->text[baddr].chr;

		if((baddr%h->cols) == 0 && sz > 0)
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

	if(!lib3270_connected(h))
	{
		errno = ENOTCONN;
		return NULL;
	}

	maxlen = (h->rows * (h->cols+1)) - offset;
	if(maxlen <= 0 || offset < 0)
	{
		errno = EINVAL;
		return NULL;
	}

	if(len < 0 || len > maxlen)
		len = maxlen;

	buffer	= lib3270_malloc(len+1);
	ptr		= buffer;

//	trace("len=%d buffer=%p",len,buffer);

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

		if(lf && (offset%h->cols) == 0 && len > 0)
		{
			*(ptr++) = lf;
			len--;
		}
	}
//	trace("len=%d buffer=%p pos=%d",len,buffer,ptr-buffer);

	*ptr = 0;

	return buffer;
}

LIB3270_EXPORT char * lib3270_get_string_at(H3270 *h, int row, int col, int len, char lf)
{
	CHECK_SESSION_HANDLE(h);
	return lib3270_get_string_at_address(h, ((row-1) * h->cols) + (col-1), len, lf);
}

LIB3270_EXPORT int lib3270_cmp_text_at(H3270 *h, int row, int col, const char *text, char lf)
{
	int		  rc;
	size_t	  sz		= strlen(text);
	char	* contents;

	contents = lib3270_get_string_at(h,row,col,sz,lf);
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
LIB3270_EXPORT char * lib3270_get_field_text_at(H3270 *session, int baddr)
{
	int first = lib3270_field_addr(session,baddr);

	if(first < 0)
		return NULL;

	return lib3270_get_string_at_address(session,first,lib3270_field_length(session,first)+1,0);
}

LIB3270_EXPORT int lib3270_has_selection(H3270 *hSession)
{
	if(check_online_session(hSession))
		return 0;

	return hSession->selected != 0;
}

LIB3270_EXPORT char * lib3270_get_selected(H3270 *hSession)
{
	return get_text(hSession,0,0,0);
}

LIB3270_EXPORT char * lib3270_cut_selected(H3270 *hSession)
{
	return get_text(hSession,0,0,1);
}

