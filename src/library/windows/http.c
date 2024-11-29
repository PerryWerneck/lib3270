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
 * Este programa está nomeado como - e possui - linhas de código.
 *
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

#include <config.h>
#include <internals.h>
#include <lib3270/log.h>
#include <lib3270/trace.h>
#include <winhttp.h>
#include <utilc.h>
#include <lib3270/win32.h>

/*--[ Implement ]------------------------------------------------------------------------------------*/

static void lib3270_autoptr_cleanup_HINTERNET(HINTERNET **hInternet) {
	if(*hInternet)
		WinHttpCloseHandle(*hInternet);
	*hInternet = 0;
}

char * lib3270_url_get_using_http(H3270 *hSession, const char *url, const char **error_message) {
	wchar_t wHostname[4096];
	wchar_t wPath[4096];

	lib3270_write_nettrace(hSession,"Getting data from %s\n",url);

	{
		// Strip URL
		char * unescaped = lib3270_unescape(url);

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
		lib3270_autoptr(char) windows_error = lib3270_win32_translate_error_code(GetLastError());
		lib3270_write_nettrace(hSession,"Can't open session for %s: %s\n",url, windows_error);

		*error_message = _( "Can't open HTTP session" );
		errno = EINVAL;
		return NULL;
	}

	// Connect to server
	lib3270_autoptr(HINTERNET) hConnect = WinHttpConnect(httpSession, wHostname, INTERNET_DEFAULT_HTTP_PORT, 0);
	if(!hConnect) {
		lib3270_autoptr(char) windows_error = lib3270_win32_translate_error_code(GetLastError());
		lib3270_write_nettrace(hSession,"Can't connect to %s: %s\n", url, windows_error);

		*error_message = _( "Can't connect to HTTP server." );

		return NULL;
	}

	// Create request.
	lib3270_autoptr(HINTERNET) hRequest = WinHttpOpenRequest(hConnect, L"GET", wPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_ESCAPE_PERCENT);
	if(!hConnect) {
		lib3270_autoptr(char) windows_error = lib3270_win32_translate_error_code(GetLastError());
		lib3270_write_nettrace(hSession,"Can't open request for %s: %s\n", url, windows_error);

		*error_message = _( "Can't create HTTP request." );

		errno = EINVAL;
		return NULL;
	}

	WinHttpSetOption(hRequest, WINHTTP_OPTION_CLIENT_CERT_CONTEXT, WINHTTP_NO_CLIENT_CERT_CONTEXT, 0);

	// Send request.
	if(!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
		lib3270_autoptr(char) windows_error = lib3270_win32_translate_error_code(GetLastError());
		lib3270_write_nettrace(hSession,"Can't send request for %s: %s\n", url, windows_error);

		*error_message = _( "Can't send HTTP request." );

		errno = EINVAL;
		return NULL;
	}

	// Get response
	if(!WinHttpReceiveResponse(hRequest, NULL)) {
		lib3270_autoptr(char) windows_error = lib3270_win32_translate_error_code(GetLastError());
		lib3270_write_nettrace(hSession,"Can't receive response for %s: %s\n", url, windows_error);

		*error_message = _( "Error receiving HTTP response." );

		errno = EINVAL;
		return NULL;

	}

	DWORD szResponse = 0;
	if(!WinHttpQueryDataAvailable(hRequest, &szResponse)) {
		lib3270_autoptr(char) windows_error = lib3270_win32_translate_error_code(GetLastError());
		lib3270_write_nettrace(hSession,"Error checking for available data after response to %s: %s\n", url, windows_error);

		*error_message = _( "Empty response from HTTP server." );

		errno = EINVAL;
		return NULL;
	}

	char * httpText = lib3270_malloc(szResponse+1);
	memset(httpText,0,szResponse+1);

	debug("Data block: %p",httpText);
	debug("Response before: %u", (unsigned int) szResponse);

	if(!WinHttpReadData(hRequest,httpText,szResponse,&szResponse)) {
		lib3270_autoptr(char) windows_error = lib3270_win32_translate_error_code(GetLastError());
		lib3270_write_nettrace(hSession,"Can't read response size for %s: %s\n", url, windows_error);

		*error_message = _( "Can't read HTTP response size." );

		lib3270_free(httpText);

		errno = EINVAL;
		return NULL;
	}


	lib3270_write_nettrace(hSession,"Got %u bytes from %s\n",(unsigned int) szResponse, url);

	return httpText;

}
