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
 * Este programa está nomeado como screen.c e possui 894 linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

/**
 *	@file screen.c
 *
 *	@brief A callback based 3270 Terminal Emulator Screen drawing
 *
 */

#include <internals.h>
#include <signal.h>
#include "3270ds.h"
#include "resources.h"
#include "ctlrc.h"
#include "hostc.h"
#include "kybdc.h"
#include "screenc.h"
#include "trace_dsc.h"
#include "utilc.h"
#include "widec.h"
#include "xioc.h"
#include "screen.h"
#include "errno.h"
#include "statusc.h"
#include "togglesc.h"
#include <lib3270/actions.h>
#include <lib3270/log.h>
#include <lib3270/toggle.h>

#if defined(_WIN32)
	#include <windows.h>
	#include <wincon.h>
	#include "winversc.h"
#else
	#include <stdarg.h>
#endif


#define get_color_pair(fg,bg) (((bg&0x0F) << 4) | (fg&0x0F))
#define DEFCOLOR_MAP(f) ((((f) & FA_PROTECT) >> 4) | (((f) & FA_INT_HIGH_SEL) >> 3))

/*--[ Implement ]------------------------------------------------------------------------------------*/

static void status_connect(H3270 *session, int ignored, void *dunno);
static void status_3270_mode(H3270 *session, int ignored, void *dunno);
static unsigned short color_from_fa(H3270 *hSession, unsigned char fa);

/*--[ Implement ]------------------------------------------------------------------------------------*/

static void addch(H3270 *session, int baddr, unsigned char c, unsigned short attr, int *first, int *last)
{
	// If set to keep selection adjust corresponding flag based on the current state
	if(lib3270_get_toggle(session,LIB3270_TOGGLE_KEEP_SELECTED))
		attr |= (session->text[baddr].attr & LIB3270_ATTR_SELECTED);

	if(session->text[baddr].chr == c && session->text[baddr].attr == attr)
		return;

	if(*first < 0)
		*first = baddr;
	*last = baddr;

	/* Converted char has changed, update it */
	session->text[baddr].chr  = c;
	session->text[baddr].attr = attr;

	session->cbk.update(session,baddr,c,attr,baddr == session->cursor_addr);
}

LIB3270_EXPORT LIB3270_ATTR lib3270_get_attribute_at_address(H3270 *hSession, unsigned int baddr)
{
	if(check_online_session(hSession))
		return (LIB3270_ATTR) -1;

	if(!hSession->text ||baddr > (hSession->view.rows*hSession->view.cols))
	{
		errno = EOVERFLOW;
		return (LIB3270_ATTR) -1;
	}

	return hSession->text[baddr].attr;
}

LIB3270_EXPORT int lib3270_is_selected(H3270 *hSession, unsigned int baddr)
{
    FAIL_IF_NOT_ONLINE(hSession);

	if(!hSession->text || baddr > (hSession->view.rows * hSession->view.cols))
	{
		errno = EOVERFLOW;
		return -1;
	}

	return (hSession->text[baddr].attr & LIB3270_ATTR_SELECTED) != 0;
}

LIB3270_EXPORT int lib3270_get_element(H3270 *hSession, unsigned int baddr, unsigned char *c, unsigned short *attr)
{
    FAIL_IF_NOT_ONLINE(hSession);

	if(!hSession->text || baddr > (hSession->view.rows * hSession->view.cols))
	{
		errno = EOVERFLOW;
		return -1;
	}

	*c		= hSession->text[baddr].chr;
	*attr	= hSession->text[baddr].attr;

	return 0;
}

/**
 * Initialize the screen.
 *
 * @return 0 if ok, non zero if not
 *
 */
int screen_init(H3270 *session)
{
	CHECK_SESSION_HANDLE(session);

	/* Set up callbacks for state changes. */
	lib3270_register_schange(session,LIB3270_STATE_CONNECT, status_connect,0);
	lib3270_register_schange(session,LIB3270_STATE_3270_MODE, status_3270_mode,0);
//	lib3270_register_schange(session,LIB3270_STATE_PRINTER, status_printer,0);

	/* Set up the controller. */
	ctlr_init(session,-1);
	ctlr_model_changed(session);

	/* Finish screen initialization. */
	session->cbk.suspend(session);

	return 0;
}

