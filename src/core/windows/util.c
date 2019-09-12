/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270. Registro no INPI sob
 * o nome G3270.
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
 * @brief Win32 Utility functions.
 */

#include <winsock2.h>
#include <windows.h>
#include <lmcons.h>
#include <lib3270-internals.h>

#include "winversc.h"
#include <ws2tcpip.h>
#include <stdio.h>
#include <errno.h>
#include "w3miscc.h"
#include <malloc.h>

#ifdef HAVE_ICONV
	#include <iconv.h>
#endif // HAVE_ICONV

#include <lib3270/log.h>

#define my_isspace(c)	isspace((unsigned char)c)

int is_nt = 1;
int has_ipv6 = 1;

int get_version_info(void)
{
	OSVERSIONINFO info;

	// Figure out what version of Windows this is.
	memset(&info, '\0', sizeof(info));
	info.dwOSVersionInfoSize = sizeof(info);
	if(GetVersionEx(&info) == 0)
	{
		lib3270_write_log(NULL,"lib3270","%s","Can't get Windows version");
		return -1;
	}

	// Yes, people still run Win98.
	if (info.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		is_nt = 0;

	// Win2K and earlier is IPv4-only.  WinXP and later can have IPv6.
	if (!is_nt || info.dwMajorVersion < 5 || (info.dwMajorVersion == 5 && info.dwMinorVersion < 1))
	{
		has_ipv6 = 0;
	}

	return 0;
}

// Convert a network address to a string.
#ifndef HAVE_INET_NTOP
const char * inet_ntop(int af, const void *src, char *dst, socklen_t cnt)
{
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

LIB3270_EXPORT char * lib3270_win32_translate_error_code(int lasterror)
{
	char * buffer = lib3270_malloc(4096);

	if(FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,lasterror,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),buffer,4096,NULL) == 0)
	{
	    snprintf(buffer, 4095, _( "Windows error %d" ), lasterror);
	}

#ifdef HAVE_ICONV
	{
		// Convert from windows codepage to pw3270´s default charset (UTF-8)
		iconv_t hConv = iconv_open("UTF-8",lib3270_win32_local_charset());

		trace("[%s]",buffer);

		if(hConv == (iconv_t) -1)
		{
			lib3270_write_log(NULL,"iconv","%s: Error creating charset conversion",__FUNCTION__);
		}
		else
		{
			size_t				  in 		= strlen(buffer);
			size_t				  out 		= (in << 1);
			char				* ptr;
			char				* outBuffer = (char *) malloc(out);
			ICONV_CONST char	* inBuffer	= (ICONV_CONST char	*) buffer;

			memset(ptr=outBuffer,0,out);

			iconv(hConv,NULL,NULL,NULL,NULL);	// Reset state

			if(iconv(hConv,&inBuffer,&in,&ptr,&out) != ((size_t) -1))
			{
				strncpy(buffer,outBuffer,4095);
			}

			free(outBuffer);

			iconv_close(hConv);
		}

	}
#endif // HAVE_ICONV

	return buffer;
}

// Decode a Win32 error number.
LIB3270_EXPORT const char * lib3270_win32_strerror(int e)
{
	static char buffer[4096];

	if(FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,e,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),buffer,sizeof(buffer),NULL) == 0)
	{
	    snprintf(buffer, 4095, _( "Windows error %d" ), e);
		return buffer;
	}

#ifdef HAVE_ICONV
	{
		// Convert from windows codepage to UTF-8 pw3270´s default charset
		iconv_t hConv = iconv_open("UTF-8",lib3270_win32_local_charset());

		trace("[%s]",buffer);

		if(hConv == (iconv_t) -1)
		{
			lib3270_write_log(NULL,"iconv","%s: Error creating charset conversion",__FUNCTION__);
		}
		else
		{
			size_t				  in 		= strlen(buffer);
			size_t				  out 		= (in << 1);
			char				* ptr;
			char				* outBuffer = (char *) malloc(out);
			ICONV_CONST char	* inBuffer	= (ICONV_CONST char	*) buffer;

			memset(ptr=outBuffer,0,out);

			iconv(hConv,NULL,NULL,NULL,NULL);	// Reset state

			if(iconv(hConv,&inBuffer,&in,&ptr,&out) != ((size_t) -1))
			{
				strncpy(buffer,outBuffer,4095);
			}

			free(outBuffer);

			iconv_close(hConv);
		}

	}
