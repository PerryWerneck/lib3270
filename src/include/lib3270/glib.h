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

/**
 * Tn3270Session:
 *
 * A TN3270 terminal session.
 *
 * A session is created with tn3270_session_new() and starts disconnected.
 * tn3270_session_connect() opens the host connection described by the
 * session URL. Host toggles and the lib3270 properties are exposed as
 * #GObject properties, together with the connection state, associated LU,
 * URL, terminal model, TLS state, selection, program message and whether
 * the next paste is allowed.
 *
 * The underlying lib3270 handle is available from tn3270_session_get_h3270().
 * It is owned by the session.
 */
#define TN3270_TYPE_SESSION tn3270_session_get_type()
G_DECLARE_DERIVABLE_TYPE (Tn3270Session, tn3270_session, TN3270, SESSION, GObject)

/**
 * Tn3270SessionProperties:
 * @TN3270_SESSION_PROPERTY_NONE: No property.
 * @TN3270_SESSION_PROPERTY_CONNECTED: The session is connected to the host.
 * @TN3270_SESSION_PROPERTY_ASSOCIATED_LU: The associated logical unit name.
 * @TN3270_SESSION_PROPERTY_URL: The host URL.
 * @TN3270_SESSION_PROPERTY_MODEL_NUMBER: The terminal model.
 * @TN3270_SESSION_PROPERTY_SSL_STATE: The TLS state of the connection.
 * @TN3270_SESSION_PROPERTY_HAS_SELECTION: The screen selection.
 * @TN3270_SESSION_PROPERTY_PROGRAM_MESSAGE: The program message shown in the OIA.
 * @TN3270_SESSION_PROPERTY_CAN_PASTE_NEXT: Whether the next paste is allowed.
 * @TN3270_SESSION_PROPERTY_COUNT: Number of built-in properties.
 *
 * Identifiers for the built-in #Tn3270Session properties.
 */
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

/**
 * Tn3270SessionClass:
 * @parent_class: The parent class.
 * @toggle_changed: A toggle changed. Chain up so the property notification is emitted.
 * @connect_changed: The connection state changed. Chain up so the property notification is emitted.
 * @luname_changed: The associated LU name changed. Chain up so the property notification is emitted.
 * @url_changed: The connection URL changed. Chain up so the property notification is emitted.
 * @model_changed: The terminal model changed. Chain up so the property notification is emitted.
 * @ssl_changed: The TLS state changed. Chain up so the property notification is emitted.
 * @message_changed: The program message changed. Chain up so the property notification is emitted.
 * @selection_changed: The selection range changed. Chain up so the property notification is emitted.
 * @display: The screen should be redrawn.
 * @erase: The screen was erased.
 * @changed: A region of the screen changed.
 * @set_timer: The session became busy or idle. Chain up so the property notification is emitted.
 * @ring_bell: The terminal bell rang.
 *
 * Virtual methods of #Tn3270Session. Subclasses override these to follow
 * session activity. Chain up to the parent implementation for every method
 * that publishes a #GObject property.
 */
struct _Tn3270SessionClass {

	GObjectClass parent_class;

	/*< private >*/
	struct {
		void *authority;
		void *subject;
	} polkit;

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

	/*< public >*/
	void (*toggle_changed)(Tn3270Session *session, LIB3270_TOGGLE_ID id, unsigned char value, LIB3270_TOGGLE_TYPE reason, const char *name);
	void (*connect_changed)(Tn3270Session *session, unsigned char connected);
	void (*luname_changed)(Tn3270Session *session, const char *name);
	void (*url_changed)(Tn3270Session *session, const char *name);
	void (*model_changed)(Tn3270Session *session, const char *name, int model, int rows, int cols);
	void (*ssl_changed)(Tn3270Session *session, LIB3270_SSL_STATE state);
	void (*message_changed)(Tn3270Session *hSession, LIB3270_MESSAGE id);
	void (*selection_changed)(Tn3270Session *session, int start, int end);

	void (*display)(Tn3270Session *session);
	void (*erase)(Tn3270Session *session);
	void (*changed)(Tn3270Session *session, int offset, int len);

	void (*set_timer)(Tn3270Session *session, gboolean busy);
	void (*ring_bell)(Tn3270Session *session, int id);

	/*< private >*/
	gpointer padding[8];

};

GType tn3270_session_get_type(void);

/**
 * tn3270_session_new:
 *
 * Creates a new #Tn3270Session.
 *
 * The session is disconnected until tn3270_session_connect() is called.
 *
 * Returns: (transfer full): the new session
 */
Tn3270Session * tn3270_session_new(void);

/**
 * tn3270_session_get_h3270:
 * @session: a #Tn3270Session
 *
 * Gets the lib3270 handle owned by @session.
 *
 * Returns: (transfer none) (nullable): the session handle, or %NULL if
 * @session is not a #Tn3270Session
 */
H3270 * tn3270_session_get_h3270(GObject *session);

/**
 * tn3270_session_connect:
 * @session: a #Tn3270Session
 * @timeout: seconds to wait for the connection
 *
 * Connects @session to the host configured on the session.
 *
 * Returns: 0 on success. On failure a negative value is returned and errno is set.
 */
int tn3270_session_connect(GObject *session, int timeout);

/**
 * tn3270_session_disconnect:
 * @session: a #Tn3270Session
 *
 * Disconnects @session from the host and releases the connection.
 *
 * Returns: 0 on success. On failure a negative value is returned and errno is set.
 */
int tn3270_session_disconnect(GObject *session);

 G_END_DECLS