/* Map a field attribute to its default colors. */
static unsigned short color_from_fa(H3270 *hSession, unsigned char fa)
{
	if (hSession->m3279)
		return get_color_pair(DEFCOLOR_MAP(fa),0) | LIB3270_ATTR_FIELD;

	// Green on black
	return get_color_pair(0,0) | LIB3270_ATTR_FIELD | ((FA_IS_HIGH(fa)) ? LIB3270_ATTR_INTENSIFY : 0);
}

/*
 * Find the display attributes for a baddr, fa_addr and fa.
 */
static unsigned short calc_attrs(H3270 *session, int baddr, int fa_addr, int fa)
{
	unsigned short fg=0, bg=0, a;
	int gr;

	/* Compute the color. */

	/* Monochrome is easy, and so is color if nothing is specified. */
	if (!session->m3279 ||
		(!session->ea_buf[baddr].fg &&
		 !session->ea_buf[fa_addr].fg &&
		 !session->ea_buf[baddr].bg &&
		 !session->ea_buf[fa_addr].bg))
	{
		a = color_from_fa(session,fa);
	}
	else
	{

		/* The current location or the fa specifies the fg or bg. */
		if (session->ea_buf[baddr].fg)
		{
			fg = session->ea_buf[baddr].fg & 0x0f;
		}
		else if (session->ea_buf[fa_addr].fg)
		{
			fg = session->ea_buf[fa_addr].fg & 0x0f;
		}
		else
		{
			fg = DEFCOLOR_MAP(fa);
		}

		if (session->ea_buf[baddr].bg)
			bg = session->ea_buf[baddr].bg & 0x0f;
		else if (session->ea_buf[fa_addr].bg)
			bg = session->ea_buf[fa_addr].bg & 0x0f;
		else
			bg = 0;

		a = get_color_pair(fg, bg);
	}

	/* Compute the display attributes. */

	if (session->ea_buf[baddr].gr)
		gr = session->ea_buf[baddr].gr;
	else if (session->ea_buf[fa_addr].gr)
		gr = session->ea_buf[fa_addr].gr;
	else
		gr = 0;

	if(gr & GR_BLINK)
		a |= LIB3270_ATTR_BLINK;

	if( (gr & GR_UNDERLINE) && lib3270_get_toggle(session,LIB3270_TOGGLE_UNDERLINE))
		a |= LIB3270_ATTR_UNDERLINE;

	if(session->m3279 && (gr & (GR_BLINK | GR_UNDERLINE)) && !(gr & GR_REVERSE))
    	a |= LIB3270_ATTR_BACKGROUND_INTENSITY;

	if(!session->m3279 &&	((gr & GR_INTENSIFY) || FA_IS_HIGH(fa)))
		a |= LIB3270_ATTR_INTENSIFY;

	if (gr & GR_REVERSE)
		a = get_color_pair(((a & 0xF0) >> 4),(a & 0x0F)) | (a&0xFF00); // a = reverse_colors(a);

	return a;
}

LIB3270_EXPORT unsigned int lib3270_get_length(const H3270 *h)
{
	return h->view.rows * h->view.cols;
}

LIB3270_EXPORT void lib3270_get_screen_size(const H3270 *h, unsigned int *r, unsigned int *c)
{
	*r = h->view.rows;
	*c = h->view.cols;
}

LIB3270_EXPORT unsigned int lib3270_get_width(const H3270 *h)
{
	return h->view.cols;
}

LIB3270_EXPORT unsigned int lib3270_get_height(const H3270 *h)
{
	return h->view.rows;
}

LIB3270_EXPORT unsigned int lib3270_get_max_width(const H3270 *h)
{
	return h->max.cols;
}

LIB3270_EXPORT unsigned int lib3270_get_max_height(const H3270 *h)
{
	return h->max.rows;
}

