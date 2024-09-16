/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes paul.mattes@case.edu), de emulação de terminal 3270 para acesso a
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
 * programa;  se  não, escreva para a Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA, 02111-1307, USA
 *
 * Este programa está nomeado como actions.c e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

#include <internals.h>
#include <lib3270/log.h>
#include <lib3270/trace.h>
#include <lib3270/actions.h>
#include <utilc.h>

struct lib3270_action_callback {
	LIB3270_LINKED_LIST_HEAD
	void (*func)(H3270 *, void *);							/**< @brief Function to call */
};

/*---[ Implement ]------------------------------------------------------------------------------------------------------------*/

const LIB3270_ACTION * lib3270_action_get_by_name(const char *name) {
	const LIB3270_ACTION * actions = lib3270_get_actions();
	size_t f;

	for(f=0; actions[f].name; f++) {
		if(!strcasecmp(name,actions[f].name))
			return actions+f;
	}

	// Check only alphabetic and numeric (for compatibility)
	for(f=0; actions[f].name; f++) {
		if(!lib3270_compare_alnum(name,actions[f].name))
			return actions+f;
	}

	errno = ENOTSUP;
	return NULL;
}

const LIB3270_ACTION * lib3270_get_action(const char *name) {
	return lib3270_action_get_by_name(name);
}

LIB3270_EXPORT int lib3270_action_is_activatable(const LIB3270_ACTION *action, H3270 *hSession) {
	return action->activatable(hSession) > 0;
}

LIB3270_EXPORT int lib3270_action_activate(const LIB3270_ACTION *action, H3270 *hSession) {

	if(!action->activatable(hSession)) {
		lib3270_write_event_trace(hSession,"Action \"%s\" is disabled\n",action->name);
		return errno = EPERM;
	}

	lib3270_write_event_trace(hSession,"Activating action \"%s\"\n",action->name);

	return action->activate(hSession);

}

LIB3270_EXPORT int lib3270_activate_by_name(H3270 *hSession, const char *name) {
	const LIB3270_ACTION *action = lib3270_action_get_by_name(name);

	if(action)
		return lib3270_action_activate(action, hSession);

	return hSession->cbk.action(hSession,name);
}

LIB3270_EXPORT int lib3270_action(H3270 *hSession, const char *name) {
	return lib3270_activate_by_name(hSession,name);
}

LIB3270_EXPORT void lib3270_action_group_notify(H3270 *hSession, LIB3270_ACTION_GROUP group) {

	if(group < (sizeof(hSession->listeners.actions)/sizeof(hSession->listeners.actions[0]))) {
		struct lib3270_linked_list_node * node;

		for(node = hSession->listeners.actions[group].first; node; node = node->next) {
			((struct lib3270_action_callback *) node)->func(hSession,node->userdata);
		}

	}

}

LIB3270_EXPORT const void * lib3270_register_action_group_listener(H3270 *hSession, LIB3270_ACTION_GROUP group, void (*func)(H3270 *, void *),void *data) {

	if(group < (sizeof(hSession->listeners.actions)/sizeof(hSession->listeners.actions[0]))) {
		struct lib3270_action_callback *st = (struct lib3270_action_callback *) lib3270_linked_list_append_node(&hSession->listeners.actions[group], sizeof(struct lib3270_action_callback), data);
		st->func = func;
		return (void *) st;
	}

	return NULL;
}

LIB3270_EXPORT int lib3270_unregister_action_group_listener(H3270 *hSession, LIB3270_ACTION_GROUP group, const void *id) {
	if(group < (sizeof(hSession->listeners.actions)/sizeof(hSession->listeners.actions[0])))
		return lib3270_linked_list_delete_node(&hSession->listeners.actions[group], id);

	return errno = EINVAL;
}
