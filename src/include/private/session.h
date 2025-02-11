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

 #pragma once

 #include <config.h>
 #include <lib3270.h>
 #include <lib3270/toggle.h>
 #include <lib3270/session.h>
 #include <lib3270/charset.h>
 #include <lib3270/trace.h>
 #include <lib3270/log.h>
 #include <sys/time.h>

 #include <networking.h>
 #include <linkedlist.h>
 
 #define LIB3270_FULL_MODEL_NAME_LENGTH	13
 #define LIB3270_LU_MAX					32
 #define LIB3270_TELNET_N_OPTS			256
 #define LIB3270_MB_MAX					16

 #ifndef LIB3270_TA
	 #define LIB3270_TA void
 #endif // !LIB3270_TA

 struct _h3270 {

	/// @brief Session Identifier.
	char id;

	struct lib3270_session_callbacks	  cbk;					///< @brief Callback table - Always the first one.

	/// @brief I/O handlers for this session.
	struct {

		struct {
			void *	(*add)(H3270 *session, unsigned long interval_ms, int (*proc)(H3270 *session, void *userdata), void *userdata);
			void	(*remove)(H3270 *session, void *timer);
		} timer;

		struct {
			void *	(*add)(H3270 *session, int fd, LIB3270_IO_FLAG flag, void(*proc)(H3270 *, int, LIB3270_IO_FLAG, void *), void *userdata );
			void	(*remove)(H3270 *session, void *id);
			void	(*set_state)(H3270 *session, void *id, int enabled);
		} poll;

	} io;

	int		(*event_dispatcher)(H3270 *session,int wait);
	int 	(*wait)(H3270 *session, int seconds);
	void	(*ring_bell)(H3270 *session);
	int		(*run)(H3270 *session, const char *name, int(*callback)(H3270 *, void *), void *parm);

	// Networking
	struct {

		/// @brief Connection context.
		LIB3270_NET_CONTEXT * context;

		LIB3270_CSTATE		  state;							///< @brief Connection state.
		unsigned int		  timeout;							///< @brief Connection timeout in seconds.
		unsigned int		  retry;							///< @brief Time to retry when connection ends with error (0 = none).
		LIB3270_POPUP		* error;							///< @brief Last connection error.

		int (*write)(H3270 *hSession, const void *buffer, size_t length, LIB3270_NET_CONTEXT *context);
		int (*except)(H3270 *hSession, LIB3270_NET_CONTEXT *context);
		
	} connection;

	// flags
	LIB3270_HOST_TYPE		  host_type;						///< @brief Host type.

	unsigned int		  	  selected					: 1;	///< @brief Has selected region?
	unsigned int			  has_copy					: 1;	///< @brief Has copy?
	unsigned int			  rectsel					: 1;	///< @brief Selected region is a rectangle ?
	unsigned int			  vcontrol					: 1;	///< @brief Visible control ?
	unsigned int			  modified_sel				: 1;
	unsigned int			  mono						: 1;	///< @brief Forces monochrome display
	unsigned int			  m3279						: 1;
	unsigned int 			  extended					: 1;	///< @brief Extended data stream.
	unsigned int			  typeahead					: 1;
	unsigned int			  numeric_lock				: 1;
	unsigned int			  oerr_lock					: 1;	///< @brief If true, operator errors will lock the keyboard.
	unsigned int			  unlock_delay				: 1;	///< @brief If true the unlock delay feature is enabled. @see lib3270_set_unlock_delay
	unsigned int 			  auto_reconnect_inprogress	: 1;
	unsigned int			  colors					: 5;
	unsigned int			  apl_mode					: 1;	///< @brief If true enables APL mode.
	unsigned int			  icrnl						: 1;
	unsigned int			  inlcr						: 1;
	unsigned int			  onlcr						: 1;
	unsigned int			  bsd_tm					: 1;
	unsigned int			  syncing					: 1;
	unsigned int			  reverse                   : 1;    ///< @brief reverse-input mode
	unsigned int			  dbcs						: 1;
	unsigned int	   		  linemode					: 1;
	unsigned int			  trace_skipping			: 1;
	unsigned int			  need_tls_follows			: 1;
	unsigned int			  cut_xfer_in_progress		: 1;
	unsigned int			  formatted					: 1;	///< @brief Formatted screen flag
	unsigned int			  starting					: 1;	///< @brief Is starting (no first screen)?

	struct lib3270_toggle {
		char value;																		///< @brief toggle value
		void (*upcall)(H3270 *, const struct lib3270_toggle *, LIB3270_TOGGLE_TYPE);	///< @brief change value
	} toggle[LIB3270_TOGGLE_COUNT];

	// Network & Termtype
	char					* connected_type;
	char					  full_model_name[LIB3270_FULL_MODEL_NAME_LENGTH+1];
	char					* model_name;
	unsigned int			  model_num;
	char  	     	    	* termtype;

	struct {
		char 	   	    	* url;				/**< The host URL, for use in reconnecting */
		char				* current;			/**< The hostname part, stripped of qualifiers, luname and port number */
		char				* srvc;				/**< The service name */
		char	   	    	* qualified;
	} host;

	char					* termname;

	struct lib3270_charset	  charset;

	struct {
		LIB3270_MESSAGE		  status;
		unsigned char		  flag[LIB3270_FLAG_COUNT];
	} oia;

	unsigned short			  current_port;

	// Misc
	H3270FT					* ft;					/**< @brief Active file transfer data */

	// screen info

	// Oversize.
	struct {
		char				* str;
		unsigned int		  rows;
		unsigned int		  cols;
	} oversize;

	// Maximum screen size.
	struct {
		unsigned int		  rows;
		unsigned int		  cols;
	} max;

	// View size
	struct {
		unsigned int		  rows;
		unsigned int		  cols;
	} view;

	// LIB3270_POINTER			  pointer;				/**< @brief Current pointer. */
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
	unsigned int 			  trace_primed 		: 1;
	unsigned int			  ticking			: 1;
	unsigned int			  mticking			: 1;
	int						  crm_nattr;
	unsigned char			  crm_attr[16];
	unsigned char 			* zero_buf;				/**< @brief Empty buffer, for area clears */

//	struct timeval			  t_start;
//	void					* tick_id;
//	struct timeval			  t_want;

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

	/// @brief LU
	struct {
		char		  reported[LIB3270_LU_MAX+1];
		const char	* associated;						///< @brief The LU name associated with the session.
		char		**names;							///< @brief Array with the LU names to try.
		char		**curr;
		const char	* try;

	} lu;

	char					  reported_type[LIB3270_LU_MAX+1];

	// TN3270e
	enum {
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
	struct {
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
	unsigned int			  rpq_complained : 1;
#if !defined(_WIN32)
	unsigned int			  omit_due_space_limit : 1;
#endif

	char					* rpq_warnbuf;
	int						  rpq_wbcnt;

	// User data (Usually points to session's widget)
	void					* user_data;

	// selection
	char					* paste_buffer;
	struct {
		int start;
		int end;
	} select;

	// ansi.c
	int      				  scroll_top;
	int						  scroll_bottom;
	int						  once_cset;
	int						  saved_cursor;

	unsigned int			  held_wrap					: 1;

	unsigned int			  insert_mode				: 1;
	unsigned int			  auto_newline_mode			: 1;

	unsigned int			  appl_cursor				: 1;
	unsigned int			  saved_appl_cursor			: 1;

	unsigned int  			  wraparound_mode			: 1;
	unsigned int			  saved_wraparound_mode		: 1;

	unsigned int			  rev_wraparound_mode 		: 1;
	unsigned int			  saved_rev_wraparound_mode	: 1;

	unsigned int			  allow_wide_mode			: 1;
	unsigned int			  saved_allow_wide_mode		: 1;

	unsigned int			  wide_mode 				: 1;
	unsigned int			  saved_wide_mode			: 1;

	unsigned int			  saved_altbuffer			: 1;
	unsigned int			  ansi_reset				: 1;	/**< @brief Non zero if the ansi_reset() was called in this session */

	int      				  ansi_ch;
	int						  cs_to_change;

	/** @brief ANSI Character sets. */
	enum lib3270_ansi_cs {
		LIB3270_ANSI_CS_G0 = 0,
		LIB3270_ANSI_CS_G1 = 1,
		LIB3270_ANSI_CS_G2 = 2,
		LIB3270_ANSI_CS_G3 = 3
	}						  cset;
	enum lib3270_ansi_cs	  saved_cset;

	/** @brief Character set designations. */
	enum lib3270_ansi_csd {
		LIB3270_ANSI_CSD_LD = 0,
		LIB3270_ANSI_CSD_UK = 1,
		LIB3270_ANSI_CSD_US = 2
	} 						  csd[4];
	enum lib3270_ansi_csd 	  saved_csd[4];

	enum lib3270_ansi_state {
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

	struct lib3270_linked_list_head timeouts;

	struct {
		struct lib3270_linked_list_head	list;
		unsigned int changed : 1;
	} input;

	// Trace methods.
	struct {
		char *file;	///< @brief Trace file name (if set).
		LIB3270_TRACE_HANDLER handler;
		void *userdata;
	} trace;

	struct {
		char *file; 		///< @brief Log file name (if set).
		LIB3270_LOG_HANDLER handler;
		void *userdata;
	} log;

	struct {
		unsigned int					  host			: 1;		///< @brief Non zero if host requires SSL.
		unsigned int					  download_crl	: 1;		///< @brief Non zero to download CRL.
		LIB3270_SSL_STATE				  state;
		int 							  error;
		const LIB3270_SSL_MESSAGE		* message;					///< @brief Pointer to SSL messages for current state.
		unsigned short					  crl_preferred_protocol;	///< @brief The CRL Preferred protocol.
	} ssl;

	/// @brief Event Listeners.
	struct {
		/// @brief State listeners.
		struct lib3270_linked_list_head state[LIB3270_STATE_USER];

		/// @brief Toggle listeners.
		struct lib3270_linked_list_head toggle[LIB3270_TOGGLE_COUNT];

		/// @brief Action listeners.
		struct lib3270_linked_list_head actions[LIB3270_ACTION_GROUP_CUSTOM];

	} listeners;

	unsigned int tasks;

};
