/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2025 Banco do Brasil S.A.
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

 #include <config.h>
 #include <private/session.h>
 #include <private/network.h>
 #include <private/intl.h>

 LIB3270_INTERNAL int start_tls(H3270 *hSession) {

	LIB3270_POPUP popup = {
		.name		= "openssl-not-available",
		.type		= LIB3270_NOTIFY_NO_TLS,
		.title		= _("Connection error"),
		.summary	= _("Unable to activate TLS/SSL"),
		.body		= _("The OpenSSL library is not available"),
		.label		= _("Ok")
	};

	connection_close(hSession,ENOTSUP);
	lib3270_popup(hSession, &popup, 0);

	return errno = ENOTSUP;

 }
