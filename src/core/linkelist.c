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
 * Este programa está nomeado como linkedlist.c e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */


/**
 * @brief Handle linked lists.
 */

 #include <lib3270.h>
 #include <lib3270/log.h>
 #include <linkedlist.h>
 #include <string.h>

/*---[ Implement ]------------------------------------------------------------------------------------------------------------*/

void * lib3270_linked_list_append_node(struct lib3270_linked_list_head *head, size_t szBlock, void *userdata)
{
	struct lib3270_linked_list_node * node = lib3270_malloc(szBlock);

	memset(node,0,szBlock);
	node->userdata = userdata;

	if(head->last)
	{
        head->last->next = node;
        node->prev = head->last;
	}
	else
	{
		head->first = node;
	}

	head->last = node;

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

	return (void *) node;

}

int lib3270_linked_list_delete_node(struct lib3270_linked_list_head *head, const void *node)
{

	struct lib3270_linked_list_node * current;

	for(current = head->first;current;current = current->next)
	{

		if(current == node)
		{

			if(current->prev)
				current->prev->next = current->next;
			else
				head->first = current->next;

			if(current->next)
				current->next->prev = current->prev;
			else
				head->last = current->prev;

			lib3270_free(current);

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

			return 0;

		}

	}

	return errno = ENOENT;
}

void lib3270_linked_list_free(struct lib3270_linked_list_head *head)
{
	struct lib3270_linked_list_node * node = head->first;

	while(node)
	{
		void * ptr = (void *) node;
		node = node->next;
		lib3270_free(ptr);
	}

	head->first = head->last = NULL;

}

