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

#ifndef LIB3270_KEYBOARD_H_INCLUDED

#define LIB3270_KEYBOARD_H_INCLUDED 1

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief keyboard lock states
 */
typedef enum lib3270_keyboard_lock_state {
	LIB3270_KL_UNLOCKED			= 0x0000,	///< @brief Keyboard is unlocked.

	// Operator errors
	LIB3270_KL_OERR_MASK		= 0x000f,
	LIB3270_KL_OERR_PROTECTED	= 0x0001,
	LIB3270_KL_OERR_NUMERIC		= 0x0002,
	LIB3270_KL_OERR_OVERFLOW	= 0x0003,
	LIB3270_KL_OERR_DBCS		= 0x0004,

	LIB3270_KL_NOT_CONNECTED	= 0x0010,	///< @brief Not connected to host.
	LIB3270_KL_AWAITING_FIRST	= 0x0020,
	LIB3270_KL_OIA_TWAIT		= 0x0040,
	LIB3270_KL_OIA_LOCKED		= 0x0080,
	LIB3270_KL_DEFERRED_UNLOCK	= 0x0100,
	LIB3270_KL_ENTER_INHIBIT	= 0x0200,
	LIB3270_KL_SCROLLED			= 0x0400,
	LIB3270_KL_OIA_MINUS		= 0x0800

} LIB3270_KEYBOARD_LOCK_STATE;

/**
 * @brief Wait for keyboard unlock.
 *
 * Return status if the keyboard is locked by operator error ou if disconnected from host, waits until keyboard is unlocked if not.
 *
 * @param seconds	Number of seconds to wait.
 *
 * @return keyboard lock status.
 *
 * @retval LIB3270_KL_UNLOCKED	Keyboard unlocked, acess ok.
 */
LIB3270_EXPORT LIB3270_KEYBOARD_LOCK_STATE lib3270_wait_for_keyboard_unlock(H3270 *hSession, int seconds);

LIB3270_EXPORT LIB3270_KEYBOARD_LOCK_STATE lib3270_get_keyboard_lock_state(const H3270 *hSession);

/**
 * @brief Set te operator error lock.
 *
 * If lock is enabled (the default), operator errors (typing into protected fields, insert overflow, etc.)
 * will cause the keyboard to lock with an error message in the OIA (status line). If disabled, these errors
 * will simply cause the terminal bell will ring, without any keyboard lock or message.
 *
 * @param hSession	Session handle.
 * @param enable	Non zero to enable operator lock, zero to disable.
 *
 */
LIB3270_EXPORT int lib3270_set_lock_on_operator_error(H3270 *hSession, int enable);
LIB3270_EXPORT int lib3270_get_lock_on_operator_error(const H3270 *hSession);

LIB3270_EXPORT int lib3270_set_numeric_lock(H3270 *hSession, int enable);
LIB3270_EXPORT int lib3270_get_numeric_lock(const H3270 *hSession);


#ifdef __cplusplus
}
#endif

#endif // LIB3270_KEYBOARD_H_INCLUDED

