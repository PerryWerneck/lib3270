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
 * programa; se não, escreva para a Free Software Foundation, Inc., 51 Franklin
 * St, Fifth Floor, Boston, MA  02110-1301  USA
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
 *	@file	toggles/init.c
 *	@brief	Toggle description table.
 */

#include <config.h>
#include <lib3270-internals.h>
#include <lib3270/toggle.h>
#include "togglesc.h"

/*---[ Implement ]------------------------------------------------------------------------------------------------------------*/

const LIB3270_TOGGLE_ENTRY toggle_descriptor[LIB3270_TOGGLE_COUNT+1] =
{
	{
		.id = LIB3270_TOGGLE_MONOCASE,
		.name = "monocase",
		.def = False,
		.key = NULL,		// Default keycode
		.icon = NULL,		// Icon name
		.label = N_( "Monocase" ),
		.summary = N_( "Uppercase only" ),
		.description = N_( "If set, the terminal operates in uppercase-only mode" )
	},

	{
		.id = LIB3270_TOGGLE_CURSOR_BLINK,
		.name = "cursorblink",
		.def = True,
		.key = NULL,		// Default keycode
		.icon = NULL,		// Icon name
		.label = N_( "Blinking Cursor" ),
		.summary = N_( "Blinking Cursor" ),
		.description = N_( "If set, the cursor blinks" )
	},
	{
		.id = LIB3270_TOGGLE_SHOW_TIMING,
		.name = "showtiming",
		.def = True,
		.key = NULL,		// Default keycode
		.icon = NULL,		// Icon name
		.label = N_( "Show timer when processing" ),
		.summary = N_( "Show timer when processing" ),
		.description = N_( "If set, the time taken by the host to process an AID is displayed on the status line" )
	},
	{
		.id = LIB3270_TOGGLE_CURSOR_POS,
		.name = "cursorpos",
		.def = True,
		.key = NULL,		// Default keycode
		.icon = NULL,		// Icon name
		.label = N_( "Track Cursor" ),
		.summary = N_( "Track Cursor" ),
		.description = N_( "Display the cursor location in the OIA (the status line)" )
	},
	{
		.id = LIB3270_TOGGLE_DS_TRACE,
		.name = "dstrace",
		.def = False,
		.key = NULL,		// Default keycode
		.icon = NULL,		// Icon name
		.label = N_( "Data Stream" ),
		.summary = N_( "Trace Data Stream" ),
		.description = ""
	},
	{
		.id = LIB3270_TOGGLE_LINE_WRAP,
		.name = "linewrap",
		.def = False,
		.key = NULL,		// Default keycode
		.icon = NULL,		// Icon name
		.label = N_("Wrap around"),
		.summary = N_("Wrap around"),
		.description = N_("If set, the NVT terminal emulator automatically assumes a NEWLINE character when it reaches the end of a line.")
	},
	{
		.id = LIB3270_TOGGLE_BLANK_FILL,
		.name = "blankfill",
		.def = False,
		.key = NULL,		// Default keycode
		.icon = NULL,		// Icon name
		.label = N_( "Blank Fill" ),
		.summary = N_( "Blank Fill" ),
		.description = N_( "Automatically convert trailing blanks in a field to NULLs in order to insert a character, and will automatically convert leading NULLs to blanks so that input data is not squeezed to the left" )
	},
	{
		.id = LIB3270_TOGGLE_SCREEN_TRACE,
		.name = "screentrace",
		.def = False,
		.key = NULL,		// Default keycode
		.icon = NULL,		// Icon name
		.label = N_( "Screens" ),
		.summary = N_( "Trace screen contents" ),
		.description = ""
	},
	{
		.id = LIB3270_TOGGLE_EVENT_TRACE,
		.name = "eventtrace",
		.def = False,
		.key = NULL,		// Default keycode
		.icon = NULL,		// Icon name
		.label = N_( "Events" ),
		.summary = N_( "Trace interface and application events" ),
		.description = ""
	},
	{
		.id = LIB3270_TOGGLE_MARGINED_PASTE,
		.name = "marginedpaste",
		.def = False,
		.key = NULL,		// Default keycode
		.icon = NULL,		// Icon name
		.label = N_( "Paste with left margin" ),
		.summary = N_( "Paste with left margin" ),
		.description = N_( "If set, puts restrictions on how pasted text is placed on the screen. The position of the cursor at the time the paste operation is begun is used as a left margin. No pasted text will fill any area of the screen to the left of that position. This option is useful for pasting into certain IBM editors that use the left side of the screen for control information" )
	},
	{
		.id = LIB3270_TOGGLE_RECTANGLE_SELECT,
		.name = "rectselect",
		.def = False,
		.key = NULL,		// Default keycode
		.icon = NULL,		// Icon name
		.label = N_( "Select by rectangles" ),
		.summary = N_( "Select by rectangles" ),
		.description = N_( "If set, the terminal will always select rectangular areas of the screen. Otherwise, it selects continuous regions of the screen" )
	},
	{
		.id = LIB3270_TOGGLE_CROSSHAIR,
		.name = "crosshair",
		.def = False,
		.key = "<alt>x",		// Default keycode
		.icon = NULL,			// Icon name
		.label = N_( "Cross hair cursor" ),
		.summary = N_( "Cross hair cursor" ),
		.description = N_( "If set, the terminal will display a crosshair over the cursor: lines extending the full width and height of the screen, centered over the cursor position. This makes locating the cursor on the screen much easier" )
	},
	{
		.id = LIB3270_TOGGLE_FULL_SCREEN,
		.name = "fullscreen",
		.def = False,
		.key = "<alt>Home",		// Default keycode
		.icon = NULL,				// Icon name
		.label = N_( "Full Screen" ),
		.summary = N_( "Full Screen" ),
		.description = N_( "If set, asks to place the toplevel window in the fullscreen state" )
	},
	{
		.id = LIB3270_TOGGLE_RECONNECT,
		.name = "autoreconnect",
		.def = False,
		.key = NULL,		// Default keycode
		.icon = NULL,		// Icon name
		.label = N_( "Auto-Reconnect" ),
		.summary = N_( "Auto-Reconnect" ),
		.description = N_( "Automatically reconnect to the host if it ever disconnects" )
	},
	{
		.id = LIB3270_TOGGLE_INSERT,
		.name = "insert",
		.def = False,
		.key = "Insert",			// Default keycode
		.icon = "insert-text",		// Icon name
		.label = N_( "Insert" ),
		.summary = N_( "Set insert mode" ),
		.description = ""
	},
	{
		.id = LIB3270_TOGGLE_SMART_PASTE,
		.name = "smartpaste",
		.def = False,
		.key = NULL,		// Default keycode
		.icon = NULL,		// Icon name
		.label = N_( "Smart paste" ),
		.summary = N_( "Smart paste" ),
		.description = ""
	},
	{
		.id = LIB3270_TOGGLE_BOLD,
		.name = "bold",
		.def = False,
		.key = NULL,					// Default keycode
		.icon = "format-text-bold",		// Icon name
		.label = N_( "Bold" ),
		.summary = N_( "Bold" ),
		.description = ""
	},
	{
		.id = LIB3270_TOGGLE_KEEP_SELECTED,
		.name = "keepselected",
		.def = False,
		.key = NULL,		// Default keycode
		.icon = NULL,		// Icon name
		.label = N_( "Keep selected" ),
		.summary = N_( "Keep selected" ),
		.description = ""
	},
	{
		.id = LIB3270_TOGGLE_UNDERLINE,
		.name = "underline",
		.def = True,
		.key = NULL,							// Default keycode
		.icon = "format-text-underline",		// Icon name
		.label = N_( "Underline" ),
		.summary = N_( "Show Underline" ),
		.description = ""
	},
	{
		.id = LIB3270_TOGGLE_CONNECT_ON_STARTUP,
		.name = "autoconnect",
		.def = False,
		.key = NULL,		// Default keycode
		.icon = NULL,		// Icon name
		.label = N_( "Auto connect" ),
		.summary = N_( "Connect on startup" ),
		.description = ""
	},
	{
		.id = LIB3270_TOGGLE_KP_ALTERNATIVE,
		.name = "kpalternative",
		.def = False,
		.key = NULL,		// Default keycode
		.icon = NULL,		// Icon name
		.label = N_( "Use +/- for field navigation" ),
		.summary = N_( "Use +/- for field navigation" ),
		.description = N_( "Use the keys +/- from keypad to select editable fields" )
	},
	{
		.id = LIB3270_TOGGLE_BEEP,
		.name = "beep",
		.def = True,
		.key = NULL,		// Default keycode
		.icon = NULL,		// Icon name
		.label = N_( "Sound" ),
		.summary = N_( "Alert sound" ),
		.description = N_( "Beep on errors" )
	},
	{
		.id = LIB3270_TOGGLE_VIEW_FIELD,
		.name = "fieldattr",
		.def = False,
		.key = NULL,		// Default keycode
		.icon = NULL,		// Icon name
		.label = N_( "Show Field" ),
		.summary = N_( "Show Field attributes" ),
		.description = ""
	},
	{
		.id = LIB3270_TOGGLE_ALTSCREEN,
		.name = "altscreen",
		.def = True,
		.key = NULL,		// Default keycode
		.icon = NULL,		// Icon name
		.label = N_( "Alternate screen" ),
		.summary = N_( "Resize on alternate screen" ),
		.description = N_( "Auto resize on altscreen" )
	},
	{
		.id = LIB3270_TOGGLE_KEEP_ALIVE,
		.name = "keepalive",
		.def = True,
		.key = NULL,		// Default keycode
		.icon = NULL,		// Icon name
		.label = N_( "Network keep alive" ),
		.summary = N_( "Network keep alive" ),
		.description = N_( "Enable network keep-alive with SO_KEEPALIVE" )
	},
	{
		.id = LIB3270_TOGGLE_NETWORK_TRACE,
		.name = "nettrace",
		.def = False,
		.key = NULL,		// Default keycode
		.icon = NULL,		// Icon name
		.label = N_( "Network data" ),
		.summary = N_( "Trace network data flow" ),
		.description = N_( "Enable network in/out trace" )
	},
	{
		.id = LIB3270_TOGGLE_SSL_TRACE,
		.name = "ssltrace",
		.def = False,
		.key = NULL,		// Default keycode
		.icon = NULL,		// Icon name
		.label = N_( "SSL negotiation" ),
		.summary = N_( "Trace SSL negotiation" ),
		.description = N_( "Enable security negotiation trace" )
	},

	{
		.id = LIB3270_TOGGLE_COUNT,
		.name = NULL,
		.def = False,
		.key = NULL,		// Default keycode
		.icon = NULL,		// Icon name
		.label = NULL,
		.summary = NULL,
		.description = NULL

	}

};

