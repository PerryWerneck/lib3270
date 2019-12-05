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
  * @brief Implements the boolean properties.
  *
  */

 #include <config.h>
 #include <internals.h>
 #include <string.h>
 #include <lib3270.h>
 #include <lib3270/properties.h>
 #include <lib3270/keyboard.h>
 #include <lib3270/selection.h>

 int lib3270_is_starting(const H3270 *hSession)
 {
	return hSession->starting != 0;
 }

 const LIB3270_INT_PROPERTY * lib3270_get_boolean_properties_list(void)
 {

	static const LIB3270_INT_PROPERTY properties[] = {
		{
			.name = "ready",												//  Property name.
			.description = N_( "Is terminal ready" ),						//  Property description.
			.get = lib3270_is_ready,										//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "connected",											//  Property name.
			.description = N_( "Is terminal connected" ),					//  Property description.
			.get = lib3270_is_connected,									//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "secure",												//  Property name.
			.description = N_( "Is connection secure" ),					//  Property description.
			.get = lib3270_is_secure,										//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "tso",													//  Property name.
			.group = LIB3270_ACTION_GROUP_OFFLINE,							// Property group.
			.description = N_( "Non zero if the host is TSO." ),			//  Property description.
			.get = lib3270_is_tso,											//  Get value.
			.set = lib3270_set_tso											//  Set value.
		},

		{
			.name = "as400",												//  Property name.
			.group = LIB3270_ACTION_GROUP_OFFLINE,							// Property group.
			.description = N_( "Non zero if the host is AS400." ),			//  Property description.
			.get = lib3270_is_as400,										//  Get value.
			.set = lib3270_set_as400										//  Set value.
		},

		{
			.name = "pconnected",											//  Property name.
			.description = "",												//  Property description.
			.get = lib3270_pconnected,										//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "half_connected",										//  Property name.
			.description = "",												//  Property description.
			.get = lib3270_half_connected,									//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "neither",												//  Property name.
			.description = "",												//  Property description.
			.get = lib3270_in_neither,										//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "ansi",													//  Property name.
			.description = "",												//  Property description.
			.get = lib3270_in_ansi,											//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "tn3270",												//  Property name.
			.description = N_( "State is 3270, TN3270e or SSCP" ),			//  Property description.
			.get = lib3270_in_3270,											//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "sscp",													//  Property name.
			.description = "",												//  Property description.
			.get = lib3270_in_sscp,											//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "tn3270e",												//  Property name.
			.description = "",												//  Property description.
			.get = lib3270_in_tn3270e,										//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "e",													//  Property name.
			.description = N_( "Is terminal in the INITIAL_E state?" ),		//  Property description.
			.get = lib3270_in_e,											//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "has_selection",										//  Property name.
			.description = N_( "Has selected area" ),						//  Property description.
			.get = lib3270_has_selection,									//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "can_paste_next",										//  Property name.
			.description = N_( "Still have text to paste" ),				//  Property description.
			.get = lib3270_can_paste_next,									//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "starting",												//  Property name.
			.description = N_( "Is starting (no first screen)?" ),			//  Property description.
			.get = lib3270_is_starting,										//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "formatted",											//  Property name.
			.description = N_( "Formatted screen" ),						//  Property description.
			.get = lib3270_is_formatted,									//  Get value.
			.set = NULL														//  Set value.
		},

		{
			.name = "oerrlock",												//  Property name.
			.description = N_( "Lock keyboard on operator error" ),			//  Property description.
			.get = lib3270_get_lock_on_operator_error,						//  Get value.
			.set = lib3270_set_lock_on_operator_error						//  Set value.
		},

		{
			.name = "numericlock",											//  Property name.
			.description = N_( "numeric lock" ),							//  Property description.
			.get = lib3270_get_numeric_lock,								//  Get value.
			.set = lib3270_set_numeric_lock									//  Set value.
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