void update_model_info(H3270 *hSession, unsigned int model, unsigned int cols, unsigned int rows)
{
	if(model == hSession->model_num && hSession->max.rows == rows && hSession->max.cols == cols)
		return;

	hSession->max.cols  	= cols;
	hSession->max.rows  	= rows;
	hSession->model_num	= model;

	/* Update the model name. */
	(void) sprintf(hSession->model_name, "327%c-%d%s",hSession->m3279 ? '9' : '8',hSession->model_num,hSession->extended ? "-E" : "");

	if (hSession->termname != CN)
		hSession->termtype = hSession->termname;
	else if(hSession->oversize.rows || hSession->oversize.cols)
		hSession->termtype = "IBM-DYNAMIC";
	else
		hSession->termtype = hSession->full_model_name;

	trace("Termtype: %s",hSession->termtype);

	hSession->cbk.update_model(hSession, hSession->model_name,hSession->model_num,rows,cols);
}

LIB3270_EXPORT int lib3270_get_contents(H3270 *h, int first, int last, unsigned char *chr, unsigned short *attr)
{
	int baddr;
	int len;

    CHECK_SESSION_HANDLE(h);

	len = h->view.rows * h->view.cols;

	if(first > len || last > len || first < 0 || last < 0)
		return EFAULT;

	for(baddr = first; baddr <= last;baddr++)
	{
		*(chr++)  = h->text[baddr].chr ? h->text[baddr].chr : ' ';
		*(attr++) = h->text[baddr].attr;
	}

	return 0;
}

/* Display what's in the buffer. */
void screen_update(H3270 *session, int bstart, int bend)
{
	int				baddr;
	unsigned short	a;
	int				attr = COLOR_GREEN;
	unsigned char	fa;
	int				fa_addr;
	int				first	= -1;
	int				last	= -1;

	fa		= get_field_attribute(session,bstart);
	a  		= color_from_fa(session,fa);
	fa_addr = lib3270_field_addr(session,bstart); // may be -1, that's okay

	for(baddr = bstart; baddr < bend; baddr++)
	{
		if(session->ea_buf[baddr].fa)
		{
			// Field attribute.
			fa_addr = baddr;
			fa = session->ea_buf[baddr].fa;
			a = calc_attrs(session, baddr, baddr, fa);
			addch(session,baddr,' ',(attr = COLOR_GREEN)|LIB3270_ATTR_MARKER,&first,&last);
		}
		else if (FA_IS_ZERO(fa))
		{
			// Blank.
			addch(session,baddr,' ',attr=a,&first,&last);
		}
		else
		{
			// Normal text.
			if (!(session->ea_buf[baddr].gr || session->ea_buf[baddr].fg || session->ea_buf[baddr].bg))
			{
				attr = a;
			}
			else
			{
				attr = calc_attrs(session, baddr, fa_addr, fa);
			}

			if (session->ea_buf[baddr].cs == CS_LINEDRAW)
			{
				addch(session,baddr,session->ea_buf[baddr].cc,attr,&first,&last);
			}
			else if (session->ea_buf[baddr].cs == CS_APL || (session->ea_buf[baddr].cs & CS_GE))
			{
				addch(session,baddr,session->ea_buf[baddr].cc,attr|LIB3270_ATTR_CG,&first,&last);
			}
			else
			{
				if(lib3270_get_toggle(session,LIB3270_TOGGLE_MONOCASE))
					addch(session,baddr,session->charset.asc2uc[session->charset.ebc2asc[session->ea_buf[baddr].cc]],attr,&first,&last);
				else
					addch(session,baddr,session->charset.ebc2asc[session->ea_buf[baddr].cc],attr,&first,&last);
			}
		}
	}

	if(first >= 0)
	{
		int len = (last - first)+1;
		int f;

		for(f=first;f<last;f++)
		{
			if(f%session->view.cols == 0)
				len++;
		}

		session->cbk.changed(session,first,len);
	}

	if(session->starting && session->formatted && !session->kybdlock && lib3270_in_3270(session))
	{
		session->starting = 0;

//		cursor_move(session,next_unprotected(session,0));
//		lib3270_emulate_input(session,"\\n",-1,0);
		session->cbk.autostart(session);

#ifdef DEBUG
		{
			char *text = lib3270_get_string_at_address(session,0,-1,'\n');
			trace("First screen:\n%s\n",text);
			lib3270_free(text);
		}
#endif
	}

//	trace("%s ends",__FUNCTION__);

}

