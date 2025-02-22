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
 #include <getopt.h>

 static GMainLoop *loop = NULL;

 #ifdef HAVE_SIGNAL_H
 #include <signal.h>
 static void handle_signal(int sig) {
	if(loop) {
		g_main_loop_quit(loop);
	}
 }
 #endif // HAVE_SIGNAL_H

 int main(int argc, char *argv[]) {

	loop = g_main_loop_new(g_main_context_default(), FALSE);

	g_autoptr(Tn3270Session) object = tn3270_session_new();

	H3270 * hSession = tn3270_session_get_h3270(object);

	if(argc == 1) {

		lib3270_set_url(hSession,"tn3270://127.0.0.1:3270");
	
		lib3270_trace_open_file(hSession,"lib3270.trace");
		lib3270_log_open_file(hSession,"lib3270.log",86400);
	
		lib3270_log_write(hSession,"TEST","Testprogram %s starts (%s)",argv[0],LIB3270_STRINGIZE_VALUE_OF(PRODUCT_NAME));
	
		lib3270_set_toggle(hSession,LIB3270_TOGGLE_DS_TRACE,1);
		lib3270_set_toggle(hSession,LIB3270_TOGGLE_NETWORK_TRACE,1);
		lib3270_set_toggle(hSession,LIB3270_TOGGLE_EVENT_TRACE,1);
		lib3270_set_toggle(hSession,LIB3270_TOGGLE_SCREEN_TRACE,1);
		lib3270_set_toggle(hSession,LIB3270_TOGGLE_SSL_TRACE,1);
		
		lib3270_set_toggle(hSession,LIB3270_TOGGLE_CONNECT_ON_STARTUP,1);
		
		lib3270_connect(hSession,5);
	
	} else {

		int c;
		while( (c = getopt(argc, argv, "u:t:l:ac")) != -1) {

			switch(c) {
			case 'u':
				lib3270_set_url(hSession,optarg);
				break;

			case 't':
				lib3270_trace_open_file(hSession,optarg ? optarg : "lib3270.trace");
				break;

			case 'l':
				lib3270_log_open_file(hSession,optarg ? optarg : "lib3270.log",86400);
				lib3270_log_write(hSession,"TEST","Testprogram %s starts (%s)",argv[0],LIB3270_STRINGIZE_VALUE_OF(PRODUCT_NAME));
				break;

			case 'a':
				lib3270_set_toggle(hSession,LIB3270_TOGGLE_CONNECT_ON_STARTUP,1);
				break;

			case 'c':
				lib3270_set_toggle(hSession,LIB3270_TOGGLE_CONNECT_ON_STARTUP,0);
				lib3270_connect(hSession,5);
				break;
			}

		}
	}

	#ifdef HAVE_SIGNAL_H
		signal(SIGINT,handle_signal);
		signal(SIGTERM,handle_signal);
		signal(SIGQUIT,handle_signal);
		signal(SIGHUP,handle_signal);
	#endif // HAVE_SIGNAL_H

	g_main_loop_run(loop);
	g_main_loop_unref(loop);

	return 0;
	
 }

