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
	size_t ix;

	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	klass->properties.count = 1;

  	object_class->dispose = tn3270_session_dispose;
  	object_class->finalize = tn3270_session_finalize;
	object_class->set_property = tn3270_session_set_property;
  	object_class->get_property = tn3270_session_get_property;

	// Setup toggle properties
	{
		const LIB3270_TOGGLE * toggles = lib3270_get_toggles();

		for(ix = 0; ix < LIB3270_TOGGLE_COUNT; ix++)
		{
			if(!toggles[ix].name)
			{
				g_warning("Unexpected toggle id: %u", (unsigned int) ix);
				klass->properties.toggle[ix] = NULL;
				continue;
			}

			klass->properties.toggle[ix] =
				g_param_spec_boolean(
					toggles[ix].name,
					toggles[ix].name,
					toggles[ix].description,
					(toggles[ix].def == 0 ? FALSE : TRUE),
					G_PARAM_WRITABLE|G_PARAM_READABLE
				);

			g_object_class_install_property(object_class, klass->properties.count++, klass->properties.toggle[ix]);

		}

		// TODO: Watch toggle changes, emit property-changed signal.


	}

	// Setup boolean properties
	{
		klass->properties.type.boolean = klass->properties.count;

		const LIB3270_INT_PROPERTY * props = lib3270_get_boolean_properties_list();

		for(ix = 0; props[ix].name; ix++)
		{
			GParamSpec *spec = g_param_spec_boolean(
						props[ix].name,
						props[ix].name,
						props[ix].description,
						props[ix].default_value != 0,
						(props[ix].set == NULL ? G_PARAM_READABLE : (G_PARAM_READABLE|G_PARAM_WRITABLE))
			);

			g_object_class_install_property(object_class, klass->properties.count++, spec);

			// TODO: Watch property changes, emit property-changed signal.

		}

	}

	// Setup signed integer properties
	{
		klass->properties.type.integer = klass->properties.count;

		const LIB3270_INT_PROPERTY * props = lib3270_get_int_properties_list();

		for(ix = 0; props[ix].name; ix++)
		{
			GParamSpec *spec = g_param_spec_int(
				props[ix].name,
				props[ix].name,
				props[ix].description,
				0,			// Min
				INT_MAX,	// Máx
				0,			// Def
				(props[ix].set == NULL ? G_PARAM_READABLE : (G_PARAM_READABLE|G_PARAM_WRITABLE))
			);

			g_object_class_install_property(object_class, klass->properties.count++, spec);

			// TODO: Watch property changes, emit property-changed signal.

		}

	}

	// Setup unsigned int properties
	{
		klass->properties.type.uint = klass->properties.count;

		const LIB3270_UINT_PROPERTY * props = lib3270_get_unsigned_properties_list();

		for(ix = 0; props[ix].name; ix++)
		{
			GParamSpec *spec = g_param_spec_uint(
				props[ix].name,
				props[ix].name,
				props[ix].description,
				props[ix].min,															// Min
				(props[ix].max ? props[ix].max : UINT_MAX),								// Máx
				(props[ix].default_value ? props[ix].default_value : props[ix].min),	// Def
				(props[ix].set == NULL ? G_PARAM_READABLE : (G_PARAM_READABLE|G_PARAM_WRITABLE))
			);

			g_object_class_install_property(object_class, klass->properties.count++, spec);

			// TODO: Watch property changes, emit property-changed signal.
		}

	}

	// Setup string properties
	{
		klass->properties.type.str = klass->properties.count;

		const LIB3270_STRING_PROPERTY * props = lib3270_get_string_properties_list();

		for(ix = 0; props[ix].name; ix++)
		{
			GParamSpec *spec = g_param_spec_string(
				props[ix].name,
				props[ix].name,
				props[ix].description,
				props[ix].default_value,
				(props[ix].set == NULL ? G_PARAM_READABLE : (G_PARAM_READABLE|G_PARAM_WRITABLE))
			);

			g_object_class_install_property(object_class, klass->properties.count++, spec);

			// TODO: Watch property changes, emit property-changed signal.
		}

	}

	// TODO: Setup I/O Handlers

 }

 static void
 tn3270_session_init (TN3270Session *gobject)
 {
	TN3270SessionPrivate *self = tn3270_session_get_instance_private(gobject);
	self->handler = lib3270_session_new("");
	lib3270_set_user_data(self->handler,self);

 }

 static gboolean 
 delayed_cleanup(H3270 *hSession) 
 {
	if(lib3270_get_task_count(hSession))
		return G_SOURCE_CONTINUE;
	g_message("Delayed cleanup complete");
	lib3270_free(hSession);
	return G_SOURCE_REMOVE;
 }

 static void 
 tn3270_session_dispose(GObject *object)
 {
	TN3270SessionPrivate *self = tn3270_session_get_instance_private(TN3270_SESSION(object));

	if(self->handler) {

		lib3270_disconnect(self->handler);

		if(lib3270_get_task_count(self->handler))
		{
			// Should wait.
			g_message("TN3270 session is busy, delaying cleanup");
			lib3270_set_user_data(self->handler,NULL);
			g_idle_add((GSourceFunc) delayed_cleanup,self->handler);
		}
		else
		{
			lib3270_session_free(self->handler);
		}

		self->handler = NULL;
	}

	lib3270_free(self->handler);
	self->handler = NULL;


	G_OBJECT_CLASS (tn3270_session_parent_class)->dispose (object);
 }

 static void
 tn3270_session_finalize(GObject *object) {

	TN3270SessionPrivate *self = tn3270_session_get_instance_private(TN3270_SESSION(object));

	if(self->handler) {
		lib3270_disconnect(self->handler);
	}

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