LIB3270_EXPORT unsigned int lib3270_get_cursor_address(const H3270 *hSession)
{
	if(check_online_session(hSession))
		return 0;

    return hSession->cursor_addr;
}

/**
 * @brief Converts row/col in a buffer address.
 *
 * @param hSession	TN3270 Session.
 * @param row		Row inside the screen.
 * @param col		Col inside the screen.
 *
 * @return Current address or -1 if invalid (sets errno).
 *
 */
LIB3270_EXPORT int lib3270_translate_to_address(const H3270 *hSession, unsigned int row, unsigned int col)
{
    FAIL_IF_NOT_ONLINE(hSession);

	row--;
	col--;

	if(row > hSession->view.rows || col > hSession->view.cols)
		return - (errno = EOVERFLOW);

	return (row * hSession->view.cols) + col;
}


LIB3270_EXPORT int lib3270_set_cursor_address(H3270 *hSession, unsigned int baddr)
{
    FAIL_IF_NOT_ONLINE(hSession);

	trace("%s(%d)",__FUNCTION__,baddr);

	if(baddr > (hSession->view.rows * hSession->view.cols))
		return - (errno = EOVERFLOW);

	if(hSession->selected && !lib3270_get_toggle(hSession,LIB3270_TOGGLE_KEEP_SELECTED))
		lib3270_unselect(hSession);

	return cursor_move(hSession,baddr);
}

LIB3270_EXPORT int lib3270_set_cursor_position(H3270 *hSession, unsigned int row, unsigned int col)
{
	int baddr = lib3270_translate_to_address(hSession, row, col);
	if(baddr < 0)
		return -errno;

	return lib3270_set_cursor_address(hSession,baddr);
}

/**
 * @brief Move cursor to a new position.
 *
 * @see lib3270_set_cursor_address
 *
 * @param hSession	TN3270 session.
 * @param baddr		New cursor position.
 *
 * @return Old cursor position.
 *
 */
int cursor_move(H3270 *hSession, int baddr)
{
    int ret = hSession->cursor_addr;

	if(ret != baddr && baddr >= 0)
	{
		hSession->cursor_addr = baddr;
		hSession->cbk.update_cursor(
			hSession,
			(unsigned short) (baddr/hSession->view.cols),
			(unsigned short) (baddr%hSession->view.cols),
			hSession->text[baddr].chr,
			hSession->text[baddr].attr
		);
	}

    return ret;
}

/**
 * @brief Status line stuff.
 */
void set_status(H3270 *session, LIB3270_FLAG id, Boolean on)
{
	session->oia.flag[id] = (on != 0);
	session->cbk.update_oia(session,id,session->oia.flag[id]);
}

void status_ctlr_done(H3270 *session)
{
	CHECK_SESSION_HANDLE(session);
	set_status(session,LIB3270_FLAG_UNDERA,True);
	session->cbk.ctlr_done(session);
}

void status_oerr(H3270 *session, int error_type)
{
	LIB3270_MESSAGE sts = LIB3270_MESSAGE_USER;

	CHECK_SESSION_HANDLE(session);

	switch (error_type)
	{
	case KL_OERR_PROTECTED:
		sts = LIB3270_MESSAGE_PROTECTED;
		break;
	case KL_OERR_NUMERIC:
		sts = LIB3270_MESSAGE_NUMERIC;
		break;
	case KL_OERR_OVERFLOW:
		sts = LIB3270_MESSAGE_OVERFLOW;
		break;

	default:
		return;
	}

	status_changed(session,sts);

}

/**
 * @brief Resolving DNS name.
 *
 */
void status_resolving(H3270 *hSession)
{
	lib3270_set_cstate(hSession,LIB3270_RESOLVING);
	lib3270_st_changed(hSession, LIB3270_STATE_RESOLVING, True);

	mcursor_set(hSession,LIB3270_POINTER_LOCKED);
	status_changed(hSession, LIB3270_MESSAGE_RESOLVING);
}

