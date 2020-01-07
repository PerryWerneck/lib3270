/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270. Registro no INPI sob o nome G3270.
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
 * Este programa está nomeado como - e possui - linhas de código.
 *
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
static char parse_ctlchar(char *s)
{
	if (!s || !*s)
		return 0;

	if ((int) strlen(s) > 1)
	{
		if (*s != '^')
			return 0;
		else if (*(s+1) == '?')
			return 0177;
		else
			return *(s+1) - '@';
	} else
		return *s;
}

int lib3270_loaded(void)
{
	trace("%s",__FUNCTION__);

	ansictl.vintr   = parse_ctlchar("^C");
	ansictl.vquit   = parse_ctlchar("^\\");
	ansictl.verase  = parse_ctlchar("^H");
	ansictl.vkill   = parse_ctlchar("^U");
	ansictl.veof    = parse_ctlchar("^D");
	ansictl.vwerase = parse_ctlchar("^W");
	ansictl.vrprnt  = parse_ctlchar("^R");
	ansictl.vlnext  = parse_ctlchar("^V");

#ifdef _WIN32
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
#else
	bindtextdomain(GETTEXT_PACKAGE, LIB3270_STRINGIZE_VALUE_OF(LOCALEDIR));
#endif // _WIN32

	bind_textdomain_codeset("lib" LIB3270_STRINGIZE_VALUE_OF(LIB3270_NAME), "UTF-8");

#ifdef HAVE_LIBCURL
	trace("%s.curl_global_init",__FUNCTION__);
	curl_global_init(CURL_GLOBAL_DEFAULT);
#endif // HAVE_LIBCURL

    return 0;
}

int lib3270_unloaded(void)
{
	trace("%s",__FUNCTION__);

#ifdef HAVE_LIBCURL
	trace("%s.curl_global_cleanup",__FUNCTION__);
	curl_global_cleanup();
#endif // HAVE_LIBCURL

#ifdef HAVE_SYSLOG
	if(use_syslog)
	{
		closelog();
	}
#endif // HAVE_SYSLOG

    return 0;
}


#if defined WIN32

BOOL WINAPI DllMain(HANDLE hInstance, DWORD dwcallpurpose, LPVOID GNUC_UNUSED(lpvResvd))
{
    switch(dwcallpurpose)
    {
    case DLL_PROCESS_ATTACH:
    	hModule = hInstance;
		hEventLog = RegisterEventSource(NULL, LIB3270_STRINGIZE_VALUE_OF(LIB3270_NAME));
		get_version_info();
		lib3270_loaded();
		break;

	case DLL_PROCESS_DETACH:
		lib3270_unloaded();
		DeregisterEventSource(hEventLog);
		hEventLog = 0;
		break;

    }

    return TRUE;
}

#endif




