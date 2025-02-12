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
 * Este programa está nomeado como paste.c e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

#include <internals.h>

/*
#if defined(X3270_DISPLAY)
	#include <X11/Xatom.h>
#endif
*/

#define XK_3270

#if defined(X3270_APL)
#define XK_APL
#endif

#include <fcntl.h>

#include "3270ds.h"
//#include "resources.h"

//#include "actionsc.h"
#include "ansic.h"
//#include "aplc.h"
#include "ctlrc.h"
#include "ftc.h"
#include "hostc.h"
// #include "keypadc.h"
#include "kybdc.h"
#include "popupsc.h"
// #include "printc.h"
#include "screenc.h"

/*
#if defined(X3270_DISPLAY)
	#include "selectc.h"
#endif
*/

#include "statusc.h"
// #include "tablesc.h"
#include "telnetc.h"
#include "togglesc.h"
#include "trace_dsc.h"
//#include "utf8c.h"
#include "utilc.h"
#if defined(X3270_DBCS) /*[*/
#include "widec.h"
#endif /*]*/
// #include "api.h"

#include <lib3270/popup.h>
#include <lib3270/selection.h>
#include <lib3270/log.h>
#include <lib3270/toggle.h>

/*---[ Struct ]-------------------------------------------------------------------------------------------------*/

typedef struct _paste_data {
	int qtd;
	int row;
	int orig_addr;
	int orig_col;
} PASTE_DATA;

/*---[ Implement ]----------------------------------------------------------------------------------------------*/

/**
 * @brief Move the cursor back within the legal paste area.
 *
 * @return A Boolean indicating success.
 */
static int remargin(H3270 *hSession, int lmargin) {
	int ever = False;
	int baddr, b0 = 0;
	int faddr;
	unsigned char fa;

	if(lib3270_get_toggle(hSession,LIB3270_TOGGLE_MARGINED_PASTE)) {
		baddr = hSession->cursor_addr;
		while(BA_TO_COL(baddr) < ((unsigned int) lmargin)) {
			baddr = ROWCOL_TO_BA(BA_TO_ROW(baddr), lmargin);
			if (!ever) {
				b0 = baddr;
				ever = True;
			}

			faddr = lib3270_field_addr(hSession,baddr);
			fa = hSession->ea_buf[faddr].fa;
			if (faddr == baddr || FA_IS_PROTECTED(fa)) {
				baddr = lib3270_get_next_unprotected(hSession,baddr);
				if (baddr <= b0)
					return 0;
			}

		}
		cursor_move(hSession,baddr);
	}

	return -1;
}

static int paste_char(H3270 *hSession, PASTE_DATA *data, unsigned char c) {

	if(lib3270_get_toggle(hSession,LIB3270_TOGGLE_SMART_PASTE)) {
		int faddr = lib3270_field_addr(hSession,hSession->cursor_addr);
		if(FA_IS_PROTECTED(hSession->ea_buf[faddr].fa))
			hSession->cursor_addr++;
		else
			key_ACharacter(hSession, c, KT_STD, IA_PASTE, NULL);
	} else {
		key_ACharacter(hSession, c, KT_STD, IA_PASTE, NULL);
	}

	data->qtd++;

	if(BA_TO_ROW(hSession->cursor_addr) != ((unsigned int) data->row)) {
		trace("Row changed from %d to %d",data->row,BA_TO_ROW(hSession->cursor_addr));
		if(!remargin(hSession,data->orig_col))
			return 0;
		data->row = BA_TO_ROW(hSession->cursor_addr);
		return '\n';
	}

	return c;
}

static int set_string(H3270 *hSession, const unsigned char *str, int length) {
	PASTE_DATA data;
	unsigned char last = 1;
	int ix;

	memset(&data,0,sizeof(data));
	data.row		= BA_TO_ROW(hSession->cursor_addr);
	data.orig_addr	= hSession->cursor_addr;
	data.orig_col	= BA_TO_COL(hSession->cursor_addr);

	if(length < 0)
		length = (int) strlen((const char *) str);

//	while(*str && last && !hSession->kybdlock && hSession->cursor_addr >= data.orig_addr)
	for(ix = 0; ix < length && *str && last && !hSession->kybdlock && hSession->cursor_addr >= data.orig_addr; ix++) {
		switch(*str) {
		case '\t':
			last = paste_char(hSession,&data, ' ');
			break;

		case '\n':
			if(last != '\n') {
				int baddr;
				int faddr;
				unsigned char fa;

				baddr = (hSession->cursor_addr + hSession->view.cols) % (hSession->view.cols * hSession->view.rows);   /* down */
				baddr = (baddr / hSession->view.cols) * hSession->view.cols;               /* 1st col */
				faddr = lib3270_field_addr(hSession,baddr);
				fa = hSession->ea_buf[faddr].fa;
				if (faddr != baddr && !FA_IS_PROTECTED(fa))
					cursor_move(hSession,baddr);
				else
					cursor_move(hSession,lib3270_get_next_unprotected(hSession,baddr));
				data.row = BA_TO_ROW(hSession->cursor_addr);
			}
			last = ' ';
			data.qtd++;
			break;

		default:
			last = paste_char(hSession,&data, *str);

		}

		str++;

		if(IN_3270 && lib3270_get_toggle(hSession,LIB3270_TOGGLE_MARGINED_PASTE) && BA_TO_COL(hSession->cursor_addr) < ((unsigned int) data.orig_col)) {
			if(!remargin(hSession,data.orig_col))
				last = 0;
		}

		if(hSession->cursor_addr == data.orig_addr)
			break;
	}

	return data.qtd;
}

