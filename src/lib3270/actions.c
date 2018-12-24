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
 * programa;  se  não, escreva para a Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA, 02111-1307, USA
 *
 * Este programa está nomeado como actions.c e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

#include "private.h"
#include <lib3270/trace.h>
#include <lib3270/actions.h>

/*---[ Globals ]--------------------------------------------------------------------------------------------------------------*/

/*---[ Statics ]--------------------------------------------------------------------------------------------------------------*/

/*---[ Implement ]------------------------------------------------------------------------------------------------------------*/

/**
 * @brief Get LIB3270 action table;
 *
 */
 LIB3270_EXPORT const LIB3270_ACTION_ENTRY * lib3270_get_action_table()
 {
	#undef DECLARE_LIB3270_ACTION
	#undef DECLARE_LIB3270_CLEAR_SELECTION_ACTION
	#undef DECLARE_LIB3270_KEY_ACTION
	#undef DECLARE_LIB3270_CURSOR_ACTION
	#undef DECLARE_LIB3270_FKEY_ACTION

	#define DECLARE_LIB3270_ACTION( name, description )						{ #name, description, lib3270_ ## name			},
	#define DECLARE_LIB3270_CLEAR_SELECTION_ACTION( name, description )		{ #name, description, lib3270_ ## name			},
	#define DECLARE_LIB3270_KEY_ACTION( name, description )					{ #name, description, lib3270_ ## name			},
	#define DECLARE_LIB3270_CURSOR_ACTION( name, description )				{ #name, description, lib3270_cursor_ ## name	},
	#define DECLARE_LIB3270_FKEY_ACTION( name, description )				// name

	static const LIB3270_ACTION_ENTRY actions[] =
	{
		#include <lib3270/action_table.h>
		{ NULL, NULL, NULL }
	};

	return actions;
 }


/**
 * @brief Launch an action by name.
 *
 */
LIB3270_EXPORT int lib3270_action(H3270 *hSession, const char *name)
{
	const LIB3270_ACTION_ENTRY *actions = lib3270_get_action_table();
	size_t f;

	CHECK_SESSION_HANDLE(hSession);

	for(f=0; actions[f].name; f++)
	{
		if(!strcasecmp(name,actions[f].name))
		{
			lib3270_trace_event(hSession,"Action: %s\n",actions[f].name);
			return actions[f].call(hSession);
		}

	}

	lib3270_trace_event(hSession,"Unknown action %s\n",name);
	errno = ENOENT;
	return -1;

}
