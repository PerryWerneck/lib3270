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

#error deprecated
 
 
/*
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 * References:
 *
 * http://www.openssl.org/docs/ssl/
 * https://stackoverflow.com/questions/4389954/does-openssl-automatically-handle-crls-certificate-revocation-lists-now
 *
 */

/**
 * @brief OpenSSL initialization for linux.
 */

#include <config.h>
#ifdef _WIN32
	#include <winsock2.h>
	#include <windows.h>
	#include <lib3270/win32.h>
#endif // _WIN32

#include "private.h"

#include <openssl/err.h>
#include <openssl/x509_vfy.h>

#ifndef SSL_ST_OK
#define SSL_ST_OK 3
#endif // !SSL_ST_OK

#if OPENSSL_VERSION_NUMBER >= 0x00907000L
#define INFO_CONST const
#else
#define INFO_CONST
#endif

/*--[ Implement ]------------------------------------------------------------------------------------*/

// @brief Index of h3270 handle in SSL session.
static int ssl_ex_index = 0;

/// @brief Callback for tracing protocol negotiation.
static void info_callback(INFO_CONST SSL *s, int where, int ret) {
	H3270 *hSession = (H3270 *) SSL_get_ex_data(s,ssl_ex_index);
	LIB3270_NET_CONTEXT * context = hSession->network.context;

	switch(where) {
	case SSL_CB_CONNECT_LOOP:
		trace_ssl(hSession,"SSL_connect: %s %s\n",SSL_state_string(s), SSL_state_string_long(s));
		break;

	case SSL_CB_CONNECT_EXIT:

		trace_ssl(hSession,"%s: SSL_CB_CONNECT_EXIT\n",__FUNCTION__);

		if (ret == 0) {
			context->state.message = SSL_state_string_long(s);
			trace_ssl(hSession,"SSL_connect: failed in %s\n",context->state.message);
		} else if (ret < 0) {
			unsigned long e = ERR_get_error();
			context->state.message = NULL;

			char err_buf[1024];

			if(e != 0) {
				context->state.error = e;
				(void) ERR_error_string_n(e, err_buf, 1023);
			}
#if defined(_WIN32)
			else if (GetLastError() != 0) {
				lib3270_autoptr(char) msg = lib3270_win32_strerror(GetLastError());
				strncpy(err_buf,msg,1023);
			}
#else
			else if (errno != 0) {
				strncpy(err_buf, strerror(errno),1023);
			}
#endif
			else {
				err_buf[0] = '\0';
			}

			debug("SSL Connect error %d\nMessage: %s\nState: %s\nAlert: %s\n",
			      ret,
			      err_buf,
			      SSL_state_string_long(s),
			      SSL_alert_type_string_long(ret)
			     );

			hSession->ssl.error = e;
			debug("hSession->ssl.error=%d",hSession->ssl.error);

			trace_ssl(hSession,"SSL Connect error %d\nMessage: %s\nState: %s\nAlert: %s\n",
			          ret,
			          err_buf,
			          SSL_state_string_long(s),
			          SSL_alert_type_string_long(ret)
			         );

		}
		break;

	default:
		context->state.message = SSL_state_string_long(s);
		trace_ssl(hSession,"SSL Current state is \"%s\"\n",context->state.message);
	}

	if(where & SSL_CB_EXIT) {
		trace_ssl(hSession,"SSL_CB_EXIT ret=%d\n",ret);
	}

	if(where & SSL_CB_ALERT) {
		context->state.alert = SSL_alert_type_string_long(ret);
		trace_ssl(hSession,"SSL ALERT: %s\n",context->state.alert);
	}

	if(where & SSL_CB_HANDSHAKE_DONE) {
		trace_ssl(hSession,"%s: SSL_CB_HANDSHAKE_DONE state=%04x\n",__FUNCTION__,SSL_get_state(s));
		if(SSL_get_state(s) == SSL_ST_OK)
			set_ssl_state(hSession,LIB3270_SSL_NEGOTIATED);
		else
			set_ssl_state(hSession,LIB3270_SSL_UNSECURE);
	}
}

