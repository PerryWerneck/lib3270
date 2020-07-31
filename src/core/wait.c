/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270.
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

#include <internals.h>
#include <lib3270/log.h>
#include <lib3270/trace.h>
#include "kybdc.h"
#include "utilc.h"

/*---[ Implement ]------------------------------------------------------------------------------------------*/

static int timer_expired(H3270 GNUC_UNUSED(*hSession), void *userdata)
{
	*((int *) userdata) = 1;
	return 0;
}

LIB3270_EXPORT int lib3270_wait_for_update(H3270 GNUC_UNUSED(*hSession), int GNUC_UNUSED(seconds))
{
	return errno = ENOTSUP;
}

LIB3270_EXPORT int lib3270_wait_for_ready(H3270 *hSession, int seconds)
{
	FAIL_IF_NOT_ONLINE(hSession);

	debug("%s",__FUNCTION__);
	debug("Session lock state is %d",lib3270_get_lock_status(hSession));

	int rc = 0;
	int timeout = 0;
	void * timer = AddTimer(seconds * 1000, hSession, timer_expired, &timeout);

	while(!rc)
	{
		if(timeout) {
			// Timeout! The timer was destroyed.
			debug("%s exits with ETIMEDOUT",__FUNCTION__);
			return errno = ETIMEDOUT;
		}

		if(lib3270_get_lock_status(hSession) == LIB3270_MESSAGE_NONE)
		{
			// Is unlocked, break.

			break;
		}

		if(lib3270_is_disconnected(hSession))
		{
			rc = errno = ENOTCONN;
			break;
		}

		if(hSession->kybdlock && KYBDLOCK_IS_OERR(hSession))
		{
			rc = errno = EPERM;
			break;
		}

		lib3270_main_iterate(hSession,1);
	}
	RemoveTimer(hSession,timer);

	debug("%s exits with rc=%d",__FUNCTION__,rc);
	return rc;

}

int lib3270_wait_for_string(H3270 *hSession, const char *key, int seconds)
{
	FAIL_IF_NOT_ONLINE(hSession);

	int rc = 0;
	int timeout = 0;
	void * timer = AddTimer(seconds * 1000, hSession, timer_expired, &timeout);

	while(!rc)
	{
		if(timeout) {
			// Timeout! The timer was destroyed.
			return errno = ETIMEDOUT;
		}

		// Keyboard is locked by operator error, fails!
		if(hSession->kybdlock && KYBDLOCK_IS_OERR(hSession))
		{
			rc = errno = EPERM;
			break;
		}

		if(!lib3270_is_connected(hSession))
		{
			rc = errno = ENOTCONN;
			break;
		}

		char * contents = lib3270_get_string_at_address(hSession, 0, -1, 0);
		if(!contents)
		{
			rc = errno;
			break;
		}

		if(strstr(contents,key)) {
			lib3270_free(contents);
			break;
		}

		lib3270_free(contents);

		lib3270_main_iterate(hSession,1);

	}
	RemoveTimer(hSession,timer);

	return rc;

}

int lib3270_wait_for_string_at_address(H3270 *hSession, int baddr, const char *key, int seconds)
{
	FAIL_IF_NOT_ONLINE(hSession);

	if(baddr < 0)
		baddr = lib3270_get_cursor_address(hSession);

	int rc = 0;
	int timeout = 0;
	void * timer = AddTimer(seconds * 1000, hSession, timer_expired, &timeout);

	while(!rc)
	{
		if(timeout) {
			// Timeout! The timer was destroyed.
			return errno = ETIMEDOUT;
		}

		// Keyboard is locked by operator error, fails!
		if(hSession->kybdlock && KYBDLOCK_IS_OERR(hSession))
		{
			rc = errno = EPERM;
			break;
		}

		if(!lib3270_is_connected(hSession))
		{
			rc = errno = ENOTCONN;
			break;
		}

		if(lib3270_cmp_string_at_address(hSession, baddr, key, 0) == 0)
		{
			break;
		}

		lib3270_main_iterate(hSession,1);

	}
	RemoveTimer(hSession,timer);

	return rc;

}

int lib3270_wait_for_string_at(H3270 *hSession, unsigned int row, unsigned int col, const char *key, int seconds)
{
	int baddr = lib3270_translate_to_address(hSession,row,col);
	if(baddr < 0)
		return errno;

	return lib3270_wait_for_string_at_address(hSession,baddr,key,seconds);
}
