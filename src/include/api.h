/*
 * "Software PW3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
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
 * Este programa está nomeado como api.h e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas de Mendonça)
 * licinio@bb.com.br		(Licínio Luis Branco)
 * kraucer@bb.com.br		(Kraucer Fernandes Mazuco)
 *
 */


#ifndef LIB3270_API_INCLUDED

#ifdef WIN32
	#include <winsock2.h>
	#include <windows.h>
#endif // WIN32

#ifdef __cplusplus
	extern "C" {
#endif

		#define LIB3270_API_INCLUDED "4.2"

		#include <errno.h>

		#if defined(_WIN32)
			#include <windows.h>

			#if defined (HAVE_GNUC_VISIBILITY)
				#define LOCAL_EXTERN	__attribute__((visibility("hidden"))) extern
			#else
				#define LOCAL_EXTERN extern
			#endif

		#else
			#include <stdarg.h>

			// http://gcc.gnu.org/wiki/Visibility
			#if defined(__SUNPRO_C) && (__SUNPRO_C >= 0x550)
				#define LOCAL_EXTERN __hidden extern
			#elif defined (HAVE_GNUC_VISIBILITY)
				#define LOCAL_EXTERN	__attribute__((visibility("hidden"))) extern
			#else
				#define LOCAL_EXTERN extern
			#endif


		#endif

		#ifndef HCONSOLE
			#define HCONSOLE void *
		#endif

		#ifndef ETIMEDOUT
			#define ETIMEDOUT -1238
		#endif

		#ifndef ECANCELED
			#ifdef EINTR
				#define ECANCELED EINTR
			#else
				#define ECANCELED -1125
			#endif
		#endif

		#ifndef ENOTCONN
			#define ENOTCONN -1107
		#endif

		#ifndef EISCONN
			#define EISCONN -1106
		#endif

		#ifndef CN
			#define CN ((char *) NULL)
		#endif

		#include <lib3270/log.h>

		#define OIA_FLAG_BOXSOLID	LIB3270_FLAG_BOXSOLID
		#define OIA_FLAG_UNDERA		LIB3270_FLAG_UNDERA
		#define OIA_FLAG_TYPEAHEAD	LIB3270_FLAG_TYPEAHEAD
		#define OIA_FLAG_USER		LIB3270_FLAG_COUNT
		#define OIA_FLAG			LIB3270_FLAG

//		struct lib3270_state_callback;

		#include <lib3270/session.h>


		/** Type of dialog boxes */
		#include <lib3270/popup.h>

		#define PW3270_DIALOG_INFO		LIB3270_NOTIFY_INFO
		#define PW3270_DIALOG_CRITICAL	LIB3270_NOTIFY_CRITICAL
		#define PW3270_DIALOG			LIB3270_NOTIFY

		#define GR_BLINK		0x01
		#define GR_REVERSE		0x02
		#define GR_UNDERLINE	0x04
		#define GR_INTENSIFY	0x08

		#define CS_MASK			0x03	/**< mask for specific character sets */
		#define CS_BASE			0x00	/**< base character set (X'00') */
		#define CS_APL			0x01	/**< APL character set (X'01' or GE) */
		#define CS_LINEDRAW		0x02	/**< DEC line-drawing character set (ANSI) */
		#define CS_DBCS			0x03	/**< DBCS character set (X'F8') */
		#define CS_GE			0x04	/**< cs flag for Graphic Escape */

		/* File transfer */

		#define FT_RECORD_FORMAT_FIXED			LIB3270_FT_RECORD_FORMAT_FIXED
		#define FT_RECORD_FORMAT_VARIABLE		LIB3270_FT_RECORD_FORMAT_VARIABLE
		#define FT_RECORD_FORMAT_UNDEFINED		LIB3270_FT_RECORD_FORMAT_UNDEFINED
		#define FT_RECORD_FORMAT_MASK 			LIB3270_FT_RECORD_FORMAT_MASK

		#define FT_ALLOCATION_UNITS_TRACKS		LIB3270_FT_ALLOCATION_UNITS_TRACKS
		#define FT_ALLOCATION_UNITS_CYLINDERS	LIB3270_FT_ALLOCATION_UNITS_CYLINDERS
		#define FT_ALLOCATION_UNITS_AVBLOCK		LIB3270_FT_ALLOCATION_UNITS_AVBLOCK
		#define FT_ALLOCATION_UNITS_MASK		LIB3270_FT_ALLOCATION_UNITS_MASK

		#define FT_NONE							LIB3270_FT_STATE_NONE
		#define FT_AWAIT_ACK					LIB3270_FT_STATE_AWAIT_ACK
		#define FT_RUNNING						LIB3270_FT_STATE_RUNNING
		#define FT_ABORT_WAIT					LIB3270_FT_STATE_ABORT_WAIT
		#define FT_ABORT_SENT					LIB3270_FT_STATE_ABORT_SENT

		LOCAL_EXTERN int 				CancelFileTransfer(int force);

		#define PCONNECTED		lib3270_pconnected(hSession)
		#define HALF_CONNECTED	lib3270_half_connected(hSession)
		#define CONNECTED		lib3270_connected(hSession)

		#define IN_NEITHER		lib3270_in_neither(hSession)
		#define IN_ANSI			lib3270_in_ansi(hSession)
		#define IN_3270			lib3270_in_3270(hSession)
		#define IN_SSCP			lib3270_in_sscp(hSession)
		#define IN_TN3270E		lib3270_in_tn3270e(hSession)
		#define IN_E			lib3270_in_e(hSession)

		#define COLOR_ATTR_NONE			0x0000
		#define COLOR_ATTR_FIELD		LIB3270_ATTR_FIELD
		#define COLOR_ATTR_BLINK		LIB3270_ATTR_BLINK
		#define COLOR_ATTR_INTENSIFY	LIB3270_ATTR_INTENSIFY

		#define CHAR_ATTR_UNCONVERTED	LIB3270_ATTR_CG


		/* Set/Get screen contents */
		#define find_field_attribute(s,a) lib3270_field_addr(s,a)
		#define find_field_length(s,a) find_field_length(s,a)

		LOCAL_EXTERN unsigned char get_field_attribute(H3270 *session, int baddr);
		LOCAL_EXTERN void Input_String(const unsigned char *str);
		LOCAL_EXTERN void screen_size(int *rows, int *cols);

		#define lib3270_paste_string(str) lib3270_set_string(NULL,str)

		/* Keyboard */
		LOCAL_EXTERN int			  emulate_input(char *s, int len, int pasting);

        /* Misc calls */

		#include <lib3270/actions.h>

#ifdef __cplusplus
	}
#endif

#endif // LIB3270_API_INCLUDED
