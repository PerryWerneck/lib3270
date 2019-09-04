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
 * Este programa está nomeado como charset.c e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 * licinio@bb.com.br		(Licínio Luis Branco)
 * kraucer@bb.com.br		(Kraucer Fernandes Mazuco)
 *
 */

/**
 *	@file charset.c
 *
 *	@brief This module handles character sets.
 */

#include <lib3270-internals.h>
#include <X11keysym.h>
#include <lib3270/charset.h>
#include <lib3270/log.h>
#include <lib3270/trace.h>

/*
 * EBCDIC-to-Unicode translation tables.
 * Each table maps EBCDIC codes X'41' through X'FE' to UCS-2.
 * Other codes are mapped programmatically.
 */
#define UT_SIZE		190
#define UT_OFFSET	0x41

/*---[ Statics ]--------------------------------------------------------------------------------------------------------------*/

const unsigned short ebc2asc0[256] =
{
	/*00*/	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	/*08*/	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	/*10*/	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	/*18*/	0x20, 0x20, 0x20, 0x20, 0x2a, 0x20, 0x3b, 0x20,
	/*20*/	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	/*28*/	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	/*30*/	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	/*38*/	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	/*40*/	0x20, 0x20, 0xe2, 0xe4, 0xe0, 0xe1, 0xe3, 0xe5,
	/*48*/	0xe7, 0xf1, 0xa2, 0x2e, 0x3c, 0x28, 0x2b, 0x7c,
	/*50*/	0x26, 0xe9, 0xea, 0xeb, 0xe8, 0xed, 0xee, 0xef,
	/*58*/	0xec, 0xdf, 0x21, 0x24, 0x2a, 0x29, 0x3b, 0xac,
	/*60*/	0x2d, 0x2f, 0xc2, 0xc4, 0xc0, 0xc1, 0xc3, 0xc5,
	/*68*/	0xc7, 0xd1, 0xa6, 0x2c, 0x25, 0x5f, 0x3e, 0x3f,
	/*70*/	0xf8, 0xc9, 0xca, 0xcb, 0xc8, 0xcd, 0xce, 0xcf,
	/*78*/	0xcc, 0x60, 0x3a, 0x23, 0x40, 0x27, 0x3d, 0x22,
	/*80*/	0xd8, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
	/*88*/	0x68, 0x69, 0xab, 0xbb, 0xf0, 0xfd, 0xfe, 0xb1,
	/*90*/	0xb0, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70,
	/*98*/	0x71, 0x72, 0xaa, 0xba, 0xe6, 0xb8, 0xc6, 0xa4,
	/*a0*/	0xb5, 0x7e, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
	/*a8*/	0x79, 0x7a, 0xa1, 0xbf, 0xd0, 0xdd, 0xde, 0xae,
	/*b0*/	0x5e, 0xa3, 0xa5, 0xb7, 0xa9, 0xa7, 0xb6, 0xbc,
	/*b8*/	0xbd, 0xbe, 0x5b, 0x5d, 0xaf, 0xa8, 0xb4, 0xd7,
	/*c0*/	0x7b, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	/*c8*/	0x48, 0x49, 0xad, 0xf4, 0xf6, 0xf2, 0xf3, 0xf5,
	/*d0*/	0x7d, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50,
	/*d8*/	0x51, 0x52, 0xb9, 0xfb, 0xfc, 0xf9, 0xfa, 0xff,
	/*e0*/	0x5c, 0xf7, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
	/*e8*/	0x59, 0x5a, 0xb2, 0xd4, 0xd6, 0xd2, 0xd3, 0xd5,
	/*f0*/	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	/*f8*/	0x38, 0x39, 0xb3, 0xdb, 0xdc, 0xd9, 0xda, 0x20
};

