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
#include <lib3270/log.h>
#include <lib3270/selection.h>

/*---[ Implement ]------------------------------------------------------------------------------------------------------------*/

 static int paste_file(H3270 *hSession)
 {
 	return lib3270_load(hSession,NULL);
 }

 static int connect_host(H3270 *hSession)
 {
	return lib3270_reconnect(hSession,0);
 }

 static int select_up(H3270 *hSession)
 {
 	return lib3270_move_cursor(hSession,LIB3270_DIR_UP,1);
 }

 static int select_down(H3270 *hSession)
 {
 	return lib3270_move_cursor(hSession,LIB3270_DIR_DOWN,1);
 }

 static int select_left(H3270 *hSession)
 {
 	return lib3270_move_cursor(hSession,LIB3270_DIR_LEFT,1);
 }

 static int select_right(H3270 *hSession)
 {
 	return lib3270_move_cursor(hSession,LIB3270_DIR_RIGHT,1);
 }

 static int selection_up(H3270 *hSession)
 {
 	return lib3270_move_selection(hSession,LIB3270_DIR_UP);
 }

 static int selection_down(H3270 *hSession)
 {
 	return lib3270_move_selection(hSession,LIB3270_DIR_DOWN);
 }

 static int selection_left(H3270 *hSession)
 {
 	return lib3270_move_selection(hSession,LIB3270_DIR_LEFT);
 }

 static int selection_right(H3270 *hSession)
 {
 	return lib3270_move_selection(hSession,LIB3270_DIR_RIGHT);
 }

 static int pa1(H3270 *hSession)
 {
 	return lib3270_pakey(hSession,1);
 }

 static int pa2(H3270 *hSession)
 {
 	return lib3270_pakey(hSession,1);
 }

 static int pa3(H3270 *hSession)
 {
 	return lib3270_pakey(hSession,1);
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

			.keys = NULL,
			.icon = "gtk-connect",
			.label = N_( "_Reconnect" ) ,
			.summary = N_( "Reconnect to the same host" ),
			.activate = connect_host,

			.group = LIB3270_ACTION_GROUP_OFFLINE,
			.activatable = lib3270_allow_reconnect
		},

		{
			.name = "disconnect",
			.type = LIB3270_ACTION_TYPE_NETWORK,

			.keys = NULL,
			.icon = "gtk-disconnect",
			.label = N_( "_Disconnect" ),
			.summary = N_( "Disconnect from host" ),
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

			.keys = "Up,KP_Up",
			.icon = "go-up",
			.label = N_("Up"),
			.summary = N_( "Cursor up 1 position" ),
			.activate = lib3270_cursor_up,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "down",
			.type = LIB3270_ACTION_TYPE_NAVIGATION,

			.keys = "Down,KP_Down",
			.icon = "go-down",
			.label = N_("Down"),
			.summary = N_( "Cursor down 1 position" ),
			.activate = lib3270_cursor_down,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "left",
			.type = LIB3270_ACTION_TYPE_NAVIGATION,

			.keys = "Left,KP_Left",
			.icon = "go-previous",
			.label = N_("Left"),
			.summary = N_( "Cursor left 1 position" ),
			.activate = lib3270_cursor_left,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "right",
			.type = LIB3270_ACTION_TYPE_NAVIGATION,

			.keys = "Right,KP_Right",
			.icon = "go-next",
			.label = N_("Right"),
			.summary = N_( "Cursor right 1 position" ),
			.activate = lib3270_cursor_right,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "newline",
			.type = LIB3270_ACTION_TYPE_NAVIGATION,

			.keys = "Control_R",
			.icon = NULL,
			.label = N_("New line"),
			.summary = N_( "Cursor to first field on next line or any lines after that" ),
			.activate = lib3270_newline,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "previous-word",
			.type = LIB3270_ACTION_TYPE_NAVIGATION,

			.keys = NULL,
			.icon = NULL,
			.label = N_("Previous word"),
			.summary = N_( "Cursor to previous word" ),
			.activate = lib3270_previousword,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "next-word",
			.type = LIB3270_ACTION_TYPE_NAVIGATION,

			.keys = "<Primary>Right",
			.icon = NULL,
			.label = N_("Next word"),
			.summary = N_( "Cursor to next unprotected word" ),
			.activate = lib3270_nextword,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "paste-from-file",
			.type = LIB3270_ACTION_TYPE_FILE,

			.keys = NULL,
			.icon = "document-load",
			.label = N_("Paste from file"),
			.summary = N_( "Paste from text file" ),
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

			.keys = "<Primary>a",
			.icon = "edit-select-all",
			.label = N_( "Select all" ),
			.summary = N_( "Select all" ),
			.activate = lib3270_select_all,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "unselect",
			.type = LIB3270_ACTION_TYPE_SELECTION,

			.keys = "<Primary>u",
			.icon = NULL,
			.label = N_( "Remove selection" ),
			.summary = N_( "Remove selection" ),
			.activate = lib3270_unselect,

			.group = LIB3270_ACTION_GROUP_SELECTION,
			.activatable = lib3270_get_has_selection
		},

		{
			.name = "reselect",
			.type = LIB3270_ACTION_TYPE_SELECTION,

			.keys = "<Primary>r",
			.icon = NULL,
			.label = N_( "Reselect" ),
			.summary = N_( "Reselect"),
			.activate = lib3270_reselect,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "select-word",
			.type = LIB3270_ACTION_TYPE_SELECTION,

			.keys = NULL,
			.icon = NULL,
			.label = N_( "Select word" ),
			.summary = N_( "Select word" ),
			.activate = lib3270_select_word,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "select-up",
			.type = LIB3270_ACTION_TYPE_SELECTION,

			.keys = "<Shift>Up",
			.icon = NULL,
			.label = N_( "Select UP" ),
			.summary = N_( "Move cursor up and select" ),
			.activate = select_up,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "select-down",
			.type = LIB3270_ACTION_TYPE_SELECTION,

			.keys = "<Shift>Down",
			.icon = NULL,
			.label = N_( "Select Down" ),
			.summary = N_( "Move cursor down and select" ),
			.activate = select_down,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "select-left",
			.type = LIB3270_ACTION_TYPE_SELECTION,

			.keys = "<Shift>Left",
			.icon = NULL,
			.label = N_( "Select Left" ),
			.summary = N_( "Move cursor left and select" ),
			.activate = select_left,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "select-right",
			.type = LIB3270_ACTION_TYPE_SELECTION,

			.keys = "<Shift>Right",
			.icon = NULL,
			.label = N_( "Select right" ),
			.summary = N_( "Move cursor rigth and select" ),
			.activate = select_right,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "selection-up",
			.type = LIB3270_ACTION_TYPE_SELECTION,

			.keys = "<Alt>Up",
			.icon = NULL,
			.label = N_( "Selection up" ),
			.summary = N_( "Move selection up" ),
			.activate = selection_up,

			.group = LIB3270_ACTION_GROUP_SELECTION,
			.activatable = lib3270_get_has_selection
		},

		{
			.name = "selection-down",
			.type = LIB3270_ACTION_TYPE_SELECTION,

			.keys = "<Alt>Down",
			.icon = NULL,
			.label = N_( "Selection down" ),
			.summary = N_( "Move selection down" ),
			.activate = selection_down,

			.group = LIB3270_ACTION_GROUP_SELECTION,
			.activatable = lib3270_get_has_selection
		},

		{
			.name = "selection-left",
			.type = LIB3270_ACTION_TYPE_SELECTION,

			.keys = "<Alt>Left",
			.icon = NULL,
			.label = N_( "Selection left" ),
			.summary = N_( "Move selection left" ),
			.activate = selection_left,

			.group = LIB3270_ACTION_GROUP_SELECTION,
			.activatable = lib3270_get_has_selection
		},

		{
			.name = "selection-right",
			.type = LIB3270_ACTION_TYPE_SELECTION,

			.keys = "<Alt>Right",
			.icon = NULL,
			.label = N_( "Selection right" ),
			.summary = N_( "Move selection right" ),
			.activate = selection_right,
			.activatable = lib3270_get_has_selection

		},

		//
		// Field actions.
		//
		{
			.name = "select-field",
			.type = LIB3270_ACTION_TYPE_SELECTION,

			.keys = "<Primary>f",
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

			.keys = NULL,
			.icon = NULL,
			.label = N_("Field end"),
			.summary = N_( "Move the cursor to the first blank after the last non blank in the field" ),
			.activate = lib3270_fieldend,

			.group = LIB3270_ACTION_GROUP_FORMATTED,
			.activatable = lib3270_is_formatted
		},

		{
			.name = "first-field",
			.type = LIB3270_ACTION_TYPE_NAVIGATION,

			.keys = "Home",
			.icon = "go-first",
			.label = N_("First field"),
			.summary = N_( "Move to first unprotected field on screen" ),
			.activate = lib3270_firstfield,

			.group = LIB3270_ACTION_GROUP_FORMATTED,
			.activatable = lib3270_is_formatted
		},

		{
			.name = "next-field",
			.type = LIB3270_ACTION_TYPE_NAVIGATION,

			.keys = "Tab",
			.icon = "go-next",
			.label = N_("Next field"),
			.summary = N_( "Move to the next unprotected field on screen" ),
			.activate = lib3270_nextfield,

			.group = LIB3270_ACTION_GROUP_FORMATTED,
			.activatable = lib3270_is_formatted
		},

		{
			.name = "previous-field",
			.type = LIB3270_ACTION_TYPE_NAVIGATION,

			.keys = "<Shift>ISO_Left_Tab",
			.icon = "go-previous",
			.label = N_("Previous field"),
			.summary = N_( "Move to the previous unprotected field on screen" ),
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

			.keys = "<Primary>w",
			.icon = NULL,
			.label = N_("Delete word"),
			.summary = N_( "Backspaces the cursor until it hits the front of a word" ),
			.activate = lib3270_deleteword,

			.group = LIB3270_ACTION_GROUP_LOCK_STATE,
			.activatable = lib3270_is_unlocked
		},

		{
			.name = "delete-field",
			.type = LIB3270_ACTION_TYPE_NAVIGATION,

			.keys = NULL,
			.icon = NULL,
			.label = N_( "Delete field" ),
			.summary = N_( "Delete field" ),
			.activate = lib3270_deletefield,

			.group = LIB3270_ACTION_GROUP_FORMATTED,
			.activatable = lib3270_is_formatted
		},


		{
			.name = "erase-input",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.keys = NULL,
			.icon = "edit-clear",
			.label = N_("Erase input"),
			.summary = N_("Erase all unprotected fields"),
			.activate = lib3270_eraseinput,

			.group = LIB3270_ACTION_GROUP_FORMATTED,
			.activatable = lib3270_is_connected
		},

		{
			.name = "erase-eof",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.keys = "End",
			.icon = NULL,
			.label = N_("Erase EOF"),
			.summary = N_( "Erase End Of Field" ),
			.activate = lib3270_eraseeof,

			.group = LIB3270_ACTION_GROUP_FORMATTED,
			.activatable = lib3270_is_formatted
		},

		{
			.name = "erase-eol",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.keys = NULL,
			.icon = NULL,
			.label = N_("Erase EOL"),
			.summary = N_( "Erase End Of Line" ),
			.activate = lib3270_eraseeol,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "erase",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.keys = "BackSpace",
			.icon = NULL,
			.label = N_("Erase"),
			.summary = N_("Erase"),
			.activate = lib3270_erase,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "clear",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.keys = NULL,
			.icon = "edit-clear-all",
			.label = N_("Clear"),
			.summary = N_( "Clear AID key" ),
			.activate = lib3270_clear,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		//
		// Keyboard actions
		//
		{
			.name = "enter",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.keys = NULL,
			.icon = "gtk-ok",
			.label = N_("Enter"),
			.summary = N_( "Send an \"Enter\" action" ),
			.activate = lib3270_enter,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},


		{
			.name = "kybdreset",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.keys = "Escape",
			.icon = NULL,
			.label = N_("Reset"),
			.summary = N_("Reset keyboard lock"),
			.activate = lib3270_kybdreset,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},


		{
			.name = "delete",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.keys = "Delete",
			.icon = NULL,
			.label = N_("Delete"),
			.summary = N_("Delete"),
			.activate = lib3270_delete,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "dup",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.keys = "<Shift>KP_Multiply",
			.icon = NULL,
			.label = N_("Dup"),
			.summary = N_( "DUP key" ),
			.activate = lib3270_dup,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "fieldmark",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.keys = "<Shift>Home",
			.icon = NULL,
			.label = N_("FM key"),
			.summary = N_( "Field Mark" ),
			.activate = lib3270_fieldmark,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "backspace",
			.type = LIB3270_ACTION_TYPE_NAVIGATION,

			.keys = NULL,
			.icon = NULL,
			.label = N_("Back space"),
			.summary = N_( "3270-style backspace" ),
			.activate = lib3270_backspace,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "attn",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.keys = "<shift>Escape",
			.icon = NULL,
			.label = N_("Attn"),
			.summary = N_( "ATTN key, per RFC 2355. Sends IP, regardless" ),
			.activate = lib3270_attn,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "break",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.keys = NULL,
			.icon = NULL,
			.label = N_("Break"),
			.summary = N_("Break"),
			.activate = lib3270_break,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "paste-next",
			.type = LIB3270_ACTION_TYPE_SELECTION,

			.keys = "<shift><alt>v",
			.icon = "edit-paste",
			.label = N_("Paste next"),
			.summary = N_("Paste next"),
			.activate = lib3270_paste_next,

			.group = LIB3270_ACTION_GROUP_LOCK_STATE,
			.activatable = lib3270_is_unlocked
		},

		{
			.name = "sysreq",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.keys = "<shift>Print",
			.icon = NULL,
			.label = N_("Sys Req"),
			.summary = N_("Sys Request"),
			.activate = lib3270_sysreq,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		//
		// Test actions
		//

		{
			.name = "testpattern",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.keys = NULL,
			.icon = NULL,
			.label = N_("Test pattern"),
			.summary = N_("Show test pattern"),
			.activate = lib3270_testpattern,

			.group = LIB3270_ACTION_GROUP_OFFLINE,
			.activatable = lib3270_is_disconnected
		},

		{
			.name = "charsettable",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.keys = NULL,
			.icon = NULL,
			.label = N_("Charset table"),
			.summary = N_("Show charset table"),
			.activate = lib3270_charsettable,

			.group = LIB3270_ACTION_GROUP_OFFLINE,
			.activatable = lib3270_is_disconnected
		},

		//
		// Misc Actions
		//
		{
			.name = "PA1",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.keys = "<Alt>1",
			.icon = NULL,
			.label = N_("PA1"),
			.summary = N_( "Program Action 1" ),
			.activate = pa1,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

			{
			.name = "PA2",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.keys = "<Alt>2",
			.icon = NULL,
			.label = N_("PA1"),
			.summary = N_( "Program Action 2" ),
			.activate = pa2,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
		},

		{
			.name = "PA3",
			.type = LIB3270_ACTION_TYPE_GENERIC,

			.keys = "<Alt>3",
			.icon = NULL,
			.label = N_("PA1"),
			.summary = N_( "Program Action 3" ),
			.activate = pa3,

			.group = LIB3270_ACTION_GROUP_ONLINE,
			.activatable = lib3270_is_connected
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

 LIB3270_EXPORT int lib3270_action_group_get_activatable(const H3270 *hSession, const LIB3270_ACTION_GROUP group) {

	static const struct {
		int (*get)(const H3270 *);
	} activatable[LIB3270_ACTION_GROUP_CUSTOM] = {
		{ default_activatable_state		},	// LIB3270_ACTION_GROUP_NONE
 		{ lib3270_is_connected			},	// LIB3270_ACTION_GROUP_ONLINE
 		{ lib3270_is_disconnected		},	// LIB3270_ACTION_GROUP_OFFLINE
 		{ lib3270_get_has_selection		},	// LIB3270_ACTION_GROUP_SELECTION
 		{ lib3270_is_unlocked			},	// LIB3270_ACTION_GROUP_LOCK_STATE
 		{ lib3270_is_formatted			},	// LIB3270_ACTION_GROUP_FORMATTED
 		{ lib3270_get_has_copy			},	// LIB3270_ACTION_GROUP_COPY
 	};

 	if(group < (sizeof(activatable)/sizeof(activatable[0]))) {
		return activatable[group].get(hSession);
 	}

 	return default_activatable_state(hSession);

 }
