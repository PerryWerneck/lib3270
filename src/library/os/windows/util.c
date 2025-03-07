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
 * @brief Win32 Utility functions.
 */

#include <winsock2.h>
#include <windows.h>
 
#include <config.h>
#include <lib3270/defs.h>
#include <lib3270/memory.h>
#include <lmcons.h>
#include <internals.h>
#include <io.h>

#include "winversc.h"
#include <ws2tcpip.h>
#include <stdio.h>
#include <errno.h>
#include "w3miscc.h"
#include <malloc.h>
#include <wininet.h>

#ifdef HAVE_ICONV
#include <iconv.h>
#endif // HAVE_ICONV

#ifndef ICONV_CONST
	#define ICONV_CONST
#endif // ICONV_CONST

#include <lib3270/log.h>
#include <lib3270/win32.h>

#define my_isspace(c)	isspace((unsigned char)c)

static int is_nt = 1;
static int has_ipv6 = 1;

static const struct {
	DWORD dwMessageId;
	const char *message;
 } windows_errors[] = {

	// Reference: https://learn.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2

 	// /usr/x86_64-w64-mingw32/sys-root/mingw/include/wininet.h
 	{ ERROR_INTERNET_TIMEOUT,	N_("The request has timed out. Possible causes are slow or intermittent internet connection, Antivirus software, Firewall, and Proxy settings.") },

 	// http://s.web.umkc.edu/szb53/cs423_sp16/wsock_errors.html
 	// /usr/x86_64-w64-mingw32/sys-root/mingw/include/winerror.h
	{ WSAHOST_NOT_FOUND,		N_("No such host is known. The name is not an official host name or alias, or it cannot be found in the database(s) being queried.") },

	{ WSAECONNREFUSED,			N_("No connection could be made because the target computer actively refused it. This usually results from trying to connect to a service that is inactive on the foreign host.") },

 };


int get_version_info(void) {
	OSVERSIONINFO info;

	// Figure out what version of Windows this is.
	memset(&info, '\0', sizeof(info));
	info.dwOSVersionInfoSize = sizeof(info);
	if(GetVersionEx(&info) == 0) {
		lib3270_log_write(NULL,"lib3270","%s","Can't get Windows version");
		return -1;
	}

	// Yes, people still run Win98.
	if (info.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		is_nt = 0;

	// Win2K and earlier is IPv4-only.  WinXP and later can have IPv6.
	if (!is_nt || info.dwMajorVersion < 5 || (info.dwMajorVersion == 5 && info.dwMinorVersion < 1)) {
		has_ipv6 = 0;
	}

	return 0;
}

// Convert a network address to a string.
#ifndef HAVE_INET_NTOP
const char * inet_ntop(int af, const void *src, char *dst, socklen_t cnt) {
	union {
		struct sockaddr sa;
		struct sockaddr_in sin;
		struct sockaddr_in6 sin6;
	} sa;
	DWORD ssz;
	DWORD sz = cnt;

	memset(&sa, '\0', sizeof(sa));

	switch (af) {
	case AF_INET:
		sa.sin = *(struct sockaddr_in *)src;	// struct copy
		ssz = sizeof(struct sockaddr_in);
		break;
	case AF_INET6:
		sa.sin6 = *(struct sockaddr_in6 *)src;	// struct copy
		ssz = sizeof(struct sockaddr_in6);
		break;
	default:
		if (cnt > 0)
			dst[0] = '\0';
		return NULL;
	}

	sa.sa.sa_family = af;

	if (WSAAddressToString(&sa.sa, ssz, NULL, dst, &sz) != 0) {
		if (cnt > 0)
			dst[0] = '\0';
		return NULL;
	}

	return dst;
}
#endif // HAVE_INET_NTOP

LIB3270_EXPORT char * lib3270_win32_strerror(int lasterror) {

	for(size_t ix = 0; ix < (sizeof(windows_errors)/sizeof(windows_errors[0])); ix++) {
		if(windows_errors[ix].dwMessageId == lasterror) {
			return strdup(dgettext(GETTEXT_PACKAGE,windows_errors[ix].message));
		}
	}

	char * buffer = lib3270_malloc(4096);

	debug("Winsock error %d",lasterror);

	if(FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,lasterror,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),buffer,4096,NULL) == 0) {

		free(buffer);

		return lib3270_strdup_printf(
			_("WinSock error %d (check it in https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2)"),
			lasterror
		);

	}

	for(unsigned char *ptr = (unsigned char *) buffer;*ptr;ptr++) {
		if(*ptr < ' ') {
			*ptr = ' ';
		}
	}

#ifdef HAVE_ICONV
	{
		// Convert from windows codepage to pw3270´s default charset (UTF-8)
		iconv_t hConv = iconv_open("UTF-8",lib3270_win32_local_charset());

		debug("[%s]",buffer);

		if(hConv == (iconv_t) -1) {
			lib3270_log_write(NULL,"iconv","%s: Error creating charset conversion",__FUNCTION__);
		} else {
			size_t				  in 		= strlen(buffer);
			size_t				  out 		= (in << 1);
			char				* ptr;
			char				* outBuffer = (char *) malloc(out);
#ifdef WINICONV_CONST
			WINICONV_CONST char	* inBuffer	= (WINICONV_CONST char	*) buffer;
#else
			ICONV_CONST char	* inBuffer	= (ICONV_CONST char	*) buffer;
#endif
			memset(ptr=outBuffer,0,out);

			iconv(hConv,NULL,NULL,NULL,NULL);	// Reset state

			if(iconv(hConv,&inBuffer,&in,&ptr,&out) != ((size_t) -1)) {
				strncpy(buffer,outBuffer,4095);
			}

			free(outBuffer);

			iconv_close(hConv);
		}

	}
