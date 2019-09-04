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
 *	@file toggles/getset.c
 *	@brief This module handles toggle changes.
 */

#include <config.h>
#include <lib3270/toggle.h>
#include <lib3270-internals.h>
#include <lib3270/log.h>
#include "togglesc.h"

/*---[ Implement ]------------------------------------------------------------------------------------------------------------*/

LIB3270_EXPORT unsigned char lib3270_get_toggle(H3270 *session, LIB3270_TOGGLE ix)
{
	CHECK_SESSION_HANDLE(session);

	if(ix < 0 || ix >= LIB3270_TOGGLE_COUNT)
		return 0;

	return session->toggle[ix].value != 0;
}

/**
 * @brief Call the internal update routine and listeners.
 */
static void toggle_notify(H3270 *session, struct lib3270_toggle *t, LIB3270_TOGGLE ix)
{
	struct lib3270_toggle_callback * st;

	trace("%s: ix=%d upcall=%p",__FUNCTION__,ix,t->upcall);
	t->upcall(session, t, LIB3270_TOGGLE_TYPE_INTERACTIVE);

	if(session->cbk.update_toggle)
		session->cbk.update_toggle(session,ix,t->value,LIB3270_TOGGLE_TYPE_INTERACTIVE,toggle_descriptor[ix].name);

	for(st = session->listeners.toggle.callbacks[ix]; st != (struct lib3270_toggle_callback *) NULL; st = (struct lib3270_toggle_callback *) st->next)
	{
		st->func(session, ix, t->value, st->data);
	}

}

/**
 * @brief Set toggle state.
 *
 * @param h		Session handle.
 * @param ix	Toggle id.
 * @param value	New toggle state (non zero for true).
 *
 * @returns 0 if the toggle is already at the state, 1 if the toggle was changed; < 0 on error (sets errno).
 */
LIB3270_EXPORT int lib3270_set_toggle(H3270 *session, LIB3270_TOGGLE ix, int value)
{
	char v = value ? True : False;
	struct lib3270_toggle * t;

	CHECK_SESSION_HANDLE(session);

	if(ix < 0 || ix >= LIB3270_TOGGLE_COUNT)
		return -(errno = EINVAL);

	t = &session->toggle[ix];

	if(v == t->value)
		return 0;

	t->value = v;

	toggle_notify(session,t,ix);
	return 1;
}

LIB3270_EXPORT int lib3270_toggle(H3270 *session, LIB3270_TOGGLE ix)
{
	struct lib3270_toggle	*t;

	CHECK_SESSION_HANDLE(session);

	if(ix < 0 || ix >= LIB3270_TOGGLE_COUNT)
		return 0;

	t = &session->toggle[ix];

	t->value = t->value ? False : True;
	toggle_notify(session,t,ix);

	return (int) t->value;
}

