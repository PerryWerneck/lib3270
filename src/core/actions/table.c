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
 * programa;  se  não, escreva para a Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA, 02111-1307, USA
 *
 * Este programa está nomeado como actions.c e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

#include <lib3270-internals.h>
#include <lib3270/trace.h>
#include <lib3270/actions.h>
#include <lib3270/toggle.h>

/*---[ Implement ]------------------------------------------------------------------------------------------------------------*/

 static int save_all(H3270 *hSession)
 {
 	return lib3270_save_all(hSession,NULL);
 }

 static int paste_file(H3270 *hSession)
 {
 	return lib3270_load(hSession,NULL);
 }

 static int print(H3270 *hSession)
 {
 	return lib3270_print(hSession);
 }

 static int connect_host(H3270 *hSession)
 {
	return lib3270_reconnect(hSession,0);
 }

/**
 * @brief Get LIB3270 action table;
 *
 */
 LIB3270_EXPORT const LIB3270_ACTION_ENTRY * lib3270_get_action_table()
 {

	static const LIB3270_ACTION_ENTRY actions[] =
	{
		//
		// Network actions
		//
		{
			"connect",
			NULL,
			"connect",
			NULL,
			N_( "Connect to host." ),
			connect_host
		},

		{
			"disconnect",
			NULL,
			"disconnect",
			NULL,
			N_( "Disconnect from host." ),
			lib3270_disconnect
		},

		//
		// Navigation actions
		//
		{
			"up",
			"Up",
			NULL,
			NULL,
			N_( "Cursor up 1 position." ),
			lib3270_cursor_up
		},

		{
			"down",
			"Down",
			NULL,
			NULL,
			N_( "Cursor down 1 position." ),
			lib3270_cursor_down
		},

		{
			"left",
			"Left",
			NULL,
			NULL,
			N_( "Cursor left 1 position." ),
			lib3270_cursor_left
		},

		{
			"right",
			"Right",
			NULL,
			NULL,
			N_( "Cursor right 1 position." ),
			lib3270_cursor_right
		},

		{
			"newline",
			"Control_R",
			NULL,
			NULL,
			N_( "Cursor to first field on next line or any lines after that." ),
			lib3270_newline
		},

		{
			"previousword",
			NULL,
			NULL,
			NULL,
			N_( "Cursor to previous word." ),
			lib3270_previousword
		},

		{
			"nextword",
			NULL,
			NULL,
			NULL,
			N_( "Cursor to next unprotected word." ),
			lib3270_nextword
		},

		//
		// Save/load actions
		//
		{
			"saveall",
			NULL,
			"document-save",
			NULL,
			N_( "Save screen." ),
			save_all
		},

		{
			"loadfile",
			NULL,
			"document-load",
			NULL,
			N_( "Paste file." ),
			paste_file
		},

		//
		// Selection actions
		//
		{
			"select_all",
			"<ctrl>a",
			"edit-select-all",
			NULL,
			NULL,
			lib3270_select_all
		},

		{
			"select_field",
			"<Ctrl>f",
			NULL,
			NULL,
			N_( "Select Field" ),
			lib3270_select_field
		},

		{
			"unselect",
			NULL,
			NULL,
			NULL,
			N_( "Remove selection" ),
			lib3270_unselect
		},

		{
			"reselect",
			"<Ctrl>r",
			NULL,
			NULL,
			N_( "Reselect"),
			lib3270_reselect
		},


		//
		// Field actions.
		//
		{
			"fieldend",
			NULL,
			NULL,
			NULL,
			N_( "Move the cursor to the first blank after the last nonblank in the field." ),
			lib3270_fieldend
		},

		{
			"firstfield",
			"Home",
			"go-first",
			NULL,
			N_( "Move to first unprotected field on screen." ),
			lib3270_firstfield
		},

		{
			"nextfield",
			"Tab",
			"go-next",
			NULL,
			N_( "Tab forward to next field." ),
			lib3270_nextfield
		},

		{
			"previousfield",
			"ISO_Left_Tab",
			"go-previous",
			NULL,
			N_( "Tab backward to previous field." ),
			lib3270_previousfield
		},


		//
		// Erase actions.
		//
		{
			"deleteword",
			"<Ctrl>w",
			NULL,
			NULL,
			N_( "Backspaces the cursor until it hits the front of a word." ),
			lib3270_deleteword
		},

		{
			"deletefield",
			"<Ctrl>u",
			NULL,
			NULL,
			N_( "Delete field" ),
			lib3270_deletefield
		},


		{
			"eraseinput",
			NULL,
			NULL,
			NULL,
			NULL,
			lib3270_eraseinput
		},

		{
			"eraseeof",
			"End",
			NULL,
			NULL,
			N_( "Erase End Of Field Key." ),
			lib3270_eraseeof
		},

		{
			"eraseeol",
			NULL,
			NULL,
			NULL,
			N_( "Erase End Of Line Key." ),
			lib3270_eraseeol
		},

		{
			"erase",
			"BackSpace",
			NULL,
			NULL,
			NULL,
			lib3270_erase
		},

		//
		// Keyboard actions
		//
		{
			"enter",
			NULL,
			NULL,
			NULL,
			N_( "Send an \"Enter\" action." ),
			lib3270_enter
		},


		{
			"kybdreset",
			"Escape",
			NULL,
			NULL,
			NULL,
			lib3270_kybdreset
		},

		{
			"clear",
			NULL,
			NULL,
			NULL,
			N_( "Clear AID key" ),
			lib3270_clear
		},


		{
			"delete",
			"Delete",
			NULL,
			NULL,
			NULL,
			lib3270_delete
		},

		{
			"dup",
			"<Shift>KP_Multiply",
			NULL,
			NULL,
			N_( "DUP key" ),
			lib3270_dup
		},

		{
			"fieldmark",
			NULL,
			NULL,
			NULL,
			N_( "FM key" ),
			lib3270_fieldmark
		},

		{
			"backspace",
			NULL,
			NULL,
			NULL,
			N_( "3270-style backspace." ),
			lib3270_backspace
		},

		{
			"attn",
			"<shift>Escape",
			NULL,
			NULL,
			"ATTN key, per RFC 2355.  Sends IP, regardless.",
			lib3270_attn
		},

		{
			"break",
			NULL,
			NULL,
			NULL,
			NULL,
			lib3270_break
		},

		{
			"pastenext",
			"<shift><ctrl>v",
			NULL,
			NULL,
			NULL,
			lib3270_paste_next
		},

		{
			"sysreq",
			"<shift>Print",
			NULL,
			NULL,
			NULL,
			lib3270_sysreq
		},

		//
		// Misc actions
		//
		{
			"print",
			"Print",
			"document-print",
			NULL,
			NULL,
			print
		},

		//
		// Test actions
		//

		{
			"testpattern",
			NULL,
			NULL,
			NULL,
			NULL,
			lib3270_testpattern
		},

		{
			"charsettable",
			NULL,
			NULL,
			NULL,
			NULL,
			lib3270_charsettable
		},

		{
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL
		}
	};

	return actions;
 }

