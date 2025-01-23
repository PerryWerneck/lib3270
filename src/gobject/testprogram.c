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
 #include <glib.h>
 #include <glib-object.h>
 #include <glib-tn3270.h>

 static GMainLoop *loop = NULL;
 static GMainContext *context = NULL;

 int main(int argc, char *argv[]) {

	context = g_main_context_new();
	loop = g_main_loop_new(context, FALSE);

	g_autoptr(TN3270Session) session = tn3270_session_new();

	g_object_set(session, "autoconnect", TRUE, NULL);

	g_main_loop_run(loop);

	g_main_loop_unref(loop);
	g_main_context_unref(context);

	return 0;
	
 }

