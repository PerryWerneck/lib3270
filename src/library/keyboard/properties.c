/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
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

/*
 * Contatos:
 *
 * perry.werneck@gmail.com      (Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com     (Erico Mascarenhas Mendonça)
 *
 */

#include <internals.h>
#include <lib3270/keyboard.h>
#include <lib3270/properties.h>

LIB3270_EXPORT LIB3270_KEYBOARD_LOCK_STATE lib3270_get_keyboard_lock_state(const H3270 *hSession) {
	if(check_online_session(hSession))
		return LIB3270_KL_NOT_CONNECTED;

	return (LIB3270_KEYBOARD_LOCK_STATE) hSession->kybdlock;
}

LIB3270_EXPORT int lib3270_set_lock_on_operator_error(H3270 *hSession, int enable) {
	hSession->oerr_lock = (enable ? 1 : 0);
	return 0;
}

LIB3270_EXPORT int lib3270_set_numeric_lock(H3270 *hSession, int enable) {
	hSession->numeric_lock = (enable ? 1 : 0);
	return 0;
}

int lib3270_get_lock_on_operator_error(const H3270 *hSession) {
	return (int) hSession->oerr_lock;
}

int lib3270_get_numeric_lock(const H3270 *hSession) {
	return (int) hSession->numeric_lock;
}

LIB3270_EXPORT int lib3270_set_unlock_delay(H3270 *hSession, unsigned int delay) {
	hSession->unlock_delay		= (delay == 0 ? 0 : 1);
	hSession->unlock_delay_ms 	= (unsigned short) delay;
	return 0;
}

LIB3270_EXPORT unsigned int lib3270_get_unlock_delay(const H3270 *hSession) {
	return (unsigned int) hSession->unlock_delay_ms;
}

