/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270. Registro no INPI sob
 * o nome G3270.
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
 * @brief Linux Utility functions.
 */


#include <config.h>
#include <stdarg.h>
#include <lib3270-internals.h>

static char * build_filename(const char *root, const char *str, va_list args)
{
	size_t szFilename = 1024 + strlen(root);
	char * filename = (char *) lib3270_malloc(szFilename);

	strcpy(filename,root);

	while(str) {

		size_t szCurrent = strlen(filename);

		if(filename[szCurrent-1] != '/')
			strcat(filename,"/");

		szCurrent += strlen(str);

		if(szCurrent >= szFilename)
		{
			szFilename += (szCurrent + 1024);
			filename = lib3270_realloc(filename,szFilename);
		}

		strcat(filename,str);

		str = va_arg(args, const char *);
	}

	return (char *) lib3270_realloc(filename,strlen(filename)+1);
}

char * lib3270_build_data_filename(const char *str, ...)
{
	va_list args;
	va_start (args, str);

	char *filename = build_filename(LIB3270_STRINGIZE_VALUE_OF(DATADIR), str, args);

	va_end (args);

	return filename;
}

char * lib3270_build_config_filename(const char *str, ...)
{
	va_list args;
	va_start (args, str);

	char *filename = build_filename(LIB3270_STRINGIZE_VALUE_OF(CONFDIR), str, args);

	va_end (args);

	return filename;
}
