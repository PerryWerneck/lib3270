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

 #pragma once

 #include <config.h>
 #include <lib3270/defs.h>
 #include <private/network.h>
 #include <openssl/ssl.h>
 #include <pthread.h>

 static inline void lib3270_autoptr_cleanup_BIO(BIO **ptr) {
	if(*ptr)
		BIO_free_all(*ptr);
	*ptr = NULL;
 }

 static inline void lib3270_autoptr_cleanup_SSL_CTX(SSL_CTX **ptr) {
	if(*ptr)
		SSL_CTX_free(*ptr);
	*ptr = NULL;
 }

 static inline void lib3270_autoptr_cleanup_X509(X509 **ptr) {
	if(*ptr)
		X509_free(*ptr);
	*ptr = NULL;
 }

 #define SSL_EXDATA_INDEX_INVALID -1

 LIB3270_INTERNAL pthread_mutex_t ssl_guard;
 LIB3270_INTERNAL int e_ctx_ssl_exdata_index;

 LIB3270_INTERNAL SSL_CTX * openssl_context(H3270 *hSession);

 /// @brief Get string with the openssl error stack.
 /// @return The error stack (release it with lib3270_free()).
 LIB3270_INTERNAL char * openssl_errors();

 /// @brief OpenSSL negotiation has failed.
 /// @param context OpenSSL context.
 /// @param code Error code.
 /// @param summary Summary of the error.
 LIB3270_INTERNAL void openssl_failed(H3270 *hSession, int code, const char *summary);

 /// @brief OpenSSL negotiation has succeeded, setup network I/O.
 /// @param context OpenSSL context.
 LIB3270_INTERNAL void openssl_tls_complete(H3270 *session, SSL *ssl);
 
 /// @brief Get descriptor from OpenSSL's error code.
 /// @param code The OpenSSL error code.
 /// @return The message descriptor if found, NULL if not found.
 LIB3270_INTERNAL const LIB3270_SSL_MESSAGE * openssl_message_from_code(long code);

 LIB3270_INTERNAL const LIB3270_SSL_MESSAGE * openssl_message_from_name(const char *name);

 /// @brief CHeck if the hostname matches the certificate.
 /// @param server_cert The certificate to check
 /// @param hostname The hostname
 /// @return 0 if the hostname matches the certificate, non-zero otherwise.
 LIB3270_INTERNAL const LIB3270_SSL_MESSAGE * openssl_check_fqdn(H3270 *hSession, X509 *server_cert, const char *hostname);
