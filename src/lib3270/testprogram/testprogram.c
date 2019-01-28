
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#include <lib3270.h>
#include <lib3270/actions.h>

#define MAX_ARGS 10

int main(int argc, char *argv[])
{
//	#pragma GCC diagnostic push
//	#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
	static struct option options[] = {
		{ "crl",		required_argument,	0,	'C' },
		{ "url",		required_argument,	0,	'U' },

		{ 0, 0, 0, 0}

	};
//	#pragma GCC diagnostic pop

	H3270		* h;
	int			  rc	= 0;

	h = lib3270_session_new("");
	printf("3270 session %p created\n]",h);

	lib3270_set_url(h,NULL);

	int long_index =0;
	int opt;
	while((opt = getopt_long(argc, argv, "C:U:", options, &long_index )) != -1) {
		switch(opt) {
		case 'U':
			lib3270_set_url(h,optarg);
			break;

		case 'C':
			lib3270_set_crl_url(h,optarg);
			break;
		}

	}

//	printf("HOST URL: %s\HOST CRL: %s\n",lib3270_get_url(h),lib3270_get_crl_url(h));

//	if(lib3270_set_url(h,NULL))
//		lib3270_set_url(h,"tn3270://fandezhi.efglobe.com");

	//lib3270_set_toggle(h,LIB3270_TOGGLE_DS_TRACE,1);
	lib3270_set_toggle(h,LIB3270_TOGGLE_SSL_TRACE,1);

	rc = lib3270_reconnect(h,120);
	printf("\nConnect %s exits with rc=%d\n",lib3270_get_url(h),rc);

	lib3270_wait_for_ready(h,10);

	lib3270_enter(h);

	lib3270_wait_for_ready(h,10);

	lib3270_session_free(h);

	return 0;
}
