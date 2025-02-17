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

 #include <lib3270/defs.h>
 #include <stdarg.h>
 #include <private/session.h>
 #include <errno.h>
 #include <string.h>
 #include <errno.h>

 LIB3270_EXPORT void lib3270_log_close(H3270 *hSession) {
	if(hSession->log) {
		hSession->log->finalize(hSession,hSession->log);
		hSession->log = NULL;
	}
 }

 LIB3270_EXPORT int lib3270_log_write(const H3270 *hSession, const char *module, const char *fmt, ...) {

	if(!hSession->log) {
		return EBADF;
	}

	va_list arg_ptr;
	va_start(arg_ptr, fmt);
	int rc = hSession->log->write(
		hSession,
		hSession->log,
		(module ? module : LIB3270_STRINGIZE_VALUE_OF(PRODUCT_NAME)),
		fmt,
		arg_ptr
	);
	va_end(arg_ptr);
	return rc;

 }

 LIB3270_EXPORT int lib3270_log_write_rc(const H3270 *hSession, const char *module, int rc, const char *fmt, ...) {

	if(!hSession->log) {
		return EBADF;
	}

	va_list arg_ptr;
	va_start(arg_ptr, fmt);
	lib3270_autoptr(char) message = lib3270_vsprintf(fmt,arg_ptr);
	va_end(arg_ptr);
	
	return lib3270_log_write(
				hSession,
				module,
				"%s (rc=%d - %s)",message,rc,strerror(rc));

 }

 LIB3270_EXPORT void lib3270_log_write_va(const H3270 *hSession, const char *module, const char *fmt, va_list arg) {

	if(!hSession->log) {
		return;
	}

	hSession->log->write(
		hSession,
		hSession->log,
		(module ? module : LIB3270_STRINGIZE_VALUE_OF(PRODUCT_NAME)),
		fmt,
		arg
	);

 }
