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

 #pragma once

 #include <config.h>
 #include <lib3270/defs.h>
 #include <lib3270/toggle.h>
 #include <private/linkedlist.h>

 struct lib3270_toggle {
	char value;																		///< @brief toggle value
	void (*upcall)(H3270 *, const struct lib3270_toggle *, LIB3270_TOGGLE_TYPE);	///< @brief change value
 };

 struct lib3270_toggle_callback {
	LIB3270_LINKED_LIST_HEAD
	void (*func)(H3270 *, LIB3270_TOGGLE_ID, char, void *);		/**< @brief Function to call */
 };

 LIB3270_INTERNAL void initialize_toggles(H3270 *session);
 LIB3270_INTERNAL void shutdown_toggles(H3270 *session);
 LIB3270_INTERNAL const LIB3270_TOGGLE toggle_descriptor[LIB3270_TOGGLE_COUNT+1];
 LIB3270_INTERNAL void toggle_rectselect(H3270 *hSession, const struct lib3270_toggle *t, LIB3270_TOGGLE_TYPE tt);

