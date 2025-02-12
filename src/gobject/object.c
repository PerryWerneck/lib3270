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
 * SECTION: tn3270-session
 * @short_description: A tn3270 session object
 *
 * A tn3270 session object representing a connection to a tn3270 server.
 * 
 */

 #include <config.h>
 #include <private/gobject.h>
 #include <lib3270.h>
 #include <lib3270/properties.h>
 #include <lib3270/log.h>
 #include <lib3270/session.h>
 #include <lib3270/ssl.h>
 #include <glib-object.h>
 #include <glib-tn3270.h>
 #include <private/intl.h>

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
 static void tn3270_session_install_property(TN3270SessionClass *klass, guint property_id, GParamSpec *pspec);
 
 /// @brief Internal properties
 enum Properties {
	SESSION_PROPERTY_TIMER,

	SESSION_PROPERTY_CUSTOM
 };

 static void
 tn3270_session_class_init (TN3270SessionClass *klass)
 {
	size_t ix;

	debug("%s","Initializing TN3270SessionClass");

	{
		GObjectClass *object_class = G_OBJECT_CLASS (klass);
		object_class->dispose = tn3270_session_dispose;
		object_class->finalize = tn3270_session_finalize;
		object_class->set_property = tn3270_session_set_property;
		object_class->get_property = tn3270_session_get_property;
	}

	tn3270_session_class_setup_callbacks(klass);

	// Setup internal properties
	klass->properties.count = SESSION_PROPERTY_CUSTOM;
	{
		klass->properties.timer =
			g_param_spec_boolean(
				"timer",
				"timer",
				_("Timer is enabled, usually activates when session is busy"),
				FALSE,
				G_PARAM_READABLE
			);

		g_object_class_install_property(
			G_OBJECT_CLASS (klass), 
			SESSION_PROPERTY_TIMER,
			klass->properties.timer
		);

	}

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

			debug("Registering toggle property '%s' with id %lu and value %d", toggles[ix].name,klass->properties.count,toggles[ix].def);
			klass->properties.toggle[ix] =
				g_param_spec_boolean(
					toggles[ix].name,
					toggles[ix].name,
					toggles[ix].description,
					(toggles[ix].def == 0 ? FALSE : TRUE),
					G_PARAM_WRITABLE|G_PARAM_READABLE
				);

			tn3270_session_install_property(
				klass,
				klass->properties.count++,
				klass->properties.toggle[ix]
			);

		}

	}

	// Setup boolean properties
	{
		klass->properties.type.boolean = klass->properties.count;

		const LIB3270_INT_PROPERTY * props = lib3270_get_boolean_properties_list();

		for(ix = 0; props[ix].name; ix++)
		{
			debug("Registering boolean property: %s(%d)", props[ix].name,props[ix].default_value != 0);
			GParamSpec *spec = g_param_spec_boolean(
						props[ix].name,
						props[ix].name,
						props[ix].description,
						props[ix].default_value != 0,
						(props[ix].set == NULL ? G_PARAM_READABLE : (G_PARAM_READABLE|G_PARAM_WRITABLE))
			);

			tn3270_session_install_property(
				klass,
				klass->properties.count++,
				spec
			);

		}

	}

	// Setup signed integer properties
	{
		klass->properties.type.integer = klass->properties.count;

		const LIB3270_INT_PROPERTY * props = lib3270_get_int_properties_list();

		for(ix = 0; props[ix].name; ix++)
		{
			debug("Registering int property: %s(%d)", props[ix].name,0);
			GParamSpec *spec = g_param_spec_int(
				props[ix].name,
				props[ix].name,
				props[ix].description,
				0,			// Min
				INT_MAX,	// Máx
				0,			// Def
				(props[ix].set == NULL ? G_PARAM_READABLE : (G_PARAM_READABLE|G_PARAM_WRITABLE))
			);

			tn3270_session_install_property(
				klass,
				klass->properties.count++,
				spec
			);

		}

	}

	// Setup unsigned int properties
	{
		klass->properties.type.uint = klass->properties.count;

		const LIB3270_UINT_PROPERTY * props = lib3270_get_unsigned_properties_list();

		for(ix = 0; props[ix].name; ix++)
		{
			debug("Registering uint property: %s(%u)", props[ix].name,(props[ix].default_value ? props[ix].default_value : props[ix].min));
			GParamSpec *spec = g_param_spec_uint(
				props[ix].name,
				props[ix].name,
				props[ix].description,
				props[ix].min,															// Min
				(props[ix].max ? props[ix].max : UINT_MAX),								// Máx
				(props[ix].default_value ? props[ix].default_value : props[ix].min),	// Def
				(props[ix].set == NULL ? G_PARAM_READABLE : (G_PARAM_READABLE|G_PARAM_WRITABLE))
			);

			tn3270_session_install_property(
				klass,
				klass->properties.count++,
				spec
			);

		}

	}

	// Setup string properties
	{
		klass->properties.type.str = klass->properties.count;

		const LIB3270_STRING_PROPERTY * props = lib3270_get_string_properties_list();

		for(ix = 0; props[ix].name; ix++)
		{
			debug("Registering string property: %s(%s)", props[ix].name,(props[ix].default_value ? props[ix].default_value : ""));
			GParamSpec *spec = g_param_spec_string(
				props[ix].name,
				props[ix].name,
				props[ix].description,
				props[ix].default_value,
				(props[ix].set == NULL ? G_PARAM_READABLE : (G_PARAM_READABLE|G_PARAM_WRITABLE))
			);

			tn3270_session_install_property(
				klass,
				klass->properties.count++,
				spec
			);

		}

	}

 }

 static void
 tn3270_session_init (TN3270Session *gobject)
 {
	debug("%s","Initializing TN3270Session");
	
	TN3270SessionClass *klass = TN3270_SESSION_GET_CLASS(gobject);
	TN3270SessionPrivate *self = tn3270_session_get_instance_private(gobject);
	
	self->handler = lib3270_session_new("");
	lib3270_set_user_data(self->handler,gobject);

	if(tn3270_session_setup_callbacks(klass,self))
	{
		lib3270_session_free(self->handler);
		self->handler = NULL;
		return;
	}

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
	TN3270SessionPrivate *self = tn3270_session_get_instance_private(TN3270_SESSION(object));
	TN3270SessionClass *klass = TN3270_SESSION_GET_CLASS(object);

	debug("Setting property %s with id %u (max %lu)",pspec->name,prop_id,klass->properties.count);

	if(prop_id > klass->properties.count) 
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		return;
	}

	if(prop_id > klass->properties.type.str)
	{
		debug("String property %s set to %s",pspec->name,g_value_get_string(value));
		prop_id -= klass->properties.type.str;
		(lib3270_get_string_properties_list()+prop_id)->set(self->handler,g_value_get_string(value));
		return;
	}

	if(prop_id > klass->properties.type.uint)
	{
		debug("Unsigned int property %s set to %u",pspec->name,g_value_get_uint(value));
		prop_id -= klass->properties.type.uint;
		(lib3270_get_unsigned_properties_list()+prop_id)->set(self->handler,g_value_get_uint(value));
		return;
	}

	if(prop_id > klass->properties.type.integer)
	{
		debug("Int property %s set to %d",pspec->name,g_value_get_int(value));
		prop_id -= klass->properties.type.integer;
		(lib3270_get_int_properties_list()+prop_id)->set(self->handler,g_value_get_int(value));
		return;
	}

	if(prop_id > klass->properties.type.boolean)
	{
		debug("Boolean property %s set to %s",pspec->name,g_value_get_boolean(value) ? "true" : "false");
		prop_id -= klass->properties.type.boolean;
		(lib3270_get_boolean_properties_list()+prop_id)->set(self->handler,g_value_get_boolean(value));
		return;
	}

	if(prop_id > SESSION_PROPERTY_CUSTOM)
	{
		prop_id -= SESSION_PROPERTY_CUSTOM;
		debug("Toggle property %s (%u) set to %s",pspec->name,prop_id,g_value_get_boolean(value) ? "true" : "false");
		lib3270_set_toggle(self->handler,(lib3270_get_toggles()+prop_id)->id,g_value_get_boolean(value));
		return;
	}

	debug("Setting internal property %s",pspec->name);

 	return;
 }

 static void 
 tn3270_session_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
 {
	TN3270SessionPrivate *self = tn3270_session_get_instance_private(TN3270_SESSION(object));
	TN3270SessionClass *klass = TN3270_SESSION_GET_CLASS(object);

	debug("Getting property %s with id %u (max %lu)",pspec->name,prop_id,klass->properties.count);

	if(prop_id > klass->properties.count) 
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		return;
	}

	if(prop_id > klass->properties.type.str)
	{
		prop_id -= klass->properties.type.str;
	
		return;
	}

	if(prop_id > klass->properties.type.uint)
	{
		prop_id -= klass->properties.type.uint;

		return;
	}

	if(prop_id > klass->properties.type.integer)
	{
		prop_id -= klass->properties.type.integer;

		return;
	}

	if(prop_id > klass->properties.type.boolean)
	{
		prop_id -= klass->properties.type.boolean;

		return;
	}

	if(prop_id > SESSION_PROPERTY_CUSTOM)
	{
		prop_id -= SESSION_PROPERTY_CUSTOM;
		g_value_set_boolean(value,lib3270_get_toggle(self->handler,(lib3270_get_toggles()+prop_id)->id));
	}

	debug("Getting internal property %s",pspec->name);

 	return;
 }

 static void
 tn3270_session_install_property(TN3270SessionClass *klass, guint property_id, GParamSpec *pspec)
 {
	static const char *names[] = {
		"connected",
		"associated-lu",
		"url",
		"model-number",
		"ssl-state",
		"has-selection",
		"program-message",
		"can-paste-next"
	};

	debug("---> Installing property %s with id %u",pspec->name,property_id);

	g_object_class_install_property(G_OBJECT_CLASS (klass), property_id, pspec);

	size_t ix;
	for(ix = 0; ix < G_N_ELEMENTS(names); ix++)
	{
		if(strcmp(pspec->name,names[ix]) == 0)
		{
			klass->properties.specs[ix] = pspec;
			break;
		}
	}

 }

/**
 * tn3270_session_new:
 *
 * Creates a new tn3270 session object.
 *
 * Returns: (transfer none) (type tn3270.session): a new #TN3270Session object
 */
 LIB3270_EXPORT TN3270Session * tn3270_session_new() {
	return g_object_new(TN3270_TYPE_SESSION,NULL);
 }