void status_connecting(H3270 *hSession)
{
	lib3270_set_cstate(hSession,LIB3270_RESOLVING);
	lib3270_st_changed(hSession, LIB3270_STATE_CONNECTING, True);

	mcursor_set(hSession,LIB3270_POINTER_LOCKED);
	status_changed(hSession, LIB3270_MESSAGE_CONNECTING);
}

void status_reset(H3270 *session)
{
	CHECK_SESSION_HANDLE(session);

	if (session->kybdlock & KL_ENTER_INHIBIT)
	{
		trace("%s",__FUNCTION__);
		status_changed(session,LIB3270_MESSAGE_INHIBIT);
	}
	else if (session->kybdlock & KL_DEFERRED_UNLOCK)
	{
		trace("%s",__FUNCTION__);
		status_changed(session,LIB3270_MESSAGE_X);
	}
	else
	{
		trace("%s",__FUNCTION__);
		mcursor_set(session,LIB3270_POINTER_UNLOCKED);
		status_changed(session,LIB3270_MESSAGE_NONE);
	}

	session->cbk.display(session);

}

/**
 * Query the updated terminal status.
 *
 * @return status-code.
 *
 * @see LIB3270_MESSAGE
 */
LIB3270_EXPORT LIB3270_MESSAGE lib3270_get_program_message(const H3270 *session)
{
	return session->oia.status;
}

/**
 * Check if the terminal is ready for commands.
 *
 * @param h	Session handle.
 *
 * @return 0 if the terminal is ready (no message, keyboard unlocked), LIB3270_MESSAGE if not
 *
 * @retval LIB3270_MESSAGE_KYBDLOCK	Keyboard is locked.
 * @retval LIB3270_MESSAGE_NONE		Terminal is ready.
 *
 */
LIB3270_EXPORT LIB3270_MESSAGE lib3270_get_lock_status(const H3270 *hSession)
{
	if(hSession->oia.status)
		return hSession->oia.status;

	if(hSession->kybdlock)
		return LIB3270_MESSAGE_KYBDLOCK;

	return LIB3270_MESSAGE_NONE;

}

/**
 * Check if terminal is ready for actions.
 *
 * @param h	Session handle.
 *
 * @return Non zero if terminal is ready for commands.
 *
 */
LIB3270_EXPORT int lib3270_is_ready(const H3270 *hSession)
{
	return lib3270_get_lock_status(hSession) == LIB3270_MESSAGE_NONE;
}


void status_changed(H3270 *session, LIB3270_MESSAGE id)
{
	CHECK_SESSION_HANDLE(session);

	if(id == session->oia.status || id < 0)
		return;

	session->oia.status = id;

	session->cbk.update_status(session,id);
}

void status_twait(H3270 *session)
{
	CHECK_SESSION_HANDLE(session);
	set_status(session,LIB3270_FLAG_UNDERA,False);
	status_changed(session,LIB3270_MESSAGE_TWAIT);
}

void set_viewsize(H3270 *session, unsigned int rows, unsigned int cols)
{
	CHECK_SESSION_HANDLE(session);

	if(rows == session->view.rows && session->view.cols == cols)
		return;

	session->view.rows = rows;
	session->view.cols = cols;

	trace("View size changes to %dx%d (configure=%p)",rows,cols,session->cbk.configure);

	if(session->cbk.configure)
		session->cbk.configure(session,session->view.rows,session->view.cols);

}

void status_lu(H3270 *session, const char *lu)
{
	CHECK_SESSION_HANDLE(session);

	if(session->cbk.update_luname)
		session->cbk.update_luname(session,lu);

}

static void status_connect(H3270 *hSession, int connected, void GNUC_UNUSED(*dunno))
{
	LIB3270_MESSAGE id = LIB3270_MESSAGE_USER;

	ctlr_erase(hSession,1);

	if (connected)
	{
		set_status(hSession,LIB3270_FLAG_BOXSOLID,IN_3270 && !IN_SSCP);

		if (hSession->kybdlock & KL_AWAITING_FIRST)
			id = LIB3270_MESSAGE_AWAITING_FIRST;
		else
			id = LIB3270_MESSAGE_CONNECTED;

	}
	else
	{
		set_status(hSession,LIB3270_FLAG_BOXSOLID,False);
		id = LIB3270_MESSAGE_DISCONNECTED;
	}

	status_changed(hSession,id);

}

