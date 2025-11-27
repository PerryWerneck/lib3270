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
 * References:
 *
 * https://docs.microsoft.com/en-us/windows/win32/winhttp/winhttp-autoproxy-api
 *
 */

/**
 * @brief Implements CRL download using winhttp.
 *
 */

#error deprecated

#include <config.h>
#include <internals.h>
#include <lib3270/os.h>
#include <lib3270/log.h>
#include <lib3270/trace.h>
#include <winhttp.h>
#include <private/util.h>

/*--[ Implement ]------------------------------------------------------------------------------------*/

static void lib3270_autoptr_cleanup_HINTERNET(HINTERNET **hInternet) {
	if(*hInternet)
		WinHttpCloseHandle(*hInternet);
	*hInternet = 0;
}

char * lib3270_url_get_using_http(H3270 *hSession, const char *url, const char **error_message) {
	wchar_t wHostname[4096];
	wchar_t wPath[4096];

	trace_network(hSession,"Getting data from %s\n",url);

	{
		// Strip URL
		char * unescaped = unescape(url);

		char *hostname = strstr(unescaped,"://");
		if(!hostname)
			hostname = unescaped;
		else
			hostname += 3;

		char *path = strchr(hostname,'/');
		if(path)
			*(path++) = 0;

		mbstowcs(wHostname, hostname, strlen(hostname)+1);
		mbstowcs(wPath, path, strlen(path)+1);

		lib3270_free(unescaped);

	}

	// https://docs.microsoft.com/en-us/windows/desktop/api/winhttp/nf-winhttp-winhttpopenrequest

	// Open HTTP session
	// https://docs.microsoft.com/en-us/windows/desktop/api/winhttp/nf-winhttp-winhttpopenrequest
	static const char * userAgent = PACKAGE_NAME "/" PACKAGE_VERSION;
	wchar_t wUserAgent[256];
	mbstowcs(wUserAgent, userAgent, strlen(userAgent)+1);

	// https://docs.microsoft.com/en-us/windows/win32/api/winhttp/nf-winhttp-winhttpopen

	/// @TODO Use WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY when available!
	lib3270_autoptr(HINTERNET) httpSession = WinHttpOpen(wUserAgent, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0 );

	if(!httpSession) {
		lib3270_autoptr(char) windows_error = lib3270_win32_strerror(GetLastError());
		trace_network(hSession,"Can't open session for %s: %s\n",url, windows_error);

		*error_message = _( "Can't open HTTP session" );
		errno = EINVAL;
		return NULL;
	}

	// Connect to server
	lib3270_autoptr(HINTERNET) hConnect = WinHttpConnect(httpSession, wHostname, INTERNET_DEFAULT_HTTP_PORT, 0);
	if(!hConnect) {
		lib3270_autoptr(char) windows_error = lib3270_win32_strerror(GetLastError());
		trace_network(hSession,"Can't connect to %s: %s\n", url, windows_error);

		*error_message = _( "Can't connect to HTTP server." );

		return NULL;
	}

	// Create request.
	lib3270_autoptr(HINTERNET) hRequest = WinHttpOpenRequest(hConnect, L"GET", wPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_ESCAPE_PERCENT);
	if(!hConnect) {
		lib3270_autoptr(char) windows_error = lib3270_win32_strerror(GetLastError());
		trace_network(hSession,"Can't open request for %s: %s\n", url, windows_error);

		*error_message = _( "Can't create HTTP request." );

		errno = EINVAL;
		return NULL;
	}

	WinHttpSetOption(hRequest, WINHTTP_OPTION_CLIENT_CERT_CONTEXT, WINHTTP_NO_CLIENT_CERT_CONTEXT, 0);

	// Send request.
	if(!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
		lib3270_autoptr(char) windows_error = lib3270_win32_strerror(GetLastError());
		trace_network(hSession,"Can't send request for %s: %s\n", url, windows_error);

		*error_message = _( "Can't send HTTP request." );

		errno = EINVAL;
		return NULL;
	}

	// Get response
	if(!WinHttpReceiveResponse(hRequest, NULL)) {
		lib3270_autoptr(char) windows_error = lib3270_win32_strerror(GetLastError());
		trace_network(hSession,"Can't receive response for %s: %s\n", url, windows_error);

		*error_message = _( "Error receiving HTTP response." );

		errno = EINVAL;
		return NULL;

	}

	DWORD szResponse = 0;
	if(!WinHttpQueryDataAvailable(hRequest, &szResponse)) {
		lib3270_autoptr(char) windows_error = lib3270_win32_strerror(GetLastError());
		trace_network(hSession,"Error checking for available data after response to %s: %s\n", url, windows_error);

		*error_message = _( "Empty response from HTTP server." );

		errno = EINVAL;
		return NULL;
	}

	char * httpText = lib3270_malloc(szResponse+1);
	memset(httpText,0,szResponse+1);

	debug("Data block: %p",httpText);
	debug("Response before: %u", (unsigned int) szResponse);

	if(!WinHttpReadData(hRequest,httpText,szResponse,&szResponse)) {
		lib3270_autoptr(char) windows_error = lib3270_win32_strerror(GetLastError());
		trace_network(hSession,"Can't read response size for %s: %s\n", url, windows_error);

		*error_message = _( "Can't read HTTP response size." );

		lib3270_free(httpText);

		errno = EINVAL;
		return NULL;
	}


	trace_network(hSession,"Got %u bytes from %s\n",(unsigned int) szResponse, url);

	return httpText;

}
