/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2025 Perry Werneck <perry.werneck@gmail.com>
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

#include <lib3270/defs.h>
#include <stdarg.h>

#ifndef LIB3270_POPUP_INCLUDED

#define LIB3270_POPUP_INCLUDED 1

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Notification message types.
 *
 */
typedef enum _LIB3270_NOTIFY {

	// Dialog types.
	LIB3270_NOTIFY_INFO				= 0x0001,	///< @brief Simple information dialog (dialog-information).
	LIB3270_NOTIFY_WARNING			= 0x0002,	///< @brief Warning message (dialog-warning).
	LIB3270_NOTIFY_ERROR			= 0x0003,	///< @brief Error message (dialog-error).

	// Security related messages
	LIB3270_NOTIFY_SECURITY_HIGH	= 0x0010,	///< @brief High security level (security-high).
	LIB3270_NOTIFY_SECURITY_MEDIUM	= 0x0020,	///< @brief Medium security level (security-medium).
	LIB3270_NOTIFY_SECURITY_LOW		= 0x0030,	///< @brief Low security level (security-low).

	// Connection related messages.
	LIB3270_NOTIFY_CONNECTION_ERROR	= 0x0103,	///< @brief Connection error.

	// Dialog filters.
	LIB3270_NOTIFY_DIALOG_TYPE		= 0x000F,	///< @brief Filter for dialog type.
	LIB3270_NOTIFY_SECURITY_TYPE	= 0x00F0,	///< @brief Filter for security type.
	LIB3270_NOTIFY_ALLOW_RECONNECT	= 0x0100,	///< @brief Connection related messages (allow 'Retry' button).
	LIB3270_NOTIFY_CRITICAL			= 0x0200,	///< @brief Critical error, user can abort application.
	LIB3270_NOTIFY_APPLICATION		= 0x1000,	///< @brief Application specifics.

} LIB3270_NOTIFY;

/**
 * @brief Head for popup descriptors.
 *
 */
#define LIB3270_POPUP_HEAD	\
		const char * name; \
		LIB3270_NOTIFY type; \
		const char * title; \
		const char * summary; \
		const char * body; \
		const char * label;

typedef struct _LIB3270_POPUP {
	LIB3270_POPUP_HEAD
} LIB3270_POPUP;

/**
 * @brief Replace popup handler.
 *
 */
LIB3270_EXPORT void lib3270_set_popup_handler(H3270 *hSession, int (*handler)(H3270 *, const LIB3270_POPUP *, unsigned char wait));

/**
 * @brief Pop up an error dialog, based on an error number.
 *
 * @param hSession	Session handle
 * @param errn		Error number (errno).
 * @param fmt		Message format
 * @param ...		Arguments for message
 */
LIB3270_EXPORT void lib3270_popup_an_errno(H3270 *hSession, int errn, const char *fmt, ...);

LIB3270_EXPORT void lib3270_popup_dialog(H3270 *session, LIB3270_NOTIFY id, const char *title, const char *message, const char *fmt, ...);

LIB3270_EXPORT void lib3270_popup_va(H3270 *session, LIB3270_NOTIFY id, const char *title, const char *message, const char *fmt, va_list);

LIB3270_EXPORT LIB3270_NOTIFY	lib3270_get_ssl_state_icon(const H3270 *hSession);
LIB3270_EXPORT const char *		lib3270_get_ssl_state_icon_name(const H3270 *hSession);

/**
 * @brief Clone popup object replacing the body contents.
 *
 * @param origin	Original popup definition.
 * @param fmt		Printf formatting string.
 *
 * @return New popup object with the body replaced (release it with free).
 *
 */
LIB3270_EXPORT LIB3270_POPUP * lib3270_popup_clone_printf(const LIB3270_POPUP *origin, const char *fmt, ...);

/**
 * @brief Emit popup message.
 *
 * @param hSession	TN3270 Session handle.
 * @param popup		Popup descriptor.
 * @param wait		If non zero waits for user response.
 *
 * @return User action.
 *
 * @retval 0			User has confirmed, continue action.
 * @retval ECANCELED	Operation was canceled.
 * @retval ENOTSUP		Can't decide, use default behavior.
 */
LIB3270_EXPORT int lib3270_popup(H3270 *hSession, const LIB3270_POPUP *popup, unsigned char wait);

/**
 * @brief Auto cleanup method (for use with lib3270_autoptr).
 *
 */
LIB3270_EXPORT void lib3270_autoptr_cleanup_LIB3270_POPUP(LIB3270_POPUP **ptr);

#ifdef __cplusplus
}
#endif

#endif // LIB3270_POPUP_INCLUDED


