/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270.
 *
 * Copyright (C) <2008> <Banco do Brasil S.A.>
 *
 * Este programa é software livre. Você pode redistribuí-lo e/ou modificá-lo sob
 * os termos da GPL v.2 - Licença Pública Geral  GNU,  conforme  publicado  pela
 * Free Software Foundation.
 *
 * Este programa é distribuído na expectativa de  ser  útil,  mas  SEM  QUALQUER
 * GARANTIA; sem mesmo a garantia implícita de COMERCIALIZAÇÃO ou  de  ADEQUAÇÃO
 * A QUALQUER PROPÓSITO EM PARTICULAR. Consulte a Licença Pública Geral GNU para
 * obter mais detalhes.
 *
 * Você deve ter recebido uma cópia da Licença Pública Geral GNU junto com este
 * programa; se não, escreva para a Free Software Foundation, Inc., 51 Franklin
 * St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Este programa está nomeado como connect.c e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <locale.h>

#include <internals.h>
#include <lib3270.h>
#include <lib3270/actions.h>
#include <lib3270/trace.h>
#include <lib3270/toggle.h>
#include <lib3270/log.h>
#include <lib3270/properties.h>
#include <lib3270/charset.h>

#define MAX_ARGS 10

const char *trace_file = "test.trace";

static void write_trace(H3270 GNUC_UNUSED(*session), void GNUC_UNUSED(*userdata), const char *fmt, va_list args)
{
	FILE *out = fopen(trace_file,"a");
	if(out)
	{

		vfprintf(out,fmt,args);
		fclose(out);
	}
}

static void online_group_state_changed(H3270 GNUC_UNUSED(*hSession), void GNUC_UNUSED(*dunno))
{
	printf("\n\n%s\n\n",__FUNCTION__);
}

int main(int argc, char *argv[])
{
#ifdef _WIN32
	debug("Process %s running on pid %u\n",argv[0],(unsigned int) GetCurrentProcessId());
#endif // _WIN32

#ifdef LC_ALL
	setlocale( LC_ALL, "" );
#endif

	textdomain("lib3270");

//	#pragma GCC diagnostic push
//	#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
	static struct option options[] = {
		{ "crl",		required_argument,	0,	'C' },
		{ "url",		required_argument,	0,	'U' },
		{ "tracefile",	required_argument,	0,	't' },

		{ 0, 0, 0, 0}

	};
//	#pragma GCC diagnostic pop

	H3270		* h		= lib3270_session_new("");
	int			  rc	= 0;

	lib3270_write_log(h,"TEST","Testprogram %s starts (%s)",argv[0],LIB3270_STRINGIZE_VALUE_OF(PRODUCT_NAME));

	lib3270_autoptr(char) version_info = lib3270_get_version_info();
	printf("3270 session %p created\n%s\n]",h,version_info);

#ifdef HAVE_LDAP
	lib3270_crl_set_preferred_protocol(h,"ldap");
#endif // HAVE_LDAP

	if(lib3270_set_url(h,NULL))
		lib3270_set_url(h,"tn3270://fandezhi.efglobe.com");

	int long_index =0;
	int opt;
	while((opt = getopt_long(argc, argv, "C:U:t:", options, &long_index )) != -1) {
		switch(opt) {
		case 'U':
			lib3270_set_url(h,optarg);
			break;

		case 'C':
			lib3270_crl_set_url(h,optarg);
			break;

		case 't':
			trace_file = optarg;
			lib3270_set_trace_handler(h,write_trace,NULL);
			lib3270_set_toggle(h,LIB3270_TOGGLE_DS_TRACE,1);
			break;
		}

	}

	printf("HOST URL: %s\tHOST CRL: %s\n",lib3270_get_url(h),lib3270_crl_get_url(h));

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

	{
		lib3270_autoptr(char) datafile = lib3270_build_config_filename("test","file.conf",NULL);
		printf("Configfile: \"%s\"\n",datafile);
	}

	{
		lib3270_autoptr(char) datafile = lib3270_build_filename("Makefile",NULL);
		printf("Custom file: \"%s\"\n",datafile);
	}


	//lib3270_set_toggle(h,LIB3270_TOGGLE_DS_TRACE,1);
	lib3270_set_toggle(h,LIB3270_TOGGLE_SSL_TRACE,1);

	printf("\nConnecting to %s\n",lib3270_get_url(h));

	const void * online_listener = lib3270_register_action_group_listener(h,LIB3270_ACTION_GROUP_ONLINE,online_group_state_changed,NULL);

	rc = lib3270_reconnect(h,0);
	printf("\n\nConnect exits with rc=%d (%s)\n\n",rc,strerror(rc));

	if(!rc)
	{
		rc = lib3270_wait_for_cstate(h,LIB3270_CONNECTED_TN3270E, 60);
		printf("\n\nWait for LIB3270_CONNECTED_TN3270E exits with rc=%d (%s)\n\n",rc,strerror(rc));
	}

	if(!rc)
	{
		rc = lib3270_wait_for_ready(h,60);
		printf("\n\nWait for ready exits with rc=%d (%s)\n\n",rc,strerror(rc));
	}

	if(!rc)
	{

		printf("\n\nWaiting starts %u\n",(unsigned int) time(NULL));

		{
			// Performance checks
			size_t f;
			time_t start = time(0);
			for(f=0;f < 1000; f++) {
				lib3270_wait_for_ready(h,10);
			}

			time_t tm = (time(0) - start);
			printf("\n\nTime for 1000 iterations of wait_for_ready was %d\n",(int) tm);

			if(tm > 1) {
				exit(-1);
			}

		}

		printf("\n\nWaiting ends %u\n\n",(unsigned int) time(NULL));

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

	/*
	{
		lib3270_set_lunames(h,"a,b,c,d,e");

		const char ** names = lib3270_get_lunames(h);

		size_t i;
		for(i=0;names[i];i++)
		{
			debug("[%s]",names[i]);
		}

	}
	*/

	{
		const LIB3270_PROPERTY * property = lib3270_property_get_by_name("model-number");

		debug("Model-number=%p",property);

	}

	lib3270_session_free(h);

	return 0;
}
