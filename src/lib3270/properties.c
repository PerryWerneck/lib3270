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
 *	@brief This module handles the properties get/set methods.
 */

 #include <config.h>
 #include "private.h"
 #include <string.h>
 #include <lib3270.h>
 #include <lib3270/properties.h>

#if defined(HAVE_LIBSSL)
	#include <openssl/ssl.h>
#endif

 static int lib3270_get_connection_state_as_int(H3270 *hSession)
 {
	return (int) lib3270_get_connection_state(hSession);
 }

 static int lib3270_get_program_message_as_int(H3270 *hSession)
 {
	return (int) lib3270_get_program_message(hSession);
 }

 int lib3270_is_starting(H3270 *hSession)
 {
	return hSession->starting != 0;
 }

 int lib3270_get_formatted(H3270 *hSession)
 {
	return hSession->formatted != 0;
 }

 const LIB3270_INT_PROPERTY * lib3270_get_boolean_properties_list(void) {

	static const LIB3270_INT_PROPERTY properties[] = {
		{
			"ready",											//  Property name.
			N_( "Is terminal ready" ),							//  Property description.
			lib3270_is_ready,									//  Get value.
			NULL												//  Set value.
		},

		{
			"connected",										//  Property name.
			N_( "Is terminal connected" ),						//  Property description.
			lib3270_is_connected,								//  Get value.
			NULL												//  Set value.
		},

		{
			"secure",											//  Property name.
			N_( "Is connection secure" ),						//  Property description.
			lib3270_is_secure,									//  Get value.
			NULL												//  Set value.
		},

		{
			"tso",												//  Property name.
			N_( "Non zero if the host is TSO." ),				//  Property description.
			lib3270_is_tso,										//  Get value.
			lib3270_set_tso										//  Set value.
		},

		{
			"as400",											//  Property name.
			N_( "Non zero if the host is AS400." ),				//  Property description.
			lib3270_is_as400,									//  Get value.
			lib3270_set_as400									//  Set value.
		},

		{
			"pconnected",										//  Property name.
			"",											//  Property description.
			lib3270_pconnected,									//  Get value.
			NULL												//  Set value.
		},

		{
			"half_connected",							//  Property name.
			"",											//  Property description.
			lib3270_half_connected,						//  Get value.
			NULL										//  Set value.
		},

		{
			"neither",						//  Property name.
			"",						//  Property description.
			lib3270_in_neither,				//  Get value.
			NULL							//  Set value.
		},

		{
			"ansi",							//  Property name.
			"",						//  Property description.
			lib3270_in_ansi,				//  Get value.
			NULL							//  Set value.
		},

		{
			"tn3270",									//  Property name.
			N_( "State is 3270, TN3270e or SSCP" ),		//  Property description.
			lib3270_in_3270,							//  Get value.
			NULL										//  Set value.
		},

		{
			"sscp",							//  Property name.
			"",						//  Property description.
			lib3270_in_sscp,				//  Get value.
			NULL							//  Set value.
		},

		{
			"tn3270e",						//  Property name.
			"",						//  Property description.
			lib3270_in_tn3270e,				//  Get value.
			NULL							//  Set value.
		},

		{
			"e",																				//  Property name.
			N_( "Is terminal in the INITIAL_E state?" ),										//  Property description.
			lib3270_in_e,																		//  Get value.
			NULL																				//  Set value.
		},

		{
			"has_selection",				//  Property name.
			N_( "Has selected area" ),		//  Property description.
			lib3270_has_selection,			//  Get value.
			NULL							//  Set value.
		},

		{
			"starting",																			//  Property name.
			N_( "Is starting (no first screen)?" ),												//  Property description.
			lib3270_is_starting,																//  Get value.
			NULL																				//  Set value.
		},

		{
			"formatted",																		//  Property name.
			N_( "Formatted screen" ),															//  Property description.
			lib3270_get_formatted,																//  Get value.
			NULL																				//  Set value.
		},

		/*
		{
			"",						//  Property name.
			"",				//  Property description.
			NULL,					//  Get value.
			NULL					//  Set value.
		},
		*/

		{
			NULL,
			NULL,
			NULL,
			NULL
		}

	};

	return properties;

 }

 const LIB3270_INT_PROPERTY * lib3270_get_int_properties_list(void) {

	static const LIB3270_INT_PROPERTY properties[] = {

		{
			"cursor_address",				//  Property name.
			N_( "Cursor address" ),			//  Property description.
			lib3270_get_cursor_address,		//  Get value.
			lib3270_set_cursor_address		//  Set value.
		},

		{
			"model_number",					//  Property name.
			N_( "The model number" ),		//  Property description.
			lib3270_get_model_number,		//  Get value.
			NULL							//  Set value.
		},

		{
			"color_type",					//  Property name.
			N_( "The color type" ),			//  Property description.
			lib3270_get_color_type,			//  Get value.
			lib3270_set_color_type			//  Set value.
		},

		{
			"width",//  Property name.
			N_( "Current screen width in columns" ),	//  Property description.
			lib3270_get_width,							//  Get value.
			NULL										//  Set value.
		},

		{
			"height",									//  Property name.
			N_( "Current screen width in rows" ),		//  Property description.
			lib3270_get_height,							//  Get value.
			NULL										//  Set value.
		},

		{
			"length",									//  Property name.
			N_( "Screen buffer length in bytes" ),		//  Property description.
			lib3270_get_length,							//  Get value.
			NULL										//  Set value.
		},

		{
			"cstate",									//  Property name.
			N_( "Connection state" ),					//  Property description.
			lib3270_get_connection_state_as_int,		//  Get value.
			NULL										//  Set value.
		},

		{
			"program_message",							//  Property name.
			N_( "Latest program message" ),				//  Property description.
			lib3270_get_program_message_as_int,			//  Get value.
			NULL										//  Set value.
		},

		{
			"unlock_delay",																			//  Property name.
			N_( "The delay between the host unlocking the keyboard and the actual unlock" ),		//  Property description.
			lib3270_get_unlock_delay,																//  Get value.
			lib3270_set_unlock_delay																//  Set value.
		},

		/*
		{
			"",						//  Property name.
			"",				//  Property description.
			NULL,					//  Get value.
			NULL					//  Set value.
		},
		*/

		{
			NULL,
			NULL,
			NULL,
			NULL
		}
	 };

	 return properties;
 }

 static const char * get_version(H3270 *hSession unused)
 {
	return lib3270_get_version();
 }

 static const char * get_revision(H3270 *hSession unused)
 {
	return lib3270_get_revision();
 }

 #pragma GCC diagnostic push
 #pragma GCC diagnostic ignored "-Wunused-parameter"
 const char * lib3270_get_crl_url(H3270 *hSession)
 {
#ifdef SSL_ENABLE_CRL_CHECK
	if(hSession->ssl.crl.url)
		return hSession->ssl.crl.url;

#ifdef SSL_DEFAULT_CRL_URL
	return SSL_DEFAULT_CRL_URL;
#else
	return getenv("LIB3270_DEFAULT_CRL");
#endif // SSL_DEFAULT_CRL_URL

#else
	errno = ENOTSUP;
	return "";
#endif
 }
 #pragma GCC diagnostic pop

 #pragma GCC diagnostic push
 #pragma GCC diagnostic ignored "-Wunused-parameter"
 int lib3270_set_crl_url(H3270 *hSession, const char *crl)
 {

    FAIL_IF_ONLINE(hSession);

#ifdef SSL_ENABLE_CRL_CHECK

	if(hSession->ssl.crl.url)
	{
		free(hSession->ssl.crl.url);
		hSession->ssl.crl.url = NULL;
	}

	if(hSession->ssl.crl.cert)
	{
		X509_CRL_free(hSession->ssl.crl.cert);
		hSession->ssl.crl.cert = NULL;
	}

	if(crl)
	{
		hSession->ssl.crl.url = strdup(crl);
	}

	return 0;

#else

	return errno = ENOTSUP;

#endif // SSL_ENABLE_CRL_CHECK

 }
 #pragma GCC diagnostic pop

 LIB3270_EXPORT const LIB3270_STRING_PROPERTY * lib3270_get_string_properties_list(void)
 {
	 static const LIB3270_STRING_PROPERTY properties[] = {

		{
			"luname",									//  Property name.
			N_( "The name of the active LU" ),			//  Property description.
			lib3270_get_luname,							//  Get value.
			lib3270_set_luname							//  Set value.
		},

		{
			"url",										//  Property name.
			N_( "URL of the current host" ),			//  Property description.
			lib3270_get_url,							//  Get value.
			lib3270_set_url								//  Set value.
		},

		{
			"model",									//  Property name.
			N_( "Model name" ),							//  Property description.
			lib3270_get_model,							//  Get value.
			lib3270_set_model							//  Set value.
		},

		{
			"host_type",								//  Property name.
			N_( "Host type name" ),						//  Property description.
			lib3270_get_host_type_name,					//  Get value.
			lib3270_set_host_type_by_name				//  Set value.
		},

		{
			"host_charset",								//  Property name.
			N_( "Host charset" ),						//  Property description.
			lib3270_get_host_charset,					//  Get value.
			lib3270_set_host_charset					//  Set value.
		},

		{
			"display_charset",							//  Property name.
			N_( "Display charset" ),					//  Property description.
			lib3270_get_display_charset,				//  Get value.
			NULL										//  Set value.
		},

		{
			"version",									//  Property name.
			N_( "lib3270 version" ),					//  Property description.
			get_version,								//  Get value.
			NULL										//  Set value.
		},

		{
			"revision",									//  Property name.
			N_( "lib3270 revision" ),					//  Property description.
			get_revision,								//  Get value.
			NULL										//  Set value.
		},

		{
			"crlpath",											//  Property name.
			N_( "URL for the certificate revocation list" ),	//  Property description.
			lib3270_get_crl_url,								//  Get value.
			lib3270_set_crl_url,								//  Set value.
		},

		{
			"default_host",										//  Property name.
			N_( "Default host URL" ),							//  Property description.
			lib3270_get_default_host,							//  Get value.
			NULL												//  Set value.
		},

		{
			"sslmessage",										//  Property name.
			N_( "The security state" ),							//  Property description.
			lib3270_get_ssl_state_message,						//  Get value.
			NULL												//  Set value.
		},

		{
			"ssldescription",									//  Property name.
			N_( "Description of the current security state" ),	//  Property description.
			lib3270_get_ssl_state_description,					//  Get value.
			NULL												//  Set value.
		},

		/*
		{
			"",											//  Property name.
			"",									//  Property description.
			,											//  Get value.
			NULL										//  Set value.
		},
		*/

		{
			NULL,
			NULL,
			NULL,
			NULL
		}

	 };

	 return properties;

 }

 /*
 int lib3270_set_connected(H3270 *hSession, int state) {

	if(state) {

		if(lib3270_reconnect(hSession,120))
			return -1;

	} else {

		return lib3270_disconnect(hSession);
	}

	return 0;
 }
 */

