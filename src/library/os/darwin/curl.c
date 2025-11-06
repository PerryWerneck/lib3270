/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright 1995, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006 by Paul Mattes.
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

#include <config.h>

#if defined(HAVE_LIBCURL)

#include <internals.h>
#include <lib3270.h>
#include <lib3270/log.h>
#include <lib3270/trace.h>
#include <lib3270/memory.h>
#include <private/trace.h>
#include <curl/curl.h>

#define CRL_DATA_LENGTH 2048

/*--[ Implement ]------------------------------------------------------------------------------------*/

static inline void lib3270_autoptr_cleanup_CURL(CURL **ptr) {
	debug("%s(%p)",__FUNCTION__,*ptr);
	if(*ptr)
		curl_easy_cleanup(*ptr);
	*ptr = NULL;
}

typedef struct _curldata {
	size_t		  		  length;
	H3270				* hSession;
	char 				  errbuf[CURL_ERROR_SIZE];
	struct {
		size_t			  length;
		unsigned char	* contents;
	} data;
} CURLDATA;

static inline void lib3270_autoptr_cleanup_CURLDATA(CURLDATA **ptr) {
	debug("%s(%p)",__FUNCTION__,*ptr);
	if(*ptr) {
		CURLDATA *cdata = *ptr;

		if(cdata->data.contents) {
			lib3270_free(cdata->data.contents);
			cdata->data.contents = NULL;
		}
		lib3270_free(cdata);
	}
	*ptr = NULL;
}

static size_t internal_curl_write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
	CURLDATA * data = (CURLDATA *) userp;

	debug("%s",__FUNCTION__);

	size_t realsize = size * nmemb;

	debug("%s size=%d data->length=%d crldatalength=%d",__FUNCTION__,(int) size, (int) data->length, CRL_DATA_LENGTH);

	if((realsize + data->length) > data->data.length) {
		data->data.length += (CRL_DATA_LENGTH + realsize);
		data->data.contents = lib3270_realloc(data->data.contents,data->data.length);
		memset(&(data->data.contents[data->length]),0,data->data.length-data->length);
	}

	debug("%s",__FUNCTION__);

	if(lib3270_get_toggle(data->hSession,LIB3270_TOGGLE_SSL_TRACE)) {
		trace_data(
		    data->hSession,
		    "Received",
		    (const unsigned char *) contents,
		    realsize
		);
	}

	debug("%s",__FUNCTION__);

	memcpy(&(data->data.contents[data->length]),contents,realsize);
	data->length += realsize;

	debug("%s",__FUNCTION__);

	return realsize;
}

static int internal_curl_trace_callback(CURL GNUC_UNUSED(*handle), curl_infotype type, char *data, size_t size, void *userp) {
	const char * text = NULL;

	switch (type) {
	case CURLINFO_TEXT:
		lib3270_log_write(((CURLDATA *) userp)->hSession,"curl","%s",data);
		return 0;

	case CURLINFO_HEADER_OUT:
		text = "=> Send header";
		break;

	case CURLINFO_DATA_OUT:
		text = "=> Send data";
		break;

	case CURLINFO_SSL_DATA_OUT:
		text = "=> Send SSL data";
		break;

	case CURLINFO_HEADER_IN:
		text = "<= Recv header";
		break;

	case CURLINFO_DATA_IN:
		text = "<= Recv data";
		break;

	case CURLINFO_SSL_DATA_IN:
		text = "<= Recv SSL data";
		break;

	default:
		return 0;

	}

	trace_data(
	    ((CURLDATA *) userp)->hSession,
	    text,
	    (const unsigned char *) data,
	    size
	);

	return 0;
}

char * lib3270_url_get_using_curl(H3270 *hSession, const char *url, const char **error) {
	trace_event(hSession,"Getting data from %s",url);

	// Use CURL to download the CRL
	lib3270_autoptr(CURLDATA)	curl_data		= lib3270_malloc(sizeof(CURLDATA));
	lib3270_autoptr(CURL)		hCurl			= curl_easy_init();

	memset(curl_data,0,sizeof(CURLDATA));
	curl_data->hSession			= hSession;
	curl_data->data.length		= CRL_DATA_LENGTH;
	curl_data->data.contents	= lib3270_malloc(curl_data->data.length);

	if(!hCurl) {
		*error = _( "Can't initialize curl operation" );
		return NULL;
	}

	CURLcode res;

	curl_easy_setopt(hCurl, CURLOPT_URL, url);
	curl_easy_setopt(hCurl, CURLOPT_FOLLOWLOCATION, 1L);

	curl_easy_setopt(hCurl, CURLOPT_ERRORBUFFER, curl_data->errbuf);

	curl_easy_setopt(hCurl, CURLOPT_WRITEFUNCTION, internal_curl_write_callback);
	curl_easy_setopt(hCurl, CURLOPT_WRITEDATA, (void *) curl_data);

	curl_easy_setopt(hCurl, CURLOPT_USERNAME, "");

	if(lib3270_get_toggle(hSession,LIB3270_TOGGLE_SSL_TRACE)) {
		curl_easy_setopt(hCurl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(hCurl, CURLOPT_DEBUGFUNCTION, internal_curl_trace_callback);
		curl_easy_setopt(hCurl, CURLOPT_DEBUGDATA, (void *) curl_data);
	}

	res = curl_easy_perform(hCurl);

	if(res != CURLE_OK) {
		if(curl_data->errbuf[0])
			lib3270_log_write(hSession,"curl","%s: %s",url, curl_data->errbuf);

		*error = curl_easy_strerror(res);

		lib3270_log_write(hSession,"curl","%s: %s",url, *error);
		errno = EINVAL;
		return NULL;

	}

	char * httpText = lib3270_malloc(curl_data->length+1);
	memset(httpText,0,curl_data->length+1);
	memcpy(httpText,curl_data->data.contents,curl_data->length);

	return httpText;

}

#endif // HAVE_LIBCURL
