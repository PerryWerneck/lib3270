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
 * @brief Handle text arrays.
 */

#include <config.h>
#include <string.h>

#include <lib3270.h>
#include <lib3270/log.h>
#include <array.h>

/*---[ Implement ]------------------------------------------------------------------------------------------------------------*/

LIB3270_STRING_ARRAY * lib3270_string_array_new(void) {
	LIB3270_STRING_ARRAY * array = lib3270_malloc(sizeof(LIB3270_STRING_ARRAY));
	memset(array,0,sizeof(LIB3270_STRING_ARRAY));

	return array;
}

void lib3270_string_array_free(LIB3270_STRING_ARRAY *array) {
	size_t ix;

	if(array) {
		for(ix = 0; ix < array->length; ix++)
			lib3270_free((char *) array->str[ix]);

		lib3270_free(array->str);
		lib3270_free(array);
	}
}

static void lib3270_string_array_realloc(LIB3270_STRING_ARRAY *array) {
	if(array->str) {
		array->str = lib3270_realloc(array->str,(array->length + 1) * sizeof(char *));
	} else {
		array->str = lib3270_malloc(sizeof(char *));
		array->length = 0; // Just in case.
	}

}

void lib3270_string_array_append(LIB3270_STRING_ARRAY *array, const char *str) {
	lib3270_string_array_realloc(array);
	array->str[array->length++] = strdup(str);
}

void lib3270_string_array_append_with_length(LIB3270_STRING_ARRAY *array, const char *str, size_t length) {
	lib3270_string_array_realloc(array);

	char * buffer = lib3270_malloc(length+1);
	memcpy(buffer,str,length);
	buffer[length] = 0;

	array->str[array->length++] = buffer;
}

void lib3270_autoptr_cleanup_LIB3270_STRING_ARRAY(LIB3270_STRING_ARRAY **ptr) {
	lib3270_string_array_free(*ptr);
}