LIB3270_EXPORT const LIB3270_TOGGLE_ENTRY * lib3270_get_toggle_list()
{
	return toggle_descriptor;
}

LIB3270_EXPORT const char * lib3270_get_toggle_summary(LIB3270_TOGGLE ix)
{
	if(ix < LIB3270_TOGGLE_COUNT)
		return toggle_descriptor[ix].summary;
	return "";
}

LIB3270_EXPORT const char * lib3270_get_toggle_label(LIB3270_TOGGLE ix)
{
	if(ix < LIB3270_TOGGLE_COUNT)
		return toggle_descriptor[ix].label;
	return "";
}


LIB3270_EXPORT const char * lib3270_get_toggle_description(LIB3270_TOGGLE ix)
{
	if(ix < LIB3270_TOGGLE_COUNT)
		return toggle_descriptor[ix].description;
	return "";
}

LIB3270_EXPORT const char * lib3270_get_toggle_name(LIB3270_TOGGLE ix)
{
	if(ix < LIB3270_TOGGLE_COUNT)
		return toggle_descriptor[ix].name;
	return "";
}

LIB3270_EXPORT LIB3270_TOGGLE lib3270_get_toggle_id(const char *name)
{
	if(name)
	{
		int f;
		for(f=0;f<LIB3270_TOGGLE_COUNT;f++)
		{
			if(!strcasecmp(name,toggle_descriptor[f].name))
				return f;
		}
	}
	return -1;
}
