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

#ifndef LIB3270_TRACE_H_INCLUDED

#define LIB3270_TRACE_H_INCLUDED 1

#include <lib3270.h>
#include <lib3270/os.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define LIB3270_AS_PRINTF(a,b) /* __attribute__((format(printf, a, b))) */
#else
#define LIB3270_AS_PRINTF(a,b) __attribute__((format(printf, a, b)))
#endif

typedef int (*LIB3270_TRACE_HANDLER)(const H3270 *, void *, const char *);

/**
 * @brief Set trace filename.
 *
 * @param hSession	TN3270 Session handle.
 * @param name		The trace file name (null to disable).
 *
 */
LIB3270_EXPORT int lib3270_set_trace_filename(H3270 * hSession, const char *name);

/**
 * @brief Get trace file name.
 *
 * @param hSession	TN3270 Session handle.
 * @return The trace file name or NULL if disabled.
 *
 */
LIB3270_EXPORT const char * lib3270_get_trace_filename(const H3270 * hSession);

/**
 * @brief Set trace handle callback.
 *
 * @param hSession	TN3270 Session handle.
 * @param handler	Callback to write in trace file or show trace window (NULL send all trace to stdout/syslog).
 * @param userdata	User data to pass to the trace handler.
 *
 */
LIB3270_EXPORT void lib3270_set_trace_handler(H3270 *hSession, LIB3270_TRACE_HANDLER handler, void *userdata);

/**
 * @brief Get trace handle callback.
 *
 * @param hSession	TN3270 Session handle.
 * @param handler	Callback to write in trace file or show trace window (NULL send all trace to stdout/syslog).
 * @param userdata	User data to pass to the trace handler.
 *
 */
LIB3270_EXPORT void lib3270_get_trace_handler(H3270 *hSession, LIB3270_TRACE_HANDLER *handler, void **userdata);

/**
 * @brief Write on trace file.
 *
 * Write text on trace file.
 *
 * @param fmt 	String format.
 * @param ...	Arguments.
 *
 */
LIB3270_EXPORT void lib3270_write_trace(H3270 *session, const char *fmt, ...) LIB3270_AS_PRINTF(2,3);

/**
 * @brief Write on trace file.
 *
 * Write text on trace file, if DStrace is enabled.
 *
 * @param fmt 	String format.
 * @param ...	Arguments.
 *
 */
LIB3270_EXPORT void lib3270_write_dstrace(H3270 *session, const char *fmt, ...) LIB3270_AS_PRINTF(2,3);

/**
 * @brief Write on trace file.
 *
 * Write text on trace file, if network trace is enabled.
 *
 * @param fmt 	String format.
 * @param ...	Arguments.
 *
 */
LIB3270_EXPORT void lib3270_write_nettrace(H3270 *session, const char *fmt, ...) LIB3270_AS_PRINTF(2,3);

/**
* @brief Write on trace file.
*
* Write text on trace file, if screen trace is enabled.
*
* @param fmt 	String format.
* @param ...	Arguments.
*
*/
LIB3270_EXPORT void lib3270_write_screen_trace(H3270 *session, const char *fmt, ...) LIB3270_AS_PRINTF(2,3);

/**
 * @brief Write on trace file.
 *
 * Write text on trace file, if event is enabled.
 *
 * @param fmt 	String format.
 * @param ...	Arguments.
 *
 */
LIB3270_EXPORT void lib3270_write_event_trace(H3270 *session, const char *fmt, ...) LIB3270_AS_PRINTF(2,3);

LIB3270_EXPORT void LIB3270_DEPRECATED(lib3270_trace_event(H3270 *session, const char *fmt, ...)) LIB3270_AS_PRINTF(2,3);

/**
 * @brief Write datablock on trace file.
 *
 * @param hSession	TN3270 Session handle.
 * @param msg		Message.
 * @param data		Data block in ASCII.
 * @param datalen	Length of the data block.
 *
 */
LIB3270_EXPORT void lib3270_trace_data(H3270 *hSession, const char *msg, const unsigned char *data, size_t datalen);


#ifdef __cplusplus
}
#endif

#endif // LIB3270_TRACE_H_INCLUDED
