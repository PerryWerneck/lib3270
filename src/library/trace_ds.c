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


/**
 * @brief 3270 data stream tracing.
 *
 */

#include <config.h>
#include <lib3270/malloc.h>

#include <internals.h>
#include <lib3270/trace.h>

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
#include "ctlrc.h"
#include "popupsc.h"
#include "telnetc.h"
#include <private/trace.h>
#include "utilc.h"
#include <lib3270/toggle.h>
#include <private/session.h>
#include <lib3270/trace.h>

#include <linux/limits.h>

/* Maximum size of a tracefile header. */
// #define MAX_HEADER_SIZE		(10*1024)

char * trace_filename(const H3270 *session, const char *template) {

	char buffer[PATH_MAX+1];

	{
		time_t ltime;
		time(&ltime);

#ifdef HAVE_LOCALTIME_R
		struct tm tm;
		strftime(buffer, PATH_MAX, template, localtime_r(&ltime,&tm));
#else
		strftime(buffer, PATH_MAX, template, localtime(&ltime));
#endif // HAVE_LOCALTIME_R

	}

	return lib3270_strdup(buffer);

}

/**
 * @brief Write to the trace file.
 */
static void wtrace(const H3270 *hSession, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	hSession->trace.write(hSession,hSession->trace.context,fmt,args);
	va_end(args);
}

/* display a (row,col) */
const char * rcba(const H3270 *hSession, int baddr) {
	static char buf[48];
	(void) snprintf(buf, 48, "(%d,%d)", baddr/hSession->view.cols + 1, baddr%hSession->view.cols + 1);
	return buf;
}

/* Data Stream trace print, handles line wraps */
static void trace_ds_s(const H3270 *hSession, char *s, Boolean can_break) {

	static int      dscnt = 0;
	int len = strlen(s);
	Boolean nl = False;

	if (s && s[len-1] == '\n') {
		len--;
		nl = True;
	}

	if (!can_break && dscnt + len >= 75) {
		wtrace(hSession,"...\n... ");
		dscnt = 0;
	}

	while (dscnt + len >= 75) {
		int plen = 75-dscnt;

		wtrace(hSession,"%.*s ...\n... ", plen, s);
		dscnt = 4;
		s += plen;
		len -= plen;
	}

	if (len) {
		wtrace(hSession,"%.*s", len, s);
		dscnt += len;
	}

	if (nl) {
		wtrace(hSession,"\n");
		dscnt = 0;
	}
}

void trace_ds(const H3270 *hSession, const char *fmt, ...) {

	if(lib3270_get_toggle(hSession,LIB3270_TOGGLE_DS_TRACE) && hSession->trace.context) {
		char	* text;
		va_list   args;

		va_start(args, fmt);

		/* print out remainder of message */
		text = lib3270_vsprintf(fmt,args);
		trace_ds_s(hSession,text, True);
		va_end(args);
		lib3270_free(text);
	}

}

void trace_ds_nb(const H3270 *hSession, const char *fmt, ...) {

	if(lib3270_get_toggle(hSession,LIB3270_TOGGLE_DS_TRACE) && hSession->trace.context) {
		char *text;
		va_list args;

		va_start(args, fmt);

		/* print out remainder of message */
		text = lib3270_vsprintf(fmt,args);
		trace_ds_s(hSession, text, False);
		lib3270_free(text);
	}
}

/**
 * @brief Conditional data stream trace, without line splitting.
 */
void trace_dsn(const H3270 *hSession, const char *fmt, ...) {

	if(lib3270_get_toggle(hSession,LIB3270_TOGGLE_DS_TRACE) && hSession->trace.context) {
		va_list args;
		va_start(args, fmt);
		hSession->trace.write(hSession,hSession->trace.context,fmt,args);
		va_end(args);
	}
	
}

/**
 * @brief Conditional ssl stream trace, without line splitting.
 */
void trace_ssl(const H3270 *hSession, const char *fmt, ...) {

	if(lib3270_get_toggle(hSession,LIB3270_TOGGLE_SSL_TRACE) && hSession->trace.context) {
		va_list args;
		va_start(args, fmt);
		hSession->trace.write(hSession,hSession->trace.context,fmt,args);
		va_end(args);
	}

}

void trace_network(const H3270 *hSession, const char *fmt, ...) {

	if(lib3270_get_toggle(hSession,LIB3270_TOGGLE_NETWORK_TRACE) && hSession->trace.context) {
		va_list args;
		va_start(args, fmt);
		hSession->trace.write(hSession,hSession->trace.context,fmt,args);
		va_end(args);
	}

}