/// @brief Set string at defined position.
LIB3270_EXPORT int lib3270_set_string_at(H3270 *hSession, unsigned int row, unsigned int col, const unsigned char *str, int length) {
	int rc = 0;

	if(!(str && *str))
		return 0;

	if(check_online_session(hSession))
		return - errno;

	// Is Keyboard locked ?
	if(hSession->kybdlock)
		return - (errno = EPERM);

	if(hSession->selected && !lib3270_get_toggle(hSession,LIB3270_TOGGLE_KEEP_SELECTED))
		lib3270_unselect(hSession);

	row--;
	col--;

	if(row > hSession->view.rows || col > hSession->view.cols)
		return - (errno = EOVERFLOW);

	hSession->cbk.suspend(hSession);

	hSession->cursor_addr = (row * hSession->view.cols) + col;
	rc = set_string(hSession, str, length);
	hSession->cbk.resume(hSession);

	trace("%s rc=%d",__FUNCTION__,rc);

	return rc;
}

LIB3270_EXPORT int lib3270_set_string_at_address(H3270 *hSession, int baddr, const unsigned char *str, int length) {
	int rc = -1;

	if(!(str && *str))
		return 0;

	if(check_online_session(hSession))
		return - errno;

	if(length < 0)
		length = (int) strlen((const char *) str);

	if(hSession->kybdlock)
		return - (errno = EPERM);

	if(baddr >= 0) {
		rc = lib3270_set_cursor_address(hSession,baddr);
		if(rc < 0)
			return rc;
	}

	if(hSession->selected && !lib3270_get_toggle(hSession,LIB3270_TOGGLE_KEEP_SELECTED))
		lib3270_unselect(hSession);

	hSession->cbk.suspend(hSession);
	rc = set_string(hSession, str, length);
	hSession->cbk.resume(hSession);

	return rc;
}

LIB3270_EXPORT int lib3270_set_field(H3270 *hSession, const char *text, int length) {
	int addr;
	int numchars = 0;

	if(!text)
		return - (errno = EINVAL);

	if(check_online_session(hSession))
		return - errno;

	if(hSession->kybdlock)
		return - (errno = EPERM);

	if (!hSession->formatted)
		return - (errno = ENOTSUP);

	if(length < 0)
		length = (int) strlen((const char *) text);

	addr = lib3270_field_addr(hSession,hSession->cursor_addr);
	if(addr < 0)
		return addr;

	if(hSession->selected && !lib3270_get_toggle(hSession,LIB3270_TOGGLE_KEEP_SELECTED))
		lib3270_unselect(hSession);

	hSession->cbk.suspend(hSession);
	hSession->cursor_addr = ++addr;
	numchars = set_string(hSession, (const unsigned char *) text, length);
	hSession->cbk.resume(hSession);

	if(numchars < 0)
		return numchars;

	// Find the end of the field.
	addr = lib3270_get_field_end(hSession,addr);
	if(addr < 0)
		return addr;

	addr = lib3270_get_next_unprotected(hSession, addr);

	if(addr > 0) {
		addr = lib3270_set_cursor_address(hSession,addr);
		if(addr < 0)
			return addr;
	}

	return hSession->cursor_addr;

}


LIB3270_EXPORT int lib3270_set_string(H3270 *hSession, const unsigned char *str, int length) {
	int rc;

	if(!(str && *str))
		return 0;

	if(check_online_session(hSession))
		return - errno;

	if(hSession->kybdlock)
		return - (errno = EPERM);

	if(hSession->selected && !lib3270_get_toggle(hSession,LIB3270_TOGGLE_KEEP_SELECTED))
		lib3270_unselect(hSession);

	hSession->cbk.suspend(hSession);
	rc = set_string(hSession, str, length);
	hSession->cbk.resume(hSession);

	return rc;
}

LIB3270_EXPORT int lib3270_paste_text(H3270 *hSession, const unsigned char *str) {
	if(check_online_session(hSession))
		return -errno;

	if(!str) {
		hSession->ring_bell(hSession);
		return -(errno = EINVAL);
	}

	if(hSession->paste_buffer) {
		lib3270_free(hSession->paste_buffer);
		hSession->paste_buffer = NULL;
	}

	int sz = lib3270_set_string(hSession,str,-1);
	if(sz < 0) {
		// Can´t paste
		lib3270_popup_dialog(
		    hSession,
		    LIB3270_NOTIFY_WARNING,
		    _( "Action failed" ),
		    _( "Unable to paste text" ),
		    "%s", sz == -EPERM ? _( "Keyboard is locked" ) : _( "Unexpected error" )
		);

		return sz;
	}

	if((int) strlen((char *) str) > sz) {
		hSession->paste_buffer = strdup((char *) (str+sz));
		lib3270_action_group_notify(hSession, LIB3270_ACTION_GROUP_COPY);
		return strlen(hSession->paste_buffer);
	}

	return 0;
}

LIB3270_EXPORT int lib3270_can_paste_next(const H3270 *hSession) {
	if(!(lib3270_is_connected(hSession) && hSession->paste_buffer))
		return 0;

	return strlen(hSession->paste_buffer);
}

LIB3270_EXPORT int lib3270_paste_next(H3270 *hSession) {
	char	* ptr;
	int		  rc;

	FAIL_IF_NOT_ONLINE(hSession);

	if(!(lib3270_is_connected(hSession) && hSession->paste_buffer)) {
		hSession->ring_bell(hSession);
		return 0;
	}

	ptr = hSession->paste_buffer;
	hSession->paste_buffer = NULL;

	rc = lib3270_paste_text(hSession,(unsigned char *) ptr);

	lib3270_free(ptr);

	if(!hSession->paste_buffer)
		lib3270_action_group_notify(hSession, LIB3270_ACTION_GROUP_COPY);

	return rc;
}
