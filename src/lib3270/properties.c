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

 static const LIB3270_INT_PROPERTY properties[] = {

 	{
		"ready",											//  Property name.
		N_( "" ),											//  Property description.
		lib3270_is_ready,									//  Get value.
		NULL												//  Set value.
 	},

 	{
		"connected",										//  Property name.
		N_( "" ),											//  Property description.
		lib3270_is_connected,								//  Get value.
		lib3270_set_connected								//  Set value.
 	},

 	{
		"secure",											//  Property name.
		N_( "" ),											//  Property description.
		lib3270_is_secure,									//  Get value.
		NULL												//  Set value.
 	},

 	{
		"tso",												//  Property name.
		N_( "Non zero if the host is TSO." ),				//  Property description.
		lib3270_is_tso,										//  Get value.
		NULL												//  Set value.
 	},

 	{
		"pconnected",										//  Property name.
		N_( "" ),											//  Property description.
		lib3270_pconnected,									//  Get value.
		NULL												//  Set value.
 	},

 	{
		"half_connected",		//  Property name.
		N_( "" ),				//  Property description.
		lib3270_half_connected,	//  Get value.
		NULL					//  Set value.
 	},

 	{
		"neither",						//  Property name.
		N_( "" ),						//  Property description.
		lib3270_in_neither,				//  Get value.
		NULL							//  Set value.
 	},

 	{
		"ansi",							//  Property name.
		N_( "" ),						//  Property description.
		lib3270_in_ansi,				//  Get value.
		NULL							//  Set value.
 	},

 	{
		"3270",							//  Property name.
		N_( "" ),						//  Property description.
		lib3270_in_3270,				//  Get value.
		NULL							//  Set value.
 	},

 	{
		"sscp",							//  Property name.
		N_( "" ),						//  Property description.
		lib3270_in_sscp,				//  Get value.
		NULL							//  Set value.
 	},

 	{
		"tn3270e",						//  Property name.
		N_( "" ),						//  Property description.
		lib3270_in_tn3270e,				//  Get value.
		NULL							//  Set value.
 	},

 	{
		"e",							//  Property name.
		N_( "" ),						//  Property description.
		lib3270_in_e,					//  Get value.
		NULL							//  Set value.
 	},

 	{
		"cursor_address",				//  Property name.
		N_( "Cursor address" ),			//  Property description.
		lib3270_get_cursor_address,		//  Get value.
		lib3270_set_cursor_address		//  Set value.
 	},

 	{
		"has_selection",				//  Property name.
		N_( "Has selected aread" ),		//  Property description.
		lib3270_has_selection,			//  Get value.
		NULL							//  Set value.
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

	/*
 	{
		"",						//  Property name.
		N_( "" ),				//  Property description.
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

 int lib3270_set_connected(H3270 *hSession, int state) {

	if(state) {

		if(lib3270_connect(hSession,120))
			return -1;

	} else {

		return lib3270_disconnect(hSession);
	}

	return 0;
 }

 const LIB3270_INT_PROPERTY * lib3270_get_int_properties_list(void) {
 	return properties;
 }

int lib3270_get_property(H3270 *hSession, const char *name, int seconds)
{
	size_t ix;

	if(seconds)
	{
		lib3270_wait_for_ready(hSession, seconds);
	}

	for(ix = 0; ix < (sizeof(properties)/sizeof(properties[0])); ix++)
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

int lib3270_set_property(H3270 *hSession, const char *name, int value, int seconds)
{
	size_t ix;

	if(seconds)
	{
		lib3270_wait_for_ready(hSession, seconds);
	}

	for(ix = 0; ix < (sizeof(properties)/sizeof(properties[0])); ix++)
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

