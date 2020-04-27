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
#include <internals.h>
#include <unistd.h>
#include <CoreFoundation/CFBundle.h>
#include <CoreFoundation/CFURL.h>
#include <sys/syslimits.h>

static char * concat(char *path, const char *name, size_t *length)
{
	size_t szCurrent = strlen(path);

	if(szCurrent > 1 && path[szCurrent-1] != '/')
		strcat(path,"/");

	szCurrent += strlen(name);

	if(szCurrent >= *length)
	{
		*length += (szCurrent + 1024);
		path = lib3270_realloc(path,*length);
	}

	strcat(path,name);

	return path;
}

static char * build_filename(const char *root, const char *str, va_list args)
{
	size_t szFilename = 1024 + strlen(root);
	char * filename = (char *) lib3270_malloc(szFilename);

	strcpy(filename,root);

	while(str) {
		filename = concat(filename,str,&szFilename);
		str = va_arg(args, const char *);
	}

	return (char *) lib3270_realloc(filename,strlen(filename)+1);
}

char * lib3270_build_data_filename(const char *str, ...)
{
	va_list args;
	va_start (args, str);

	char *filename;
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	if (mainBundle) {
	    CFURLRef url = CFBundleCopyBundleURL(mainBundle);
		if (url) {
			size_t szPath = PATH_MAX;
			char *path = (char *) lib3270_malloc(szPath);
			CFURLGetFileSystemRepresentation(url, true, path, szPath);
			CFRelease(url);
			path = concat(path, "Contents/Resources", &szPath);
			filename = build_filename(path, str, args);
			lib3270_free(path);
		} else {
			filename = build_filename(LIB3270_STRINGIZE_VALUE_OF(DATADIR), str, args);
		}
	} else {
		filename = build_filename(LIB3270_STRINGIZE_VALUE_OF(DATADIR), str, args);
	}

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

char * lib3270_build_filename(const char *str, ...)
{
	size_t szFilename = 1024;
	char * filename = (char *) lib3270_malloc(szFilename);
	char * tempname;

	// First build the base filename
	memset(filename,0,szFilename);

	va_list args;
	va_start (args, str);
	while(str) {
		filename = concat(filename,str,&szFilename);
		str = va_arg(args, const char *);
	}
	va_end (args);

	// Check paths
	size_t ix;

	static const char * paths[] =
	{
		LIB3270_STRINGIZE_VALUE_OF(DATADIR),
		LIB3270_STRINGIZE_VALUE_OF(CONFDIR),
		"."
	};

	for(ix = 0; ix < (sizeof(paths)/sizeof(paths[0])); ix++)
	{
		tempname = lib3270_strdup_printf("%s/%s",paths[ix],filename);

		if(access(tempname, F_OK) == 0)
		{
			lib3270_free(filename);
			return tempname;
		}

		lib3270_free(tempname);

	}

	// Not found! Force the standard data dir

	tempname = lib3270_strdup_printf("%s/%s",paths[0],filename);
	lib3270_free(filename);

	return tempname;

}

