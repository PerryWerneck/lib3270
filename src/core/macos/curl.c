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
 */

#include <config.h>

#if defined(HAVE_LIBCURL)

#include <internals.h>
#include <lib3270.h>
#include <lib3270/log.h>
#include <lib3270/trace.h>
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
		lib3270_trace_data(
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
		lib3270_write_log(((CURLDATA *) userp)->hSession,"curl","%s",data);
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

	lib3270_trace_data(
	    ((CURLDATA *) userp)->hSession,
	    text,
	    (const unsigned char *) data,
	    size
	);

	return 0;
}

char * lib3270_get_from_url(H3270 *hSession, const char *url, size_t *length, const char **error_message) {
	lib3270_write_event_trace(hSession,"Getting data from %s",url);

	// Use CURL to download the CRL
	lib3270_autoptr(CURLDATA)	crl_data		= lib3270_malloc(sizeof(CURLDATA));
	lib3270_autoptr(CURL)		hCurl			= curl_easy_init();

	memset(crl_data,0,sizeof(CURLDATA));
	crl_data->hSession		= hSession;
	crl_data->data.length	= CRL_DATA_LENGTH;
	crl_data->data.contents = lib3270_malloc(crl_data->data.length);

	if(!hCurl) {
		*error_message= _( "Can't initialize curl operation" );
		errno = EINVAL;
		return NULL;
	}

	CURLcode res;

	curl_easy_setopt(hCurl, CURLOPT_URL, url);
	curl_easy_setopt(hCurl, CURLOPT_FOLLOWLOCATION, 1L);

	curl_easy_setopt(hCurl, CURLOPT_ERRORBUFFER, crl_data->errbuf);

	curl_easy_setopt(hCurl, CURLOPT_WRITEFUNCTION, internal_curl_write_callback);
	curl_easy_setopt(hCurl, CURLOPT_WRITEDATA, (void *) crl_data);

	curl_easy_setopt(hCurl, CURLOPT_USERNAME, "");

	if(lib3270_get_toggle(hSession,LIB3270_TOGGLE_SSL_TRACE)) {
		curl_easy_setopt(hCurl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(hCurl, CURLOPT_DEBUGFUNCTION, internal_curl_trace_callback);
		curl_easy_setopt(hCurl, CURLOPT_DEBUGDATA, (void *) crl_data);
	}

	res = curl_easy_perform(hCurl);

	if(res != CURLE_OK) {
		if(crl_data->errbuf[0])
			lib3270_write_log(hSession,"curl","%s: %s",url, crl_data->errbuf);

		*error_message = curl_easy_strerror(res);

		lib3270_write_log(hSession,"curl","%s: %s",url, *error_message);
		errno = EINVAL;
		return NULL;

	}

	if(length)
		*length = (size_t) crl_data->length;

	char * httpText = lib3270_malloc(crl_data->length+1);
	memset(httpText,0,crl_data->length+1);
	memcpy(httpText,crl_data->data.contents,crl_data->length);

	return httpText;

}

#endif // HAVE_LIBCURL