static void status_3270_mode(H3270 *hSession, int GNUC_UNUSED(ignored), void GNUC_UNUSED(*dunno))
{
	Boolean oia_boxsolid = (IN_3270 && !IN_SSCP);

	CHECK_SESSION_HANDLE(hSession);

	if(oia_boxsolid)
		set_status(hSession,LIB3270_FLAG_UNDERA,True);
	set_status(hSession,LIB3270_FLAG_BOXSOLID,oia_boxsolid);

}

/*
static void status_printer(H3270 *session, int on, void *dunno)
{
	set_status(session,LIB3270_FLAG_PRINTER,on);
}
*/

void status_timing(H3270 GNUC_UNUSED(*session), struct timeval GNUC_UNUSED(*t0), struct timeval GNUC_UNUSED(*t1))
{
}

void status_untiming(H3270 *session)
{
	CHECK_SESSION_HANDLE(session);

	if(session->cbk.set_timer)
		session->cbk.set_timer(session,0);
}

/*
static int logpopup(H3270 *session, void *widget, LIB3270_NOTIFY type, const char *title, const char *msg, const char *fmt, va_list arg)
{
#ifdef ANDROID
	char *mask = xs_buffer("%s\n",fmt);
	__android_log_vprint(ANDROID_LOG_VERBOSE, PACKAGE_NAME, mask, arg);
	lib3270_free(mask);
#else
	lib3270_write_log(session,"popup","%s",title);
	lib3270_write_log(session,"popup","%s",msg);
	lib3270_write_va_log(session,"popup",fmt,arg);
#endif // ANDROID
	return 0;
}
*/

void Error(H3270 *session, const char *fmt, ...)
{
	va_list arg_ptr;

	CHECK_SESSION_HANDLE(session);

	trace("%s: title=%s fmt=%s",__FUNCTION__,"3270 Error",fmt);

	va_start(arg_ptr, fmt);

	session->popups++;
	session->cbk.popup(session,LIB3270_NOTIFY_ERROR, _( "3270 Error" ),NULL,fmt,arg_ptr);
	session->popups--;

	va_end(arg_ptr);

}

void Warning(H3270 *session, const char *fmt, ...)
{
	va_list arg_ptr;

	CHECK_SESSION_HANDLE(session);

	trace("%s: title=%s fmt=%s",__FUNCTION__,"3270 Warning",fmt);

	va_start(arg_ptr, fmt);

	session->popups++;
	session->cbk.popup(session,LIB3270_NOTIFY_WARNING, _( "3270 Warning" ),NULL,fmt,arg_ptr);
	session->popups--;

	va_end(arg_ptr);

}

/* Pop up an error dialog. */
void popup_an_error(H3270 *session, const char *fmt, ...)
{
	va_list	args;

	CHECK_SESSION_HANDLE(session);

	trace("%s: title=%s fmt=%s",__FUNCTION__,"3270 Error",fmt);

	va_start(args, fmt);

	session->popups++;
	session->cbk.popup(session,LIB3270_NOTIFY_ERROR,_( "3270 Error" ),NULL,fmt,args);
	session->popups--;

	va_end(args);

}

void popup_system_error(H3270 *session, const char *title, const char *message, const char *fmt, ...)
{
	va_list	args;

	CHECK_SESSION_HANDLE(session);

	trace("%s: title=%s msg=%s",__FUNCTION__,"3270 Error",message);

	va_start(args, fmt);

	session->popups++;
	session->cbk.popup(session,LIB3270_NOTIFY_ERROR,title ? title : _( "3270 Error" ), message,fmt,args);
	session->popups--;

	va_end(args);
}

void mcursor_set(H3270 *hSession,LIB3270_POINTER m)
{
	if(hSession->pointer != ((unsigned short) m)) {

		// Pointer changed
		hSession->pointer = (unsigned short) m;
		hSession->cbk.cursor(hSession,m & 0x03);

		// Notify lock state change.
		lib3270_notify_actions(hSession, LIB3270_ACTION_GROUP_LOCK_STATE);

	}
}

