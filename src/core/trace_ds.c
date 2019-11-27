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
 * Este programa está nomeado como trace_ds.c e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 * licinio@bb.com.br		(Licínio Luis Branco)
 * kraucer@bb.com.br		(Kraucer Fernandes Mazuco)
 * macmiranda@bb.com.br		(Marco Aurélio Caldas Miranda)
 *
 */


/**
 * @brief 3270 data stream tracing.
 *
 */

#include <internals.h>
#include <lib3270/trace.h>

#if defined(X3270_TRACE) /*[*/

#if defined(X3270_DISPLAY) /*[*/
#include <X11/StringDefs.h>
#include <X11/Xaw/Dialog.h>
#endif /*]*/
#if defined(_WIN32) /*[*/
#include <windows.h>
#endif /*]*/
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <fcntl.h>
#include "3270ds.h"
#include "resources.h"

// #include "charsetc.h"
#include "ctlrc.h"
#include "popupsc.h"
// #include "tablesc.h"
#include "telnetc.h"
#include "trace_dsc.h"
#include "utilc.h"
#include <lib3270/toggle.h>

/* Maximum size of a tracefile header. */
#define MAX_HEADER_SIZE		(10*1024)


#undef trace

/* Statics */
static void	wtrace(H3270 *session, const char *fmt, ...);

/* display a (row,col) */
const char * rcba(H3270 *hSession, int baddr)
{
	static char buf[48];
	(void) snprintf(buf, 48, "(%d,%d)", baddr/hSession->view.cols + 1, baddr%hSession->view.cols + 1);
	return buf;
}

/* Data Stream trace print, handles line wraps */
static void trace_ds_s(H3270 *hSession, char *s, Boolean can_break)
{
	static int      dscnt = 0;
	int len = strlen(s);
	Boolean nl = False;

	if (!lib3270_get_toggle(hSession,LIB3270_TOGGLE_DS_TRACE) || !len)
		return;

	if (s && s[len-1] == '\n')
	{
		len--;
		nl = True;
	}

	if (!can_break && dscnt + len >= 75)
	{
		wtrace(hSession,"...\n... ");
		dscnt = 0;
	}

	while (dscnt + len >= 75)
	{
		int plen = 75-dscnt;

		wtrace(hSession,"%.*s ...\n... ", plen, s);
		dscnt = 4;
		s += plen;
		len -= plen;
	}

	if (len)
	{
		wtrace(hSession,"%.*s", len, s);
		dscnt += len;
	}

	if (nl)
	{
		wtrace(hSession,"\n");
		dscnt = 0;
	}
}

void trace_ds(H3270 *hSession, const char *fmt, ...)
{
	char	* text;
	va_list   args;

	if (!lib3270_get_toggle(hSession,LIB3270_TOGGLE_DS_TRACE))
		return;

	va_start(args, fmt);

	/* print out remainder of message */
	text = lib3270_vsprintf(fmt,args);
	trace_ds_s(hSession,text, True);
	va_end(args);
	lib3270_free(text);
}

void trace_ds_nb(H3270 *hSession, const char *fmt, ...)
{
	char *text;
	va_list args;

	if (!lib3270_get_toggle(hSession,LIB3270_TOGGLE_DS_TRACE))
		return;

	va_start(args, fmt);

	/* print out remainder of message */
	text = lib3270_vsprintf(fmt,args);
	trace_ds_s(hSession, text, False);
	lib3270_free(text);
}

/**
 * @brief Conditional data stream trace, without line splitting.
 */
void trace_dsn(H3270 *session, const char *fmt, ...)
{
	va_list args;

	if (!lib3270_get_toggle(session,LIB3270_TOGGLE_DS_TRACE))
		return;

	/* print out message */
	va_start(args, fmt);
	session->trace.handler(session,session->trace.userdata,fmt, args);
	va_end(args);
}

/**
 * @brief Conditional ssl stream trace, without line splitting.
 */
void trace_ssl(H3270 *session, const char *fmt, ...)
{
	va_list args;

	if (!lib3270_get_toggle(session,LIB3270_TOGGLE_SSL_TRACE))
		return;

	/* print out message */
	va_start(args, fmt);
	session->trace.handler(session,session->trace.userdata,fmt, args);
	va_end(args);
}


/**
 * @brief Write to the trace file.
 */
static void wtrace(H3270 *session, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	session->trace.handler(session,session->trace.userdata,fmt, args);
	va_end(args);
}

LIB3270_EXPORT void lib3270_write_trace(H3270 *session, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	session->trace.handler(session,session->trace.userdata,fmt, args);
	va_end(args);
}