static const unsigned short asc2ebc0[256] =
{
	/*00*/  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*08*/  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*10*/  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*18*/  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*20*/  0x40, 0x5a, 0x7f, 0x7b, 0x5b, 0x6c, 0x50, 0x7d,
	/*28*/  0x4d, 0x5d, 0x5c, 0x4e, 0x6b, 0x60, 0x4b, 0x61,
	/*30*/  0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
	/*38*/  0xf8, 0xf9, 0x7a, 0x5e, 0x4c, 0x7e, 0x6e, 0x6f,
	/*40*/  0x7c, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
	/*48*/  0xc8, 0xc9, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6,
	/*50*/  0xd7, 0xd8, 0xd9, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6,
	/*58*/  0xe7, 0xe8, 0xe9, 0xba, 0xe0, 0xbb, 0xb0, 0x6d,
	/*60*/  0x79, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	/*68*/  0x88, 0x89, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96,
	/*70*/  0x97, 0x98, 0x99, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6,
	/*78*/  0xa7, 0xa8, 0xa9, 0xc0, 0x4f, 0xd0, 0xa1, 0x00,
	/*80*/  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*88*/  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*90*/  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*98*/  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*a0*/  0x41, 0xaa, 0x4a, 0xb1, 0x9f, 0xb2, 0x6a, 0xb5,
	/*a8*/  0xbd, 0xb4, 0x9a, 0x8a, 0x5f, 0xca, 0xaf, 0xbc,
	/*b0*/  0x90, 0x8f, 0xea, 0xfa, 0xbe, 0xa0, 0xb6, 0xb3,
	/*b8*/  0x9d, 0xda, 0x9b, 0x8b, 0xb7, 0xb8, 0xb9, 0xab,
	/*c0*/  0x64, 0x65, 0x62, 0x66, 0x63, 0x67, 0x9e, 0x68,
	/*c8*/  0x74, 0x71, 0x72, 0x73, 0x78, 0x75, 0x76, 0x77,
	/*d0*/  0xac, 0x69, 0xed, 0xee, 0xeb, 0xef, 0xec, 0xbf,
	/*d8*/  0x80, 0xfd, 0xfe, 0xfb, 0xfc, 0xad, 0xae, 0x59,
	/*e0*/  0x44, 0x45, 0x42, 0x46, 0x43, 0x47, 0x9c, 0x48,
	/*e8*/  0x54, 0x51, 0x52, 0x53, 0x58, 0x55, 0x56, 0x57,
	/*f0*/  0x8c, 0x49, 0xcd, 0xce, 0xcb, 0xcf, 0xcc, 0xe1,
	/*f8*/  0x70, 0xdd, 0xde, 0xdb, 0xdc, 0x8d, 0x8e, 0xdf
};

static const unsigned short asc2uc[UT_SIZE] =
{
	/*40*/	      0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	/*48*/	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
	/*50*/	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
	/*58*/	0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
	/*60*/	0x60, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	/*68*/	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
	/*70*/	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
	/*78*/	0x58, 0x59, 0x5a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
	/*80*/	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	/*88*/	0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	/*90*/	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
	/*98*/	0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	/*a0*/	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	/*a8*/	0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
	/*b0*/	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
	/*b8*/	0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	/*c0*/	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
	/*c8*/	0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
	/*d0*/	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
	/*d8*/	0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
	/*e0*/	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
	/*e8*/	0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
	/*f0*/	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xf7,
	/*f8*/	0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde
};

typedef struct _info
{
	const char				* name;
	unsigned long			  cgcsgid;
	const unsigned short	* chr;
} remap;

static const remap charset[] =
{
	{
		"us",
		LIB3270_DEFAULT_CGEN | LIB3270_DEFAULT_CSET,
		(const unsigned short [])
		{
			0x0000,	0x0000
		}
	},

	{
		"bracket",
		LIB3270_DEFAULT_CGEN|LIB3270_DEFAULT_CSET,
		(const unsigned short [])
		{
			0x00ad, '[',
			0x00ba, XK_Yacute,
			0x00bd, ']',
			0x00bb, XK_diaeresis,
			0x0000,	0x0000
		}
	},

	{
		"cp500",
		LIB3270_DEFAULT_CGEN|0x000001F4,
		(const unsigned short [])
		{
			0x004a, '[',
			0x004f, '!',
			0x005a, ']',
			0x005f, '^',
			0x00b0, XK_percent,
			0x00ba, XK_notsign,
			0x00bb, XK_bar
		}
	},

	// Terminate list
	{
		NULL
	}

};

/*---[ Implement ]------------------------------------------------------------------------------------------------------------*/

static void copy_charset(const unsigned short *from, unsigned short *to)
{
	int f;
	for(f=0;f < UT_SIZE;f++)
		to[f+UT_OFFSET] = from[f];
}

