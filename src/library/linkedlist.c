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
 * @file linkedlist.c
 * @brief Implementation of linked list methods.
 *
 * This file contains the functions and data structures necessary to create,
 * manipulate, and manage linked lists. It provides basic operations such as
 * insertion and deletion within a linked list.
 */

#include <config.h>
#include <lib3270.h>
#include <lib3270/memory.h>
#include <lib3270/log.h>
#include <private/linkedlist.h>
#include <string.h>
#include <errno.h>

/*---[ Implement ]------------------------------------------------------------------------------------------------------------*/

void * lib3270_linked_list_append_node(lib3270_linked_list *head, size_t szBlock, void *userdata) {
	struct lib3270_linked_list_node * node = lib3270_malloc(szBlock);

	memset(node,0,szBlock);
	node->userdata = userdata;

	if(head->last) {
		head->last->next = node;
		node->prev = head->last;
	} else {
		head->first = node;
	}

	head->last = node;

	/*
	#ifdef DEBUG
	{
		struct lib3270_linked_list_node * dCurrent;

		debug("%s: head=%p first=%p last=%p", __FUNCTION__, head, head->first, head->last);

		for(dCurrent = head->first; dCurrent; dCurrent = dCurrent->next)
		{
			debug("node=%p prev=%p next=%p",dCurrent,dCurrent->prev,dCurrent->next);
		}

	}
	#endif // DEBUG
	*/

	return (void *) node;

}

int lib3270_linked_list_delete_node(lib3270_linked_list *head, const void *node) {

	struct lib3270_linked_list_node * current;

	for(current = head->first; current; current = current->next) {

		if(current == node) {

			if(current->prev)
				current->prev->next = current->next;
			else
				head->first = current->next;

			if(current->next)
				current->next->prev = current->prev;
			else
				head->last = current->prev;

			lib3270_free(current);

			/*
			#ifdef DEBUG
			{
				struct lib3270_linked_list_node * dCurrent;

				debug("%s: head=%p first=%p last=%p", __FUNCTION__, head, head->first, head->last);

				for(dCurrent = head->first; dCurrent; dCurrent = dCurrent->next)
				{
					debug("node=%p prev=%p next=%p",dCurrent,dCurrent->prev,dCurrent->next);
				}

			}
			#endif // DEBUG
			*/

			return 0;

		}

	}

	return errno = ENOENT;
}

void lib3270_linked_list_free(lib3270_linked_list *head) {
	struct lib3270_linked_list_node * node = head->first;

	while(node) {
		void * ptr = (void *) node;
		node = node->next;
		lib3270_free(ptr);
	}

	head->first = head->last = NULL;

}