#endif // HAVE_ICONV

	return buffer;
}

LIB3270_EXPORT const char * lib3270_win32_local_charset(void) {
	// Reference:
	// http://msdn.microsoft.com/en-us/library/windows/desktop/dd318070(v=vs.85).aspx

	/// TODO: Use GetACP() to identify the correct code page

	debug("Windows CHARSET is %u",GetACP());

	return "CP1252";
}

#define SECS_BETWEEN_EPOCHS	11644473600ULL
#define SECS_TO_100NS		10000000ULL /* 10^7 */

LIB3270_EXPORT char	* lib3270_get_installation_path() {
	char lpFilename[MAX_PATH+1];

	memset(lpFilename,0,sizeof(lpFilename));
	DWORD szPath = GetModuleFileName(hModule,lpFilename,MAX_PATH);
	lpFilename[szPath] = 0;

	char *ptr = strrchr(lpFilename,'\\');
	if(ptr) {
		ptr[0] = 0;

		ptr = strrchr(lpFilename,'\\');
		if(ptr && !(strcasecmp(ptr,"\\bin") && strcasecmp(ptr,"\\lib"))) {
			*ptr = 0;
		}

		strncat(lpFilename,"\\",MAX_PATH);
	}

	return strdup(lpFilename);
}

/*
char * lib3270_get_user_name() {
	char	username[UNLEN + 1];
	DWORD	szName = UNLEN;

	memset(username,0,UNLEN + 1);
	GetUserName(username, &szName);

	return strdup(username);

}
*/

static char * concat(char *path, const char *name, size_t *length) {
	char *ptr;
	size_t szCurrent = strlen(path);

	for(ptr=path; *ptr; ptr++) {
		if(*ptr == '/')
			*ptr = '\\';
	}

	if(szCurrent > 1 && path[szCurrent-1] != '\\')
		strcat(path,"\\");

	szCurrent += strlen(name);

	if(szCurrent >= *length) {
		*length += (szCurrent + 1024);
		path = lib3270_realloc(path,*length);
	}

	strcat(path,name);

	return path;
}

static char * build_filename(const char *str, va_list args) {
	size_t szFilename = MAX_PATH;
	char *ptr;
	char * filename = (char *) lib3270_malloc(szFilename);

	memset(filename,0,szFilename);

	DWORD szPath = GetModuleFileName(hModule,filename,szFilename);
	filename[szPath] = 0;

	ptr = strrchr(filename,'\\');
	if(ptr) {
		ptr[0] = 0;

		ptr = strrchr(filename,'\\');
		if(ptr && !(strcasecmp(ptr,"\\bin") && strcasecmp(ptr,"\\lib"))) {
			*ptr = 0;
		}

		strncat(filename,"\\",szFilename);
	}

	while(str) {
		filename = concat(filename,str,&szFilename);
		str = va_arg(args, const char *);
	}

	return (char *) lib3270_realloc(filename,strlen(filename)+1);
}

char * lib3270_build_data_filename(const char *str, ...) {

	char *ptr;
	lib3270_autoptr(char) instpath = lib3270_get_installation_path();

	for(ptr = instpath; *ptr; ptr++) {
		if(*ptr == '/') {
			*ptr = '\\';
		}
	}

	if( *(instpath+strlen(instpath)-1) == '\\') {
		instpath[strlen(instpath)-1] = 0;
	}

	char relative[MAX_PATH+1];
	memset(relative,0,MAX_PATH);

	{
		va_list args;
		va_start (args, str);

		while(str) {

			if(str[0] == '\\' || str[0] == '/') {
				strncat(relative,str,MAX_PATH);
			} else {
				strncat(relative,"\\",MAX_PATH);
				strncat(relative,str,MAX_PATH);
			}

			str = va_arg(args, const char *);
		}

		va_end (args);
	}

	for(ptr = relative; *ptr; ptr++) {
		if(*ptr == '/') {
			*ptr = '\\';
		}
	}

	char filename[MAX_PATH+1];
	memset(filename,0,MAX_PATH+1);

	// Check instdir
	strncpy(filename,instpath,MAX_PATH);
	strncat(filename,"\\share",MAX_PATH);
	strncat(filename,relative,MAX_PATH);

	if(access(filename,0) == 0) {
		return strdup(filename);
	}

	strncpy(filename,instpath,MAX_PATH);
	strncat(filename,"\\share\\",MAX_PATH);
	strncat(filename,LIB3270_STRINGIZE_VALUE_OF(PRODUCT_NAME),MAX_PATH);
	strncat(filename,relative,MAX_PATH);

	if(access(filename,0) == 0) {
		return strdup(filename);
	}

	// Default behavior.
	strncpy(filename,instpath,MAX_PATH);
	strncat(filename,relative,MAX_PATH);

	return strdup(filename);
}

char * lib3270_build_config_filename(const char *str, ...) {
	va_list args;
	va_start (args, str);

	char *filename = build_filename(str, args);

	va_end (args);

	return filename;
}

char * lib3270_build_filename(const char *str, ...) {
	va_list args;
	va_start (args, str);

	char *filename = build_filename(str, args);

	va_end (args);

	return filename;
}

LIB3270_EXPORT void lib3270_autoptr_cleanup_HKEY(HKEY *hKey) {
	if(*hKey) {
		RegCloseKey(*hKey);
		*hKey = 0;
	}
}