LIB3270_EXPORT void lib3270_reset_charset(H3270 *hSession, const char * host, const char * display, unsigned long cgcsgid)
{
	int f;

	#define replace_pointer(x,v) if(x) { lib3270_free(x); }; x = strdup(v)

	replace_pointer(hSession->charset.host, host);
	replace_pointer(hSession->charset.display,display);

	hSession->charset.cgcsgid = cgcsgid;

	memcpy(hSession->charset.ebc2asc,	ebc2asc0,	sizeof(hSession->charset.ebc2asc));
	memcpy(hSession->charset.asc2ebc,	asc2ebc0,	sizeof(hSession->charset.asc2ebc));

	for(f=0;f<UT_OFFSET;f++)
		hSession->charset.asc2uc[f] = f;

	copy_charset(asc2uc,hSession->charset.asc2uc);

}

LIB3270_EXPORT int lib3270_set_host_charset(H3270 *hSession, const char *name)
{
	int f;

	if(!name)
	{
		lib3270_reset_charset(hSession,"us","ISO-8859-1", LIB3270_DEFAULT_CGEN | LIB3270_DEFAULT_CSET);
		return 0;
	}

	if(hSession->charset.host && !strcasecmp(name,hSession->charset.host))
		return 0;

	for(f=0;charset[f].name != NULL;f++)
	{
		if(!strcasecmp(name,charset[f].name))
		{
			// Found required charset
			int c;

			debug("%s: %s -> %s",__FUNCTION__,hSession->charset.host,charset[f].name);

			lib3270_reset_charset(hSession,charset[f].name,"ISO-8859-1", charset[f].cgcsgid);

			for(c=0;charset[f].chr[c];c+=2)
				lib3270_remap_char(hSession,charset[f].chr[c],charset[f].chr[c+1], BOTH, 0);

			return 0;
		}
	}

	return ENOENT;

}

LIB3270_EXPORT const char * lib3270_get_default_charset(void)
{
	return "ISO-8859-1";
}

LIB3270_EXPORT const char * lib3270_get_display_charset(const H3270 *hSession)
{
	return hSession->charset.display ? hSession->charset.display : "ISO-8859-1";
}

LIB3270_EXPORT const char * lib3270_get_host_charset(const H3270 *hSession)
{
	return hSession->charset.host;
}

