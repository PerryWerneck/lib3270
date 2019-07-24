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
 *
 * References:
 *
 * http://www.openssl.org/docs/ssl/
 * https://stackoverflow.com/questions/4389954/does-openssl-automatically-handle-crls-certificate-revocation-lists-now
 *
 */

#define CRL_DATA_LENGTH 2048

#include <config.h>

#include <winsock2.h>
#include <windows.h>
#include <winldap.h>

#if defined(HAVE_LIBSSL) && defined(SSL_ENABLE_CRL_CHECK)

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509_vfy.h>
#include <openssl/x509.h>

#ifdef HAVE_LIBCURL
	#include <curl/curl.h>
#endif // HAVE_LIBCURL

#include "../../private.h"
#include <trace_dsc.h>
#include <errno.h>
#include <lib3270.h>
#include <lib3270/trace.h>
#include <lib3270/log.h>

/*--[ Implement ]------------------------------------------------------------------------------------*/

static inline void lib3270_autoptr_cleanup_FILE(FILE **file)
{
	if(*file)
		fclose(*file);
}

#ifdef HAVE_LIBCURL
static inline void lib3270_autoptr_cleanup_CURL(CURL **ptr)
{
	debug("%s(%p)",__FUNCTION__,*ptr);
	if(*ptr)
		curl_easy_cleanup(*ptr);
	*ptr = NULL;
}

typedef struct _curldata
{
	size_t		  		  length;
	H3270				* hSession;
	SSL_ERROR_MESSAGE	* message;
	char 				  errbuf[CURL_ERROR_SIZE];
	struct {
		size_t			  length;
		unsigned char	* contents;
	} data;
} CURLDATA;

static inline void lib3270_autoptr_cleanup_CURLDATA(CURLDATA **ptr)
{
	debug("%s(%p)",__FUNCTION__,*ptr);
	if(*ptr)
	{
		CURLDATA *cdata = *ptr;

		if(cdata->data.contents) {
			lib3270_free(cdata->data.contents);
			cdata->data.contents = NULL;
		}
		lib3270_free(cdata);
	}
	*ptr = NULL;
}

static inline void lib3270_autoptr_cleanup_BIO(BIO **ptr)
{
	debug("%s(%p)",__FUNCTION__,*ptr);
	if(*ptr)
		BIO_free_all(*ptr);
	*ptr = NULL;
}

static size_t internal_curl_write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
	CURLDATA * data = (CURLDATA *) userp;

	size_t realsize = size * nmemb;
	size_t ix;

	debug("Received %u bytes (datablock is %p)", (unsigned int) realsize, data);

	unsigned char *ptr = (unsigned char *) contents;

	if(lib3270_get_toggle(data->hSession,LIB3270_TOGGLE_SSL_TRACE))
		lib3270_trace_data(data->hSession,"curl_write:",(const char *) contents, realsize);

	if((realsize + data->length) > data->data.length)
	{
		data->data.length += (CRL_DATA_LENGTH + realsize);
		data->data.contents = lib3270_realloc(data->data.contents,data->data.length);

		for(ix = data->length; ix < data->data.length; ix++)
		{
			data->data.contents[ix] = 0;
		}

	}

	for(ix = 0; ix < realsize; ix++)
	{
		data->data.contents[data->length++] = *(ptr++);
	}

	return realsize;
}

static int internal_curl_trace_callback(CURL GNUC_UNUSED(*handle), curl_infotype type, char *data, size_t size, void *userp)
{
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
		data,
		size
	);

	return 0;
}

#endif // HAVE_LIBCURL


