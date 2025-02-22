/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright 1995, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006 by Paul Mattes.
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

/**
 *	@file array.h
 *	@brief Global declarations for array.c.
 */

#ifndef LIB3270_ARRAY_H_INCLUDED

#define LIB3270_ARRAY_H_INCLUDED

#include <stddef.h>
#include <lib3270.h>

typedef struct _lib3270_string_array {
	size_t length;	///< @brief Number of elements.
	const char **str;
} LIB3270_STRING_ARRAY;

LIB3270_INTERNAL LIB3270_STRING_ARRAY * lib3270_string_array_new(void);
LIB3270_INTERNAL void lib3270_string_array_free(LIB3270_STRING_ARRAY *object);
LIB3270_INTERNAL void lib3270_string_array_append(LIB3270_STRING_ARRAY *object, const char *str);
LIB3270_INTERNAL void lib3270_string_array_append_with_length(LIB3270_STRING_ARRAY *array, const char *str, size_t length);

LIB3270_INTERNAL void lib3270_autoptr_cleanup_LIB3270_STRING_ARRAY(LIB3270_STRING_ARRAY **ptr);

#endif // LIB3270_ARRAY_H_INCLUDED