LIB3270_EXPORT void lib3270_write_dstrace(H3270 *session, const char *fmt, ...)
{
	va_list args;

	if(!lib3270_get_toggle(session,LIB3270_TOGGLE_DS_TRACE))
		return;

	va_start(args, fmt);
	session->trace.handler(session,session->trace.userdata,fmt, args);
	va_end(args);
}

LIB3270_EXPORT void lib3270_write_nettrace(H3270 *session, const char *fmt, ...)
{
	va_list args;

	if(!lib3270_get_toggle(session,LIB3270_TOGGLE_NETWORK_TRACE))
		return;

	va_start(args, fmt);
	session->trace.handler(session,session->trace.userdata,fmt, args);
	va_end(args);
}

LIB3270_EXPORT void lib3270_write_screen_trace(H3270 *session, const char *fmt, ...)
{
	va_list args;

	if(!lib3270_get_toggle(session,LIB3270_TOGGLE_SCREEN_TRACE))
		return;

	va_start(args, fmt);
	session->trace.handler(session,session->trace.userdata,fmt, args);
	va_end(args);
}

LIB3270_EXPORT void lib3270_write_event_trace(H3270 *session, const char *fmt, ...)
{
	va_list args;

	if(!lib3270_get_toggle(session,LIB3270_TOGGLE_EVENT_TRACE))
		return;

	va_start(args, fmt);
	session->trace.handler(session,session->trace.userdata,fmt, args);
	va_end(args);
}

LIB3270_EXPORT void lib3270_trace_event(H3270 *session, const char *fmt, ...)
{
	va_list args;

	if(!lib3270_get_toggle(session,LIB3270_TOGGLE_EVENT_TRACE))
		return;

	va_start(args, fmt);
	session->trace.handler(session,session->trace.userdata,fmt, args);
	va_end(args);
}


/**
 * Screen trace function, called when the host clears the screen.
 *
 * @param session	Session Handle
 */
void trace_screen(H3270 *session)
{
	session->trace_skipping = 0;

	if (lib3270_get_toggle(session,LIB3270_TOGGLE_SCREEN_TRACE))
	{
		unsigned int row, baddr;

		for(row=baddr=0;row < session->view.rows;row++)
		{
			unsigned int col;
			wtrace(session,"%02d ",row+1);

			for(col = 0; col < session->view.cols;col++)
			{
				if(session->text[baddr].attr & LIB3270_ATTR_CG)
					wtrace(session,"%c",'.');
				else if(session->text[baddr].chr)
					wtrace(session,"%c",session->text[baddr].chr);
				else
					wtrace(session,"%c",'.');
				baddr++;
			}
			wtrace(session,"%s\n","");
		}
	}
}

/* Called from ANSI emulation code to log a single character. */
void trace_char(H3270 *hSession, char c)
{
	if (lib3270_get_toggle(hSession,LIB3270_TOGGLE_SCREEN_TRACE))
		wtrace(hSession,"%c",c);
	return;
}

/**
 * Called when disconnecting in ANSI modeto finish off the trace file.
 *
 * Called when disconnecting in ANSI mode to finish off the trace file
 * and keep the next screen clear from re-recording the screen image.
 * (In a gross violation of data hiding and modularity, trace_skipping is
 * manipulated directly in ctlr_clear()).
 *
 *
 */
void trace_ansi_disc(H3270 *hSession)
{
	unsigned int i;

	wtrace(hSession,"%c",'\n');
	for (i = 0; i < hSession->view.cols; i++)
		wtrace(hSession,"%c",'=');
	wtrace(hSession,"%c",'\n');

	hSession->trace_skipping = 1;
}

void lib3270_trace_data(H3270 *hSession, const char *msg, const unsigned char *data, size_t datalen)
{
	// 00000000001111111111222222222233333333334444444444555555555566666666667777777777
	// 01234567890123456789012345678901234567890123456789012345678901234567890123456789
	// xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx . . . . . . . . . . . . . . . .

	size_t ix;
	char buffer[80];
	char hexvalue[3];

	memset(buffer,0,sizeof(buffer));

	wtrace(hSession, "%s (%u bytes)\n", msg, (unsigned int) datalen);

	for(ix = 0; ix < datalen; ix++)
	{
		size_t col = (ix%15);

		if(col == 0)
		{
			if(ix)
				wtrace(hSession,"   %s\n",buffer);

			memset(buffer,' ',79);
			buffer[79] = 0;
		}

		snprintf(hexvalue,3,"%02x",data[ix]);
		memcpy(buffer+(col*3),hexvalue,2);

		if(data[ix] > ' ')
			buffer[48 + (col*2)] = data[ix];

	}

	wtrace(hSession,"   %s\n",buffer);

}

#endif
