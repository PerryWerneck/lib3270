/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2008 Banco do Brasil S.A.
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

/*
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */
 
 #include <config.h>
 #include <stdio.h>
 #include <iconv.h>
 #include <lib3270/session.h>
 #include <lib3270/actions.h>
 #include <lib3270/memory.h>
 #include <lib3270/contents.h>
 #include <lib3270/charset.h>

 #include <private/session.h>

 static char enabled = 1;

 static void run(H3270 *hSession) {
#ifdef _WIN32
	BOOL bRet;
	MSG msg;
	while( (bRet = GetMessage( &msg, NULL, 0, 0 )) != 0) { 
		if (bRet == -1) {
			exit(-1);
		} else {
			TranslateMessage(&msg); 
			DispatchMessage(&msg); 
		}
	} 
#else
	while(enabled) {
		if(lib3270_mainloop_run(hSession,1) < 0) {
			break;
		}
	}
#endif

 }

 static int test_charset(H3270 *hSession, const char *charset) {

	printf("\n------------------------- Testing charset: %s\n",charset);
	int rc = lib3270_set_display_charset(hSession,charset);
	if(rc) {
		printf("lib3270_set_display_charset(%s) failed with rc=%d\n",charset,rc);
		return rc;
	}

	lib3270_testpattern(hSession);

	unsigned int cols = lib3270_get_width(hSession);

	lib3270_autoptr(LIB3270_ICONV) iconv = lib3270_iconv_new(charset,"UTF-8");

	for(unsigned int row = 0; row < lib3270_get_height(hSession); row++) {

		int baddr = lib3270_translate_to_address(hSession,row+1,1);
		lib3270_autoptr(char) line = lib3270_get_string_at_address(hSession,baddr,cols,0);
		lib3270_autoptr(char) text = lib3270_iconv_from_host(iconv, line, cols);

		printf("%03d %05d |%s|\n",row,baddr,text);

	}

	return 0;

 }

 static int test_connect(H3270 *hSession, const char *host) {

	lib3270_set_url(hSession,host);
	int rc = lib3270_connect(hSession,30);

 	if(rc) {
		printf("lib3270_connect(%s) failed with rc=%d\n",host,rc);
		return rc;
	}

	run(hSession);

	return 0;
 }

 int main(int argc, const char *argv[]) {

#ifdef _WIN32
	{
		WSADATA WSAData;
		WSAStartup(MAKEWORD(2,2), &WSAData);
	}
#endif // _WIN32

	lib3270_autoptr(H3270) hSession = lib3270_session_new("2",0);
	lib3270_set_toggle(hSession,LIB3270_TOGGLE_DS_TRACE,1);
	lib3270_set_toggle(hSession,LIB3270_TOGGLE_NETWORK_TRACE,1);
	lib3270_set_toggle(hSession,LIB3270_TOGGLE_EVENT_TRACE,1);
	lib3270_set_toggle(hSession,LIB3270_TOGGLE_SCREEN_TRACE,1);
	lib3270_set_toggle(hSession,LIB3270_TOGGLE_SSL_TRACE,1);

	lib3270_trace_open_file(hSession,"lib3270.trace");
	lib3270_log_open_file(hSession,"lib3270.log",86400);

	// test_charset(hSession,"ISO-8859-1");
	test_connect(hSession,"tn3270://_not_found_:3270");

	return 0;
 }


 /*
 #include <config.h>
 #include <lib3270/defs.h>
 #include <lib3270.h>
 #include <lib3270/session.h>
 #include <private/session.h>
 #include <lib3270/trace.h>
 #include <lib3270/log.h>
 #include <private/network.h>
 #include <stdio.h>
 #include <getopt.h>

 #ifdef HAVE_SIGNAL_H
 	#include <signal.h>
 #endif // HAVE_SIGNAL_H
 
 #if defined(HAVE_SIGNAL_H) && !defined(_WIN32)
 static int enabled = 1;
 static void handle_signal(int sig) {
 	enabled = 0;
 }
 #endif // HAVE_SIGNAL_H

 int main(int argc, char *argv[]) {

#ifdef _WIN32
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2,2), &WSAData);
#endif // _WIN32

	lib3270_autoptr(H3270) hSession = lib3270_session_new("2",0);

	if(argc == 1) {

		// lib3270_set_url(hSession,"tn3270s://127.0.0.1:4270");
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

#if defined(HAVE_SIGNAL_H) && !defined(_WIN32)
		signal(SIGINT,handle_signal);
		signal(SIGTERM,handle_signal);
		signal(SIGQUIT,handle_signal);
		signal(SIGHUP,handle_signal);
#endif // HAVE_SIGNAL_H
   

	debug("Exiting %s",argv[0]);
	return 0;

 }
*/


