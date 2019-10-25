/*
 * "Software G3270, desenvolvido com base nos códigos fontes do WC3270  e  X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270.
 *
 * Copyright (C) <2008> <Banco do Brasil S.A.>
 *
 * Este programa é software livre. Você pode redistribuí-lo e/ou modificá-lo sob
 * os termos da GPL v.2 - Licença Pública Geral  ',  conforme  publicado  pela
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
 * Este programa está nomeado como private.h e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas de Mendonça)
 *
 */

#ifdef WIN32
	#include <winsock2.h>
	#include <windows.h>
#endif // WIN32

#include <config.h>				/* autoconf settings */
#include <lib3270.h>			/* lib3270 API calls and defs */
#include <linkedlist.h>
#include <lib3270/charset.h>
#include <lib3270/session.h>
#include <lib3270/actions.h>

#if defined(HAVE_LIBSSL)
	#include <openssl/ssl.h>
	#include <openssl/x509v3.h>

#endif // HAVE_LIBSSL

#if defined(X3270_TN3270E) && !defined(X3270_ANSI) /*[*/
		#define X3270_ANSI	1	/* RFC2355 requires NVT mode */
#endif /*]*/

#if defined(HAVE_VASPRINTF) && !defined(_GNU_SOURCE) /*[*/
	#define _GNU_SOURCE		/* vasprintf isn't POSIX */
#endif /*]*/

/*
 * gettext stuff
 */
#ifdef ANDROID
	#undef HAVE_LIBINTL
	#undef HAVE_LIBSSL
#endif

#ifdef HAVE_LIBINTL
	#include <libintl.h>
	#define _( x ) 			gettext(x)
	#define N_( x ) 		x
#else
	#define _( x ) 			x
	#define N_( x ) 		x
#endif // HAVE_LIBINTL

#define action_name(x)  #x

//
// Compiler-specific #defines.
//
// Reference: GLIBC gmacros.h
//
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)

	#define GNUC_UNUSED \
		__attribute__((__unused__))

#else

	#define unused
	#define GNUC_UNUSED
	#define printflike(s, f)

#endif

#if defined(_WIN32) || defined(_MSC_VER)

	#include <winsock2.h>
	#include <windows.h>

#else

	#include <unistd.h>				/* Unix system calls */
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

#define RECONNECT_MS		2000	/**< @brief 2 sec before reconnecting to host. */
#define RECONNECT_ERR_MS	5000	/**< @brief 5 sec before reconnecting to host when failed */

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

// Version strings
LIB3270_INTERNAL const char * build;
LIB3270_INTERNAL const char * app_defaults_version;
LIB3270_INTERNAL const char * sccsid;
LIB3270_INTERNAL const char * build_rpq_timestamp;
LIB3270_INTERNAL const char * build_rpq_version;
LIB3270_INTERNAL const char * build_rpq_revision;

#if defined(X3270_DBCS) /*[*/
	LIB3270_INTERNAL Boolean		dbcs;
#endif /*]*/


/**
 * @brief toggle names
 */ /*
struct toggle_name {
	const char *name;
	int index;
}; */

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

/// @brief Shorthand macros
#define CN	((char *) NULL)
#define PN	((XtPointer) NULL)
#define Replace(var, value) { lib3270_free(var); var = (value); };

/// @brief Configuration change masks.
//#define NO_CHANGE		0x0000	/// @brief no change
// #define MODEL_CHANGE	0x0001	/// @brief screen dimensions changed
//#define FONT_CHANGE		0x0002	/// @brief emulator font changed
//#define COLOR_CHANGE	0x0004	/// @brief color scheme or 3278/9 mode changed
//#define SCROLL_CHANGE	0x0008	/// @brief scrollbar snapped on or off
//#define CHARSET_CHANGE	0x0010	/// @brief character set changed
// #define ALL_CHANGE		0xffff	/// @brief everything changed

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

/* DBCS Preedit Types */ /*
#if defined(X3270_DBCS)
	#define PT_ROOT				"Root"
	#define PT_OVER_THE_SPOT	"OverTheSpot"
	#define PT_OFF_THE_SPOT		"OffTheSpot"
	#define PT_ON_THE_SPOT		"OnTheSpot"
#endif */

