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
 #include <lib3270/glib.h>

 int main(int argc, char *argv[]) {

	GMainLoop *loop = g_main_loop_new(g_main_context_default(), FALSE);

	g_autoptr(Tn3270Session) hSession = tn3270_session_new();
	g_object_set(hSession, "autoconnect", TRUE, NULL);

	g_main_loop_run(loop);
	g_main_loop_unref(loop);

	return 0;
	
 }

