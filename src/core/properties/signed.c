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

 static int lib3270_get_connection_state_as_int(H3270 *hSession)
 {
	return (int) lib3270_get_connection_state(hSession);
 }

 static int lib3270_get_program_message_as_int(H3270 *hSession)
 {
	return (int) lib3270_get_program_message(hSession);
 }

 static int lib3270_get_ssl_state_as_int(H3270 * hSession)
 {
 	return (int) lib3270_get_ssl_state(hSession);
 }

 const LIB3270_INT_PROPERTY * lib3270_get_int_properties_list(void)
 {

	static const LIB3270_INT_PROPERTY properties[] = {

		{
			"model_number",								//  Property name.
			N_( "The model number" ),					//  Property description.
			lib3270_get_model_number,					//  Get value.
			NULL										//  Set value.
		},

		{
			"color_type",								//  Property name.
			N_( "The color type" ),						//  Property description.
			lib3270_get_color_type,						//  Get value.
			lib3270_set_color_type						//  Set value.
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
			"ssl_state",								//  Property name.
			N_( "ID of the session security state" ),	//  Property description.
			lib3270_get_ssl_state_as_int,				//  Get value.
			NULL										//  Set value.
		},

		/*
		{
			"",						//  Property name.
			"",						//  Property description.
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

