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
#include "private.h"

#if defined(HAVE_LIBSSL) && defined(SSL_ENABLE_CRL_CHECK)

#include <winhttp.h>
#include <utilc.h>

/*--[ Implement ]------------------------------------------------------------------------------------*/

static void lib3270_autoptr_cleanup_HINTERNET(HINTERNET **hInternet)
{
	if(*hInternet)
		WinHttpCloseHandle(*hInternet);
	*hInternet = 0;
}

X509_CRL * get_crl_using_http(H3270 *hSession, SSL_ERROR_MESSAGE * message, const char *consturl)
{
	// Strip URL.
	lib3270_autoptr(char) urldup = lib3270_unescape(consturl);

	char *hostname = strstr(urldup,"://");
	if(!hostname)
		hostname = urldup;
	else
		hostname += 3;

	char *path = strchr(hostname,'/');
	if(path)
		*(path++) = 0;

	// https://docs.microsoft.com/en-us/windows/desktop/api/winhttp/nf-winhttp-winhttpopenrequest

	// Open HTTP session
	// https://docs.microsoft.com/en-us/windows/desktop/api/winhttp/nf-winhttp-winhttpopenrequest
	static const char * userAgent = PACKAGE_NAME "/" PACKAGE_VERSION;
	wchar_t wUserAgent[256];
	mbstowcs(wUserAgent, userAgent, strlen(userAgent)+1);
	lib3270_autoptr(HINTERNET) httpSession = WinHttpOpen(wUserAgent, WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0 );
	if(!httpSession)
	{
		lib3270_autoptr(char) windows_error = lib3270_win32_translate_error_code(GetLastError());
		lib3270_write_log(hSession,"ssl","%s: %s",consturl, windows_error);

		message->error = hSession->ssl.error = 0;
		message->title = _( "Security error" );
		message->text = _( "Can't open HTTP session" );
		debug("%s",message->text);
		errno = EINVAL;
		return NULL;
	}

	// Connect to server
	debug("Hostname: \"%s\"",hostname);
	wchar_t wHostname[4096];
	mbstowcs(wHostname, hostname, strlen(hostname)+1);
	lib3270_autoptr(HINTERNET) hConnect = WinHttpConnect(httpSession, wHostname, INTERNET_DEFAULT_HTTP_PORT, 0);
	if(!hConnect)
	{
		lib3270_autoptr(char) windows_error = lib3270_win32_translate_error_code(GetLastError());
		lib3270_write_log(hSession,"ssl","%s: %s",consturl, windows_error);

		message->error = hSession->ssl.error = 0;
		message->title = _( "Security error" );
		message->text = _( "Can't connect to HTTP server." );
		debug("%s",message->text);
		errno = EINVAL;
		return NULL;
	}

	// Create request.
	debug("Path: \"%s\"",path);
	wchar_t wPath[4096];
	mbstowcs(wPath, path, strlen(path)+1);
	lib3270_autoptr(HINTERNET) hRequest = WinHttpOpenRequest(hConnect, L"GET", wPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_ESCAPE_PERCENT);
	if(!hConnect)
	{
		message->error = hSession->ssl.error = 0;
		message->title = _( "Security error" );
		message->text = _( "Can't create HTTP request." );
		debug("%s",message->text);
		errno = EINVAL;
		return NULL;
	}

	WinHttpSetOption(hRequest, WINHTTP_OPTION_CLIENT_CERT_CONTEXT, WINHTTP_NO_CLIENT_CERT_CONTEXT, 0);

	// Send request.
	if(!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
	{
		message->error = hSession->ssl.error = 0;
		message->title = _( "Security error" );
		message->text = _( "Can't send HTTP request." );
		debug("%s",message->text);
		errno = EINVAL;
		return NULL;
	}

	// Get response
	if(!WinHttpReceiveResponse(hRequest, NULL))
	{
		message->error = hSession->ssl.error = 0;
		message->title = _( "Security error" );
		message->text = _( "Can't receive HTTP response." );
		debug("%s",message->text);
		errno = EINVAL;
		return NULL;
	}

	DWORD szResponse = 0;
	if(!WinHttpQueryDataAvailable(hRequest, &szResponse))
	{
		message->error = hSession->ssl.error = 0;
		message->title = _( "Security error" );
		message->text = _( "Empty response from HTTP server." );
		debug("%s",message->text);
		errno = EINVAL;
		return NULL;
	}

	lib3270_autoptr(char) httpText = lib3270_malloc(szResponse+1);
	memset(httpText,0,szResponse+1);

	debug("Response length: %u", (unsigned int) szResponse);

	if(!WinHttpReadData(hRequest,httpText,szResponse,&szResponse)){
		message->error = hSession->ssl.error = 0;
		message->title = _( "Security error" );
		message->text = _( "Can't read HTTP response." );
		debug("%s",message->text);
		errno = EINVAL;
		return NULL;
	}

	//
	// Parse CRL
	//
	X509_CRL * x509_crl = NULL;

	if(!d2i_X509_CRL(&x509_crl, (const unsigned char **) &httpText, szResponse))
	{
		message->error = hSession->ssl.error = ERR_get_error();
		message->title = _( "Security error" );
		message->text = _( "Can't decode certificate revocation list" );
		lib3270_write_log(hSession,"ssl","%s: %s",consturl, message->text);
		return NULL;
	}

	return x509_crl;

}

#endif //  defined(HAVE_LIBSSL) && defined(SSL_ENABLE_CRL_CHECK)
