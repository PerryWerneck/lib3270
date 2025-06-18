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

/**
 *	@brief Init/Deinit lib3270 internals.
 */

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#endif // _WIN32

#include <config.h>
#include <locale.h>

#ifdef HAVE_LIBCURL
#include <curl/curl.h>
#endif // HAVE_LIBCURL

#include <lib3270.h>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include "winversc.h"
#endif // _WIN32

#include <lib3270/log.h>
#include <internals.h>

#include <private/intl.h>

#ifdef HAVE_SYSLOG
#include <syslog.h>
#endif // HAVE_SYSLOG

#if defined WIN32
BOOL WINAPI DllMain(HANDLE hinst, DWORD dwcallpurpose, LPVOID lpvResvd);
#else
int lib3270_loaded(void) __attribute__((constructor));
int lib3270_unloaded(void) __attribute__((destructor));
#endif

/*---[ Globals ]--------------------------------------------------------------------------------------------------------------*/

#ifdef _WIN32
/// @brief Windows Event Log Handler.
HANDLE hEventLog = 0;
HANDLE hModule = 0;
#endif // _WIN32

/**
 * @brief Parse an stty control-character specification; a cheap, non-complaining implementation.
 */
static char parse_ctlchar(char *s) {
	if (!s || !*s)
		return 0;

	if ((int) strlen(s) > 1) {
		if (*s != '^')
			return 0;
		else if (*(s+1) == '?')
			return 0177;
		else
			return *(s+1) - '@';
	} else
		return *s;
}

int lib3270_loaded(void) {
	trace("%s",__FUNCTION__);

	ansictl.vintr   = parse_ctlchar("^C");
	ansictl.vquit   = parse_ctlchar("^\\");
	ansictl.verase  = parse_ctlchar("^H");
	ansictl.vkill   = parse_ctlchar("^U");
	ansictl.veof    = parse_ctlchar("^D");
	ansictl.vwerase = parse_ctlchar("^W");
	ansictl.vrprnt  = parse_ctlchar("^R");
	ansictl.vlnext  = parse_ctlchar("^V");

#if defined(_WIN32) && defined(HAVE_LIBINTL)
	{
		char lpFilename[4096];

		memset(lpFilename,0,sizeof(lpFilename));
		DWORD szPath = GetModuleFileName(hModule,lpFilename,sizeof(lpFilename));
		lpFilename[szPath] = 0;

		char * ptr = strrchr(lpFilename,'\\');
		if(ptr)
			ptr[1] = 0;

		strncat(lpFilename,"locale",4095);
		bindtextdomain(GETTEXT_PACKAGE,lpFilename);
	}
#elif defined(__APPLE__) && defined(HAVE_LIBINTL)
	{
		lib3270_autoptr(char) localedir = lib3270_build_data_filename("locale",NULL);
		debug("LocaleDIR(%s)=%s",PACKAGE_NAME,localedir);
		bindtextdomain(PACKAGE_NAME, localedir);
		bind_textdomain_codeset(PACKAGE_NAME, "UTF-8");
	}
#elif defined(LOCALEDIR)

	bindtextdomain(GETTEXT_PACKAGE, LIB3270_STRINGIZE_VALUE_OF(LOCALEDIR));

#else

	#error "LOCALEDIR is not defined"

#endif // _WIN32

#if defined(HAVE_LIBINTL)
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif // HAVE_LIBINTL

#ifdef HAVE_LIBCURL
	trace("%s.curl_global_init",__FUNCTION__);
	curl_global_init(CURL_GLOBAL_DEFAULT);
#endif // HAVE_LIBCURL

	return 0;
}

int lib3270_unloaded(void) {
	trace("%s",__FUNCTION__);

#ifdef HAVE_LIBCURL
	trace("%s.curl_global_cleanup",__FUNCTION__);
	curl_global_cleanup();
#endif // HAVE_LIBCURL

#ifdef HAVE_SYSLOG
	if(use_syslog) {
		closelog();
	}
#endif // HAVE_SYSLOG

	return 0;
}


#if defined WIN32

BOOL WINAPI DllMain(HANDLE hInstance, DWORD dwcallpurpose, LPVOID GNUC_UNUSED(lpvResvd)) {
	debug("%s starts",__FUNCTION__);

	switch(dwcallpurpose) {
	case DLL_PROCESS_ATTACH:
		hModule = hInstance;
		hEventLog = RegisterEventSource(NULL, LIB3270_STRINGIZE_VALUE_OF(PRODUCT_NAME));
		get_version_info();
		lib3270_loaded();
		break;

	case DLL_PROCESS_DETACH:
		lib3270_unloaded();
		if(hEventLog) {
			DeregisterEventSource(hEventLog);
		}
		hEventLog = NULL;
		break;

	}

	debug("%s ends",__FUNCTION__);

	return TRUE;
}

#endif




