/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright 1995, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006 by Paul Mattes.
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

#pragma once

#include <stdio.h>
#include <sys/time.h>
#include <private/session.h>

typedef enum _lib3270_FT_OPTION {

	LIB3270_FT_OPTION_SEND 					= 0x0000,

	LIB3270_FT_OPTION_RECEIVE				= 0x0001,
	LIB3270_FT_OPTION_ASCII					= 0x0002,		///< @brief Convert to ascii
	LIB3270_FT_OPTION_CRLF					= 0x0004,		///< @brief Add crlf to each line
	LIB3270_FT_OPTION_APPEND				= 0x0008,
	LIB3270_FT_OPTION_REMAP					= 0x0010,		///< @brief Remap ASCII<->EBCDIC
	LIB3270_FT_OPTION_UNIX					= 0x0020,		///< @brief Unix text file

	LIB3270_FT_RECORD_FORMAT_DEFAULT		= 0x0000,
	LIB3270_FT_RECORD_FORMAT_FIXED			= 0x0100,
	LIB3270_FT_RECORD_FORMAT_VARIABLE		= 0x0200,
	LIB3270_FT_RECORD_FORMAT_UNDEFINED		= 0x0300,

	LIB3270_FT_ALLOCATION_UNITS_DEFAULT		= 0x0000,
	LIB3270_FT_ALLOCATION_UNITS_TRACKS		= 0x1000,
	LIB3270_FT_ALLOCATION_UNITS_CYLINDERS	= 0x2000,
	LIB3270_FT_ALLOCATION_UNITS_AVBLOCK		= 0x3000

} LIB3270_FT_OPTION;

#define LIB3270_FT_ALLOCATION_UNITS_MASK		LIB3270_FT_ALLOCATION_UNITS_AVBLOCK
#define LIB3270_FT_RECORD_FORMAT_MASK 			LIB3270_FT_RECORD_FORMAT_UNDEFINED

typedef enum _lib3270_ft_value {

	LIB3270_FT_VALUE_LRECL,
	LIB3270_FT_VALUE_BLKSIZE,
	LIB3270_FT_VALUE_PRIMSPACE,
	LIB3270_FT_VALUE_SECSPACE,
	LIB3270_FT_VALUE_DFT,

	LIB3270_FT_VALUE_COUNT

} LIB3270_FT_VALUE;

typedef enum _lib3270_ft_state {
	LIB3270_FT_STATE_NONE,			/**< No transfer in progress */
	LIB3270_FT_STATE_AWAIT_ACK,		/**< IND$FILE sent, awaiting acknowledgement message */
	LIB3270_FT_STATE_RUNNING,		/**< Ack received, data flowing */
	LIB3270_FT_STATE_ABORT_WAIT,	/**< Awaiting chance to send an abort */
	LIB3270_FT_STATE_ABORT_SENT		/**< Abort sent; awaiting response */
} LIB3270_FT_STATE;

#define LIB3270_XLATE_NBUF	4

struct lib3270_ft_callbacks {
	void (*complete)(H3270 *hSession, unsigned long length,double kbytes_sec,const char *msg, void *userdata);
	void (*failed)(H3270 *hSession, unsigned long length,double kbytes_sec,const char *msg, void *userdata);
	void (*message)(H3270 *hSession, const char *msg, void *userdata);
	void (*update)(H3270 *hSession, unsigned long current, unsigned long length, double kbytes_sec, void *userdata);
	void (*running)(H3270 *hSession, int is_cut, void *userdata);
	void (*aborting)(H3270 *hSession, const char *reason, void *userdata);
	void (*state_changed)(H3270 *hSession, LIB3270_FT_STATE state, const char *text, void *userdata);
};

/**
 * @brief File transfer data.
 *
 */
struct _h3270ft {
	
	struct lib3270_ft_callbacks	  cbk;				///< @brief Callback table - Always the first one.

	unsigned int				  ft_last_cr	: 1;	///< @brief CR was last char in local file
	unsigned int 				  remap_flag	: 1;	///< @brief Remap ASCII<->EBCDIC
	unsigned int				  cr_flag		: 1;	///< @brief Add crlf to each line
	unsigned int				  unix_text		: 1;	///< @brief Following the convention for UNIX text files.
	unsigned int				  message_flag	: 1;	///< @brief Open Request for msg received
	unsigned int				  ascii_flag	: 1;	///< @brief Convert to ascii
	unsigned int				  ft_is_cut		: 1;	///< @brief File transfer is CUT-style
	unsigned int				  dft_eof		: 1;


	H3270					* host;
	void					* user_data;			///< @brief File transfer dialog handle
	FILE 					* local_file;			///< @brief File descriptor for local file
	unsigned long			  length;				///< @brief File length

	LIB3270_FT_STATE		  state;
	LIB3270_FT_OPTION		  flags;

