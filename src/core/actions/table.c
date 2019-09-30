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
 const LIB3270_ACTION * lib3270_get_actions()
 {

	static const LIB3270_ACTION actions[] =
	{
		//
		// Network actions
		//
		{
			.name = "reconnect",
			.key = NULL,
			.icon = "connect",
			.label = NULL,
			.summary = N_( "Connect to host." ),
			.activate = connect_host,
			.activatable = lib3270_is_disconnected
		},

		{
			.name = "disconnect",
			.key = NULL,
			.icon = "disconnect",
			.label = NULL,
			.summary = N_( "Disconnect from host." ),
			.activate = lib3270_disconnect,
			.activatable = lib3270_is_connected
		},

		//
		// Navigation actions
		//
		{
			.name = "up",
			.key = "Up",
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Cursor up 1 position." ),
			.activate = lib3270_cursor_up,
			.activatable = lib3270_is_connected
		},

		{
			.name = "down",
			.key = "Down",
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Cursor down 1 position." ),
			.activate = lib3270_cursor_down,
			.activatable = lib3270_is_connected
		},

		{
			.name = "left",
			.key = "Left",
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Cursor left 1 position." ),
			.activate = lib3270_cursor_left,
			.activatable = lib3270_is_connected
		},

		{
			.name = "right",
			.key = "Right",
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Cursor right 1 position." ),
			.activate = lib3270_cursor_right,
			.activatable = lib3270_is_connected
		},

		{
			.name = "newline",
			.key = "Control_R",
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Cursor to first field on next line or any lines after that." ),
			.activate = lib3270_newline,
			.activatable = lib3270_is_connected
		},

		{
			.name = "previousword",
			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Cursor to previous word." ),
			.activate = lib3270_previousword,
			.activatable = lib3270_is_connected
		},

		{
			.name = "nextword",
			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Cursor to next unprotected word." ),
			.activate = lib3270_nextword,
			.activatable = lib3270_is_connected
		},

		//
		// Save/load actions
		//
		{
			.name = "saveall",
			.key = NULL,
			.icon = "document-save",
			.label = NULL,
			.summary = N_( "Save screen." ),
			.activate = save_all,
			.activatable = lib3270_is_connected
		},

		{
			.name = "saveselected",
			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Save selected area." ),
			.activate = save_selected,
			.activatable = lib3270_has_selection
		},

		{
			.name = "savecopy",
			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = NULL,
			.activate = save_copy,
			.activatable = lib3270_is_connected
		},

		{
			.name = "loadfile",
			.key = NULL,
			.icon = "document-load",
			.label = NULL,
			.summary = N_( "Paste file." ),
			.activate = paste_file,
			.activatable = lib3270_is_connected
		},

		//
		// Selection actions
		//
		{
			.name = "select_all",
			.key = "<ctrl>a",
			.icon = "edit-select-all",
			.label = NULL,
			.summary = NULL,
			.activate = lib3270_select_all,
			.activatable = lib3270_is_connected
		},

		{
			.name = "unselect",
			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Remove selection" ),
			.activate = lib3270_unselect,
			.activatable = lib3270_has_selection
		},

		{
			.name = "reselect",
			.key = "<Ctrl>r",
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Reselect"),
			.activate = lib3270_reselect,
			.activatable = lib3270_is_connected
		},

		//
		// Field actions.
		//
		{
			.name = "select_field",
			.key = "<Ctrl>f",
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Select Field" ),
			.activate = lib3270_select_field,
			.activatable = lib3270_is_formatted
		},


		{
			.name = "fieldend",
			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Move the cursor to the first blank after the last nonblank in the field." ),
			.activate = lib3270_fieldend,
			.activatable = lib3270_is_formatted
		},

		{
			.name = "firstfield",
			.key = "Home",
			.icon = "go-first",
			.label = NULL,
			.summary = N_( "Move to first unprotected field on screen." ),
			.activate = lib3270_firstfield,
			.activatable = lib3270_is_formatted
		},

		{
			.name = "nextfield",
			.key = "Tab",
			.icon = "go-next",
			.label = NULL,
			.summary = N_( "Tab forward to next field." ),
			.activate = lib3270_nextfield,
			.activatable = lib3270_is_formatted
		},

		{
			.name = "previousfield",
			.key = "ISO_Left_Tab",
			.icon = "go-previous",
			.label = NULL,
			.summary = N_( "Tab backward to previous field." ),
			.activate = lib3270_previousfield,
			.activatable = lib3270_is_formatted
		},


		//
		// Erase actions.
		//
		{
			.name = "deleteword",
			.key = "<Ctrl>w",
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Backspaces the cursor until it hits the front of a word." ),
			.activate = lib3270_deleteword,
			.activatable = lib3270_is_connected
		},

		{
			.name = "deletefield",
			.key = "<Ctrl>u",
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Delete field" ),
			.activate = lib3270_deletefield,
			.activatable = lib3270_is_formatted
		},


		{
			.name = "eraseinput",
			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = NULL,
			.activate = lib3270_eraseinput,
			.activatable = lib3270_is_connected
		},

		{
			.name = "eraseeof",
			.key = "End",
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Erase End Of Field Key." ),
			.activate = lib3270_eraseeof,
			.activatable = lib3270_is_formatted
		},

		{
			.name = "eraseeol",
			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Erase End Of Line Key." ),
			.activate = lib3270_eraseeol,
			.activatable = lib3270_is_connected
		},

		{
			.name = "erase",
			.key = "BackSpace",
			.icon = NULL,
			.label = NULL,
			.summary = NULL,
			.activate = lib3270_erase,
			.activatable = lib3270_is_connected
		},

		//
		// Keyboard actions
		//
		{
			.name = "enter",
			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Send an \"Enter\" action." ),
			.activate = lib3270_enter,
			.activatable = lib3270_is_connected
		},


		{
			.name = "kybdreset",
			.key = "Escape",
			.icon = NULL,
			.label = NULL,
			.summary = NULL,
			.activate = lib3270_kybdreset,
			.activatable = lib3270_is_connected
		},

		{
			.name = "clear",
			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Clear AID key" ),
			.activate = lib3270_clear,
			.activatable = lib3270_is_connected
		},


		{
			.name = "delete",
			.key = "Delete",
			.icon = NULL,
			.label = NULL,
			.summary = NULL,
			.activate = lib3270_delete,
			.activatable = lib3270_is_connected
		},

		{
			.name = "dup",
			.key = "<Shift>KP_Multiply",
			.icon = NULL,
			.label = NULL,
			.summary = N_( "DUP key" ),
			.activate = lib3270_dup,
			.activatable = lib3270_is_connected
		},

		{
			.name = "fieldmark",
			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = N_( "FM key" ),
			.activate = lib3270_fieldmark,
			.activatable = lib3270_is_connected
		},

		{
			.name = "backspace",
			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = N_( "3270-style backspace." ),
			.activate = lib3270_backspace,
			.activatable = lib3270_is_connected
		},

		{
			.name = "attn",
			.key = "<shift>Escape",
			.icon = NULL,
			.label = NULL,
			.summary = N_( "ATTN key, per RFC 2355.  Sends IP, regardless." ),
			.activate = lib3270_attn,
			.activatable = lib3270_is_connected
		},

		{
			.name = "break",
			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = NULL,
			.activate = lib3270_break,
			.activatable = lib3270_is_connected
		},

		{
			.name = "pastenext",
			.key = "<shift><ctrl>v",
			.icon = NULL,
			.label = NULL,
			.summary = NULL,
			.activate = lib3270_paste_next,
			.activatable = lib3270_is_connected
		},

		{
			.name = "sysreq",
			.key = "<shift>Print",
			.icon = NULL,
			.label = NULL,
			.summary = NULL,
			.activate = lib3270_sysreq,
			.activatable = lib3270_is_connected
		},

		//
		// Misc actions
		//
		{
			.name = "print",
			.key = "Print",
			.icon = "document-print",
			.label = NULL,
			.summary  = N_("Send to print"),
			.description = N_("If the terminal has selected area print it, if not, print all contents."),
			.activate = lib3270_print,
			.activatable = lib3270_is_connected
		},

		{
			.name = "printall",
			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = N_("Print screen contents"),
			.activate = lib3270_print_all,
			.activatable = lib3270_is_connected
		},

		{
			.name = "printselected",
			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Print selected area." ),
			.activate = lib3270_print_selected,
			.activatable = lib3270_has_selection
		},

		{
			.name = "printcopy",
			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = N_("Print copy (if available)"),
			.activate = lib3270_print_copy,
			.activatable = lib3270_is_connected
		},

		//
		// Test actions
		//

		{
			.name = "testpattern",
			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = NULL,
			.activate = lib3270_testpattern,
			.activatable = lib3270_is_disconnected
		},

		{
			.name = "charsettable",
			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = NULL,
			.activate = lib3270_charsettable,
			.activatable = lib3270_is_disconnected
		},

		{
			.name = NULL,
			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = NULL,
			.activate = NULL,
			.activatable = NULL
		}
	};

	return actions;
 }