LIB3270_EXPORT int lib3270_testpattern(H3270 *hSession)
{
	static const unsigned char text_pat[] =
	{
		// Latin-1 chars
		0x48, 0x68, 0x45, 0x51, 0x55, 0xce, 0xde, 0x46, 0xcf, 0x65, 0x71, 0x75, 0xee, 0xfe, 0x66, 0xef,
		0x90, 0x9a, 0x9b, 0xb5, 0x42, 0x52, 0x56, 0xcb, 0xdb, 0x62, 0x72, 0x76, 0xeb, 0xfb,

		// Regular ASCII chars
		0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86,
		0x87, 0x88, 0x89, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0xa2, 0xa3, 0xa4, 0xa5,
		0xa6, 0xa7, 0xa8, 0xa9, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xd1, 0xd2, 0xd3,
		0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0x7d, 0x5a,
		0x7c, 0x7b, 0x5b, 0x6c, 0x50, 0x5c, 0x4d, 0x5d, 0x60, 0x6d, 0x4e, 0x7e, 0x7d, 0x79, 0xad, 0xc0,
		0xa1, 0xb0, 0xd0, 0xbd, 0x6b, 0x4c, 0x4b, 0x6e, 0x5e, 0x7a, 0x61, 0x6f, 0x7f,

		// End marker
		0x00
	};

	static const unsigned char cg_pat[] =
	{
		0x8c, // CG 0xf7, less or equal "≤"
		0xae, // CG 0xd9, greater or equal "≥"
		0xbe, // CG 0x3e, not equal "≠"
		0xad, // "["
		0xbd, // "]"
		0xc6, // CG 0xa5, left tee
		0xd3, // CG 0xab, plus
		0xa2, // CG 0x92, horizontal line
		0xc7, // CG 0xa6, bottom tee
		0xd7, // CG 0xaf, top tee
		0xd6, // CG 0xae, right tee
		0xc5, // CG 0xa4, UL corner
		0xd5, // CG 0xad, UR corner
		0x85, // CG 0x184, vertical line
		0xc4, // CG 0xa3, LL corner
		0xd4, // CG 0xac, LR corner

		0x00

	};

	static const struct _pat
	{
		unsigned char cs;
		const unsigned char *cc;
	} pat[] =
	{
		{ CS_APL,	cg_pat		},
		{ 0,		text_pat	},
	};

	static const unsigned char gr[] = { 0, GR_UNDERLINE, GR_BLINK };

	unsigned int row		= 0;
	int max;
	int pos 	= 0;
	unsigned int grpos	= 0;
	int f;
	int fg		= COLOR_BLUE;

	FAIL_IF_ONLINE(hSession);

	max = (hSession->max.rows * hSession->max.cols);
	for(f=0;f<max;f++)
	{
		if(!pat[row].cc[pos])
		{
			if(++row >= (sizeof(pat)/sizeof(struct _pat)) )
			{
				row = 0;
				if(++fg > COLOR_WHITE)
				{
					fg = COLOR_BLUE;
					if(++grpos > (sizeof(gr)/sizeof(gr[0])))
						grpos = 0;
				}
			}
			pos = 0;
		}
		hSession->ea_buf[f].fg = fg;
		hSession->ea_buf[f].bg = (fg == COLOR_BLACK) ? COLOR_WHITE : COLOR_BLACK;
		hSession->ea_buf[f].cs = pat[row].cs;
		hSession->ea_buf[f].cc = pat[row].cc[pos++];
		hSession->ea_buf[f].gr = gr[grpos];
	}

	hSession->cbk.display(hSession);

	return 0;
}

LIB3270_EXPORT void lib3270_popup_dialog(H3270 *session, LIB3270_NOTIFY id , const char *title, const char *message, const char *fmt, ...)
{
	va_list	args;
	va_start(args, fmt);
    lib3270_popup_va(session, id, title, message, fmt, args);
	va_end(args);
}

LIB3270_EXPORT void lib3270_popup_va(H3270 *session, LIB3270_NOTIFY id , const char *title, const char *message, const char *fmt, va_list args)
{
	CHECK_SESSION_HANDLE(session);

	session->popups++;
	session->cbk.popup(session,id,title ? title : _( "3270 Error" ), message,fmt,args);
	session->popups--;

}