LIB3270_EXPORT int lib3270_charsettable(H3270 *hSession)
{
	static const char * hChars = "0123456789ABCDEF";
	static const char * label  = "Name:";

	int f;
	int margin_left = 5;
	int baddr;
	int chr;
	int s,r;
	const char *ptr;

	CHECK_SESSION_HANDLE(hSession);

	trace("%s","Showing charset table");

	(void) memset((char *) hSession->ea_buf, 0, hSession->view.rows * hSession->view.cols * sizeof(struct lib3270_ea));

	baddr = margin_left+hSession->max.cols;
	s = (hSession->max.cols * 0x11);
	for(f=4;f<=0x0f;f++)
	{
		baddr += 2;
		hSession->ea_buf[baddr+s].fg = hSession->ea_buf[baddr].fg = LIB3270_ATTR_COLOR_GRAY;
		hSession->ea_buf[baddr+s].bg = hSession->ea_buf[baddr].bg = LIB3270_ATTR_COLOR_BLACK;
		hSession->ea_buf[baddr+s].cs = hSession->ea_buf[baddr].cs = 0;
		hSession->ea_buf[baddr+s].cc = hSession->ea_buf[baddr].cc = hSession->charset.asc2ebc[(int) hChars[f]];
		hSession->ea_buf[baddr+s].gr = hSession->ea_buf[baddr].gr = 0;
	}

	baddr = margin_left+(hSession->max.cols*2);
	s = 0x1a;
	for(f=0;f<=0x0f;f++)
	{
		hSession->ea_buf[baddr+s].fg = hSession->ea_buf[baddr].fg = LIB3270_ATTR_COLOR_GRAY;
		hSession->ea_buf[baddr+s].bg = hSession->ea_buf[baddr].bg = LIB3270_ATTR_COLOR_BLACK;
		hSession->ea_buf[baddr+s].cs = hSession->ea_buf[baddr].cs = 0;
		hSession->ea_buf[baddr+s].cc = hSession->ea_buf[baddr].cc = hSession->charset.asc2ebc[(int) hChars[f]];
		hSession->ea_buf[baddr+s].gr = hSession->ea_buf[baddr].gr = 0;
		baddr += hSession->max.cols;
	}

	chr = 0x40;

	for(f=0;f<0x0c;f++)
	{
		baddr = (margin_left+(hSession->max.cols*2))+(f*2)+2;
		for(r=0;r<=0x0f;r++)
		{
			hSession->ea_buf[baddr].fg = LIB3270_ATTR_COLOR_YELLOW;
			hSession->ea_buf[baddr].bg = LIB3270_ATTR_COLOR_BLACK;
			hSession->ea_buf[baddr].cs = 0;
			hSession->ea_buf[baddr].cc = chr++;
			hSession->ea_buf[baddr].gr = 0;
			baddr += hSession->max.cols;
		}
	}

	baddr = margin_left+0x1d+(hSession->max.cols*2);
	for(ptr=label;*ptr;ptr++)
	{
		hSession->ea_buf[baddr].fg = LIB3270_ATTR_COLOR_WHITE;
		hSession->ea_buf[baddr].bg = LIB3270_ATTR_COLOR_BLACK;
		hSession->ea_buf[baddr].cs = 0;
		hSession->ea_buf[baddr].cc = hSession->charset.asc2ebc[(int) *ptr];
		hSession->ea_buf[baddr].gr = 0;
		baddr++;
	}
	baddr++;

	for(ptr=hSession->charset.host;*ptr;ptr++)
	{
		hSession->ea_buf[baddr].fg = LIB3270_ATTR_COLOR_YELLOW;
		hSession->ea_buf[baddr].bg = LIB3270_ATTR_COLOR_BLACK;
		hSession->ea_buf[baddr].cs = 0;
		hSession->ea_buf[baddr].cc = hSession->charset.asc2ebc[(int) *ptr];
		hSession->ea_buf[baddr].gr = 0;
		baddr++;
	}

	hSession->cbk.display(hSession);

	return 0;
}

LIB3270_EXPORT const char * lib3270_asc2ebc(H3270 *hSession, unsigned char *buffer, int sz)
{
	int f;
	if(sz < 0)
		sz = strlen((const char *) buffer);

	if(sz > 0)
	{
		for(f=0;f<sz;f++)
			buffer[f] = hSession->charset.asc2ebc[buffer[f]];
	}

	return (const char *) buffer;
}

LIB3270_EXPORT const char * lib3270_ebc2asc(H3270 *hSession, unsigned char *buffer, int sz)
{
	int f;
	if(sz < 0)
		sz = strlen((const char *) buffer);

	if(sz > 0)
	{
		for(f=0;f<sz;f++)
			buffer[f] = hSession->charset.ebc2asc[buffer[f]];
	}

	return (const char *) buffer;
}


///
/// @brief Process a single character definition.
///
LIB3270_EXPORT void lib3270_remap_char(H3270 *hSession, unsigned short ebc, unsigned short iso, lib3270_remap_scope scope, unsigned char one_way)
{
	//	unsigned char cg;
	CHECK_SESSION_HANDLE(hSession);

	// Ignore mappings of EBCDIC control codes and the space character.
	if (ebc <= 0x40)
		return;

	// If they want to map to a NULL or a blank, make it a one-way blank.
	if (iso == 0x0)
		iso = 0x20;
	if (iso == 0x20)
		one_way = True;

	if (iso <= 0xff)
	{
		if (scope == BOTH || scope == CS_ONLY)
		{
			if (ebc > 0x40)
			{
				hSession->charset.ebc2asc[ebc] = iso;
				if (!one_way)
					hSession->charset.asc2ebc[iso] = ebc;
			}
		}

	}
}