	int 					  lrecl;
	int 					  blksize;
	int						  primspace;
	int						  secspace;
	int						  dft;

	unsigned long			  ft_length;			///< Length of transfer

	struct timeval			  starting_time;		///< Starting time

	const char 				* local;				///< Local filename
	const char				* remote;				///< Remote filename

	// ft_dft.c
	char 					* abort_string;
	unsigned long			  recnum;
	unsigned char			* dft_savebuf;
	int						  dft_savebuf_len;
	int						  dft_savebuf_max;

	// ft_cut.c
	int						  quadrant;
	unsigned long			  expanded_length;
	char					* saved_errmsg;
	int						  xlate_buffered;					///< buffer count
	int						  xlate_buf_ix;						///< buffer index
	unsigned char			  xlate_buf[LIB3270_XLATE_NBUF];	///< buffer

	// Charset
	struct {
		char			* host;
		char			* display;
		unsigned long	  cgcsgid;

		// Translation tables
		unsigned short		  ebc2asc[256];
		unsigned short 		  asc2ebc[256];
		// unsigned short		  asc2uc[256];

	} charset

};

typedef struct _lib3270_ft_message {
	const char		* id;					///< @brief Message ID.
	unsigned char	  failed;				///< @brief Non zero if this message indicates a failure.
	const char		* message;				///< @brief Message text.
	const char 		* description;			///< @brief Message description.
} LIB3270_FT_MESSAGE;

/**
 * @brief Create a new file transfer session.
 *
 * @param hSession
 * @param flags
 * @param local
 * @param remote
 * @param lrecl
 * @param blksize
 * @param primspace
 * @param secspace
 * @param dft
 * @param msg		Pointer to receive message text.
 *
 * @return Filetransfer session handle.
  *
 */
LIB3270_EXPORT H3270FT						* lib3270_ft_new(H3270 *hSession, LIB3270_FT_OPTION flags, const char *local, const char *remote, int lrecl, int blksize, int primspace, int secspace, int dft, const char **msg);

LIB3270_EXPORT int							  lib3270_ft_start(H3270 *hSession);
LIB3270_EXPORT int							  lib3270_ft_destroy(H3270 *hSession, const char *reason);

LIB3270_EXPORT int							  lib3270_ft_cancel(H3270 *hSession, int force, const char *reason);

LIB3270_EXPORT void							  lib3270_ft_set_user_data(H3270 *h, void *ptr);
LIB3270_EXPORT void						 	* lib3270_ft_get_user_data(H3270 *h);

LIB3270_EXPORT LIB3270_FT_STATE				  lib3270_get_ft_state(H3270 *session);

LIB3270_EXPORT struct lib3270_ft_callbacks	* lib3270_get_ft_callbacks(H3270 *session, unsigned short sz);

LIB3270_EXPORT int							  lib3270_ft_set_lrecl(H3270 *hSession, int lrecl);
LIB3270_EXPORT int							  lib3270_ft_set_blksize(H3270 *hSession, int blksize);
LIB3270_EXPORT int							  lib3270_ft_set_primspace(H3270 *hSession, int primspace);
LIB3270_EXPORT int							  lib3270_ft_set_secspace(H3270 *hSession, int secspace);

/**
 * @brief Update the buffersize for generating a Query Reply.
 */
LIB3270_EXPORT int							  lib3270_set_dft_buffersize(H3270 *hSession, int dft_buffersize);

LIB3270_EXPORT int							  lib3270_ft_set_options(H3270 *hSession, LIB3270_FT_OPTION options);

/**
 * @brief Send file.
 *
 * @param from	Origin filename
 * @param to	Destination filename
 * @param args	Null terminated file transfer options.
 *
 * @return 0 if ok, error code if not.
 *
 * @retval	EBUSY	File transfer is already active.
 *
 */
LIB3270_EXPORT int							  lib3270_send(H3270 *hSession, const char *from, const char *to, const char **args);

/**
 * @brief Receive file.
 *
 * @param from	Origin filename
 * @param to	Destination filename
 * @param args	Null terminated file transfer options.
 *
 * @return 0 if ok, error code if not.
 *
 * @retval	EBUSY	File transfer is already active.
 *
 */
LIB3270_EXPORT int							  lib3270_receive(H3270 *hSession, const char *from, const char *to, const char **args);

/**
 * @brief Set all FT callbacks to default valides.
 *
 * @param hSession	TN3270 session to reset.
 *
 * @return 0 if hSession has a valid FT session, non zero if not (sets errno).
 *
 */
LIB3270_EXPORT int							  lib3270_reset_ft_callbacks(H3270 *hSession);

/**
 * @brief Translate IND$FILE response.
 *
 * @param msg	Message received.
 *
 * @return Structure with message description and type.
 *
 */
LIB3270_EXPORT const LIB3270_FT_MESSAGE * lib3270_translate_ft_message(const char *msg);
