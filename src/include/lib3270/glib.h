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
 #include <lib3270/toggle.h>
 #include <lib3270/ssl.h>
 #include <glib-object.h>

 G_BEGIN_DECLS

 // Type declaration
 #define TN3270_TYPE_SESSION tn3270_session_get_type()
 G_DECLARE_DERIVABLE_TYPE (Tn3270Session, tn3270_session, TN3270, SESSION, GObject)

 enum Tn3270SessionProperties {
	TN3270_SESSION_PROPERTY_NONE,
	TN3270_SESSION_PROPERTY_CONNECTED,
	TN3270_SESSION_PROPERTY_ASSOCIATED_LU,
	TN3270_SESSION_PROPERTY_URL,
	TN3270_SESSION_PROPERTY_MODEL_NUMBER,
	TN3270_SESSION_PROPERTY_SSL_STATE,
	TN3270_SESSION_PROPERTY_HAS_SELECTION,
	TN3270_SESSION_PROPERTY_PROGRAM_MESSAGE,
	TN3270_SESSION_PROPERTY_CAN_PASTE_NEXT,

	TN3270_SESSION_PROPERTY_COUNT
 };
 
 struct _Tn3270SessionClass {
	GObjectClass parent_class;

	struct {
		size_t count;
		GParamSpec * toggle[LIB3270_TOGGLE_COUNT];
		GParamSpec * specs[TN3270_SESSION_PROPERTY_COUNT];
		GParamSpec * timer;

		struct {
			size_t boolean;
			size_t integer;
			size_t uint;
			size_t str;
		} type;

	} properties;

	// Properties
 	void (*toggle_changed)(Tn3270Session *session, LIB3270_TOGGLE_ID id, unsigned char value, LIB3270_TOGGLE_TYPE reason, const char *name);
	void (*connect_changed)(Tn3270Session *session, unsigned char connected);
	void (*luname_changed)(Tn3270Session *session, const char *name);
 	void (*url_changed)(Tn3270Session *session, const char *name);
	void (*model_changed)(Tn3270Session *session, const char *name, int model, int rows, int cols);
	void (*ssl_changed)(Tn3270Session *session, LIB3270_SSL_STATE state);
 	void (*status_changed)(Tn3270Session *hSession, LIB3270_MESSAGE id);
	void (*selection_changed)(Tn3270Session *session, int start, int end);

	// Terminal contents
	void (*display)(Tn3270Session *session);
	void (*erase)(Tn3270Session *session);
	void (*changed)(Tn3270Session *session, int offset, int len);

	// States
	void (*set_timer)(Tn3270Session *session, gboolean busy);
	void (*ring_bell)(Tn3270Session *session, int id);

	gpointer padding[8];
	
 };

 LIB3270_EXPORT Tn3270Session * tn3270_session_new();
 LIB3270_EXPORT GType 			tn3270_session_get_type(void);

 LIB3270_EXPORT H3270 * tn3270_session_get_h3270(Tn3270Session *session);

 G_END_DECLS