#endif // HAVE_ICONV

	return buffer;
}

LIB3270_EXPORT const char * lib3270_win32_local_charset(void)
{
	// Reference:
	// http://msdn.microsoft.com/en-us/library/windows/desktop/dd318070(v=vs.85).aspx

	/// TODO: Use GetACP() to identify the correct code page

	trace("Windows CHARSET is %u",GetACP());

	return "CP1252";
}

#define SECS_BETWEEN_EPOCHS	11644473600ULL
#define SECS_TO_100NS		10000000ULL /* 10^7 */

int gettimeofday(struct timeval *tv, void GNUC_UNUSED(*ignored))
{
	FILETIME t;
	ULARGE_INTEGER u;

	GetSystemTimeAsFileTime(&t);
	memcpy(&u, &t, sizeof(ULARGE_INTEGER));

	/* Isolate seconds and move epochs. */
	tv->tv_sec = (DWORD)((u.QuadPart / SECS_TO_100NS) - SECS_BETWEEN_EPOCHS);
	tv->tv_usec = (u.QuadPart % SECS_TO_100NS) / 10ULL;
	return 0;
}

LIB3270_EXPORT char	* lib3270_get_installation_path()
{
	char lpFilename[4096];

	memset(lpFilename,0,sizeof(lpFilename));
	DWORD szPath = GetModuleFileName(hModule,lpFilename,sizeof(lpFilename));
	lpFilename[szPath] = 0;

	char * ptr = strrchr(lpFilename,'\\');
	if(ptr)
		ptr[1] = 0;

	return strdup(lpFilename);
}

char * lib3270_get_user_name()
{
	char	username[UNLEN + 1];
	DWORD	szName = UNLEN;

	memset(username,0,UNLEN + 1);
	GetUserName(username, &szName);

	return strdup(username);

}

static char * concat(char *path, const char *name, size_t *length)
{
	char *ptr;
    size_t szCurrent = strlen(path);

	for(ptr=path;*ptr;ptr++)
	{
		if(*ptr == '/')
			*ptr = '\\';
	}

	if(szCurrent > 1 && path[szCurrent-1] != '\\')
		strcat(path,"\\");

	szCurrent += strlen(name);

	if(szCurrent >= *length)
	{
		*length += (szCurrent + 1024);
		path = lib3270_realloc(path,*length);
	}

	strcat(path,name);

	return path;
}

static char * build_filename(const char *str, va_list args)
{
	size_t szFilename = MAX_PATH;
	char *ptr;
	char * filename = (char *) lib3270_malloc(szFilename);

	memset(filename,0,szFilename);

#ifdef DEBUG
	filename[0] = '.';
	filename[1] = '\\';
#else
	DWORD szPath = GetModuleFileName(hModule,filename,szFilename);
	filename[szPath] = 0;
#endif // DEBUG

	ptr = strrchr(filename,'\\');
	if(ptr)
		ptr[1] = 0;

	while(str) {
		filename = concat(filename,str,&szFilename);
		str = va_arg(args, const char *);
	}

	return (char *) lib3270_realloc(filename,strlen(filename)+1);
}

char * lib3270_build_data_filename(const char *str, ...)
{
	va_list args;
	va_start (args, str);

	char *filename = build_filename(str, args);

	va_end (args);

	return filename;
}

char * lib3270_build_config_filename(const char *str, ...)
{
	va_list args;
	va_start (args, str);

	char *filename = build_filename(str, args);

	va_end (args);

	return filename;
}

char * lib3270_build_filename(const char *str, ...)
{
	va_list args;
	va_start (args, str);

	char *filename = build_filename(str, args);

	va_end (args);

	return filename;
}

