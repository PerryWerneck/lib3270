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

#include <internals.h>
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
			.type = LIB3270_ACTION_TYPE_NETWORK,

			.key = NULL,
			.icon = "gtk-connect",
			.label = N_( "_Connect" ) ,
			.summary = N_( "Connect to host." ),
			.activate = connect_host,

			.group = LIB3270_ACTION_GROUP_OFFLINE,
			.activatable = lib3270_is_disconnected
		},

		{
			.name = "disconnect",
			.type = LIB3270_ACTION_TYPE_NETWORK,

			.key = NULL,
			.icon = "gtk-disconnect",
			.label = N_( "_Disconnect" ),
			.summary = N_( "Disconnect from host." ),
			.activate = lib3270_disconnect,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		//
		// Navigation actions
		//
		{
			.name = "up",
			.type = LIB3270_ACTION_TYPE_NAVIGATION,

			.key = "Up",
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Cursor up 1 position." ),
			.activate = lib3270_cursor_up,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "down",
			.type = LIB3270_ACTION_TYPE_NAVIGATION,

			.key = "Down",
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Cursor down 1 position." ),
			.activate = lib3270_cursor_down,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "left",
			.type = LIB3270_ACTION_TYPE_NAVIGATION,

			.key = "Left",
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Cursor left 1 position." ),
			.activate = lib3270_cursor_left,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "right",
			.type = LIB3270_ACTION_TYPE_NAVIGATION,

			.key = "Right",
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Cursor right 1 position." ),
			.activate = lib3270_cursor_right,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "newline",
			.type = LIB3270_ACTION_TYPE_NAVIGATION,

			.key = "Control_R",
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Cursor to first field on next line or any lines after that." ),
			.activate = lib3270_newline,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "previous-word",
			.type = LIB3270_ACTION_TYPE_NAVIGATION,

			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Cursor to previous word." ),
			.activate = lib3270_previousword,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "next-word",
			.type = LIB3270_ACTION_TYPE_NAVIGATION,

			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Cursor to next unprotected word." ),
			.activate = lib3270_nextword,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		//
		// Save/load actions
		//
		{
			.name = "save-all",
			.type = LIB3270_ACTION_TYPE_FILE,

			.key = NULL,
			.icon = "document-save",
			.label = NULL,
			.summary = N_( "Save screen." ),
			.activate = save_all,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "save-selected",
			.type = LIB3270_ACTION_TYPE_FILE,

			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Save selected area." ),
			.activate = save_selected,

			.group = LIB3270_ACTION_GROUP_SELECTION,
			.activatable = lib3270_has_selection
		},

		{
			.name = "save-copy",
			.type = LIB3270_ACTION_TYPE_FILE,

			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = NULL,
			.activate = save_copy,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "paste-from-file",
			.type = LIB3270_ACTION_TYPE_FILE,

			.key = NULL,
			.icon = "document-load",
			.label = NULL,
			.summary = N_( "Paste file." ),
			.activate = paste_file,

			.group = LIB3270_ACTION_GROUP_LOCK_STATE,
			.activatable = lib3270_is_unlocked
		},

		//
		// Selection actions
		//
		{
			.name = "select-all",
			.type = LIB3270_ACTION_TYPE_SELECTION,

			.key = "<ctrl>a",
			.icon = "edit-select-all",
			.label = N_( "Select all" ),
			.summary = NULL,
			.activate = lib3270_select_all,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "unselect",
			.type = LIB3270_ACTION_TYPE_SELECTION,

			.key = NULL,
			.icon = NULL,
			.label = N_( "Remove selection" ),
			.summary = N_( "Remove selection" ),
			.activate = lib3270_unselect,

			.group = LIB3270_ACTION_GROUP_SELECTION,
			.activatable = lib3270_has_selection
		},

		{
			.name = "reselect",
			.type = LIB3270_ACTION_TYPE_SELECTION,

			.key = "<Ctrl>r",
			.icon = NULL,
			.label = N_( "Reselect" ),
			.summary = N_( "Reselect"),
			.activate = lib3270_reselect,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		//
		// Field actions.
		//
		{
			.name = "select-field",
			.type = LIB3270_ACTION_TYPE_SELECTION,

			.key = "<Ctrl>f",
			.icon = NULL,
			.label = N_( "Select field" ),
			.summary = N_( "Select Field" ),
			.activate = lib3270_select_field,

			.group = LIB3270_ACTION_GROUP_FORMATTED,
			.activatable = lib3270_is_formatted
		},


		{
			.name = "field-end",
			.type = LIB3270_ACTION_TYPE_NAVIGATION,

			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Move the cursor to the first blank after the last nonblank in the field." ),
			.activate = lib3270_fieldend,

			.group = LIB3270_ACTION_GROUP_FORMATTED,
			.activatable = lib3270_is_formatted
		},

		{
			.name = "first-field",
			.type = LIB3270_ACTION_TYPE_NAVIGATION,

			.key = "Home",
			.icon = "go-first",
			.label = NULL,
			.summary = N_( "Move to first unprotected field on screen." ),
			.activate = lib3270_firstfield,

			.group = LIB3270_ACTION_GROUP_FORMATTED,
			.activatable = lib3270_is_formatted
		},

		{
			.name = "next-field",
			.type = LIB3270_ACTION_TYPE_NAVIGATION,

			.key = "Tab",
			.icon = "go-next",
			.label = NULL,
			.summary = N_( "Tab forward to next field." ),
			.activate = lib3270_nextfield,

			.group = LIB3270_ACTION_GROUP_FORMATTED,
			.activatable = lib3270_is_formatted
		},

		{
			.name = "previous-field",
			.type = LIB3270_ACTION_TYPE_NAVIGATION,

			.key = "ISO_Left_Tab",
			.icon = "go-previous",
			.label = NULL,
			.summary = N_( "Tab backward to previous field." ),
			.activate = lib3270_previousfield,

			.group = LIB3270_ACTION_GROUP_FORMATTED,
			.activatable = lib3270_is_formatted
		},


		//
		// Erase actions.
		//
		{
			.name = "delete-word",
			.type = LIB3270_ACTION_TYPE_NAVIGATION,

			.key = "<Ctrl>w",
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Backspaces the cursor until it hits the front of a word." ),
			.activate = lib3270_deleteword,

			.group = LIB3270_ACTION_GROUP_LOCK_STATE,
			.activatable = lib3270_is_unlocked
		},

		{
			.name = "delete-field",
			.type = LIB3270_ACTION_TYPE_NAVIGATION,

			.key = "<Ctrl>u",
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Delete field" ),
			.activate = lib3270_deletefield,

			.group = LIB3270_ACTION_GROUP_FORMATTED,
			.activatable = lib3270_is_formatted
		},


		{
			.name = "erase-input",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.key = NULL,
			.icon = "edit-clear",
			.label = N_("Erase input"),
			.summary = NULL,
			.activate = lib3270_eraseinput,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "erase-eof",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.key = "End",
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Erase End Of Field Key." ),
			.activate = lib3270_eraseeof,

			.group = LIB3270_ACTION_GROUP_FORMATTED,
			.activatable = lib3270_is_formatted
		},

		{
			.name = "erase-eol",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Erase End Of Line Key." ),
			.activate = lib3270_eraseeol,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "erase",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.key = "BackSpace",
			.icon = NULL,
			.label = NULL,
			.summary = NULL,
			.activate = lib3270_erase,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		//
		// Keyboard actions
		//
		{
			.name = "enter",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Send an \"Enter\" action." ),
			.activate = lib3270_enter,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},


		{
			.name = "kybdreset",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.key = "Escape",
			.icon = NULL,
			.label = NULL,
			.summary = NULL,
			.activate = lib3270_kybdreset,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "clear",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.key = NULL,
			.icon = NULL,
			.label = N_("Clear"),
			.summary = N_( "Clear AID key" ),
			.activate = lib3270_clear,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},


		{
			.name = "delete",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.key = "Delete",
			.icon = NULL,
			.label = NULL,
			.summary = NULL,
			.activate = lib3270_delete,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "dup",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.key = "<Shift>KP_Multiply",
			.icon = NULL,
			.label = NULL,
			.summary = N_( "DUP key" ),
			.activate = lib3270_dup,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "fieldmark",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = N_( "FM key" ),
			.activate = lib3270_fieldmark,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "backspace",
			.type = LIB3270_ACTION_TYPE_NAVIGATION,

			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = N_( "3270-style backspace." ),
			.activate = lib3270_backspace,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "attn",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.key = "<shift>Escape",
			.icon = NULL,
			.label = NULL,
			.summary = N_( "ATTN key, per RFC 2355.  Sends IP, regardless." ),
			.activate = lib3270_attn,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "break",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = NULL,
			.activate = lib3270_break,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "paste-next",
			.type = LIB3270_ACTION_TYPE_SELECTION,

			.key = "<shift><ctrl>v",
			.icon = NULL,
			.label = NULL,
			.summary = NULL,
			.activate = lib3270_paste_next,

			.group = LIB3270_ACTION_GROUP_LOCK_STATE,
			.activatable = lib3270_is_unlocked
		},

		{
			.name = "sysreq",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.key = "<shift>Print",
			.icon = NULL,
			.label = NULL,
			.summary = NULL,
			.activate = lib3270_sysreq,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		//
		// Misc actions
		//
		{
			.name = "print",
			.type = LIB3270_ACTION_TYPE_PRINTER,

			.key = "Print",
			.icon = "document-print",
			.label = N_("Print"),
			.summary  = N_("Send to print"),
			.description = N_("If the terminal has selected area print it, if not, print all contents."),
			.activate = lib3270_print,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "print-all",
			.type = LIB3270_ACTION_TYPE_PRINTER,

			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = N_("Print screen contents"),
			.activate = lib3270_print_all,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "print-selected",
			.type = LIB3270_ACTION_TYPE_PRINTER,

			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = N_( "Print selected area." ),
			.activate = lib3270_print_selected,

			.group = LIB3270_ACTION_GROUP_SELECTION,
			.activatable = lib3270_has_selection
		},

		{
			.name = "print-copy",
			.type = LIB3270_ACTION_TYPE_PRINTER,

			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = N_("Print copy (if available)"),
			.activate = lib3270_print_copy,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		//
		// Test actions
		//

		{
			.name = "testpattern",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = NULL,
			.activate = lib3270_testpattern,

			.group = LIB3270_ACTION_GROUP_OFFLINE,
			.activatable = lib3270_is_disconnected
		},

		{
			.name = "charsettable",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.key = NULL,
			.icon = NULL,
			.label = NULL,
			.summary = NULL,
			.activate = lib3270_charsettable,

			.group = LIB3270_ACTION_GROUP_OFFLINE,
			.activatable = lib3270_is_disconnected
		},

		{
			.name = NULL,
		}
	};

	return actions;
 }

 static int default_activatable_state(const H3270 *hSession)
 {
 	return hSession == NULL ? 0 : 1;
 }

 LIB3270_EXPORT int lib3270_action_group_get_activatable(const H3270 *hSession, const LIB3270_ACTION_GROUP group)
 {
	static const struct
	{
		int (*get)(const H3270 *);
	} activatable[LIB3270_ACTION_CUSTOM] =
	{
		{ default_activatable_state	},	// LIB3270_ACTION_GROUP_NONE
 		{ lib3270_is_connected		},	// LIB3270_ACTION_GROUP_ONLINE
 		{ lib3270_is_disconnected	},	// LIB3270_ACTION_GROUP_OFFLINE
 		{ lib3270_has_selection		},	// LIB3270_ACTION_GROUP_SELECTION
 		{ lib3270_is_unlocked		},	// LIB3270_ACTION_GROUP_LOCK_STATE
 		{ lib3270_is_formatted		},	// LIB3270_ACTION_GROUP_FORMATTED
 	};

 	if(group < (sizeof(activatable)/sizeof(activatable[0]))) {
		return activatable[group].get(hSession);
 	}

 	return default_activatable_state(hSession);

 }
