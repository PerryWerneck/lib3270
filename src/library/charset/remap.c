/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2008 Banco do Brasil S.A.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

/**
 *	@file charset/remap.c
 *
 *	@brief
 */

#include <internals.h>
#include <lib3270/charset.h>
#include <lib3270/log.h>
#include <lib3270/trace.h>
#include <X11keysym.h>
#include <string.h>

/*---[ Implement ]------------------------------------------------------------------------------------------------------------*/

///
/// @brief Process a single character definition.
///
LIB3270_EXPORT void lib3270_remap_char(H3270 *hSession, unsigned short ebc, unsigned short iso, lib3270_remap_scope scope, unsigned char one_way) {
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

	if (iso <= 0xff) {
		if (scope == BOTH || scope == CS_ONLY) {
			if (ebc > 0x40) {
				hSession->charset.ebc2asc[ebc] = iso;
				if (!one_way)
					hSession->charset.asc2ebc[iso] = ebc;
			}
		}

	}
}

LIB3270_EXPORT unsigned short lib3270_translate_char(const char *id) {

	static const struct {
		const char		* name;
		unsigned short	  keysym;
	}
	latin[] = {
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

		if(sscanf(id + 2, "%x", &rc) != 1) {
			errno = EINVAL;
			return 0;
		}

		return (unsigned short) rc;

	}

	for(ix=0; ix < (sizeof(latin)/sizeof(latin[0])); ix++) {
		if(!strcasecmp(id,latin[ix].name))
			return latin[ix].keysym;
	}

	if(strlen(id) != 1) {
		errno = EINVAL;
		return 0;
	}

	return (unsigned short) *id;

}
