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

#include <lib3270-internals.h>
#include <lib3270/trace.h>
#include <lib3270/actions.h>

/*---[ Implement ]------------------------------------------------------------------------------------------------------------*/

const LIB3270_ACTION * lib3270_action_get_by_name(const char *name)
{
	const LIB3270_ACTION * actions = lib3270_get_actions();
	size_t f;

	for(f=0; actions[f].name; f++)
	{
		if(!strcasecmp(name,actions[f].name))
			return actions+f;
	}

	errno = ENOTSUP;
	return NULL;
}

const LIB3270_ACTION * lib3270_get_action(const char *name) {
	return lib3270_action_get_by_name(name);
}

LIB3270_EXPORT int lib3270_action_is_activatable(const LIB3270_ACTION *action, H3270 *hSession)
{
	return action->activatable(hSession) > 0;
}

LIB3270_EXPORT int lib3270_action_activate(const LIB3270_ACTION *action, H3270 *hSession)
{

	if(!action->activatable(hSession))
	{
		lib3270_trace_event(hSession,"Action \"%s\" is disabled\n",action->name);
		return errno = EPERM;
	}

	lib3270_trace_event(hSession,"Activating action \"%s\"\n",action->name);

	return action->activate(hSession);

}

LIB3270_EXPORT int lib3270_action_activate_by_name(const char *name, H3270 *hSession)
{
	const LIB3270_ACTION *action = lib3270_action_get_by_name(name);

	if(!action)
	{
		lib3270_trace_event(hSession,"Can't find action \"%s\"\n",name);
		return errno;
	}

	return lib3270_action_activate(action, hSession);
}

LIB3270_EXPORT int lib3270_action(H3270 *hSession, const char *name)
{
	return lib3270_action_activate_by_name(name,hSession);
}