/*
#include <config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <internals.h>
#include <lib3270.h>
#include <lib3270/actions.h>
#include <lib3270/trace.h>
#include <lib3270/toggle.h>
#include <lib3270/log.h>
#include <lib3270/properties.h>
#include <lib3270/charset.h>
#include <stdio.h>
#include <locale.h>

#ifdef HAVE_LIBINTL
	#include <libintl.h>
#endif // HAVE_LIBINTL

#define MAX_ARGS 10

const char *trace_file = "test.trace";

static void online_group_state_changed(H3270 GNUC_UNUSED(*hSession), void GNUC_UNUSED(*dunno)) {
	printf("\n\n%s\n\n",__FUNCTION__);
}

static void reconnect_test(H3270 *hSession) {

	lib3270_reconnect(hSession,0);

	int rc = lib3270_wait_for_ready(hSession,10);

	printf("\n\nlib3270_wait_for_ready exits with rc %d (%s)\n\n",rc,strerror(rc));

}

int main(int argc, char *argv[]) {
#ifdef _WIN32
	debug("Process %s running on pid %u\n",argv[0],(unsigned int) GetCurrentProcessId());
#endif // _WIN32

#ifdef LC_ALL
	setlocale( LC_ALL, "" );
#endif

	// textdomain("lib3270");


//	#pragma GCC diagnostic push
//	#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
	static struct option options[] = {
		{ "crl",					required_argument,	0,	'C' },
		{ "disable-crl-download",	no_argument,		0,	'D' },
		{ "url",					required_argument,	0,	'U' },
		{ "tracefile",				required_argument,	0,	't' },
		{ "reconnect",				no_argument,		0,	'r' },

		{ 0, 0, 0, 0}

	};

	H3270		* h		= lib3270_session_new("");
	int			  rc	= 0;

	lib3270_set_log_filename(h,"testprogram.log");
	lib3270_log_write(h,"TEST","Testprogram %s starts (%s)",argv[0],LIB3270_STRINGIZE_VALUE_OF(PRODUCT_NAME));

	lib3270_autoptr(char) version_info = lib3270_get_version_info();
	printf("3270 session %p created\n%s\n]",h,version_info);

#ifdef HAVE_LDAP
	lib3270_crl_set_preferred_protocol(h,"ldap");
#endif // HAVE_LDAP

	lib3270_ssl_set_crl_download(h,0);

	if(lib3270_set_url(h,NULL))
		lib3270_set_url(h,"tn3270://127.0.0.1");
	//lib3270_set_url(h,"tn3270://localhost:3270");

	int long_index =0;
	int opt;
	while((opt = getopt_long(argc, argv, "C:U:t:r", options, &long_index )) != -1) {
		switch(opt) {
		case 'U':
			lib3270_set_url(h,optarg);
			break;

		case 'D':
			lib3270_ssl_set_crl_download(h,0);
			break;

//		case 'C':
//			lib3270_crl_set_url(h,optarg);
//			break;

		case 'r':
			reconnect_test(h);
			return 0;

		case 't':
			lib3270_set_trace_filename(h,optarg);
			lib3270_set_toggle(h,LIB3270_TOGGLE_DS_TRACE,1);
			break;
		}

	}

//	printf("HOST URL: %s\tHOST CRL: %s\n",lib3270_get_url(h),lib3270_crl_get_url(h));

#ifdef _WIN32
	{
		lib3270_autoptr(char) apppath = lib3270_get_installation_path();
		printf("Application path: \"%s\"\n",apppath);
	}
#endif // _WIN32

	{
		lib3270_autoptr(char) datafile = lib3270_build_data_filename("data","file.txt",NULL);
		printf("Datafile: \"%s\"\n",datafile);
	}

	//lib3270_set_toggle(h,LIB3270_TOGGLE_DS_TRACE,1);
	lib3270_set_toggle(h,LIB3270_TOGGLE_SSL_TRACE,1);

	printf("\nConnecting to %s\n",lib3270_get_url(h));

	const void * online_listener = lib3270_register_action_group_listener(h,LIB3270_ACTION_GROUP_ONLINE,online_group_state_changed,NULL);

	rc = lib3270_reconnect(h,0);
	printf("\n\nConnect exits with rc=%d (%s)\n\n",rc,strerror(rc));

	if(!rc) {
		rc = lib3270_wait_for_cstate(h,LIB3270_CONNECTED_TN3270E, 60);
		printf("\n\nWait for LIB3270_CONNECTED_TN3270E exits with rc=%d (%s)\n\n",rc,strerror(rc));
	}

	if(!rc) {
		rc = lib3270_wait_for_ready(h,60);
		printf("\n\nWait for ready exits with rc=%d (%s)\n\n",rc,strerror(rc));
	}

	if(!rc) {

		lib3270_wait_for_ready(h,10);


		lib3270_enter(h);
		lib3270_wait(h,5);

		{
			lib3270_autoptr(char) text = lib3270_get_string_at_address(h,0,-1,'\n');
			if(text)
				printf("Screen:\n[%s]\n",text);

			lib3270_wait(h,2);

		}

		{
			printf("\n\nSet field exits with %d\n",lib3270_set_field(h,"aaa",-1));
			printf("\n\nSet field exits with %d\n",lib3270_set_field(h,"bbb",-1));
			printf("\n\nSet field exits with %d\n",lib3270_set_field(h,"ccc",-1));

			lib3270_autoptr(char) text = lib3270_get_string_at_address(h,0,-1,'\n');
			if(text)
				printf("Screen:\n[%s]\n",text);
		}

		lib3270_disconnect(h);

	}

	lib3270_unregister_action_group_listener(h,LIB3270_ACTION_GROUP_ONLINE,online_listener);

	lib3270_disconnect(h);

	{
		const LIB3270_PROPERTY * property = lib3270_property_get_by_name("model-number");

		debug("Model-number=%p",property);

	}

	lib3270_session_free(h);

	return 0;
}
*/
