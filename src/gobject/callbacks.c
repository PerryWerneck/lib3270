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

 #include <config.h>
 #include <lib3270.h>
 #include <lib3270/properties.h>
 #include <lib3270/log.h>
 #include <lib3270/popup.h>
 #include <lib3270/session.h>
 #include <glib-object.h>
 #include <glib/tn3270.h>
 #include <private/intl.h>
 #include <private/gobject.h>
 #include <lib3270/mainloop.h>
 #include <errno.h>

 /// @brief Callback called when a toggle property changes.
 static void handle_toggle_changed(H3270 *session, LIB3270_TOGGLE_ID id, unsigned char value, LIB3270_TOGGLE_TYPE reason, const char *name)
 {
	TN3270Session *self = (TN3270Session *) lib3270_get_user_data(session);
	TN3270SessionClass *klass = TN3270_SESSION_GET_CLASS(self);
	klass->toggle_changed(self,id,value,reason,name);
 }

 /// @brief Callback called when connect state changes.
 static void handle_connect_changed(H3270 *session, unsigned char connected)
 {
	TN3270Session *self = (TN3270Session *) lib3270_get_user_data(session);
	TN3270SessionClass *klass = TN3270_SESSION_GET_CLASS(self);
	klass->connect_changed(self,connected);
 }

 /// @brief Callback called when associated lu changes
 static void handle_luname_changed(H3270 *session, const char *name)
 {
	TN3270Session *self = (TN3270Session *) lib3270_get_user_data(session);
	TN3270SessionClass *klass = TN3270_SESSION_GET_CLASS(self);
	klass->luname_changed(self,name);
 }

 /// @brief Callback called when the connection URL changes.
 static void handle_url_changed(H3270 *session, const char *name)
 {
	TN3270Session *self = (TN3270Session *) lib3270_get_user_data(session);
	TN3270SessionClass *klass = TN3270_SESSION_GET_CLASS(self);
	klass->url_changed(self,name);
 }

 /// @brief Callback called when the model changes.
 static void handle_model_changed(H3270 *session, const char *name, int model, int rows, int cols)
 {
	TN3270Session *self = (TN3270Session *) lib3270_get_user_data(session);
	TN3270SessionClass *klass = TN3270_SESSION_GET_CLASS(self);
	klass->model_changed(self,name,model,rows,cols);
 }

 /// @brief Callback called when the SSL state changes.
 static void handle_ssl_changed(H3270 *session, G_GNUC_UNUSED LIB3270_SSL_STATE state) 
 {
	TN3270Session *self = (TN3270Session *) lib3270_get_user_data(session);
	TN3270SessionClass *klass = TN3270_SESSION_GET_CLASS(self);
	klass->ssl_changed(self,state);
 }
 
 static void handle_set_timer(H3270 *session, unsigned char on)
 {
	TN3270Session *self = (TN3270Session *) lib3270_get_user_data(session);
	TN3270SessionClass *klass = TN3270_SESSION_GET_CLASS(self);
	klass->set_timer(self,(gboolean) on);
 }
 
 static void handle_changed(H3270 *session, int offset, int len)
 {
	TN3270Session *self = (TN3270Session *) lib3270_get_user_data(session);
	TN3270SessionClass *klass = TN3270_SESSION_GET_CLASS(self);
	klass->changed(self,offset,len);
 }

 static void handle_selection_changed(H3270 *session, int start, int end)
 {
	TN3270Session *self = (TN3270Session *) lib3270_get_user_data(session);
	TN3270SessionClass *klass = TN3270_SESSION_GET_CLASS(self);
	klass->selection_changed(self,start,end);
 }

 static void toggle_changed(TN3270Session *session, LIB3270_TOGGLE_ID id, unsigned char value, LIB3270_TOGGLE_TYPE reason, const char *name)
 {
 	g_object_notify_by_pspec(
		G_OBJECT(session), 
		TN3270_SESSION_GET_CLASS(session)->properties.toggle[id]
	);
 }

 static void connect_changed(TN3270Session *session, unsigned char connected)
 {
 	g_object_notify_by_pspec(
		G_OBJECT(session), 
		TN3270_SESSION_GET_CLASS(session)->properties.specs[TN3270_SESSION_PROPERTY_CONNECTED]
	);
 }

 static void luname_changed(TN3270Session *session, const char *name)
 {
 	g_object_notify_by_pspec(
		G_OBJECT(session), 
		TN3270_SESSION_GET_CLASS(session)->properties.specs[TN3270_SESSION_PROPERTY_ASSOCIATED_LU]
	);
 }
 
 static void url_changed(TN3270Session *session, const char *name)
 {
 	g_object_notify_by_pspec(
		G_OBJECT(session), 
		TN3270_SESSION_GET_CLASS(session)->properties.specs[TN3270_SESSION_PROPERTY_URL]
	);
 }
 
 static void model_changed(TN3270Session *session, const char *name, int model, int rows, int cols)
 {
 	g_object_notify_by_pspec(
		G_OBJECT(session), 
		TN3270_SESSION_GET_CLASS(session)->properties.specs[TN3270_SESSION_PROPERTY_MODEL_NUMBER]
	);
 }
 
 static void ssl_changed(TN3270Session *session, LIB3270_SSL_STATE state)
 {
 	g_object_notify_by_pspec(
		G_OBJECT(session), 
		TN3270_SESSION_GET_CLASS(session)->properties.specs[TN3270_SESSION_PROPERTY_SSL_STATE]
	);
 }

 static void set_timer(TN3270Session *session, gboolean busy)
 {
	g_object_notify_by_pspec(
		G_OBJECT(session), 
		TN3270_SESSION_GET_CLASS(session)->properties.timer
	);
 }

 static void nop_void() {
 }

 void tn3270_session_class_setup_callbacks(TN3270SessionClass *klass)
 {
 	klass->toggle_changed = toggle_changed;
 	klass->connect_changed = connect_changed;
 	klass->luname_changed = luname_changed;
 	klass->url_changed = url_changed;
 	klass->model_changed = model_changed;
 	klass->ssl_changed = ssl_changed;
	klass->set_timer = set_timer;
	klass->changed = nop_void;
	klass->selection_changed = nop_void;
 }

 int tn3270_session_setup_callbacks(TN3270SessionClass *klass, TN3270SessionPrivate *self)
 {
	struct lib3270_session_callbacks *cbk = lib3270_get_session_callbacks(self->handler,G_STRINGIFY(LIB3270_REVISION),sizeof(struct lib3270_session_callbacks));
	if(!cbk)
	{
		if(g_ascii_strcasecmp(RPQ_REVISION,lib3270_get_revision()))
		{
				lib3270_popup_dialog(
					self->handler, 
					LIB3270_NOTIFY_CRITICAL, 
					_("Initialization error"), 
					_("Version mismatch"), 
					_("Invalid callback table, the release %s of %s can't be used (expecting revision %s)"),
						lib3270_get_revision(),
						PACKAGE_NAME,
						RPQ_REVISION
				);

				g_error(
					_("Invalid callback table, the release %s of %s can't be used (expecting revision %s)"),
					lib3270_get_revision(),
					PACKAGE_NAME,
					RPQ_REVISION
				);

		}
		else
		{
				lib3270_popup_dialog(
					self->handler, 
					LIB3270_NOTIFY_CRITICAL, 
					_("Initialization error"), 
					_("Version mismatch"), 
					_("Unexpected callback table, the release %s of lib%s is invalid"),
						lib3270_get_revision(),
						PACKAGE_NAME
				);

				g_error(
					_("Unexpected callback table, the release %s of lib%s is invalid"),
					lib3270_get_revision(),
					PACKAGE_NAME
				);

		}

		lib3270_session_free(self->handler);
		self->handler = NULL;
		return EINVAL;

	}

	cbk->update_toggle = handle_toggle_changed;
	cbk->update_connect = handle_connect_changed;
	cbk->update_luname = handle_luname_changed;
	cbk->update_ssl = handle_ssl_changed;
	cbk->update_model = handle_model_changed;
	cbk->update_url = handle_url_changed;
	cbk->set_timer = handle_set_timer;
	cbk->changed = handle_changed;
	cbk->update_selection = handle_selection_changed;

	return 0;
 }
