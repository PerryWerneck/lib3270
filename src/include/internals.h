/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2025 Perry Werneck <perry.werneck@gmail.com>
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

#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#endif // WIN32

#include <config.h>				/* autoconf settings */
#include <lib3270.h>			/* lib3270 API calls and defs */
#include <private/linkedlist.h>
#include <lib3270/charset.h>
#include <lib3270/session.h>
#include <lib3270/actions.h>
#include <lib3270/popup.h>
#include <lib3270/log.h>
#include <lib3270/trace.h>
#include <private/mainloop.h>

#include <private/session.h>
#include <private/defs.h>

#if defined(HAVE_LDAP) && defined (HAVE_LIBSSL)
#include <openssl/x509.h>
#endif // !HAVE_LDAP && HAVE_LIBSSL

#if defined(X3270_TN3270E) && !defined(X3270_ANSI) /*[*/
#define X3270_ANSI	1	// RFC2355 requires NVT mode
#endif /*]*/

#if defined(HAVE_VASPRINTF) && !defined(_GNU_SOURCE) /*[*/
#define _GNU_SOURCE		// vasprintf isn't POSIX
#endif /*]*/

// gettext stuff
#include <private/intl.h>

#define action_name(x)  #x


#if defined(_WIN32) || defined(_MSC_VER)

	#include <winsock2.h>
	#include <windows.h>

#else

	#include <sys/time.h>			/* System time-related data types */

#endif // _WIN32

/*
 * Prerequisite #includes.
 */
#include <stdio.h>				/* Unix standard I/O library */
#include <ctype.h>				/* Character classes */
#include <string.h>				/* String manipulations */
#include <sys/types.h>			/* Basic system data types */
#include <time.h>				/* C library time functions */
#include "localdefs.h"			/* {s,tcl,c}3270-specific defines */

/*
 * Cancel out contradictory parts.
 */
#if !defined(X3270_DISPLAY) /*[*/
#undef X3270_KEYPAD
#undef X3270_MENUS
#endif /*]*/

/**
 * @brief types of internal actions
 */
enum iaction {
	IA_STRING, IA_PASTE, IA_REDRAW,
	IA_KEYPAD, IA_DEFAULT, IA_KEY,
	IA_MACRO, IA_SCRIPT, IA_PEEK,
	IA_TYPEAHEAD, IA_FT, IA_COMMAND, IA_KEYMAP,
	IA_IDLE
};

#if defined(X3270_DBCS) /*[*/
LIB3270_INTERNAL Boolean		dbcs;
#endif /*]*/

/// @brief State macros
#define PCONNECTED		lib3270_pconnected(hSession)
#define HALF_CONNECTED	lib3270_half_connected(hSession)
#define CONNECTED		lib3270_is_connected(hSession)

#define IN_NEITHER		lib3270_in_neither(hSession)
#define IN_ANSI			lib3270_in_ansi(hSession)
#define IN_3270			lib3270_in_3270(hSession)
#define IN_SSCP			lib3270_in_sscp(hSession)
#define IN_TN3270E		lib3270_in_tn3270e(hSession)
#define IN_E			lib3270_in_e(hSession)

/// @brief Naming convention for private actions.
#define PA_PFX	"PA-"

#define GR_BLINK		0x01
#define GR_REVERSE		0x02
#define GR_UNDERLINE	0x04
#define GR_INTENSIFY	0x08

#define CS_MASK			0x03	///< @brief mask for specific character sets */
#define CS_BASE			0x00	///< @brief base character set (X'00') */
#define CS_APL			0x01	///< @brief APL character set (X'01' or GE) */
#define CS_LINEDRAW		0x02	///< @brief DEC line-drawing character set (ANSI) */
#define CS_DBCS			0x03	///< @brief DBCS character set (X'F8') */
#define CS_GE			0x04	///< @brief cs flag for Graphic Escape */



/* Portability macros */

/*   Equivalent of setlinebuf */

#if defined(_IOLBF) /*[*/
#define SETLINEBUF(s)	setvbuf(s, (char *)NULL, _IOLBF, BUFSIZ)
#else /*][*/
#define SETLINEBUF(s)	setlinebuf(s)
#endif /*]*/

/*   Motorola version of gettimeofday */

#if defined(MOTOROLA)
#define gettimeofday(tp,tz)	gettimeofday(tp)
#endif

/* Default DFT file transfer buffer size. */
#if defined(X3270_FT) && !defined(DFT_BUF) /*[*/
#define DFT_BUF		(4 * 1024)
#endif /*]*/

/**
 * @brief input key type
 */
enum keytype {
	KT_STD,
	KT_GE
};

LIB3270_INTERNAL struct _ansictl {
	char     vintr;
	char     vquit;
	char     verase;
	char     vkill;
	char     veof;
	char     vwerase;
	char     vrprnt;
	char     vlnext;
} ansictl;





