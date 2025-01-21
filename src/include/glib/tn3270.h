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

 #pragma once


 #include <lib3270.h>
 #include <glib-object.h>

 G_BEGIN_DECLS

 // Type declaration
 #define TN3270_TYPE_SESSION tn3270_session_get_type()
 G_DECLARE_DERIVABLE_TYPE (TN3270Session, tn3270_session, TN3270, SESSION, GObject)
 
 struct _TN3270SessionClass {
	GObjectClass parent_class;

	gpointer padding[8];
	
 };

 LIB3270_EXPORT TN3270Session * tn3270_session_new();


 G_END_DECLS
