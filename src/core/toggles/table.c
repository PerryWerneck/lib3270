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
		LIB3270_TOGGLE_MONOCASE,
		"monocase",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Monocase" ),
		N_( "Uppercase only" ),
		N_( "If set, the terminal operates in uppercase-only mode" )
	},
	{
		LIB3270_TOGGLE_CURSOR_BLINK,
		"cursorblink",
		True,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Blinking Cursor" ),
		N_( "Blinking Cursor" ),
		N_( "If set, the cursor blinks" )
	},
	{
		LIB3270_TOGGLE_SHOW_TIMING,
		"showtiming",
		True,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Show timer when processing" ),
		N_( "Show timer when processing" ),
		N_( "If set, the time taken by the host to process an AID is displayed on the status line" )
	},
	{
		LIB3270_TOGGLE_CURSOR_POS,
		"cursorpos",
		True,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Track Cursor" ),
		N_( "Track Cursor" ),
		N_( "Display the cursor location in the OIA (the status line)" )
	},
	{
		LIB3270_TOGGLE_DS_TRACE,
		"dstrace",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Data Stream" ),
		N_( "Trace Data Stream" ),
		""
	},
	{
		LIB3270_TOGGLE_LINE_WRAP,
		"linewrap",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_("Wrap around"),
		N_("Wrap around"),
		N_("If set, the NVT terminal emulator automatically assumes a NEWLINE character when it reaches the end of a line.")
	},
	{
		LIB3270_TOGGLE_BLANK_FILL,
		"blankfill",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Blank Fill" ),
		N_( "Blank Fill" ),
		N_( "Automatically convert trailing blanks in a field to NULLs in order to insert a character, and will automatically convert leading NULLs to blanks so that input data is not squeezed to the left" )
	},
	{
		LIB3270_TOGGLE_SCREEN_TRACE,
		"screentrace",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Screens" ),
		N_( "Trace screen contents" ),
		""
	},
	{
		LIB3270_TOGGLE_EVENT_TRACE,
		"eventtrace",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Events" ),
		N_( "Trace interface and application events" ),
		""
	},
	{
		LIB3270_TOGGLE_MARGINED_PASTE,
		"marginedpaste",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Paste with left margin" ),
		N_( "Paste with left margin" ),
		N_( "If set, puts restrictions on how pasted text is placed on the screen. The position of the cursor at the time the paste operation is begun is used as a left margin. No pasted text will fill any area of the screen to the left of that position. This option is useful for pasting into certain IBM editors that use the left side of the screen for control information" )
	},
	{
		LIB3270_TOGGLE_RECTANGLE_SELECT,
		"rectselect",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Select by rectangles" ),
		N_( "Select by rectangles" ),
		N_( "If set, the terminal will always select rectangular areas of the screen. Otherwise, it selects continuous regions of the screen" )
	},
	{
		LIB3270_TOGGLE_CROSSHAIR,
		"crosshair",
		False,
		"<alt>x",		// Default keycode
		NULL,			// Icon name
		N_( "Cross hair cursor" ),
		N_( "Cross hair cursor" ),
		N_( "If set, the terminal will display a crosshair over the cursor: lines extending the full width and height of the screen, centered over the cursor position. This makes locating the cursor on the screen much easier" )
	},
	{
		LIB3270_TOGGLE_FULL_SCREEN,
		"fullscreen",
		False,
		"<alt>Home",		// Default keycode
		NULL,				// Icon name
		N_( "Full Screen" ),
		N_( "Full Screen" ),
		N_( "If set, asks to place the toplevel window in the fullscreen state" )
	},
	{
		LIB3270_TOGGLE_RECONNECT,
		"reconnect",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Auto-Reconnect" ),
		N_( "Auto-Reconnect" ),
		N_( "Automatically reconnect to the host if it ever disconnects" )
	},
	{
		LIB3270_TOGGLE_INSERT,
		"insert",
		False,
		"Insert",			// Default keycode
		"insert-text",		// Icon name
		N_( "Insert" ),
		N_( "Set insert mode" ),
		""
	},
	{
		LIB3270_TOGGLE_SMART_PASTE,
		"smartpaste",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Smart paste" ),
		N_( "Smart paste" ),
		""
	},
	{
		LIB3270_TOGGLE_BOLD,
		"bold",
		False,
		NULL,					// Default keycode
		"format-text-bold",		// Icon name
		N_( "Bold" ),
		N_( "Bold" ),
		""
	},
	{
		LIB3270_TOGGLE_KEEP_SELECTED,
		"keepselected",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Keep selected" ),
		N_( "Keep selected" ),
		""
	},
	{
		LIB3270_TOGGLE_UNDERLINE,
		"underline",
		True,
		NULL,							// Default keycode
		"format-text-underline",		// Icon name
		N_( "Underline" ),
		N_( "Show Underline" ),
		""
	},
	{
		LIB3270_TOGGLE_CONNECT_ON_STARTUP,
		"autoconnect",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Auto connect" ),
		N_( "Connect on startup" ),
		""
	},
	{
		LIB3270_TOGGLE_KP_ALTERNATIVE,
		"kpalternative",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Use +/- for field navigation" ),
		N_( "Use +/- for field navigation" ),
		N_( "Use the keys +/- from keypad to select editable fields" )
	},
	{
		LIB3270_TOGGLE_BEEP,
		"beep",
		True,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Sound" ),
		N_( "Alert sound" ),
		N_( "Beep on errors" )
	},
	{
		LIB3270_TOGGLE_VIEW_FIELD,
		"fieldattr",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Show Field" ),
		N_( "Show Field attributes" ),
		""
	},
	{
		LIB3270_TOGGLE_ALTSCREEN,
		"altscreen",
		True,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Alternate screen" ),
		N_( "Resize on alternate screen" ),
		N_( "Auto resize on altscreen" )
	},
	{
		LIB3270_TOGGLE_KEEP_ALIVE,
		"keepalive",
		True,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Network keep alive" ),
		N_( "Network keep alive" ),
		N_( "Enable network keep-alive with SO_KEEPALIVE" )
	},
	{
		LIB3270_TOGGLE_NETWORK_TRACE,
		"nettrace",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Network data" ),
		N_( "Trace network data flow" ),
		N_( "Enable network in/out trace" )
	},
	{
		LIB3270_TOGGLE_SSL_TRACE,
		"ssltrace",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "SSL negotiation" ),
		N_( "Trace SSL negotiation" ),
		N_( "Enable security negotiation trace" )
	},

	{
		LIB3270_TOGGLE_COUNT,
		NULL,
		0,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
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
