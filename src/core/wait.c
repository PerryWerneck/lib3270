/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2008 Perry Werneck <perry.werneck@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
