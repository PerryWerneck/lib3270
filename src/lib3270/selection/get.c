/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270.
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

 #include "../private.h"
 #include <lib3270.h>
 #include <lib3270/session.h>
 #include <lib3270/selection.h>
 #include "3270ds.h"

 /*--[ Implement ]------------------------------------------------------------------------------------*/

static void clear_chr(H3270 *hSession, int baddr)
{
	hSession->text[baddr].chr = ' ';

	hSession->ea_buf[baddr].cc = EBC_null;
	hSession->ea_buf[baddr].cs = 0;

	hSession->cbk.update(	hSession,
							baddr,
							hSession->text[baddr].chr,
							hSession->text[baddr].attr,
							baddr == hSession->cursor_addr );
}

LIB3270_EXPORT char * lib3270_get_selection(H3270 *hSession, char tok, LIB3270_SELECTION_OPTIONS options)
{
	int				  row, col, baddr;
	char 			* ret;
	size_t			  buflen	= (hSession->rows * (hSession->cols+1))+1;
	size_t			  sz		= 0;
    unsigned short	  attr		= 0xFFFF;
    char			  cut		= (options & LIB3270_SELECTION_CUT) != 0;
    char			  all		= (options & LIB3270_SELECTION_ALL) != 0;

	if(check_online_session(hSession))
		return NULL;

	if(!hSession->selected || hSession->select.start == hSession->select.end)
		return NULL;

	ret = lib3270_malloc(buflen);

	baddr = 0;
	unsigned char fa = 0;

	for(row=0;row < ((int) hSession->rows);row++)
	{
		int cr = 0;

		for(col = 0; col < ((int) hSession->cols);col++)
		{
			if(hSession->ea_buf[baddr].fa) {
				fa = hSession->ea_buf[baddr].fa;
			}

			if(all || hSession->text[baddr].attr & LIB3270_ATTR_SELECTED)
			{
				if(tok && attr != hSession->text[baddr].attr)
				{
					attr = hSession->text[baddr].attr;
					ret[sz++] = tok;
					ret[sz++] = (attr & 0x0F);
					ret[sz++] = ((attr & 0xF0) >> 4);

				}

				cr++;
				ret[sz++] = hSession->text[baddr].chr;

				if(cut && !FA_IS_PROTECTED(fa)) {
					clear_chr(hSession,baddr);
				}

			}
			baddr++;
		}

		if(cr)
			ret[sz++] = '\n';

        if((sz+10) > buflen)
        {
            buflen += 100;
       		ret = lib3270_realloc(ret,buflen);
        }
	}

	if(!sz)
	{
		lib3270_free(ret);
		errno = ENOENT;
		return NULL;
	}
	else if(sz > 1 && ret[sz-1] == '\n') // Remove ending \n
	{
		ret[sz-1] = 0;
	}

	ret[sz++] = 0;

	if(sz != buflen)
		ret = lib3270_realloc(ret,sz);

	return ret;
}

LIB3270_EXPORT char * lib3270_get_selected(H3270 *hSession)
{
	return lib3270_get_selection(hSession,0,0);
}

LIB3270_EXPORT char * lib3270_cut_selected(H3270 *hSession)
{
	return lib3270_get_selection(hSession,0,LIB3270_SELECTION_CUT);
}


