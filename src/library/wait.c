/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright 1995, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006 by Paul Mattes.
 * Copyright (C) 2008 Banco do Brasil S.A.
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
#include <lib3270/memory.h>
#include "kybdc.h"
#include <private/util.h>

/*---[ Implement ]------------------------------------------------------------------------------------------*/

static int timer_expired(H3270 GNUC_UNUSED(*hSession), void *userdata) {
	*((int *) userdata) = 1;
	return 0;
}

LIB3270_EXPORT int lib3270_wait_for_update(H3270 GNUC_UNUSED(*hSession), int GNUC_UNUSED(seconds)) {
	return errno = ENOTSUP;
}

LIB3270_EXPORT int lib3270_wait_for_ready(H3270 *hSession, int seconds) {

	debug("%s",__FUNCTION__);
	debug("Session lock state is %d",lib3270_get_lock_status(hSession));

	int rc = 0;
	int timeout = 0;
	void * timer = hSession->timer.add(hSession, seconds * 1000, timer_expired, &timeout);

	while(!rc) {
		if(timeout) {
			// Timeout! The timer was destroyed.
			debug("%s exits with ETIMEDOUT",__FUNCTION__);
			return errno = ETIMEDOUT;
		}

		if(lib3270_get_lock_status(hSession) == LIB3270_MESSAGE_NONE) {
			// Is unlocked, break.

			break;
		}

		if(lib3270_is_disconnected(hSession)) {
			rc = errno = ENOTCONN;
			break;
		}

		if(hSession->kybdlock && KYBDLOCK_IS_OERR(hSession)) {
			rc = errno = EPERM;
			break;
		}

		debug("%s: Waiting",__FUNCTION__);
		int msgrc = lib3270_mainloop_run(hSession,1);
		if(msgrc < 0) {
			rc = -msgrc;
			break;
		}

	}
	hSession->timer.remove(hSession,timer);

	debug("%s exits with rc=%d",__FUNCTION__,rc);
	return rc;

}

int lib3270_wait_for_string(H3270 *hSession, const char *key, int seconds) {

	FAIL_IF_NOT_ONLINE(hSession);

	int rc = 0;
	int timeout = 0;
	void * timer = hSession->timer.add(hSession, seconds * 1000, timer_expired, &timeout);

	while(!rc) {
		if(timeout) {
			// Timeout! The timer was destroyed.
			return errno = ETIMEDOUT;
		}

		// Keyboard is locked by operator error, fails!
		if(hSession->kybdlock && KYBDLOCK_IS_OERR(hSession)) {
			rc = errno = EPERM;
			break;
		}

		if(!lib3270_is_connected(hSession)) {
			rc = errno = ENOTCONN;
			break;
		}

		char * contents = lib3270_get_string_at_address(hSession, 0, -1, 0);
		if(!contents) {
			rc = errno;
			break;
		}

		if(strstr(contents,key)) {
			lib3270_free(contents);
			break;
		}

		lib3270_free(contents);

		int msgrc = lib3270_mainloop_run(hSession,1);
		if(msgrc < 0) {
			rc = -msgrc;
			break;
		}

	}
	hSession->timer.remove(hSession,timer);

	return rc;

}

int lib3270_wait_for_string_at_address(H3270 *hSession, int baddr, const char *key, int seconds) {
	FAIL_IF_NOT_ONLINE(hSession);

	if(baddr < 0)
		baddr = lib3270_get_cursor_address(hSession);

	int rc = 0;
	int timeout = 0;
	void * timer = hSession->timer.add(hSession, seconds * 1000, timer_expired, &timeout);

	while(!rc) {
		if(timeout) {
			// Timeout! The timer was destroyed.
			return errno = ETIMEDOUT;
		}

		// Keyboard is locked by operator error, fails!
		if(hSession->kybdlock && KYBDLOCK_IS_OERR(hSession)) {
			rc = errno = EPERM;
			break;
		}

		if(!lib3270_is_connected(hSession)) {
			rc = errno = ENOTCONN;
			break;
		}

		if(lib3270_cmp_string_at_address(hSession, baddr, key, 0) == 0) {
			break;
		}

		int msgrc = lib3270_mainloop_run(hSession,1);
		if(msgrc < 0) {
			rc = -msgrc;
			break;
		}

	}
	hSession->timer.remove(hSession,timer);

	return rc;

}

