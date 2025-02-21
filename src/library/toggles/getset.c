/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2025 Perry Werneck <perry.werneck@gmail.com>
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
 *	@file toggles/getset.c
 *	@brief This module handles toggle changes and properties.
 */

#include <config.h>
#include <internals.h>
#include <lib3270/toggle.h>
#include <lib3270/log.h>
#include <private/toggle.h>

/*---[ Implement ]------------------------------------------------------------------------------------------------------------*/

LIB3270_EXPORT unsigned char lib3270_get_toggle(const H3270 *hSession, LIB3270_TOGGLE_ID ix) {

	if(ix < 0 || ix >= LIB3270_TOGGLE_COUNT) {
		errno = EINVAL;
		return 0;
	}

	return hSession->toggle[ix].value != 0;
}

/**
 * @brief Call the internal update routine and listeners.
 */
static void toggle_notify(H3270 *session, struct lib3270_toggle *t, LIB3270_TOGGLE_ID ix) {

	debug("%s: ix=%d upcall=%p cbk=%p",__FUNCTION__,ix,t->upcall,session->cbk.update_toggle);

	t->upcall(session, t, LIB3270_TOGGLE_TYPE_INTERACTIVE);

	session->cbk.update_toggle(session,ix,t->value,LIB3270_TOGGLE_TYPE_INTERACTIVE,toggle_descriptor[ix].name);

	// Notify customers.
	struct lib3270_linked_list_node * node;
	for(node = session->listeners.toggle[ix].first; node; node = node->next) {
		((struct lib3270_toggle_callback *) node)->func(session, ix, t->value, node->userdata);
	}

}

/**
 * @brief Set the state of a specified toggle.
 *
 * This function sets the state of a toggle identified by its index. If the
 * toggle is already in the desired state, the function returns 0. If the
 * toggle state is changed, the function returns 1. In case of an error,
 * such as an invalid toggle index, the function returns a negative value
 * and sets the errno accordingly.
 *
 * @param h		Session handle.
 * @param ix	Toggle index.
 * @param value	New toggle state (non zero for true).
 *
 * @returns 0 if the toggle is already at the state, 1 if the toggle was changed; < 0 on error (sets errno).
 * @retval -EINVAL Invalid toggle index.
 * 
 */
LIB3270_EXPORT int lib3270_set_toggle(H3270 *session, LIB3270_TOGGLE_ID ix, int value) {

	char v = value ? True : False;

	struct lib3270_toggle * t;

	if(ix < 0 || ix >= LIB3270_TOGGLE_COUNT)
		return -(errno = EINVAL);

	t = &session->toggle[ix];

	if(v == t->value)
		return 0;

	t->value = v;

	toggle_notify(session,t,ix);
	return 1;
}

/**
 * @brief Gets the status of a specific toggle in the given 3270 session.
 *
 * This function retrieves the status of the toggle identified by the given
 * toggle index for the specified 3270 session.
 *
 * @param session A pointer to the H3270 session structure.
 * @param ix The toggle index of the setting to be retrieved.
 * @return An integer indicating the status of the toggle.
 *         Returns 1 if the toggle is enabled, 0 if it is disabled, or a
 *         negative error code on failure.
 * @retval -EINVAL The specified toggle index is invalid.
 */
LIB3270_EXPORT int lib3270_toggle(H3270 *session, LIB3270_TOGGLE_ID ix) {
	struct lib3270_toggle	*t;

	if(ix < 0 || ix >= LIB3270_TOGGLE_COUNT) {
		return -(errno = EINVAL);
	}

	t = &session->toggle[ix];

	t->value = t->value ? False : True;
	toggle_notify(session,t,ix);

	return (int) t->value;
}

LIB3270_EXPORT const char * lib3270_toggle_get_name(const LIB3270_TOGGLE *toggle) {

	if(toggle && toggle->name)
		return dgettext(GETTEXT_PACKAGE,toggle->name);

	return "";

}

LIB3270_EXPORT const char * lib3270_toggle_get_label(const LIB3270_TOGGLE *toggle) {

	if(toggle && toggle->label)
		return dgettext(GETTEXT_PACKAGE,toggle->label);

	return "";

}

LIB3270_EXPORT const char * lib3270_toggle_get_summary(const LIB3270_TOGGLE *toggle) {

	if(toggle && toggle->summary)
		return dgettext(GETTEXT_PACKAGE,toggle->summary);

	return "";

}

LIB3270_EXPORT const char * lib3270_toggle_get_description(const LIB3270_TOGGLE *toggle) {

	if(toggle && toggle->description)
		return dgettext(GETTEXT_PACKAGE,toggle->description);

	return "";

}
