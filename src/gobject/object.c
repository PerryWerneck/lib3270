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
 #include <glib-object.h>
 #include <glib/tn3270.h>

 typedef struct _TN3270SessionPrivate {

	void *handler;

 } TN3270SessionPrivate;


 //
 // References:
 //
 //		// https://docs.gtk.org/gobject/tutorial.html
 //

 G_DEFINE_TYPE_WITH_PRIVATE (TN3270Session, tn3270_session, G_TYPE_OBJECT)

 static void tn3270_session_dispose(GObject *gobject);
 static void tn3270_session_finalize(GObject *gobject);
 static void tn3270_session_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
 static void tn3270_session_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

 static void
 tn3270_session_class_init (TN3270SessionClass *klass)
 {
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

  	object_class->dispose = tn3270_session_dispose;
  	object_class->finalize = tn3270_session_finalize;
	object_class->set_property = tn3270_session_set_property;
  	object_class->get_property = tn3270_session_get_property;

 }

 static void
 tn3270_session_init (TN3270Session *gobject)
 {
	TN3270SessionPrivate *self = tn3270_session_get_instance_private(gobject);
	self->handler = lib3270_session_new("");

 }

 static void 
 tn3270_session_dispose(GObject *object)
 {
	TN3270SessionPrivate *self = tn3270_session_get_instance_private(TN3270_SESSION(object));

	lib3270_free(self->handler);
	self->handler = NULL;


	G_OBJECT_CLASS (tn3270_session_parent_class)->dispose (object);
 }

 static void
 tn3270_session_finalize(GObject *object) {


	G_OBJECT_CLASS (tn3270_session_parent_class)->finalize (object);
 }

 static void 
 tn3270_session_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
 {
//	TN3270SessionPrivate *self = tn3270_session_get_instance_private(TN3270_SESSION(object));


	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
 }

 static void 
 tn3270_session_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
 {
//	TN3270SessionPrivate *self = tn3270_session_get_instance_private(TN3270_SESSION(object));


	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
 }

