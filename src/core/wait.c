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
#include <lib3270/keyboard.h>
#include "kybdc.h"
#include "utilc.h"

/*---[ Implement ]------------------------------------------------------------------------------------------*/

LIB3270_EXPORT int lib3270_wait_for_update(H3270 GNUC_UNUSED(*hSession), int GNUC_UNUSED(seconds)) {
	return errno = ENOTSUP;
}

LIB3270_EXPORT int lib3270_wait_for_ready(H3270 *hSession, int seconds) {

	debug("%s",__FUNCTION__);
	debug("Session lock state is %d",lib3270_get_lock_status(hSession));

	time_t limit = time(0)+seconds;
	do {

		if(lib3270_get_lock_status(hSession) == LIB3270_MESSAGE_NONE) {
			return 0;
		}

		if(lib3270_is_disconnected(hSession)) {
			return errno = ENOTCONN;
		}

		if(hSession->kybdlock && KYBDLOCK_IS_OERR(hSession)) {
			return errno = EPERM;
		}

		debug("%s: Waiting",__FUNCTION__);
		lib3270_main_loop_iterate(1000);
	} while(time(0) < limit);

	debug("%s exits with rc=%d",__FUNCTION__,ETIMEDOUT);
	return errno = ETIMEDOUT;

}

int lib3270_wait_for_string(H3270 *hSession, const char *key, int seconds) {

	FAIL_IF_NOT_ONLINE(hSession);

	time_t limit = time(0)+seconds;
	do {

		// Keyboard is locked by operator error, fails!
		if(hSession->kybdlock && KYBDLOCK_IS_OERR(hSession)) {
			return errno = EPERM;
		}

		if(!lib3270_is_connected(hSession)) {
			return errno = ENOTCONN;
		}

		char * contents = lib3270_get_string_at_address(hSession, 0, -1, 0);
		if(!contents) {
			return errno;
		}

		if(strstr(contents,key)) {
			lib3270_free(contents);
			return 0;
		}

		lib3270_free(contents);
		lib3270_main_loop_iterate(1000);

	} while(time(0) < limit);

	return errno = ETIMEDOUT;

}

int lib3270_wait_for_string_at_address(H3270 *hSession, int baddr, const char *key, int seconds) {
	FAIL_IF_NOT_ONLINE(hSession);

	if(baddr < 0)
		baddr = lib3270_get_cursor_address(hSession);

	time_t limit = time(0)+seconds;
	do {

		// Keyboard is locked by operator error, fails!
		if(hSession->kybdlock && KYBDLOCK_IS_OERR(hSession)) {
			return errno = EPERM;
		}

		if(!lib3270_is_connected(hSession)) {
			return errno = ENOTCONN;
		}

		if(lib3270_cmp_string_at_address(hSession, baddr, key, 0) == 0) {
			return 0;
		}

		lib3270_main_loop_iterate(1000);

	} while(time(0) < limit);

	return errno = ETIMEDOUT;

}

LIB3270_EXPORT int lib3270_wait_for_string_at(H3270 *hSession, unsigned int row, unsigned int col, const char *key, int seconds) {
	int baddr = lib3270_translate_to_address(hSession,row,col);
	if(baddr < 0)
		return errno;
	return lib3270_wait_for_string_at_address(hSession,baddr,key,seconds);
}

LIB3270_EXPORT int lib3270_wait_for_connected(H3270 *hSession, int seconds) {

	time_t limit = time(0)+seconds;
	do {

		if(hSession->connection.state == LIB3270_NOT_CONNECTED) {
			return errno = ENOTCONN;
		}

		if(!hSession->starting && hSession->connection.state >= (int)LIB3270_CONNECTED_INITIAL) {
			return 0;
		}

		lib3270_main_loop_iterate(1000);

	} while(time(0) < limit);

	return errno = ETIMEDOUT;

}

LIB3270_EXPORT int lib3270_wait_for_cstate(H3270 *hSession, LIB3270_CSTATE cstate, int seconds) {

	time_t limit = time(0)+seconds;
	do {

		if(hSession->connection.state == LIB3270_NOT_CONNECTED) {
			return errno = ENOTCONN;
		}

		if(!hSession->starting && hSession->connection.state == cstate) {
			return 0;
		}

		lib3270_main_loop_iterate(1000);

	} while(time(0) < limit);

	return errno = ETIMEDOUT;

}

LIB3270_EXPORT LIB3270_KEYBOARD_LOCK_STATE lib3270_wait_for_keyboard_unlock(H3270 *hSession, int seconds) {

	debug("Session lock state is %d",lib3270_get_lock_status(hSession));

	time_t limit = time(0)+seconds;

	do {

		if(hSession->kybdlock == LIB3270_KL_NOT_CONNECTED) {
			errno = ENOTCONN;
			return (LIB3270_KEYBOARD_LOCK_STATE) hSession->kybdlock;
		}

		if(KYBDLOCK_IS_OERR(hSession)) {
			errno = EPERM;
			return (LIB3270_KEYBOARD_LOCK_STATE) hSession->kybdlock;
		}

		if(hSession->kybdlock == LIB3270_KL_UNLOCKED) {
			return (LIB3270_KEYBOARD_LOCK_STATE) hSession->kybdlock;
		}

		debug("%s: Waiting",__FUNCTION__);
		lib3270_main_loop_iterate(1000);

	} while(time(0) < limit);

	errno = ETIMEDOUT;
	debug("%s exits with rc=%d",__FUNCTION__,errno);

	return (LIB3270_KEYBOARD_LOCK_STATE) hSession->kybdlock;

}
