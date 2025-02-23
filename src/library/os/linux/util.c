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
 * @brief Linux Utility functions.
 */


#include <config.h>
#include <stdarg.h>
#include <internals.h>
#include <lib3270/memory.h>
#include <fcntl.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif // HAVE_UNISTD_H

static char * concat(char *path, const char *name, size_t *length) {
	size_t szCurrent = strlen(path);

	if(szCurrent > 1 && path[szCurrent-1] != '/')
		strcat(path,"/");

	szCurrent += strlen(name);

	if(szCurrent >= *length) {
		*length += (szCurrent + 1024);
		path = lib3270_realloc(path,*length);
	}

	strcat(path,name);

	return path;
}

static char * build_filename(const char *root, const char *str, va_list args) {
	size_t szFilename = 1024 + strlen(root);
	char * filename = (char *) lib3270_malloc(szFilename);

	strcpy(filename,root);

	while(str) {
		filename = concat(filename,str,&szFilename);
		str = va_arg(args, const char *);
	}

	return (char *) lib3270_realloc(filename,strlen(filename)+1);
}

char * lib3270_build_data_filename(const char *str, ...) {
	va_list args;
	va_start (args, str);

	char *filename = build_filename(LIB3270_STRINGIZE_VALUE_OF(DATADIR), str, args);

	va_end (args);

	return filename;
}

char * lib3270_build_config_filename(const char *str, ...) {
	va_list args;
	va_start (args, str);

	char *filename = build_filename(LIB3270_STRINGIZE_VALUE_OF(CONFDIR), str, args);

	va_end (args);

	return filename;
}

char * lib3270_build_filename(const char *str, ...) {
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

	static const char * paths[] = {
		LIB3270_STRINGIZE_VALUE_OF(DATADIR),
		LIB3270_STRINGIZE_VALUE_OF(CONFDIR),
		"."
	};

	for(ix = 0; ix < (sizeof(paths)/sizeof(paths[0])); ix++) {
		tempname = lib3270_strdup_printf("%s/%s",paths[ix],filename);

		if(access(tempname, F_OK) == 0) {
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