int lib3270_get_int_property(H3270 *hSession, const char *name, int seconds)
{
	size_t ix;
	const LIB3270_INT_PROPERTY * properties;

	if(seconds)
	{
		lib3270_wait_for_ready(hSession, seconds);
	}

	// Check for boolean properties
	properties = lib3270_get_boolean_properties_list();
	for(ix = 0; properties[ix].name; ix++)
	{
		if(!strcasecmp(name,properties[ix].name))
		{
			if(properties[ix].get)
			{
				return properties[ix].get(hSession);
			}
			else
			{
				errno = EPERM;
				return -1;
			}
		}


	}

	// Check for int properties
	properties = lib3270_get_int_properties_list();
	for(ix = 0; properties[ix].name; ix++)
	{
		if(!strcasecmp(name,properties[ix].name))
		{
			if(properties[ix].get)
			{
				return properties[ix].get(hSession);
			}
			else
			{
				errno = EPERM;
				return -1;
			}
		}


	}

	errno = ENOENT;
	return -1;
}

int lib3270_set_int_property(H3270 *hSession, const char *name, int value, int seconds)
{
	size_t ix;
	const LIB3270_INT_PROPERTY * properties;

	if(seconds)
	{
		lib3270_wait_for_ready(hSession, seconds);
	}

	// Check for boolean properties
	properties = lib3270_get_boolean_properties_list();
	for(ix = 0; properties[ix].name; ix++)
	{
		if(!strcasecmp(name,properties[ix].name))
		{
			if(properties[ix].set)
			{
				return properties[ix].set(hSession, value);
			}
			else
			{
				errno = EPERM;
				return -1;
			}
		}

	}

	// Check for INT Properties
	properties = lib3270_get_int_properties_list();
	for(ix = 0; properties[ix].name; ix++)
	{
		if(!strcasecmp(name,properties[ix].name))
		{
			if(properties[ix].set)
			{
				return properties[ix].set(hSession, value);
			}
			else
			{
				errno = EPERM;
				return -1;
			}
		}

	}

	errno = ENOENT;
	return -1;

}

