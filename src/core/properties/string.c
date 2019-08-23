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

 #include <config.h>
 #include <lib3270-internals.h>
 #include <string.h>
 #include <lib3270.h>
 #include <lib3270/properties.h>
 #include <lib3270/keyboard.h>

 static const char * get_version(H3270 GNUC_UNUSED(*hSession))
 {
	return lib3270_get_version();
 }

 static const char * get_revision(H3270 GNUC_UNUSED(*hSession))
 {
	return lib3270_get_revision();
 }

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

		{
			"oversize",												//  Property name.
			N_( "Screen oversize if larger than the chosen model"),	//  Property description.
			lib3270_get_oversize,									//  Get value.
			lib3270_set_oversize									//  Set value.
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

int lib3270_set_string_property(H3270 *hSession, const char *name, const char * value, int seconds)
{
	size_t ix;

	if(seconds)
	{
		lib3270_wait_for_ready(hSession, seconds);
	}

	//
	// Check for string property
	//
	{
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
	}

	//
	// Check for signed int property
	//
    {
		const LIB3270_INT_PROPERTY * properties = lib3270_get_int_properties_list();
		for(ix = 0; properties[ix].name; ix++)
		{
			if(!strcasecmp(name,properties[ix].name))
			{
				if(properties[ix].set)
				{
					return properties[ix].set(hSession, atoi(value));
				}
				else
				{
					errno = EPERM;
					return -1;
				}
			}

		}
    }

    //
    // Check for unsigned int property
    //
    {
		const LIB3270_UINT_PROPERTY * properties = lib3270_get_unsigned_properties_list();
		for(ix = 0; properties[ix].name; ix++)
		{
			if(!strcasecmp(name,properties[ix].name))
			{
				if(properties[ix].set)
				{
					return properties[ix].set(hSession, strtoul(value,NULL,0));
				}
				else
				{
					errno = EPERM;
					return -1;
				}
			}

		}
    }

    //
    // Check for boolean property
    //
    {
		const LIB3270_INT_PROPERTY * properties = lib3270_get_int_properties_list();
		for(ix = 0; properties[ix].name; ix++)
		{
			if(!strcasecmp(name,properties[ix].name))
			{
				if(properties[ix].set)
				{
					return properties[ix].set(hSession, atoi(value));
				}
				else
				{
					errno = EPERM;
					return -1;
				}
			}

		}
    }

	errno = ENOENT;
	return -1;

}

