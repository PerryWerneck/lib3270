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

	g_autoptr(GObject) object = G_OBJECT(tn3270_session_new());

	if(argc == 1) {

		g_object_set(
			object,
				"url","tn3270://127.0.0.1:3270",
				"tracefile","lib3270.trace",
				"logfile","lib3270.log",
				"screentrace", TRUE,
				"eventtrace", TRUE,
				"autoconnect", TRUE,
				"nettrace", TRUE,
				"ssltrace", TRUE,
				NULL
		);	

		g_message("Testprogram %s starts (%s)",argv[0],LIB3270_STRINGIZE_VALUE_OF(PRODUCT_NAME));
	
		tn3270_session_connect(object,5);

	} else {

		int c;
		while( (c = getopt(argc, argv, "u:t:l:ac")) != -1) {

			switch(c) {
			case 'u':	
				g_object_set(object,"url",optarg,NULL);
				break;

			case 't':
				g_object_set(object,"tracefile",optarg ? optarg : "lib3270.trace",NULL);
				break;

			case 'l':
				g_object_set(object,"logfile",optarg ? optarg : "lib3270.log",NULL);
				g_message("Testprogram %s starts (%s)",argv[0],LIB3270_STRINGIZE_VALUE_OF(PRODUCT_NAME));
				break;

			case 'a':
				g_object_set(object,"autoconnect",TRUE,NULL);
				break;

			case 'c':
				g_object_set(object,"autoconnect",FALSE,NULL);
				tn3270_session_connect(object,5);
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