/**
 *
 * @brief Timeout control structure.
 *
 */ /*
typedef struct timeout {
	LIB3270_LINKED_LIST_HEAD

	unsigned char in_play;

#if defined(_WIN32)
	unsigned long long ts;
#else
	struct timeval tv;
#endif

	int (*proc)(H3270 *session, void *userdata);

} timeout_t;

*/

/**
 *
 * @brief I/O events.
 *
 */ /*
 ypedef struct _input_t {
	LIB3270_LINKED_LIST_HEAD

	unsigned char	  enabled;
	int 			  fd;
	LIB3270_IO_FLAG	  flag;

	void (*call)(H3270 *, int, LIB3270_IO_FLAG, void *);

} input_t;
*/


struct lib3270_state_callback {
	LIB3270_LINKED_LIST_HEAD

	void (*func)(H3270 *, int, void *);							/**< @brief Function to call */
};

#define SELECTION_LEFT			0x01
#define SELECTION_TOP			0x02
#define SELECTION_RIGHT			0x04
#define SELECTION_BOTTOM		0x08

#define SELECTION_SINGLE_COL	0x10
#define SELECTION_SINGLE_ROW	0x20

#define SELECTION_ACTIVE		0x80

#ifdef _WIN32
/// @brief Windows Event Log Handler.
LIB3270_INTERNAL HANDLE hModule;
#endif // _WIN32

/* Library internal calls */
LIB3270_INTERNAL int	key_ACharacter(H3270 *hSession, unsigned char c, enum keytype keytype, enum iaction cause,Boolean *skipped);
LIB3270_INTERNAL int	cursor_move(H3270 *session, int baddr);

// LIB3270_INTERNAL void	toggle_rectselect(H3270 *session, const struct lib3270_toggle *t, LIB3270_TOGGLE_TYPE tt);
// LIB3270_INTERNAL void	remove_input_calls(H3270 *session);

// LIB3270_INTERNAL int	lib3270_sock_send(H3270 *hSession, unsigned const char *buf, int len);

// LIB3270_INTERNAL int	lib3270_default_event_dispatcher(H3270 *hSession, int block);

LIB3270_INTERNAL int 	do_select(H3270 *h, unsigned int start, unsigned int end, unsigned int rect);

// LIB3270_INTERNAL void	connection_failed(H3270 *hSession, const char *message);

/**
 * @brief Activate auto-reconnect timer.
 *
 * @param hSession	TN3270 Session handle.
 * @param msec		Time to reconnect.
 *
 * @return 0 if ok or error code if not.
 *
 * @retval EBUSY	Auto reconnect is already active.
 */
LIB3270_INTERNAL int lib3270_activate_auto_reconnect(H3270 *hSession, unsigned long msec);

LIB3270_INTERNAL int	non_blocking(H3270 *session, Boolean on);

LIB3270_INTERNAL void	set_ssl_state(H3270 *session, LIB3270_SSL_STATE state);

/// @brief Clear element at adress.
LIB3270_INTERNAL void clear_chr(H3270 *hSession, int baddr);

LIB3270_INTERNAL unsigned char get_field_attribute(H3270 *session, int baddr);

/// @brief Default log writer.
// LIB3270_INTERNAL int default_loghandler(const H3270 *session, void *dunno, const char *module, int rc, const char *message);

// LIB3270_INTERNAL char * lib3270_get_user_name();

/// @brief Query data from URL.
///
/// @param hSession		Handle of the TN3270 Session.
/// @param url			The url to get.
/// @param length		Pointer to the response lenght (can be NULL).
/// @param error		Pointer to the detailed error message.
///
/// @return The data from URL (release it with lib3270_free) or NULL on error.
///
LIB3270_INTERNAL char * lib3270_url_get(H3270 *hSession, const char *url, const char **error);

/// @brief Load text file.
///
/// @param hSession		Handle of the TN3270 Session.
/// @param filename		The file name.
///
/// @return The file contents (release it with lib3270_free or NULL on error (sets errno).
///
LIB3270_INTERNAL char * lib3270_file_get_contents(H3270 *hSession, const char *filename);

// LIB3270_INTERNAL void lib3270_notify_tls(H3270 *hSession);


#if defined(HAVE_LDAP) && defined (HAVE_LIBSSL)
/**
 * @brief Download X509 CRL using LDAP backend.
 *
 * @param hSession	tn3270 session handle.
 * @param url		URL for Ldap access.
 * @param error		pointer to error message.
 *
 */
LIB3270_INTERNAL X509_CRL * lib3270_crl_get_using_ldap(H3270 *hSession, const char *url, const char **error);
#endif // HAVE_LDAP

#ifdef _WIN32

LIB3270_INTERNAL char * lib3270_get_from_url(H3270 *hSession, const char *url, const char **error_message);

#endif // _WIN32
