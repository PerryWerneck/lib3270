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
 *
 */

/**
 *	@file charset/iconv.c
 *
 *	@brief This module implements the ICONV wrapper methods.
 */

#include <config.h>
#include <lib3270.h>
#include <lib3270/charset.h>
#include <iconv.h>
#include <string.h>

struct _lib3270_iconv
{
	/// @brief Convert strings from host codepage to local codepage.
	iconv_t local;

	/// @brief Convert string from local codepage to host codepage.
	iconv_t host;
};

/*---[ Implement ]------------------------------------------------------------------------------------------------------------*/

 LIB3270_ICONV * lib3270_iconv_new(const char *remote, const char *local)
 {
	LIB3270_ICONV * converter = lib3270_malloc(sizeof(LIB3270_ICONV));
	memset(converter,0,sizeof(LIB3270_ICONV));

	if(strcmp(local,remote)) {

		// Local and remote charsets aren't the same, setup conversion
		converter->local = iconv_open(local, remote);
		converter->host  = iconv_open(remote,local);

	} else {

		// Same charset, doesn't convert
		converter->local = converter->host = (iconv_t)(-1);

	}

	return converter;
 }

 void lib3270_iconv_free(LIB3270_ICONV *conv)
 {
	if(conv->local != (iconv_t) (-1))
		iconv_close(conv->local);

	if(conv->host != (iconv_t) (-1))
		iconv_close(conv->host);

	conv->local = conv->host = (iconv_t) (-1);

	lib3270_free(conv);
 }

 static char *convert(iconv_t *converter, const char * str, int length)
 {
	if(length < 0)
		length = (int) strlen(str);

	if(length && converter != (iconv_t)(-1))
	{

		size_t				  in		= length;
		size_t				  out		= (length << 1);
		char				* ptr;
		char				* outBuffer	= (char *) lib3270_malloc(out);
		ICONV_CONST char	* inBuffer	= (ICONV_CONST char *) str;

		memset(ptr=outBuffer,0,out);

		iconv(converter,NULL,NULL,NULL,NULL);   // Reset state

		if(iconv(converter,&inBuffer,&in,&ptr,&out) != ((size_t) -1))
			return (char *) outBuffer;

	}

	return NULL;
 }

 char * lib3270_iconv_from_host(LIB3270_ICONV *conv, const char *str, int len)
 {
	return convert(&conv->local,str,len);
 }

 char * lib3270_iconv_to_host(LIB3270_ICONV *conv, const char *str, int len)
 {
	return convert(&conv->host,str,len);
 }
