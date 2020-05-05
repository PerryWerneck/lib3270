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

 static int lib3270_get_connection_state_as_int(const H3270 *hSession)
 {
	return (int) lib3270_get_connection_state(hSession);
 }

 static int lib3270_get_program_message_as_int(const H3270 *hSession)
 {
	return (int) lib3270_get_program_message(hSession);
 }

 static int lib3270_get_ssl_state_as_int(const H3270 * hSession)
 {
 	return (int) lib3270_get_ssl_state(hSession);
 }

 static int lib3270_set_ssl_minimum_protocol_version(H3270 *hSession, int value)
 {
#ifdef HAVE_LIBSSL
	FAIL_IF_ONLINE(hSession);
	hSession->ssl.protocol.min_version = value;
	return 0;
#else
	return ENOTSUP;
#endif // HAVE_LIBSSL
 }

 static int lib3270_set_ssl_maximum_protocol_version(H3270 *hSession, int value)
 {
#ifdef HAVE_LIBSSL
	FAIL_IF_ONLINE(hSession);
	hSession->ssl.protocol.max_version = value;
	return 0;
#else
	return ENOTSUP;
#endif // HAVE_LIBSSL
 }

 static int lib3270_get_ssl_minimum_protocol_version(const H3270 *hSession)
 {
#ifdef HAVE_LIBSSL
	return hSession->ssl.protocol.min_version;
#else
	errno = ENOTSUP;
	return 0;
#endif // HAVE_LIBSSL
 }

 static int lib3270_get_ssl_maximum_protocol_version(const H3270 *hSession)
 {
#ifdef HAVE_LIBSSL
	return hSession->ssl.protocol.max_version;
#else
	errno = ENOTSUP;
	return 0;
#endif // HAVE_LIBSSL
 }

 const LIB3270_INT_PROPERTY * lib3270_get_int_properties_list(void)
 {

	static const LIB3270_INT_PROPERTY properties[] = {

		{
			.name = "cstate",									//  Property name.
			.description = N_( "Connection state" ),			//  Property description.
			.get = lib3270_get_connection_state_as_int,			//  Get value.
			.set = NULL											//  Set value.
		},

		{
			.name = "program_message",							//  Property name.
			.description = N_( "Latest program message" ),		//  Property description.
			.get = lib3270_get_program_message_as_int,			//  Get value.
			.set = NULL											//  Set value.
		},

		{
			.name = "ssl_state",										//  Property name.
			.description = N_( "ID of the session security state" ),	//  Property description.
			.get = lib3270_get_ssl_state_as_int,						//  Get value.
			.set = NULL													//  Set value.
		},

 		{
			.name = "ssl_min_protocol_version",									//  Property name.
			.description = N_( "ID of the minimum supported SSL protocol version" ),	//  Property description.
			.default_value = 0,
			.get = lib3270_get_ssl_minimum_protocol_version,				//  Get value.
			.set = lib3270_set_ssl_minimum_protocol_version				//  Set value.
		},

 		{
			.name = "ssl_max_protocol_version",									//  Property name.
			.description = N_( "ID of the maximum supported SSL protocol version" ),	//  Property description.
			.default_value = 0,
			.get = lib3270_get_ssl_maximum_protocol_version,				//  Get value.
			.set = lib3270_set_ssl_maximum_protocol_version				//  Set value.
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

