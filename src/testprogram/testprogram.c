
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

	printf("3270 session %p created\n]",h);

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

	rc = lib3270_reconnect(h,120);
	printf("\n\nConnect exits with rc=%d (%s)\n\n",rc,strerror(rc));

	if(!rc)
	{
		printf("\n\nWaiting starts %u\n",(unsigned int) time(NULL));
		lib3270_wait_for_ready(h,10);
		printf("Waiting ends %u\n\n",(unsigned int) time(NULL));

		/*
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
		*/

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