/**
 * @brief input key type
 */
enum keytype
{
	KT_STD,
	KT_GE
};

LIB3270_INTERNAL struct _ansictl
{
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
 * @brief Extended attributes
 */
struct lib3270_ea
{
	unsigned char cc;		///< @brief EBCDIC or ASCII character code
	unsigned char fa;		///< @brief field attribute, it nonzero
	unsigned char fg;		///< @brief foreground color (0x00 or 0xf<n>)
	unsigned char bg;		///< @brief background color (0x00 or 0xf<n>)
	unsigned char gr;		///< @brief ANSI graphics rendition bits
	unsigned char cs;		///< @brief character set (GE flag, or 0..2)
	unsigned char ic;		///< @brief input control (DBCS)
	unsigned char db;		///< @brief DBCS state
};

struct lib3270_text
{
	unsigned char  chr;		///< @brief ASCII character code
	unsigned short attr;	///< @brief Converted character attribute (color & etc)
};

#ifndef LIB3270_TA
	#define LIB3270_TA void
#endif // !LIB3270_TA

#define LIB3270_MB_MAX					16

#define LIB3270_FULL_MODEL_NAME_LENGTH	13
#define LIB3270_LU_MAX					32

#define LIB3270_TELNET_N_OPTS			256

/**
 *
 * @brief Timeout control structure.
 *
 */
typedef struct timeout
{
	LIB3270_LINKED_LIST_HEAD

	unsigned char in_play;

#if defined(_WIN32) /*[*/
	unsigned long long ts;
#else /*][*/
	struct timeval tv;
#endif /*]*/

	int (*proc)(H3270 *session);

} timeout_t;


/**
 *
 * @brief I/O events.
 *
 */
typedef struct _input_t
{
	LIB3270_LINKED_LIST_HEAD

	unsigned char	  enabled;
	int 			  fd;
	LIB3270_IO_FLAG	  flag;

	void (*call)(H3270 *, int, LIB3270_IO_FLAG, void *);

} input_t;

struct lib3270_state_callback
{
	LIB3270_LINKED_LIST_HEAD

	void (*func)(H3270 *, int, void *);							/**< @brief Function to call */
};

struct lib3270_toggle_callback
{
	LIB3270_LINKED_LIST_HEAD

	void (*func)(H3270 *, LIB3270_TOGGLE_ID, char, void *);		/**< @brief Function to call */
};

/**
 *
 * @brief lib3270 session data
 *
 */
struct _h3270
{
	struct lib3270_session_callbacks	  cbk;					///< @brief Callback table - Always the first one.

	// Session info
	char					  id;								///< @brief Session Identifier.

	// Connection info
	struct {
		int					  sock;								///< @brief Network socket.
		LIB3270_CSTATE		  state;							///< @brief Connection state.
	} connection;

	// flags
	LIB3270_HOST_TYPE		  host_type;						///< @brief Host type.

	int					  	  selected					: 1;	///< @brief Has selected region?
	int						  rectsel					: 1;	///< @brief Selected region is a rectangle ?
	int						  vcontrol					: 1;	///< @brief Visible control ?
	int						  modified_sel				: 1;
	int						  mono						: 1;	///< @brief Forces monochrome display
	int						  m3279						: 1;
	int 					  extended					: 1;	///< @brief Extended data stream.
	int						  typeahead					: 1;
	int						  numeric_lock				: 1;
	int						  oerr_lock					: 1;	///< @brief If true, operator errors will lock the keyboard.
	int						  unlock_delay				: 1;	///< @brief If true the unlock delay feature is enabled. @see lib3270_set_unlock_delay
	int			 			  auto_reconnect_inprogress	: 1;
	unsigned int			  colors					: 5;
	int						  apl_mode					: 1;	///< @brief If true enables APL mode.
	int						  icrnl						: 1;
	int						  inlcr						: 1;
	int						  onlcr						: 1;
	int						  bsd_tm					: 1;
	int 					  syncing					: 1;
	int						  reverse 					: 1;	/**< @brief reverse-input mode */
	int						  dbcs						: 1;
	int             		  linemode					: 1;
	int						  trace_skipping			: 1;
	int						  need_tls_follows			: 1;
	int						  cut_xfer_in_progress		: 1;
//		int						  auto_keymap				: 1;
	int						  formatted					: 1;	/**< @brief Formatted screen flag */
	int						  starting					: 1;	/**< @brief Is starting (no first screen)? */

	struct lib3270_toggle
	{
		char value;																/**< toggle value */
		void (*upcall)(H3270 *, struct lib3270_toggle *, LIB3270_TOGGLE_TYPE);	/**< change value */
	}						  toggle[LIB3270_TOGGLE_COUNT];

	// Network & Termtype
	char					* connected_type;
	char					  full_model_name[LIB3270_FULL_MODEL_NAME_LENGTH+1];
	char					* model_name;
	unsigned int			  model_num;
	char  	     	    	* termtype;

	struct
	{
		char 	   	    	* url;				/**< The host URL, for use in reconnecting */
		char				* current;			/**< The hostname part, stripped of qualifiers, luname and port number */
		char				* srvc;				/**< The service name */
		char	   	    	* qualified;
	} host;

	char					* termname;

	struct lib3270_charset	  charset;

	struct
	{
		LIB3270_MESSAGE		  status;
		unsigned char		  flag[LIB3270_FLAG_COUNT];
	} oia;

	unsigned short			  current_port;

	// Misc
	H3270FT					* ft;					/**< @brief Active file transfer data */

	// screen info

	// Oversize.
	struct
	{
		char				* str;
		unsigned int		  rows;
		unsigned int		  cols;
	} oversize;

	// Maximum screen size.
	struct
	{
		unsigned int		  rows;
		unsigned int		  cols;
	} max;

	// View size
	struct {
		unsigned int		  rows;
		unsigned int		  cols;
	} view;

	LIB3270_POINTER			  pointer;				/**< @brief Current pointer. */
	int						  cursor_addr;
	int						  buffer_addr;
	char					  flipped;
	int						  screen_alt;			/**< @brief alternate screen? */
	int						  is_altbuffer;

	// Screen contents
	void 					* buffer[2];			/**< @brief Internal buffers */
	struct lib3270_ea  		* ea_buf;				/**< @brief 3270 device buffer. ea_buf[-1] is the dummy default field attribute */
	struct lib3270_ea		* aea_buf;				/**< @brief alternate 3270 extended attribute buffer */
	struct lib3270_text		* text;					/**< @brief Converted 3270 chars */

	// host.c
	char	 				  std_ds_host;
	char 					  no_login_host;
	char 					  non_tn3270e_host;
	char 					  passthru_host;
	char 					  ever_3270;

	// ctlr.c
	int						  sscp_start;
	unsigned char			  default_fg;
	unsigned char			  default_bg;
	unsigned char			  default_gr;
	unsigned char			  default_cs;
	unsigned char			  default_ic;
	char					  reply_mode;
	int 					  trace_primed 		: 1;
	int						  ticking			: 1;
	int						  mticking			: 1;
	int						  crm_nattr;
	unsigned char			  crm_attr[16];
	unsigned char 			* zero_buf;				/**< @brief Empty buffer, for area clears */

	struct timeval			  t_start;
	void					* tick_id;
	struct timeval			  t_want;

	// Telnet.c
	unsigned char 			* ibuf;
	int      				  ibuf_size;			/**< @brief size of ibuf */
	time_t          		  ns_time;
	int             		  ns_brcvd;
	int             		  ns_rrcvd;
	int             		  ns_bsent;
	int             		  ns_rsent;
	struct timeval 			  ds_ts;
	unsigned short			  e_xmit_seq;			/**< @brief transmit sequence number */
	int						  response_required;
	int						  ansi_data;
	int						  lnext;
	int						  backslashed;
	char					  plu_name[LIB3270_BIND_PLU_NAME_MAX+1];

	/*
	/// @brief Proxy
	struct
	{
		char					* proxy;			///< Proxy server (type:host[:port])
		int						  type;
		char					* host;
		char					* portname;
		unsigned short			  port;
	} proxy;
	*/

	/// @brief LU
	char					**curr_lu;
	char					* try_lu;
	char					**lus;			///< @brief Array with the LU names to try.
	struct
	{
		char	  reported[LIB3270_LU_MAX+1];
		char	* connected;
		char	  name[LIB3270_LUNAME_LENGTH+1];

	} lu;

	char					  reported_type[LIB3270_LU_MAX+1];

	// TN3270e
	enum
	{
		E_NONE,
		E_3270,
		E_NVT,
		E_SSCP
	}						  tn3270e_submode;

	unsigned long			  e_funcs;				/**< @brief negotiated TN3270E functions */
	int						  tn3270e_bound;
	int						  tn3270e_negotiated;

	// Line mode
	unsigned char 			* lbuf;					/**< @brief line-mode input buffer */
	unsigned char 			* lbptr;

	// 3270 input buffer
	unsigned char 			* ibptr;

	// Output buffer.
	struct
	{
		unsigned char		* buf;				///< @brief 3270 output buffer */
		unsigned char 		* base;
		int					  length;			///< @brief Length of the output buffer.
		unsigned char		* ptr;
	} output;

	// network input buffer
	unsigned char 			* sbbuf;

	// telnet sub-option buffer
	unsigned char 			* sbptr;
	unsigned char			  telnet_state;

	unsigned char 			  myopts[LIB3270_TELNET_N_OPTS];
	unsigned char			  hisopts[LIB3270_TELNET_N_OPTS];

	// kybd.c
	unsigned int			  kybdlock;				///< @brief @brief keyboard lock state.
	unsigned char			  aid;					///< @brief @brief current attention ID.
	void					* unlock_id;
	time_t					  unlock_delay_time;
	unsigned long 			  unlock_delay_ms;		///< @brief Delay before actually unlocking the keyboard after the host permits it.
	LIB3270_TA				* ta_head;
	LIB3270_TA				* ta_tail;

	// ft_dft.c
	int						  dft_buffersize;		///< @brief Buffer size (LIMIN, LIMOUT)

	// rpq.c
	int						  rpq_complained : 1;
#if !defined(_WIN32)
	int						  omit_due_space_limit : 1;
#endif

	char					* rpq_warnbuf;
	int						  rpq_wbcnt;

	// User data (Usually points to session's widget)
	void					* user_data;

	// selection
	char					* paste_buffer;
	struct
	{
		int start;
		int end;
	} select;

	// ansi.c
	int      				  scroll_top;
	int						  scroll_bottom;
	int						  once_cset;
	int						  saved_cursor;

	int						  held_wrap					: 1;

	int						  insert_mode				: 1;
	int						  auto_newline_mode			: 1;

	int						  appl_cursor				: 1;
	int						  saved_appl_cursor			: 1;

	int  					  wraparound_mode			: 1;
	int						  saved_wraparound_mode		: 1;

	int						  rev_wraparound_mode 		: 1;
	int						  saved_rev_wraparound_mode	: 1;

	int						  allow_wide_mode			: 1;
	int						  saved_allow_wide_mode		: 1;

	int						  wide_mode 				: 1;
	int						  saved_wide_mode			: 1;

	int						  saved_altbuffer			: 1;
	int						  ansi_reset				: 1;	/**< @brief Non zero if the ansi_reset() was called in this session */

	int      				  ansi_ch;
	int						  cs_to_change;

	/** @brief ANSI Character sets. */
	enum lib3270_ansi_cs
	{
		LIB3270_ANSI_CS_G0 = 0,
		LIB3270_ANSI_CS_G1 = 1,
		LIB3270_ANSI_CS_G2 = 2,
		LIB3270_ANSI_CS_G3 = 3
	}						  cset;
	enum lib3270_ansi_cs	  saved_cset;

	/** @brief Character set designations. */
	enum lib3270_ansi_csd
	{
		LIB3270_ANSI_CSD_LD = 0,
		LIB3270_ANSI_CSD_UK = 1,
		LIB3270_ANSI_CSD_US = 2
	} 						  csd[4];
	enum lib3270_ansi_csd 	  saved_csd[4];

	enum lib3270_ansi_state
	{
		LIB3270_ANSI_STATE_DATA		= 0,
		LIB3270_ANSI_STATE_ESC		= 1,
		LIB3270_ANSI_STATE_CSDES	= 2,
		LIB3270_ANSI_STATE_N1		= 3,
		LIB3270_ANSI_STATE_DECP		= 4,
		LIB3270_ANSI_STATE_TEXT		= 5,
		LIB3270_ANSI_STATE_TEXT2	= 6,
		LIB3270_ANSI_STATE_MBPEND	= 7
	}						  state;

	unsigned char			* tabs;

	int						  pmi;
	char					  pending_mbs[LIB3270_MB_MAX];

	unsigned char 			  gr;
	unsigned char			  saved_gr;

	unsigned char			  fg;
	unsigned char			  saved_fg;

	unsigned char			  bg;
	unsigned char			  saved_bg;

	// xio
	struct {
		void 				* read;
		void 				* write;
		void 				* except;
	} xio;

	size_t					  popups;	///< @brief Count open popups.

#ifdef HAVE_LIBSSL
	/// @brief SSL Data.
	struct
	{
		char				  enabled;
		char				  host;
		LIB3270_SSL_STATE	  state;
		unsigned long 		  error;
#ifdef SSL_ENABLE_CRL_CHECK
		struct
		{
			char			* prefer;	///< @brief Prefered protocol for CRL.
			char			* url;		///< @brief URL for CRL download.
			X509_CRL 		* cert;		///< @brief Loaded CRL (can be null).
		} crl;
#endif // SSL_ENABLE_CRL_CHECK
		SSL 				* con;
	} ssl;
#endif // HAVE_LIBSSL

	struct lib3270_linked_list_head timeouts;

	struct
	{
		struct lib3270_linked_list_head	list;
		int changed : 1;
	} input;

	// Trace methods.
	struct
	{
		void (*handler)(H3270 *session, void *userdata, const char *fmt, va_list args);
		void *userdata;
	} trace;

	/// @brief Event Listeners.
	struct
	{
		/// @brief State listeners.
		struct lib3270_linked_list_head state[LIB3270_STATE_USER];

		/// @brief Toggle listeners.
		struct lib3270_linked_list_head toggle[LIB3270_TOGGLE_COUNT];

		/// @brief Action listeners.
		struct lib3270_linked_list_head actions[LIB3270_ACTION_GROUP_CUSTOM];

	} listeners;


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
LIB3270_INTERNAL HANDLE hEventLog;
LIB3270_INTERNAL HANDLE hModule;
#endif // _WIN32

#ifdef HAVE_SYSLOG
/// @brief Windows Event Log Handler.
LIB3270_INTERNAL int use_syslog;
#endif // HAVE_SYSLOG


/* Library internal calls */
LIB3270_INTERNAL int	key_ACharacter(H3270 *hSession, unsigned char c, enum keytype keytype, enum iaction cause,Boolean *skipped);
LIB3270_INTERNAL int	cursor_move(H3270 *session, int baddr);

LIB3270_INTERNAL void	toggle_rectselect(H3270 *session, struct lib3270_toggle *t, LIB3270_TOGGLE_TYPE tt);
LIB3270_INTERNAL void	remove_input_calls(H3270 *session);

LIB3270_INTERNAL int	lib3270_sock_send(H3270 *hSession, unsigned const char *buf, int len);
LIB3270_INTERNAL void	lib3270_sock_disconnect(H3270 *hSession);

LIB3270_INTERNAL int	lib3270_default_event_dispatcher(H3270 *hSession, int block);

LIB3270_INTERNAL void 	do_select(H3270 *h, unsigned int start, unsigned int end, unsigned int rect);


/**
 * @brief Called from timer to attempt an automatic reconnection.
 */
LIB3270_INTERNAL int	lib3270_check_for_auto_reconnect(H3270 *hSession);

#if defined(DEBUG)
	#define CHECK_SESSION_HANDLE(x) check_session_handle(&x,__FUNCTION__);
	LIB3270_INTERNAL void check_session_handle(H3270 **hSession, const char *fname);
#else
	#define CHECK_SESSION_HANDLE(x) check_session_handle(&x);
	LIB3270_INTERNAL void check_session_handle(H3270 **hSession);
#endif // DEBUG

LIB3270_INTERNAL int check_online_session(const H3270 *hSession);
LIB3270_INTERNAL int check_offline_session(const H3270 *hSession);

/// @brief Returns -1 if the session is invalid or not online (sets errno).
#define FAIL_IF_NOT_ONLINE(x) if(check_online_session(x)) return errno;

/// @brief Returns -1 if the session is invalid or online (sets errno).
#define FAIL_IF_ONLINE(x) if(check_offline_session(x)) return errno;

LIB3270_INTERNAL int	non_blocking(H3270 *session, Boolean on);

#if defined(HAVE_LIBSSL)

	typedef struct _ssl_error_message
	{
		int			  error;
		const char	* title;
		const char	* text;
		const char	* description;
#ifdef _WIN32
		DWORD		  lasterror;
#endif // _WIN32
	} SSL_ERROR_MESSAGE;

	struct ssl_status_msg
	{
		long			  id;
		LIB3270_NOTIFY	  icon;
		const char		* iconName;		// Icon name from https://specifications.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html
		const char		* message;
		const char		* description;
	};

	LIB3270_INTERNAL int							  ssl_ctx_init(H3270 *hSession, SSL_ERROR_MESSAGE *message);
	LIB3270_INTERNAL int							  ssl_init(H3270 *session);
	LIB3270_INTERNAL int							  ssl_negotiate(H3270 *hSession);
	LIB3270_INTERNAL void							  set_ssl_state(H3270 *session, LIB3270_SSL_STATE state);
	LIB3270_INTERNAL const struct ssl_status_msg	* ssl_get_status_from_error_code(long id);


	#if OPENSSL_VERSION_NUMBER >= 0x00907000L
		#define INFO_CONST const
	#else
		#define INFO_CONST
	#endif

	LIB3270_INTERNAL void ssl_info_callback(INFO_CONST SSL *s, int where, int ret);

	/**
	 * @brief Global SSL_CTX object as framework to establish TLS/SSL or DTLS enabled connections.
	 *
	 */
	LIB3270_INTERNAL SSL_CTX * ssl_ctx;

	/**
	 * @brief Index of h3270 handle in SSL session.
	 *
	 */
	LIB3270_INTERNAL int ssl_3270_ex_index;

	/**
	 * @brief Emit popup on ssl error.
	 *
	 */
	LIB3270_INTERNAL int popup_ssl_error(H3270 *session, int rc, const char *title, const char *summary, const char *body);

	/**
	 * @brief Emite popup on SSL error.
	 *
	 */
	LIB3270_INTERNAL int notify_ssl_error(H3270 *hSession, int rc, const SSL_ERROR_MESSAGE *message);

#endif

	/// @brief Clear element at adress.
	LIB3270_INTERNAL void clear_chr(H3270 *hSession, int baddr);

	LIB3270_INTERNAL unsigned char get_field_attribute(H3270 *session, int baddr);

	/// @brief Default log writer.
	LIB3270_INTERNAL void default_log_writer(H3270 *session, const char *module, int rc, const char *fmt, va_list arg_ptr);

	LIB3270_INTERNAL char * lib3270_get_user_name();

	/// @brief Query data from URL.
	///
	/// @param hSession			Handle of the TN3270 Session.
	/// @param url				The url to get.
	/// @param length			Pointer to the response lenght (can be NULL).
	/// @param error_message	Pointer to the error message.
	///
	/// @return The data from URL (release it with lib3270_free) or NULL on error.
	///
	LIB3270_INTERNAL char * lib3270_get_from_url(H3270 *hSession, const char *url, size_t *length, const char **error_message);

	/// @brief Fire CState change.
	LIB3270_INTERNAL int lib3270_set_cstate(H3270 *hSession, LIB3270_CSTATE cstate);

	/// @brief Notify actions.
	LIB3270_INTERNAL void lib3270_notify_actions(H3270 *hSession, LIB3270_ACTION_GROUP group);