LIB3270_EXPORT unsigned short lib3270_translate_char(const char *id)
{

	static const struct
	{
		const char		* name;
		unsigned short	  keysym;
	}
	latin[] =
	{
		{ "space", XK_space },
		{ "exclam", XK_exclam },
		{ "quotedbl", XK_quotedbl },
		{ "numbersign", XK_numbersign },
		{ "dollar", XK_dollar },
		{ "percent", XK_percent },
		{ "ampersand", XK_ampersand },
		{ "apostrophe", XK_apostrophe },
		{ "quoteright", XK_quoteright },
		{ "parenleft", XK_parenleft },
		{ "parenright", XK_parenright },
		{ "asterisk", XK_asterisk },
		{ "plus", XK_plus },
		{ "comma", XK_comma },
		{ "minus", XK_minus },
		{ "period", XK_period },
		{ "slash", XK_slash },
		{ "0", XK_0 },
		{ "1", XK_1 },
		{ "2", XK_2 },
		{ "3", XK_3 },
		{ "4", XK_4 },
		{ "5", XK_5 },
		{ "6", XK_6 },
		{ "7", XK_7 },
		{ "8", XK_8 },
		{ "9", XK_9 },
		{ "colon", XK_colon },
		{ "semicolon", XK_semicolon },
		{ "less", XK_less },
		{ "equal", XK_equal },
		{ "greater", XK_greater },
		{ "question", XK_question },
		{ "at", XK_at },
		{ "A", XK_A },
		{ "B", XK_B },
		{ "C", XK_C },
		{ "D", XK_D },
		{ "E", XK_E },
		{ "F", XK_F },
		{ "G", XK_G },
		{ "H", XK_H },
		{ "I", XK_I },
		{ "J", XK_J },
		{ "K", XK_K },
		{ "L", XK_L },
		{ "M", XK_M },
		{ "N", XK_N },
		{ "O", XK_O },
		{ "P", XK_P },
		{ "Q", XK_Q },
		{ "R", XK_R },
		{ "S", XK_S },
		{ "T", XK_T },
		{ "U", XK_U },
		{ "V", XK_V },
		{ "W", XK_W },
		{ "X", XK_X },
		{ "Y", XK_Y },
		{ "Z", XK_Z },
		{ "bracketleft", XK_bracketleft },
		{ "backslash", XK_backslash },
		{ "bracketright", XK_bracketright },
		{ "asciicircum", XK_asciicircum },
		{ "underscore", XK_underscore },
		{ "grave", XK_grave },
		{ "quoteleft", XK_quoteleft },
		{ "a", XK_a },
		{ "b", XK_b },
		{ "c", XK_c },
		{ "d", XK_d },
		{ "e", XK_e },
		{ "f", XK_f },
		{ "g", XK_g },
		{ "h", XK_h },
		{ "i", XK_i },
		{ "j", XK_j },
		{ "k", XK_k },
		{ "l", XK_l },
		{ "m", XK_m },
		{ "n", XK_n },
		{ "o", XK_o },
		{ "p", XK_p },
		{ "q", XK_q },
		{ "r", XK_r },
		{ "s", XK_s },
		{ "t", XK_t },
		{ "u", XK_u },
		{ "v", XK_v },
		{ "w", XK_w },
		{ "x", XK_x },
		{ "y", XK_y },
		{ "z", XK_z },
		{ "braceleft", XK_braceleft },
		{ "bar", XK_bar },
		{ "braceright", XK_braceright },
		{ "asciitilde", XK_asciitilde },
		{ "nobreakspace", XK_nobreakspace },
		{ "exclamdown", XK_exclamdown },
		{ "cent", XK_cent },
		{ "sterling", XK_sterling },
		{ "currency", XK_currency },
		{ "yen", XK_yen },
		{ "brokenbar", XK_brokenbar },
		{ "section", XK_section },
		{ "diaeresis", XK_diaeresis },
		{ "copyright", XK_copyright },
		{ "ordfeminine", XK_ordfeminine },
		{ "guillemotleft", XK_guillemotleft },
		{ "notsign", XK_notsign },
		{ "hyphen", XK_hyphen },
		{ "registered", XK_registered },
		{ "macron", XK_macron },
		{ "degree", XK_degree },
		{ "plusminus", XK_plusminus },
		{ "twosuperior", XK_twosuperior },
		{ "threesuperior", XK_threesuperior },
		{ "acute", XK_acute },
		{ "mu", XK_mu },
		{ "paragraph", XK_paragraph },
		{ "periodcentered", XK_periodcentered },
		{ "cedilla", XK_cedilla },
		{ "onesuperior", XK_onesuperior },
		{ "masculine", XK_masculine },
		{ "guillemotright", XK_guillemotright },
		{ "onequarter", XK_onequarter },
		{ "onehalf", XK_onehalf },
		{ "threequarters", XK_threequarters },
		{ "questiondown", XK_questiondown },
		{ "Agrave", XK_Agrave },
		{ "Aacute", XK_Aacute },
		{ "Acircumflex", XK_Acircumflex },
		{ "Atilde", XK_Atilde },
		{ "Adiaeresis", XK_Adiaeresis },
		{ "Aring", XK_Aring },
		{ "AE", XK_AE },
		{ "Ccedilla", XK_Ccedilla },
		{ "Egrave", XK_Egrave },
		{ "Eacute", XK_Eacute },
		{ "Ecircumflex", XK_Ecircumflex },
		{ "Ediaeresis", XK_Ediaeresis },
		{ "Igrave", XK_Igrave },
		{ "Iacute", XK_Iacute },
		{ "Icircumflex", XK_Icircumflex },
		{ "Idiaeresis", XK_Idiaeresis },
		{ "ETH", XK_ETH },
		{ "Eth", XK_Eth },
		{ "Ntilde", XK_Ntilde },
		{ "Ograve", XK_Ograve },
		{ "Oacute", XK_Oacute },
		{ "Ocircumflex", XK_Ocircumflex },
		{ "Otilde", XK_Otilde },
		{ "Odiaeresis", XK_Odiaeresis },
		{ "multiply", XK_multiply },
		{ "Ooblique", XK_Ooblique },
		{ "Ugrave", XK_Ugrave },
		{ "Uacute", XK_Uacute },
		{ "Ucircumflex", XK_Ucircumflex },
		{ "Udiaeresis", XK_Udiaeresis },
		{ "Yacute", XK_Yacute },
		{ "THORN", XK_THORN },
		{ "Thorn", XK_Thorn },
		{ "ssharp", XK_ssharp },
		{ "agrave", XK_agrave },
		{ "aacute", XK_aacute },
		{ "acircumflex", XK_acircumflex },
		{ "atilde", XK_atilde },
		{ "adiaeresis", XK_adiaeresis },
		{ "aring", XK_aring },
		{ "ae", XK_ae },
		{ "ccedilla", XK_ccedilla },
		{ "egrave", XK_egrave },
		{ "eacute", XK_eacute },
		{ "ecircumflex", XK_ecircumflex },
		{ "ediaeresis", XK_ediaeresis },
		{ "igrave", XK_igrave },
		{ "iacute", XK_iacute },
		{ "icircumflex", XK_icircumflex },
		{ "idiaeresis", XK_idiaeresis },
		{ "eth", XK_eth },
		{ "ntilde", XK_ntilde },
		{ "ograve", XK_ograve },
		{ "oacute", XK_oacute },
		{ "ocircumflex", XK_ocircumflex },
		{ "otilde", XK_otilde },
		{ "odiaeresis", XK_odiaeresis },
		{ "division", XK_division },
		{ "oslash", XK_oslash },
		{ "ugrave", XK_ugrave },
		{ "uacute", XK_uacute },
		{ "ucircumflex", XK_ucircumflex },
		{ "udiaeresis", XK_udiaeresis },
		{ "yacute", XK_yacute },
		{ "thorn", XK_thorn },
		{ "ydiaeresis", XK_ydiaeresis },

		// The following are, umm, hacks to allow symbolic names for
		// control codes.
	#if !defined(_WIN32)
		{ "BackSpace", 0x08 },
		{ "Tab", 0x09 },
		{ "Linefeed", 0x0a },
		{ "Return", 0x0d },
		{ "Escape", 0x1b },
		{ "Delete", 0x7f },
	#endif
	};

	size_t ix;

 	if(strncasecmp(id,"0x",2) == 0) {

        unsigned int rc = 0;

		if(sscanf(id + 2, "%x", &rc) != 1)
		{
			errno = EINVAL;
			return 0;
		}

		return (unsigned short) rc;

 	}

 	for(ix=0;ix < (sizeof(latin)/sizeof(latin[0])); ix++) {
		if(!strcasecmp(id,latin[ix].name))
			return latin[ix].keysym;
 	}

 	if(strlen(id) != 1)
	{
		errno = EINVAL;
		return 0;
	}

 	return (unsigned short) *id;

}


