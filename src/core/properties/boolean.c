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

 int lib3270_is_starting(const H3270 *hSession)
 {
	return hSession->starting != 0;
 }

 const LIB3270_INT_PROPERTY * lib3270_get_boolean_properties_list(void)
 {

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
			"",													//  Property description.
			lib3270_pconnected,									//  Get value.
			NULL												//  Set value.
		},

		{
			"half_connected",									//  Property name.
			"",													//  Property description.
			lib3270_half_connected,								//  Get value.
			NULL												//  Set value.
		},

		{
			"neither",											//  Property name.
			"",													//  Property description.
			lib3270_in_neither,									//  Get value.
			NULL												//  Set value.
		},

		{
			"ansi",												//  Property name.
			"",													//  Property description.
			lib3270_in_ansi,									//  Get value.
			NULL												//  Set value.
		},

		{
			"tn3270",											//  Property name.
			N_( "State is 3270, TN3270e or SSCP" ),				//  Property description.
			lib3270_in_3270,									//  Get value.
			NULL												//  Set value.
		},

		{
			"sscp",												//  Property name.
			"",													//  Property description.
			lib3270_in_sscp,									//  Get value.
			NULL												//  Set value.
		},

		{
			"tn3270e",											//  Property name.
			"",													//  Property description.
			lib3270_in_tn3270e,									//  Get value.
			NULL												//  Set value.
		},

		{
			"e",												//  Property name.
			N_( "Is terminal in the INITIAL_E state?" ),		//  Property description.
			lib3270_in_e,										//  Get value.
			NULL												//  Set value.
		},

		{
			"has_selection",									//  Property name.
			N_( "Has selected area" ),							//  Property description.
			lib3270_has_selection,								//  Get value.
			NULL												//  Set value.
		},

		{
			"starting",											//  Property name.
			N_( "Is starting (no first screen)?" ),				//  Property description.
			lib3270_is_starting,								//  Get value.
			NULL												//  Set value.
		},

		{
			"formatted",										//  Property name.
			N_( "Formatted screen" ),							//  Property description.
			lib3270_is_formatted,							//  Get value.
			NULL												//  Set value.
		},

		{
			"oerrlock",											//  Property name.
			N_( "Lock keyboard on operator error" ),			//  Property description.
			lib3270_get_lock_on_operator_error,					//  Get value.
			lib3270_set_lock_on_operator_error					//  Set value.
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