SSL_CTX * lib3270_openssl_get_context(H3270 *hSession) {

	static SSL_CTX * context = NULL;

	if(context)
		return context;

	SSL_load_error_strings();

#if !defined(OPENSSL_FIPS)

	lib3270_log_write(
		hSession,
		"openssl",
		"Initializing %s\n",
		SSLeay_version(SSLEAY_VERSION)
	);

#elif defined(_WIN32)
		{
			lib3270_auto_cleanup(HKEY) hKey = 0;
			DWORD disp = 0;
			LSTATUS	rc = RegCreateKeyEx(
				HKEY_LOCAL_MACHINE,
				"Software\\" LIB3270_STRINGIZE_VALUE_OF(PRODUCT_NAME) "\\tweaks",
				0,
				NULL,
				REG_OPTION_NON_VOLATILE,
				KEY_QUERY_VALUE|KEY_READ,
				NULL,
				&hKey,
				&disp);

			if(rc == ERROR_SUCCESS) {
				DWORD mode = lib3270_win32_get_dword(hKey, "fips_mode", FIPS_mode());
				if(FIPS_mode_set(mode) != 1) {

					SSL_load_error_strings();

					char err_buff[1024];
					memset(err_buff,0,sizeof(err_buff));
					(void) ERR_error_string_n(ERR_get_error(), err_buff, 1023);

					lib3270_log_write(
						hSession,
						"openssl",
						"Cant set FIPS mode to %u: %s\n",
						(unsigned int) mode,
						err_buff
					);

				} else {
					lib3270_log_write(
						hSession,
						"openssl",
						"FIPS mode set to %u\n",
						(unsigned int) mode
					);
				}
			}

			lib3270_log_write(
				hSession,
				"openssl",
				"Initializing windows %s using fips mode %u.\n",
				SSLeay_version(SSLEAY_VERSION),
				FIPS_mode()
			);

		}
#else

	lib3270_log_write(
		hSession,
		"openssl",
		"Initializing %s %s FIPS.\n",
		SSLeay_version(SSLEAY_VERSION),
		(FIPS_mode() ? "with" : "without" )
	);

#endif

	SSL_library_init();

	context = SSL_CTX_new(SSLv23_method());
	if(context == NULL) {
		static const LIB3270_SSL_MESSAGE message = {
			.type = LIB3270_NOTIFY_SECURE,
			.icon = "dialog-error",
			.summary = N_( "Can't initialize the TLS/SSL context." ),
		};

		hSession->ssl.message = &message;
		hSession->network.context->state.error = ERR_get_error();
		return NULL;
	}

	SSL_CTX_set_options(context, SSL_OP_ALL);
	SSL_CTX_set_info_callback(context, info_callback);

	SSL_CTX_set_default_verify_paths(context);

	ssl_ex_index = SSL_get_ex_new_index(0,NULL,NULL,NULL,NULL);

#ifdef SSL_ENABLE_CRL_CHECK

	// Enable CRL check
	X509_STORE *store = SSL_CTX_get_cert_store(context);
	X509_VERIFY_PARAM *param = X509_VERIFY_PARAM_new();
	X509_VERIFY_PARAM_set_flags(param, X509_V_FLAG_CRL_CHECK);
	X509_STORE_set1_param(store, param);
	X509_VERIFY_PARAM_free(param);

	trace_ssl(hSession,"OpenSSL state context initialized with CRL check.\n");

#else

	trace_ssl(hSession,"OpenSSL state context initialized without CRL check.\n");

#endif // SSL_ENABLE_CRL_CHECK

#ifdef _WIN32
	{
		// Load certs
		// https://stackoverflow.com/questions/9507184/can-openssl-on-windows-use-the-system-certificate-store
		X509_STORE * store = SSL_CTX_get_cert_store(context);

		lib3270_autoptr(char) certpath = lib3270_build_data_filename("certs","*.der",NULL);

		trace_ssl(hSession,"Loading SSL certs from %s\n",certpath);

		WIN32_FIND_DATA ffd;
		HANDLE hFind = FindFirstFile(certpath, &ffd);

		if(hFind == INVALID_HANDLE_VALUE)
		{
			static const LIB3270_SSL_MESSAGE message = {
				.type = LIB3270_NOTIFY_SECURE,
				.icon = "dialog-error",
				.summary = N_( "Cant open custom certificate directory." ),
			};

			hSession->ssl.message = &message;

			trace_ssl(hSession, _( "Can't open \"%s\" (The Windows error code was %ld)" ), certpath, (long) GetLastError());
		}
		else
		{
			do
			{
				char * filename = lib3270_build_data_filename("certs", ffd.cFileName, NULL);

				debug("Loading \"%s\"",filename);

				FILE *fp = fopen(filename,"r");
				if(!fp) {

					trace_ssl(hSession, _( "Can't open \"%s\": %s" ), filename, strerror(errno));

				}
				else
				{
					X509 * cert = d2i_X509_fp(fp, NULL);

					if(!cert)
					{
						static const LIB3270_SSL_MESSAGE message = {
							.type = LIB3270_NOTIFY_SECURE,
							.icon = "dialog-error",
							.summary = N_( "Cant read custom certificate file." ),
						};

						hSession->ssl.message = &message;
						hSession->network.context->state.error = ERR_get_error();

						trace_ssl(hSession, _( "Can't read \"%s\": %s" ), filename, ERR_lib_error_string(hSession->ssl.error));
					}
					else
					{

						if(X509_STORE_add_cert(store, cert) != 1)
						{
							static const LIB3270_SSL_MESSAGE message = {
								.type = LIB3270_NOTIFY_SECURE,
								.icon = "dialog-error",
								.summary = N_( "Cant load custom certificate file." ),
							};

							hSession->ssl.message = &message;
							hSession->network.context->state.error = ERR_get_error();

							trace_ssl(hSession, _( "Can't load \"%s\": %s" ), filename, ERR_lib_error_string(hSession->ssl.error));
						}

						X509_free(cert);
					}

					fclose(fp);
				}

				lib3270_free(filename);

			}
			while (FindNextFile(hFind, &ffd) != 0);

		}
	}
#endif // _WIN32

	return context;

}

int lib3270_openssl_get_ex_index(H3270 GNUC_UNUSED(*hSession)) {
	return ssl_ex_index;
}
