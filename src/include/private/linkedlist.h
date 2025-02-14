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

/**
 * @file linkedlist.h
 * @brief Global declarations for linkedlist.c.
 *
 * This header file contains the global declarations for the linked list
 * implementation. It provides the necessary structures, function prototypes,
 * and macros for managing linked lists.
 */
 
#ifndef LIB3270_LINKED_LIST_H_INCLUDED

#define LIB3270_LINKED_LIST_H_INCLUDED

#include <stddef.h>
#include <lib3270.h>

#define LIB3270_LINKED_LIST_HEAD	\
		struct lib3270_linked_list_node * prev; \
		struct lib3270_linked_list_node * next; \
		void * userdata;

#define LIB3270_LINKED_LIST	\
		struct lib3270_linked_list_node * first; \
		struct lib3270_linked_list_node * last;

struct lib3270_linked_list_node {
	LIB3270_LINKED_LIST_HEAD
};

typedef struct _lib3270_linked_list {
	LIB3270_LINKED_LIST
} lib3270_linked_list;

LIB3270_INTERNAL void	* lib3270_linked_list_append_node(lib3270_linked_list *head, size_t szBlock, void *userdata);
LIB3270_INTERNAL int	  lib3270_linked_list_delete_node(lib3270_linked_list *head, const void *node);
LIB3270_INTERNAL void	  lib3270_linked_list_free(lib3270_linked_list *head);

#endif // LIB3270_LINKED_LIST_H_INCLUDED
