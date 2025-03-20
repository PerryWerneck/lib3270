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
 #include <lib3270/defs.h>
 #include <lib3270/memory.h>
 #include <lib3270/log.h>
 #include <private/intl.h>
 #include <private/openssl.h>
 #include <openssl/err.h>
 
 LIB3270_INTERNAL char * openssl_errors() {

	lib3270_autoptr(BIO) out = BIO_new(BIO_s_mem());

	if(!out) {
		return strdup(_("BIO_new failed"));
	}

	ERR_print_errors(out);
	BIO_flush(out);

	unsigned char * data;
	int n = BIO_get_mem_data(out, &data);
	
	unsigned char *text = (unsigned char *) lib3270_malloc(n+1);
	memcpy(text,data,n);
	text[n] ='\0';

	return (char *) text;
}

 LIB3270_INTERNAL void openssl_failed(H3270 *hSession, int code, const char *summary) {

	lib3270_autoptr(char) name = lib3270_strdup_printf("openssl-%d",code);
	lib3270_autoptr(char) body = openssl_errors();

	LIB3270_POPUP popup = {
		.name		= name,
		.type		= LIB3270_NOTIFY_TLS_ERROR,
		.title		= _("TLS/SSL error"),
		.summary	= summary,
		.body		= body,
		.label		= _("OK")
	};

	connection_close(hSession, code ? code : -1);
	lib3270_popup_async(hSession, &popup);

 }
