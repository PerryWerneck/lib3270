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

 #include <lib3270.h>
 #include <lib3270/glib.h>
 #include <lib3270/log.h>
 #include <lib3270/trace.h>


 int main(int argc, char *argv[]) {

	GMainLoop *loop = g_main_loop_new(g_main_context_default(), FALSE);

	g_autoptr(Tn3270Session) object = tn3270_session_new();

	H3270 * hSession = tn3270_session_get_h3270(object);

	lib3270_trace_open_file(hSession,"lib3270.trace");
	lib3270_log_open_file(hSession,"lib3270.log",86400);

	lib3270_log_write(hSession,"TEST","Testprogram %s starts (%s)",argv[0],LIB3270_STRINGIZE_VALUE_OF(PRODUCT_NAME));

	lib3270_set_toggle(hSession,LIB3270_TOGGLE_DS_TRACE,1);
	lib3270_set_toggle(hSession,LIB3270_TOGGLE_NETWORK_TRACE,1);
	lib3270_set_toggle(hSession,LIB3270_TOGGLE_EVENT_TRACE,1);
	lib3270_set_toggle(hSession,LIB3270_TOGGLE_SCREEN_TRACE,1);
	lib3270_set_toggle(hSession,LIB3270_TOGGLE_SSL_TRACE,1);
	
	lib3270_set_url(hSession,"tn3270://127.0.0.1:3270");
	lib3270_connect(hSession,5);

	g_main_loop_run(loop);
	g_main_loop_unref(loop);

	return 0;
	
 }

