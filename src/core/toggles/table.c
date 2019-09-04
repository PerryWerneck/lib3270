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
#include <lib3270/toggle.h>
#include <lib3270-internals.h>
#include "togglesc.h"

/*---[ Implement ]------------------------------------------------------------------------------------------------------------*/

const LIB3270_TOGGLE_ENTRY toggle_descriptor[LIB3270_TOGGLE_COUNT+1] =
{
	{
		"monocase",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Monocase" ),
		N_( "Uppercase only" ),
		N_( "If set, the terminal operates in uppercase-only mode" )
	},
	{
		"cursorblink",
		True,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Blinking Cursor" ),
		N_( "Blinking Cursor" ),
		N_( "If set, the cursor blinks" )
	},
	{
		"showtiming",
		True,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Show timer when processing" ),
		N_( "Show timer when processing" ),
		N_( "If set, the time taken by the host to process an AID is displayed on the status line" )
	},
	{
		"cursorpos",
		True,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Track Cursor" ),
		N_( "Track Cursor" ),
		N_( "Display the cursor location in the OIA (the status line)" )
	},
	{
		"dstrace",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Data Stream" ),
		N_( "Trace Data Stream" ),
		""
	},
	{
		"linewrap",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_("Wrap around"),
		N_("Wrap around"),
		N_("If set, the NVT terminal emulator automatically assumes a NEWLINE character when it reaches the end of a line.")
	},
	{
		"blankfill",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Blank Fill" ),
		N_( "Blank Fill" ),
		N_( "Automatically convert trailing blanks in a field to NULLs in order to insert a character, and will automatically convert leading NULLs to blanks so that input data is not squeezed to the left" )
	},
	{
		"screentrace",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Screens" ),
		N_( "Trace screen contents" ),
		""
	},
	{
		"eventtrace",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Events" ),
		N_( "Trace interface and application events" ),
		""
	},
	{
		"marginedpaste",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Paste with left margin" ),
		N_( "Paste with left margin" ),
		N_( "If set, puts restrictions on how pasted text is placed on the screen. The position of the cursor at the time the paste operation is begun is used as a left margin. No pasted text will fill any area of the screen to the left of that position. This option is useful for pasting into certain IBM editors that use the left side of the screen for control information" )
	},
	{
		"rectselect",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Select by rectangles" ),
		N_( "Select by rectangles" ),
		N_( "If set, the terminal will always select rectangular areas of the screen. Otherwise, it selects continuous regions of the screen" )
	},
	{
		"crosshair",
		False,
		"<alt>x",		// Default keycode
		NULL,			// Icon name
		N_( "Cross hair cursor" ),
		N_( "Cross hair cursor" ),
		N_( "If set, the terminal will display a crosshair over the cursor: lines extending the full width and height of the screen, centered over the cursor position. This makes locating the cursor on the screen much easier" )
	},
	{
		"fullscreen",
		False,
		"<alt>Home",		// Default keycode
		NULL,				// Icon name
		N_( "Full Screen" ),
		N_( "Full Screen" ),
		N_( "If set, asks to place the toplevel window in the fullscreen state" )
	},
	{
		"reconnect",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Auto-Reconnect" ),
		N_( "Auto-Reconnect" ),
		N_( "Automatically reconnect to the host if it ever disconnects" )
	},
	{
		"insert",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Insert" ),
		N_( "Set insert mode" ),
		""
	},
	{
		"smartpaste",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Smart paste" ),
		N_( "Smart paste" ),
		""
	},
	{
		"bold",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Bold" ),
		N_( "Bold" ),
		""
	},
	{
		"keepselected",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Keep selected" ),
		N_( "Keep selected" ),
		""
	},
	{
		"underline",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Underline" ),
		N_( "Show Underline" ),
		""
	},
	{
		"autoconnect",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Auto connect" ),
		N_( "Connect on startup" ),
		""
	},
	{
		"kpalternative",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Use +/- for field navigation" ),
		N_( "Use +/- for field navigation" ),
		N_( "Use the keys +/- from keypad to select editable fields" )
	},
	{
		"beep",
		True,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Sound" ),
		N_( "Alert sound" ),
		N_( "Beep on errors" )
	},
	{
		"fieldattr",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Show Field" ),
		N_( "Show Field attributes" ),
		""
	},
	{
		"altscreen",
		True,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Alternate screen" ),
		N_( "Resize on alternate screen" ),
		N_( "Auto resize on altscreen" )
	},
	{
		"keepalive",
		True,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Network keep alive" ),
		N_( "Network keep alive" ),
		N_( "Enable network keep-alive with SO_KEEPALIVE" )
	},
	{
		"nettrace",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "Network data" ),
		N_( "Trace network data flow" ),
		N_( "Enable network in/out trace" )
	},
	{
		"ssltrace",
		False,
		NULL,		// Default keycode
		NULL,		// Icon name
		N_( "SSL negotiation" ),
		N_( "Trace SSL negotiation" ),
		N_( "Enable security negotiation trace" )
	},

	{
		NULL,
		0,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	}

};

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
