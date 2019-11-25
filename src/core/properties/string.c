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
 #include <internals.h>
 #include <string.h>
 #include <lib3270.h>
 #include <lib3270/properties.h>
 #include <lib3270/keyboard.h>
 #include <lib3270/log.h>

 static const char * get_version(const H3270 GNUC_UNUSED(*hSession))
 {
	return lib3270_get_version();
 }

 static const char * get_revision(const H3270 GNUC_UNUSED(*hSession))
 {
	return lib3270_get_revision();
 }

 static const char * lib3270_get_termtype(const H3270 *hSession)
 {
 	return hSession->termtype;
 }

 static const char * lib3270_get_termname(const H3270 *hSession)
 {
 	return hSession->termname;
 }

 LIB3270_EXPORT const LIB3270_STRING_PROPERTY * lib3270_get_string_properties_list(void)
 {
	 static const LIB3270_STRING_PROPERTY properties[] = {

		{
			.name = "associated_lu",												//  Property name.
			.description = N_( "The name of the LU associated with the session" ),	//  Property description.
			.get = lib3270_get_associated_luname,									//  Get value.
			.set = NULL																//  Set value.
		},

		{
			.name = "url",															// Property name.
#ifdef LIB3270_DEFAULT_HOST
			.default_value = LIB3270_DEFAULT_HOST,									// Default value.
#endif // LIB3270_DEFAULT_HOST
			.group = LIB3270_ACTION_GROUP_OFFLINE,									// Property group.
			.icon = "network-server",												// Property icon.
			.description = N_( "URL of the current host" ),							// Property description.
			.get = lib3270_get_url,													// Get value.
			.set = lib3270_set_url													// Set value.
		},

		{
			.name = "model",														// Property name.
			.group = LIB3270_ACTION_GROUP_OFFLINE,									// Property group.
			.description = N_( "Model name" ),										// Property description.
			.get = lib3270_get_model_name,											// Get value.
			.set = lib3270_set_model_name											// Set value.
		},

		{
			.name = "host_type_name",												// Property name.
			.group = LIB3270_ACTION_GROUP_OFFLINE,									// Property group.
			.description = N_( "Host type name" ),									// Property description.
			.get = lib3270_get_host_type_name,										// Get value.
			.set = lib3270_set_host_type_by_name									// Set value.
		},

		{
			.name = "termtype",														//  Property name.
			.description = N_( "Terminal type" ),									//  Property description.
			.get = lib3270_get_termtype,											//  Get value.
			.set = NULL																//  Set value.
		},

		{
			.name = "termname",														//  Property name.
			.description = N_( "Terminal name" ),									//  Property description.
			.get = lib3270_get_termname,											//  Get value.
			.set = NULL																//  Set value.
		},

		{
			.name = "host_charset",													// Property name.
			.default_value = "bracket",												// Default charset.
			.group = LIB3270_ACTION_GROUP_OFFLINE,									// Property group.
			.description = N_( "Host charset" ),									// Property description.
			.get = lib3270_get_host_charset,										// Get value.
			.set = lib3270_set_host_charset											// Set value.
		},

		{
			.name = "display_charset",								//  Property name.
			.description = N_( "Display charset" ),					//  Property description.
			.get = lib3270_get_display_charset,						//  Get value.
			.set = NULL												//  Set value.
		},

		{
			.name = "version",										//  Property name.
			.description = N_( "lib3270 version" ),					//  Property description.
			.get = get_version,										//  Get value.
			.set = NULL												//  Set value.
		},

		{
			.name = "revision",										//  Property name.
			.description = N_( "lib3270 revision" ),				//  Property description.
			.get = get_revision,									//  Get value.
			.set = NULL												//  Set value.
		},

		{
			.name = "crlpath",													//  Property name.
			.group = LIB3270_ACTION_GROUP_OFFLINE,								// Property group.
			.description = N_( "URL for the certificate revocation list" ),		//  Property description.
			.get = lib3270_get_crl_url,											//  Get value.
			.set = lib3270_set_crl_url,											//  Set value.
		},

		{
			.name = "crlprefer",												//  Property name.
			.group = LIB3270_ACTION_GROUP_OFFLINE,								// Property group.
			.description = N_( "Prefered protocol for CRL" ),					//  Property description.
			.get = lib3270_get_crl_prefered_protocol,							//  Get value.
			.set = lib3270_set_crl_prefered_protocol,							//  Set value.
		},

		{
			.name = "default_host",												//  Property name.
			.description = N_( "Default host URL" ),							//  Property description.
			.get = lib3270_get_default_host,									//  Get value.
			.set = NULL															//  Set value.
		},

		{
			.name = "sslmessage",												//  Property name.
			.description = N_( "The security state" ),							//  Property description.
			.get = lib3270_get_ssl_state_message,								//  Get value.
			.set = NULL															//  Set value.
		},

		{
			.name = "ssldescription",											//  Property name.
			.description = N_( "Description of the current security state" ),	//  Property description.
			.get = lib3270_get_ssl_state_description,							//  Get value.
			.set = NULL															//  Set value.
		},

		{
			.name = "oversize",														//  Property name.
			.group = LIB3270_ACTION_GROUP_OFFLINE,									// Property group.
			.description = N_( "Screen oversize if larger than the chosen model"),	//  Property description.
			.get = lib3270_get_oversize,											//  Get value.
			.set = lib3270_set_oversize												//  Set value.
		},

		{
			.name = NULL,
			.description = NULL,
			.get = NULL,
			.set = NULL
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

/*
LIB3270_EXPORT int lib3270_set_luname(H3270 *hSession, const char *luname)
{
    FAIL_IF_ONLINE(hSession);
	strncpy(hSession->lu.names,luname,LIB3270_LUNAME_LENGTH);
	return 0;
}
*/

LIB3270_EXPORT int lib3270_set_lunames(H3270 *hSession, const char *lunames)
{
    FAIL_IF_ONLINE(hSession);

    if(hSession->lu.names)
	{
		lib3270_free(hSession->lu.names);
		hSession->lu.names = NULL;
	}

	// Do I have lunames to set? If not just return.
	if(!lunames)
		return 0;

	//
	// Count the commas in the LU names.  That plus one is the
	// number of LUs to try.
	//
	char *comma;
	char *lu;
	int n_lus = 1;

	lu = (char *) lunames;
	while ((comma = strchr(lu, ',')) != CN)
	{
		n_lus++;
		lu++;
	}

	//
	// Allocate enough memory to construct an argv[] array for
	// the LUs.
	//
	Replace(hSession->lu.names,(char **)lib3270_malloc((n_lus+1) * sizeof(char *) + strlen(lunames) + 1));

	// Copy each LU into the array.
	lu = (char *)(hSession->lu.names + n_lus + 1);
	(void) strcpy(lu, lunames);

	size_t i = 0;
	do
	{
		hSession->lu.names[i++] = lu;
		comma = strchr(lu, ',');
		if (comma != CN)
		{
			*comma = '\0';
			lu = comma + 1;
		}
	} while (comma != CN);

	hSession->lu.names[i]	= CN;

    return 0;
}

LIB3270_EXPORT const char ** lib3270_get_lunames(H3270 *hSession)
{
	return (const char **) hSession->lu.names;
}