LIB3270_INTERNAL X509_CRL * lib3270_get_crl(H3270 *hSession, SSL_ERROR_MESSAGE * message, const char *consturl)
{
	X509_CRL * x509_crl = NULL;

	if(!(consturl && *consturl))
	{
		message->error = hSession->ssl.error = 0;
		message->title = _( "Security error" );
		message->text = _( "Can't open CRL File" );
		message->description = _("The URL for the CRL is undefined or empty");
		errno = ENOENT;
		return NULL;
	}

	trace_ssl(hSession, "Getting CRL from \"%s\"\n",consturl);

	if(strncasecmp(consturl,"file://",7) == 0)
	{
		lib3270_autoptr(FILE) hCRL = fopen(consturl+7,"r");

		if(!hCRL)
		{
			// Can't open CRL File.
			int err = errno;

			message->error = hSession->ssl.error = 0;
			message->title = _( "Security error" );
			message->text = _( "Can't open CRL File" );
			message->description = strerror(err);
			trace_ssl(hSession,"Can't open %s: %s\n",consturl,message->description);
			return NULL;

		}

		trace_ssl(hSession,"Loading CRL from %s\n",consturl+7);
		if(d2i_X509_CRL_fp(hCRL, &x509_crl))
		{
			message->error = hSession->ssl.error = ERR_get_error();
			message->title = _( "Security error" );
			message->text = _( "Can't decode CRL" );
			lib3270_write_log(hSession,"ssl","%s: %s",consturl, message->text);
			return NULL;
		}



	}
	else
	{
#ifdef HAVE_LIBCURL

		// Use CURL to download the CRL
		lib3270_autoptr(CURLDATA)	crl_data		= lib3270_malloc(sizeof(CURLDATA));
		lib3270_autoptr(CURL)		hCurl			= curl_easy_init();

		memset(crl_data,0,sizeof(CURLDATA));
		crl_data->message		= message;
		crl_data->hSession		= hSession;
		crl_data->data.length	= CRL_DATA_LENGTH;
		crl_data->data.contents = lib3270_malloc(crl_data->data.length);

		if(hCurl)
		{
			CURLcode res;

			curl_easy_setopt(hCurl, CURLOPT_URL, consturl);
			curl_easy_setopt(hCurl, CURLOPT_FOLLOWLOCATION, 1L);

			curl_easy_setopt(hCurl, CURLOPT_ERRORBUFFER, crl_data->errbuf);

			curl_easy_setopt(hCurl, CURLOPT_WRITEFUNCTION, internal_curl_write_callback);
			curl_easy_setopt(hCurl, CURLOPT_WRITEDATA, (void *) crl_data);

			curl_easy_setopt(hCurl, CURLOPT_USERNAME, "");

			if(lib3270_get_toggle(hSession,LIB3270_TOGGLE_SSL_TRACE))
			{
				curl_easy_setopt(hCurl, CURLOPT_VERBOSE, 1L);
				curl_easy_setopt(hCurl, CURLOPT_DEBUGFUNCTION, internal_curl_trace_callback);
				curl_easy_setopt(hCurl, CURLOPT_DEBUGDATA, (void *) crl_data);
			}

			res = curl_easy_perform(hCurl);

			if(res != CURLE_OK)
			{
				message->error = hSession->ssl.error = 0;
				message->title = _( "Security error" );

				if(crl_data->errbuf[0])
				{
					message->text = curl_easy_strerror(res);
					message->description =  crl_data->errbuf;
				}
				else
				{
					message->text = _( "Error loading CRL" );
					message->description =  curl_easy_strerror(res);
				}

				lib3270_write_log(hSession,"ssl","%s: %s",consturl, message->description);
				errno = EINVAL;
				return NULL;

			}

			char *ct = NULL;
			res = curl_easy_getinfo(hCurl, CURLINFO_CONTENT_TYPE, &ct);
			if(res != CURLE_OK)
			{
				message->error = hSession->ssl.error = 0;
				message->title = _( "Security error" );
				message->text = _( "Error loading CRL" );
				message->description =  curl_easy_strerror(res);
				lib3270_write_log(hSession,"ssl","%s: %s",consturl, message->description);
				errno = EINVAL;
				return NULL;
			}

			if(lib3270_get_toggle(crl_data->hSession,LIB3270_TOGGLE_SSL_TRACE))
				lib3270_trace_data(crl_data->hSession,"CRL Data",(const char *) crl_data->data.contents, (unsigned int) crl_data->length);

			if(ct)
			{
				const unsigned char * data = crl_data->data.contents;

				if(strcasecmp(ct,"application/pkix-crl") == 0)
				{
					// CRL File, convert it
					if(!d2i_X509_CRL(&x509_crl, &data, crl_data->length))
					{
						message->error = hSession->ssl.error = ERR_get_error();
						message->title = _( "Security error" );
						message->text = _( "Can't decode CRL" );
						lib3270_write_log(hSession,"ssl","%s: %s",consturl, message->text);
						return NULL;
					}
				}
				else
				{
					message->error = hSession->ssl.error = ERR_get_error();
					message->title = _( "Security error" );
					message->text = _( "Got an invalid CRL from server" );
					lib3270_write_log(hSession,"ssl","%s: content-type unexpected: \"%s\"",consturl, ct);
					errno = EINVAL;
					return NULL;
				}
			}
			else if(strncasecmp(consturl,"ldap://",7) == 0)
			{
				//
				// curl's LDAP query on windows returns diferently. Working with it.
				//
#ifdef DEBUG
				{
					FILE *out = fopen("downloaded.crl","w");
					if(out)
					{
						fwrite(crl_data->data.contents,crl_data->length,1,out);
						fclose(out);
					}

				}
#endif

				char * attr = strchr(consturl,'?');
				if(!attr)
				{
					message->error = hSession->ssl.error = 0;
					message->title = _( "Security error" );
					message->text = _( "No attribute in LDAP search URL" );
					errno = ENOENT;
					return NULL;
				}

				attr++;

				lib3270_autoptr(char) text = lib3270_strdup_printf("No mime-type, extracting \"%s\" directly from LDAP response\n",attr);
				trace_ssl(crl_data->hSession, text);

				lib3270_autoptr(char) key = lib3270_strdup_printf("%s: ",attr);


//				char *ptr = strcasestr((char *) crl_data->data.contents, key);

				size_t ix;
				unsigned char *from = NULL;
				size_t keylength = strlen(key);
				for(ix = 0; ix < (crl_data->length - keylength); ix++)
				{
					if(!strncasecmp( (char *) (crl_data->data.contents+ix),key,keylength))
					{
						from = crl_data->data.contents+ix;
						break;
					}
				}

				debug("strstr(%s): %p", key, from);

				if(!from)
				{
					message->error = hSession->ssl.error = 0;
					message->title = _( "Security error" );
					message->text = _( "Can't find attribute in LDAP response" );
					errno = ENOENT;
					return NULL;
				}

				from += strlen(key);
				size_t length = crl_data->length - (from - crl_data->data.contents);

				static const char terminator[] = { 0x0a, 0x0a, 0x09 };
				unsigned char *to = from+length;

				for(ix = 0; ix < (length - sizeof(terminator)); ix++)
				{
					if(!memcmp(from+ix,terminator,sizeof(terminator)))
					{
						to = from+ix;
						break;
					}
				}

				length = to - from;

				if(lib3270_get_toggle(hSession,LIB3270_TOGGLE_SSL_TRACE))
				{
					lib3270_trace_data(
						hSession,
						"CRL Data received from LDAP server",
						(const char *) from,
						length
					);
				}

				if(!d2i_X509_CRL(&x509_crl, (const unsigned char **) &from, length))
				{
					message->error = hSession->ssl.error = ERR_get_error();
					message->title = _( "Security error" );
					message->text = _( "Can't decode CRL got from LDAP Search" );
					lib3270_write_log(hSession,"ssl","%s: %s",consturl, message->text);
					errno = EINVAL;
					return NULL;
				}

			}
		}

#else
		// Can't get CRL.

		message->error = hSession->ssl.error = 0;
		message->title = _( "Security error" );
		message->text = _( "Unexpected or invalid CRL URL" );
		message->description = _("The URL scheme is unknown");
		lib3270_write_log(hSession,"ssl","%s: %s",consturl, message->description);
		errno = EINVAL;
		return NULL;

#endif // HAVE_LIBCURL
	}

	return x509_crl;

}

#endif // HAVE_LIBSSL && SSL_ENABLE_CRL_CHECK
