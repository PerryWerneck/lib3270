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
#include <internals.h>
#include <lib3270/toggle.h>
#include <lib3270/log.h>
#include <private/toggle.h>

#include <errno.h>
#include <sys/types.h>


/*---[ Implement ]------------------------------------------------------------------------------------------------------------*/

LIB3270_EXPORT const void * lib3270_register_toggle_listener(H3270 *hSession, LIB3270_TOGGLE_ID tx, void (*func)(H3270 *, LIB3270_TOGGLE_ID, char, void *),void *data) {
	struct lib3270_toggle_callback *st = (struct lib3270_toggle_callback *) lib3270_linked_list_append_node(&hSession->listeners.toggle[tx], sizeof(struct lib3270_toggle_callback), data);

	st->func = func;

	return (void *) st;

}

LIB3270_EXPORT int lib3270_unregister_toggle_listener(H3270 *hSession, LIB3270_TOGGLE_ID tx, const void *id) {
	return lib3270_linked_list_delete_node(&hSession->listeners.toggle[tx], id);
}

/**
 * @brief Register a function interested in a state change.
 *
 * @param hSession	Session handle.
 * @param tx		State ID
 * @param func		Callback
 * @param data		Data
 *
 * @return State change identifier.
 *
 */
LIB3270_EXPORT const void * lib3270_register_schange(H3270 *hSession, LIB3270_STATE tx, void (*func)(H3270 *, int, void *),void *data) {
	struct lib3270_state_callback * st = (struct lib3270_state_callback *) lib3270_linked_list_append_node(&hSession->listeners.state[tx], sizeof(struct lib3270_state_callback), data);

	st->func = func;

	return (void *) st;
}

LIB3270_EXPORT int lib3270_unregister_schange(H3270 *hSession, LIB3270_STATE tx, const void * id) {
	return lib3270_linked_list_delete_node(&hSession->listeners.state[tx], id);
}



