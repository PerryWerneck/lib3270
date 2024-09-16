/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes paul.mattes@case.edu), de emulação de terminal 3270 para acesso a
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
 *	@file charset/view.c
 *
 *	@brief This module shows the charset table.
 */

#include <internals.h>
#include <lib3270/charset.h>
#include <lib3270/log.h>
#include <lib3270/trace.h>

/*---[ Implement ]------------------------------------------------------------------------------------------------------------*/

LIB3270_EXPORT int lib3270_charsettable(H3270 *hSession) {
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

	(void) memset(
	    (char *) hSession->ea_buf,
	    0,
	    ((size_t) hSession->view.rows) * ((size_t) hSession->view.cols) * sizeof(struct lib3270_ea)
	);

	baddr = margin_left+hSession->max.cols;
	s = (hSession->max.cols * 0x11);
	for(f=4; f<=0x0f; f++) {
		baddr += 2;
		hSession->ea_buf[baddr+s].fg = hSession->ea_buf[baddr].fg = LIB3270_ATTR_COLOR_GRAY;
		hSession->ea_buf[baddr+s].bg = hSession->ea_buf[baddr].bg = LIB3270_ATTR_COLOR_BLACK;
		hSession->ea_buf[baddr+s].cs = hSession->ea_buf[baddr].cs = 0;
		hSession->ea_buf[baddr+s].cc = hSession->ea_buf[baddr].cc = hSession->charset.asc2ebc[(int) hChars[f]];
		hSession->ea_buf[baddr+s].gr = hSession->ea_buf[baddr].gr = 0;
	}

	baddr = margin_left+(hSession->max.cols*2);
	s = 0x1a;
	for(f=0; f<=0x0f; f++) {
		hSession->ea_buf[baddr+s].fg = hSession->ea_buf[baddr].fg = LIB3270_ATTR_COLOR_GRAY;
		hSession->ea_buf[baddr+s].bg = hSession->ea_buf[baddr].bg = LIB3270_ATTR_COLOR_BLACK;
		hSession->ea_buf[baddr+s].cs = hSession->ea_buf[baddr].cs = 0;
		hSession->ea_buf[baddr+s].cc = hSession->ea_buf[baddr].cc = hSession->charset.asc2ebc[(int) hChars[f]];
		hSession->ea_buf[baddr+s].gr = hSession->ea_buf[baddr].gr = 0;
		baddr += hSession->max.cols;
	}

	chr = 0x40;

	for(f=0; f<0x0c; f++) {
		baddr = (margin_left+(hSession->max.cols*2))+(f*2)+2;
		for(r=0; r<=0x0f; r++) {
			hSession->ea_buf[baddr].fg = LIB3270_ATTR_COLOR_YELLOW;
			hSession->ea_buf[baddr].bg = LIB3270_ATTR_COLOR_BLACK;
			hSession->ea_buf[baddr].cs = 0;
			hSession->ea_buf[baddr].cc = chr++;
			hSession->ea_buf[baddr].gr = 0;
			baddr += hSession->max.cols;
		}
	}

	baddr = margin_left+0x1d+(hSession->max.cols*2);
	for(ptr=label; *ptr; ptr++) {
		hSession->ea_buf[baddr].fg = LIB3270_ATTR_COLOR_WHITE;
		hSession->ea_buf[baddr].bg = LIB3270_ATTR_COLOR_BLACK;
		hSession->ea_buf[baddr].cs = 0;
		hSession->ea_buf[baddr].cc = hSession->charset.asc2ebc[(int) *ptr];
		hSession->ea_buf[baddr].gr = 0;
		baddr++;
	}
	baddr++;

	for(ptr=hSession->charset.host; *ptr; ptr++) {
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



