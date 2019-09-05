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
 * Este programa está nomeado como - e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

/**
 * @brief Implements the action table.
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

 static int save_selected(H3270 *hSession)
 {
 	return lib3270_save_selected(hSession,NULL);
 }

 static int save_copy(H3270 *hSession)
 {
 	return lib3270_save_copy(hSession,NULL);
 }

 static int paste_file(H3270 *hSession)
 {
 	return lib3270_load(hSession,NULL);
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
			"reconnect",
			NULL,
			"connect",
			NULL,
			N_( "Connect to host." ),
			connect_host,
			lib3270_is_disconnected
		},

		{
			"disconnect",
			NULL,
			"disconnect",
			NULL,
			N_( "Disconnect from host." ),
			lib3270_disconnect,
			lib3270_is_connected
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
			lib3270_cursor_up,
			lib3270_is_connected
		},

		{
			"down",
			"Down",
			NULL,
			NULL,
			N_( "Cursor down 1 position." ),
			lib3270_cursor_down,
			lib3270_is_connected
		},

		{
			"left",
			"Left",
			NULL,
			NULL,
			N_( "Cursor left 1 position." ),
			lib3270_cursor_left,
			lib3270_is_connected
		},

		{
			"right",
			"Right",
			NULL,
			NULL,
			N_( "Cursor right 1 position." ),
			lib3270_cursor_right,
			lib3270_is_connected
		},

		{
			"newline",
			"Control_R",
			NULL,
			NULL,
			N_( "Cursor to first field on next line or any lines after that." ),
			lib3270_newline,
			lib3270_is_connected
		},

		{
			"previousword",
			NULL,
			NULL,
			NULL,
			N_( "Cursor to previous word." ),
			lib3270_previousword,
			lib3270_is_connected
		},

		{
			"nextword",
			NULL,
			NULL,
			NULL,
			N_( "Cursor to next unprotected word." ),
			lib3270_nextword,
			lib3270_is_connected
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
			save_all,
			lib3270_is_connected
		},

		{
			"saveselected",
			NULL,
			NULL,
			NULL,
			N_( "Save selected area." ),
			save_selected,
			lib3270_has_selection
		},

		{
			"savecopy",
			NULL,
			NULL,
			NULL,
			NULL,
			save_copy,
			lib3270_is_connected
		},

		{
			"loadfile",
			NULL,
			"document-load",
			NULL,
			N_( "Paste file." ),
			paste_file,
			lib3270_is_connected
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
			lib3270_select_all,
			lib3270_is_connected
		},

		{
			"unselect",
			NULL,
			NULL,
			NULL,
			N_( "Remove selection" ),
			lib3270_unselect,
			lib3270_has_selection
		},

		{
			"reselect",
			"<Ctrl>r",
			NULL,
			NULL,
			N_( "Reselect"),
			lib3270_reselect,
			lib3270_is_connected
		},

		//
		// Field actions.
		//
		{
			"select_field",
			"<Ctrl>f",
			NULL,
			NULL,
			N_( "Select Field" ),
			lib3270_select_field,
			lib3270_is_formatted
		},


		{
			"fieldend",
			NULL,
			NULL,
			NULL,
			N_( "Move the cursor to the first blank after the last nonblank in the field." ),
			lib3270_fieldend,
			lib3270_is_formatted
		},

		{
			"firstfield",
			"Home",
			"go-first",
			NULL,
			N_( "Move to first unprotected field on screen." ),
			lib3270_firstfield,
			lib3270_is_formatted
		},

		{
			"nextfield",
			"Tab",
			"go-next",
			NULL,
			N_( "Tab forward to next field." ),
			lib3270_nextfield,
			lib3270_is_formatted
		},

		{
			"previousfield",
			"ISO_Left_Tab",
			"go-previous",
			NULL,
			N_( "Tab backward to previous field." ),
			lib3270_previousfield,
			lib3270_is_formatted
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
			lib3270_deleteword,
			lib3270_is_connected
		},

		{
			"deletefield",
			"<Ctrl>u",
			NULL,
			NULL,
			N_( "Delete field" ),
			lib3270_deletefield,
			lib3270_is_formatted
		},


		{
			"eraseinput",
			NULL,
			NULL,
			NULL,
			NULL,
			lib3270_eraseinput,
			lib3270_is_connected
		},

		{
			"eraseeof",
			"End",
			NULL,
			NULL,
			N_( "Erase End Of Field Key." ),
			lib3270_eraseeof,
			lib3270_is_formatted
		},

		{
			"eraseeol",
			NULL,
			NULL,
			NULL,
			N_( "Erase End Of Line Key." ),
			lib3270_eraseeol,
			lib3270_is_connected
		},

		{
			"erase",
			"BackSpace",
			NULL,
			NULL,
			NULL,
			lib3270_erase,
			lib3270_is_connected
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
			lib3270_enter,
			lib3270_is_connected
		},


		{
			"kybdreset",
			"Escape",
			NULL,
			NULL,
			NULL,
			lib3270_kybdreset,
			lib3270_is_connected
		},

		{
			"clear",
			NULL,
			NULL,
			NULL,
			N_( "Clear AID key" ),
			lib3270_clear,
			lib3270_is_connected
		},


		{
			"delete",
			"Delete",
			NULL,
			NULL,
			NULL,
			lib3270_delete,
			lib3270_is_connected
		},

		{
			"dup",
			"<Shift>KP_Multiply",
			NULL,
			NULL,
			N_( "DUP key" ),
			lib3270_dup,
			lib3270_is_connected
		},

		{
			"fieldmark",
			NULL,
			NULL,
			NULL,
			N_( "FM key" ),
			lib3270_fieldmark,
			lib3270_is_connected
		},

		{
			"backspace",
			NULL,
			NULL,
			NULL,
			N_( "3270-style backspace." ),
			lib3270_backspace,
			lib3270_is_connected
		},

		{
			"attn",
			"<shift>Escape",
			NULL,
			NULL,
			N_( "ATTN key, per RFC 2355.  Sends IP, regardless." ),
			lib3270_attn,
			lib3270_is_connected
		},

		{
			"break",
			NULL,
			NULL,
			NULL,
			NULL,
			lib3270_break,
			lib3270_is_connected
		},

		{
			"pastenext",
			"<shift><ctrl>v",
			NULL,
			NULL,
			NULL,
			lib3270_paste_next,
			lib3270_is_connected
		},

		{
			"sysreq",
			"<shift>Print",
			NULL,
			NULL,
			NULL,
			lib3270_sysreq,
			lib3270_is_connected
		},

		//
		// Misc actions
		//
		{
			"print",
			"Print",
			"document-print",
			NULL,
			N_("If the terminal has selected area print tje selected area, if not, print all contents."),
			lib3270_print,
			lib3270_is_connected
		},

		{
			"printall",
			NULL,
			NULL,
			NULL,
			N_("Print screen contents"),
			lib3270_print_all,
			lib3270_is_connected
		},

		{
			"printselected",
			NULL,
			NULL,
			NULL,
			N_( "Print selected area." ),
			lib3270_print_selected,
			lib3270_has_selection
		},

		{
			"printcopy",
			NULL,
			NULL,
			NULL,
			N_("Print copy (if available)"),
			lib3270_print_copy,
			lib3270_is_connected
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
			lib3270_testpattern,
			lib3270_is_disconnected
		},

		{
			"charsettable",
			NULL,
			NULL,
			NULL,
			NULL,
			lib3270_charsettable,
			lib3270_is_disconnected
		},

		{
			NULL,
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

