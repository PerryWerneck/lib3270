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

#include <internals.h>
#include <lib3270/log.h>
#include <private/trace.h>
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
