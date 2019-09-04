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
 * Este programa está nomeado como - e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */


/**
 *	@file toggles/listener.c
 *	@brief This module handles toggle listeners.
 */

#include <config.h>
#include <lib3270-internals.h>
#include <lib3270/toggle.h>
#include <lib3270/log.h>

#include <errno.h>
#include <sys/types.h>


/*---[ Implement ]------------------------------------------------------------------------------------------------------------*/

LIB3270_EXPORT const void * lib3270_register_toggle_listener(H3270 *hSession, LIB3270_TOGGLE tx, void (*func)(H3270 *, LIB3270_TOGGLE, char, void *),void *data)
{
	struct lib3270_toggle_callback *st;

    CHECK_SESSION_HANDLE(hSession);

	st 			= (struct lib3270_toggle_callback *) lib3270_malloc(sizeof(struct lib3270_toggle_callback));
	st->func	= func;
	st->data	= data;

	if (hSession->listeners.toggle.last[tx])
		hSession->listeners.toggle.last[tx]->next = st;
	else
		hSession->listeners.toggle.callbacks[tx] = st;

	hSession->listeners.toggle.last[tx] = st;

	return (void *) st;

}

LIB3270_EXPORT int lib3270_unregister_toggle_listener(H3270 *hSession, LIB3270_TOGGLE tx, const void *id)
{
	struct lib3270_toggle_callback *st;
	struct lib3270_toggle_callback *prev = (struct lib3270_toggle_callback *) NULL;

	for (st = hSession->listeners.toggle.callbacks[tx]; st != (struct lib3270_toggle_callback *) NULL; st = (struct lib3270_toggle_callback *) st->next)
	{
		if (st == (struct lib3270_toggle_callback *)id)
			break;

		prev = st;
	}

	if (st == (struct lib3270_toggle_callback *)NULL)
	{
		lib3270_write_log(hSession,"lib3270","Invalid call to (%s): %p wasnt found in the list",__FUNCTION__,id);
		return errno = ENOENT;
	}

	if (prev != (struct lib3270_toggle_callback *) NULL)
		prev->next = st->next;
	else
		hSession->listeners.toggle.callbacks[tx] = (struct lib3270_toggle_callback *) st->next;

	for(st = hSession->listeners.toggle.callbacks[tx]; st != (struct lib3270_toggle_callback *) NULL; st = (struct lib3270_toggle_callback *) st->next)
		hSession->listeners.toggle.last[tx] = st;

	lib3270_free((void *) id);

	return 0;

}
