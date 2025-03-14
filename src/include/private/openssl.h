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

 /// @brief Connection context for OpenSSL connections.
 typedef struct {
	
	char state;
	LIB3270_NET_CONTEXT parent;
	H3270 *hSession;
	SSL_CTX *ctx;
	SSL *ssl;
    BIO *tcp;
	int cert_error;

 } Context;

 #define SSL_EXDATA_INDEX_INVALID -1

 LIB3270_INTERNAL pthread_mutex_t ssl_guard;
 LIB3270_INTERNAL int e_ctx_ssl_exdata_index;

 LIB3270_INTERNAL SSL_CTX * get_openssl_context(H3270 *hSession);
 LIB3270_INTERNAL char * get_openssl_errors(Context *context);
