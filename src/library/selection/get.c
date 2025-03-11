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

#include <internals.h>
#include <lib3270.h>
#include <lib3270/session.h>
#include <lib3270/selection.h>
#include <lib3270/trace.h>
#include <lib3270/log.h>
#include <lib3270/memory.h>
#include <private/3270ds.h>

/*--[ Implement ]------------------------------------------------------------------------------------*/

void clear_chr(H3270 *hSession, int baddr) {
	hSession->text[baddr].chr = ' ';

	hSession->ea_buf[baddr].cc = EBC_null;
	hSession->ea_buf[baddr].cs = 0;

	hSession->cbk.update(	hSession,
	                        baddr,
	                        hSession->text[baddr].chr,
	                        hSession->text[baddr].attr,
	                        baddr == hSession->cursor_addr );
}

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