int lib3270_set_string_property(H3270 *hSession, const char *name, const char * value, int seconds)
{
	size_t ix;

	if(seconds)
	{
		lib3270_wait_for_ready(hSession, seconds);
	}

	const LIB3270_STRING_PROPERTY * properties = lib3270_get_string_properties_list();
	for(ix = 0; properties[ix].name; ix++)
	{
		if(!strcasecmp(name,properties[ix].name))
		{
			if(properties[ix].set)
			{
				return properties[ix].set(hSession, value);
			}
			else
			{
				errno = EPERM;
				return -1;
			}
		}

	}

	errno = ENOENT;
	return -1;

}

/**
 * @brief Get SSL host option.
 *
 * @return Non zero if the host URL has SSL scheme.
 *
 */
LIB3270_EXPORT int lib3270_get_secure_host(H3270 *hSession)
{
	CHECK_SESSION_HANDLE(hSession);

    // TODO: Find a better way!
	if(!hSession->host.current)
		lib3270_set_url(hSession,NULL);

#ifdef HAVE_LIBSSL
	return hSession->ssl.enabled ? 1 : 0;
#else
	return 0;
#endif // HAVE_LIBSSL

}

#ifdef SSL_ENABLE_CRL_CHECK
LIB3270_EXPORT char * lib3270_get_ssl_crl_text(H3270 *hSession)
{

	if(hSession->ssl.crl.cert)
	{

		BIO				* out = BIO_new(BIO_s_mem());
		unsigned char	* data;
		unsigned char	* text;
		int				  n;

		X509_CRL_print(out,hSession->ssl.crl.cert);

		n		= BIO_get_mem_data(out, &data);
		text	= (unsigned char *) lib3270_malloc(n+1);
		text[n]	='\0';

		memcpy(text,data,n);
		BIO_free(out);

		return (char *) text;

	}

	return NULL;

}
#else
LIB3270_EXPORT char * lib3270_get_ssl_crl_text(H3270 *hSession unused)
{
	return NULL;
}
#endif // SSL_ENABLE_CRL_CHECK


LIB3270_EXPORT char * lib3270_get_ssl_peer_certificate_text(H3270 *hSession)
{
#ifdef HAVE_LIBSSL
	if(hSession->ssl.con)
	{
		X509 * peer = SSL_get_peer_certificate(hSession->ssl.con);
		if(peer)
		{
			BIO				* out	= BIO_new(BIO_s_mem());
			unsigned char	* data;
			unsigned char	* text;
			int				  n;

			X509_print(out,peer);

			n		= BIO_get_mem_data(out, &data);
			text	= (unsigned char *) lib3270_malloc(n+1);
			text[n]	='\0';
			memcpy(text,data,n);
			BIO_free(out);

			return (char *) text;
		}
	}
#endif // HAVE_LIBSSL

	return NULL;
}