LIB3270_EXPORT int lib3270_wait_for_string_at(H3270 *hSession, unsigned int row, unsigned int col, const char *key, int seconds) {
	int baddr = lib3270_translate_to_address(hSession,row,col);
	if(baddr < 0)
		return errno;

	return lib3270_wait_for_string_at_address(hSession,baddr,key,seconds);
}

LIB3270_EXPORT int lib3270_wait_for_connected(H3270 *hSession, int seconds) {

	int rc = -1;
	int timeout = 0;
	void * timer = hSession->timer.add(hSession, seconds * 1000, timer_expired, &timeout);

	while(rc == -1) {
		if(timeout) {
			// Timeout! The timer was destroyed.
			return errno = ETIMEDOUT;
		}

		if(hSession->connection.state == LIB3270_NOT_CONNECTED) {
			rc = ENOTCONN;
			break;
		}

		if(!hSession->starting && hSession->connection.state >= (int)LIB3270_CONNECTED_INITIAL) {
			rc = 0;
			break;
		}

		int msgrc = lib3270_mainloop_run(hSession,1);
		if(msgrc < 0) {
			rc = -msgrc;
			break;
		}

	}
	hSession->timer.remove(hSession,timer);

	return errno = rc;
}


LIB3270_EXPORT int lib3270_wait_for_cstate(H3270 *hSession, LIB3270_CSTATE cstate, int seconds) {

	int rc = -1;
	int timeout = 0;
	void * timer = hSession->timer.add(hSession, seconds * 1000, timer_expired, &timeout);

	while(rc == -1) {
		if(timeout) {
			// Timeout! The timer was destroyed.
			return errno = ETIMEDOUT;
		}

		if(hSession->connection.state == LIB3270_NOT_CONNECTED) {
			rc = ENOTCONN;
			break;
		}

		if(!hSession->starting && hSession->connection.state == cstate) {
			rc = 0;
			break;
		}

		int msgrc = lib3270_mainloop_run(hSession,1);
		if(msgrc < 0) {
			rc = -msgrc;
			break;
		}

	}
	hSession->timer.remove(hSession,timer);

	return errno = rc;
}

LIB3270_EXPORT LIB3270_KEYBOARD_LOCK_STATE lib3270_wait_for_keyboard_unlock(H3270 *hSession, int seconds) {
	debug("Session lock state is %d",lib3270_get_lock_status(hSession));

	int rc = 0;
	int timeout = 0;
	void * timer = hSession->timer.add(hSession, seconds * 1000, timer_expired, &timeout);

	while(!rc) {
		if(timeout) {
			// Timeout! The timer was destroyed.
			debug("%s exits with ETIMEDOUT",__FUNCTION__);
			errno = ETIMEDOUT;
			break;
		}

		if(hSession->kybdlock == LIB3270_KL_NOT_CONNECTED) {
			errno = ENOTCONN;
			break;
		}

		if(KYBDLOCK_IS_OERR(hSession)) {
			errno = EPERM;
			break;
		}

		if(hSession->kybdlock == LIB3270_KL_UNLOCKED)
			break;

		debug("%s: Waiting",__FUNCTION__);
		int msgrc = lib3270_mainloop_run(hSession,1);
		if(msgrc < 0) {
			rc = -msgrc;
			break;
		}

	}

	hSession->timer.remove(hSession,timer);

	debug("%s exits with rc=%d",__FUNCTION__,rc);
	return (LIB3270_KEYBOARD_LOCK_STATE) hSession->kybdlock;

}
