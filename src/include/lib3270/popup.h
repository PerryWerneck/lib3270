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
	LIB3270_NOTIFY_INFO,		///< @brief Simple information dialog.
	LIB3270_NOTIFY_WARNING,		///< @brief Warning message.
	LIB3270_NOTIFY_ERROR,		///< @brief Error message.
	LIB3270_NOTIFY_CRITICAL,	///< @brief Critical error, user can abort application.
	LIB3270_NOTIFY_SECURE,		///< @brief Secure host dialog.

	LIB3270_NOTIFY_USER			///< @brief Reserved, always the last one.
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
 * @return New popup object with the body replaced (release it with g_free).
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


