/*
 * Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
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
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
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