void trace_event(const H3270 *hSession, const char *fmt, ...) {

	if(lib3270_get_toggle(hSession,LIB3270_TOGGLE_EVENT_TRACE) && hSession->trace.context) {
		va_list args;
		va_start(args, fmt);
		hSession->trace.write(hSession,hSession->trace.context,fmt,args);
		va_end(args);
	}

}

/**
 * Screen trace function, called when the host clears the screen.
 *
 * @param session	Session Handle
 */
void trace_screen(H3270 *session) {
	session->trace_skipping = 0;

	if (lib3270_get_toggle(session,LIB3270_TOGGLE_SCREEN_TRACE) && session->trace.context) {
		unsigned int row, baddr;

		for(row=baddr=0; row < session->view.rows; row++) {
			unsigned int col;
			wtrace(session,"%02d ",row+1);

			for(col = 0; col < session->view.cols; col++) {
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
void trace_char(const H3270 *hSession, char c) {
	if (lib3270_get_toggle(hSession,LIB3270_TOGGLE_SCREEN_TRACE) && hSession->trace.context)
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
 */
void trace_ansi_disc(H3270 *hSession) {
	unsigned int i;

	wtrace(hSession,"%c",'\n');
	for (i = 0; i < hSession->view.cols; i++)
		wtrace(hSession,"%c",'=');
	wtrace(hSession,"%c",'\n');

	hSession->trace_skipping = 1;
}

void trace_data(const H3270 *hSession, const char *msg, const unsigned char *data, size_t datalen) {
	// 00000000001111111111222222222233333333334444444444555555555566666666667777777777
	// 01234567890123456789012345678901234567890123456789012345678901234567890123456789
	// xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx . . . . . . . . . . . . . . . .

	size_t ix;
	char buffer[80];
	char hexvalue[3];

	memset(buffer,0,sizeof(buffer));

	wtrace(hSession, "%s (%u bytes)\n", msg, (unsigned int) datalen);

	for(ix = 0; ix < datalen; ix++) {
		size_t col = (ix%15);

		if(col == 0) {
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

 LIB3270_EXPORT const char * lib3270_trace_get_filename(const H3270 *hSession) {
	return hSession->trace.filename;
 }

static void dummy_writer(const H3270 *session, LIB3270_TRACE_CONTEXT *context, const char *fmt, va_list args) {
}

static void dummy_finalizer(const H3270 *session, LIB3270_TRACE_CONTEXT *context) {
}

LIB3270_EXPORT void lib3270_trace_close(H3270 *hSession) {

	if(hSession->trace.context) {
		hSession->trace.finalize(hSession,hSession->trace.context);
		hSession->trace.context = NULL;
	}

	hSession->trace.filename = "";
	hSession->trace.write = dummy_writer;
	hSession->trace.finalize = dummy_finalizer;

}

struct trace_file_context {
	FILE *fp;
};

 static void write_file(const H3270 *, struct trace_file_context *context, const char *fmt, va_list args) {
	vfprintf(context->fp,fmt,args);
	fflush(context->fp);
 }

 static void finalize_file(const H3270 *session, struct trace_file_context *context) {
	fclose(context->fp);
	lib3270_free(context);
 }

 LIB3270_EXPORT int lib3270_trace_open_file(H3270 *hSession, const char *template) {

	lib3270_trace_close(hSession);

	lib3270_autoptr(char) filename = trace_filename(hSession, template);
	FILE *fp = fopen(filename,"wa");
	if(!fp) {
		return errno;
	}

	struct trace_file_context *context = lib3270_malloc(sizeof(struct trace_file_context)+strlen(filename)+1);
	context->fp = fp;
	
	hSession->trace.context = (LIB3270_TRACE_CONTEXT *) context;
	hSession->trace.write = (void (*)(const H3270 *, LIB3270_TRACE_CONTEXT *, const char *, va_list)) write_file;
	hSession->trace.finalize = (void (*)(const H3270 *, LIB3270_TRACE_CONTEXT *)) finalize_file;

	hSession->trace.filename = (char *) (context+1);
	strcpy(hSession->trace.filename,filename);

	return 0;
 }

 static void console_finalize(H3270 *session, struct trace_file_context *context) {
	lib3270_free(context);
 }

 LIB3270_EXPORT int lib3270_trace_open_console(H3270 *hSession, int option) {

	lib3270_trace_close(hSession);

	struct trace_file_context *context = lib3270_new(struct trace_file_context);

	context->fp = option ? stderr : stdout;

	hSession->log.context = (LIB3270_LOG_CONTEXT *) context;
	hSession->log.write =  (void *) write_file;
	hSession->log.finalize = (void *) console_finalize;

	return 0;

 }
