
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <lib3270.h>

#define MAX_ARGS 10

int main(int numpar, char *param[])
{
	H3270		* h;
	int			  rc	= 0;

	h = lib3270_session_new("");
	printf("3270 session %p created\n]",h);

	// lib3270_set_url(h,url ? url : "tn3270://fandezhi.efglobe.com");

	if(lib3270_set_url(h,NULL))
		lib3270_set_url(h,"tn3270://fandezhi.efglobe.com");

//	lib3270_set_toggle(h,LIB3270_TOGGLE_DS_TRACE,1);
	lib3270_set_toggle(h,LIB3270_TOGGLE_SSL_TRACE,1);

	rc = lib3270_connect(h,120);

	printf("\nConnect %s exits with rc=%d\n",lib3270_get_url(h),rc);

	lib3270_wait_for_ready(h,10);

	lib3270_enter(h);

	lib3270_wait_for_ready(h,10);

	lib3270_session_free(h);

	return 0;
}
