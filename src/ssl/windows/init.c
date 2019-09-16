/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270.
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
 * Este programa está nomeado como - e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 *
 * References:
 *
 * http://www.openssl.org/docs/ssl/
 * https://stackoverflow.com/questions/4389954/does-openssl-automatically-handle-crls-certificate-revocation-lists-now
 *
 */

/**
 * @brief OpenSSL initialization for linux.
 *
 */

#include <config.h>

#if defined(HAVE_LIBSSL)

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509_vfy.h>

#ifndef SSL_ST_OK
	#define SSL_ST_OK 3
#endif // !SSL_ST_OK

#include <lib3270-internals.h>
#include <errno.h>
#include <lib3270.h>
#include <lib3270/internals.h>
#include <lib3270/trace.h>
#include <lib3270/log.h>
#include <dirent.h>

#include "trace_dsc.h"

#include <openssl/x509.h>

/*--[ Implement ]------------------------------------------------------------------------------------*/

/**
 * @brief Initialize openssl library.
 *
 * @return 0 if ok, non zero if fails.
 *
 */
int ssl_ctx_init(H3270 *hSession, SSL_ERROR_MESSAGE * message)
{
	debug("%s ssl_ctx=%p",__FUNCTION__,ssl_ctx);

	if(ssl_ctx)
		return 0;

	trace_ssl(hSession,"Initializing SSL context.\n");

	SSL_load_error_strings();
	SSL_library_init();

	ssl_ctx = SSL_CTX_new(SSLv23_method());
	if(ssl_ctx == NULL)
	{
		message->error = hSession->ssl.error = ERR_get_error();
		message->title = N_( "Security error" );
		message->text = N_( "Cant initialize the SSL context." );
		return -1;
	}

	SSL_CTX_set_options(ssl_ctx, SSL_OP_ALL);
	SSL_CTX_set_info_callback(ssl_ctx, ssl_info_callback);

	// SSL_CTX_set_default_verify_paths(ssl_ctx);

	// Load certs
	// https://stackoverflow.com/questions/9507184/can-openssl-on-windows-use-the-system-certificate-store
	X509_STORE * store = SSL_CTX_get_cert_store(ssl_ctx);

	lib3270_autoptr(char) certpath = lib3270_build_data_filename("certs","*.der",NULL);

	WIN32_FIND_DATA ffd;
	HANDLE hFind = FindFirstFile(certpath, &ffd);

	if(hFind == INVALID_HANDLE_VALUE)
	{
		lib3270_autoptr(char) message = lib3270_strdup_printf( _( "Can't read SSL certificates from \"%s\"" ), certpath);

		lib3270_popup_dialog(
			hSession,
			LIB3270_NOTIFY_ERROR,
			N_( "Security error" ),
			message,
			_("The windows error code was %d"), (int) GetLastError()
		);

	}
	else
	{
        do
		{
			char * filename = lib3270_build_data_filename("certs",ffd.cFileName,NULL);

			debug("Loading \"%s\"",filename);

			FILE *fp = fopen(filename,"r");
			if(!fp) {

				lib3270_autoptr(char) message = lib3270_strdup_printf( _( "Can't open \"%s\"" ), filename);

				lib3270_popup_dialog(
					hSession,
					LIB3270_NOTIFY_ERROR,
					N_( "Security error" ),
					message,
					"%s", strerror(errno)
				);

			}
			else
			{
				X509 * cert = d2i_X509_fp(fp, NULL);

				if(!cert)
				{
					int ssl_error = ERR_get_error();

					lib3270_autoptr(char) message = lib3270_strdup_printf( _( "Can't read \"%s\"" ), filename);

					lib3270_popup_dialog(
						hSession,
						LIB3270_NOTIFY_ERROR,
						N_( "Security error" ),
						message,
						"%s", ERR_lib_error_string(ssl_error)
					);

				}
				else
				{
					trace_ssl(hSession,"Loading %s\n",filename);

					if(X509_STORE_add_cert(store, cert) != 1)
					{
						int ssl_error = ERR_get_error();

						lib3270_autoptr(char) message = lib3270_strdup_printf( _( "Can't load \"%s\"" ), filename);

						lib3270_popup_dialog(
							hSession,
							LIB3270_NOTIFY_ERROR,
							N_( "Security error" ),
							message,
							"%s", ERR_lib_error_string(ssl_error)
						);
					}

					X509_free(cert);
				}

				fclose(fp);
			}

			lib3270_free(filename);

		}
		while (FindNextFile(hFind, &ffd) != 0);

	}

	// lib3270_build_lib3270_strdup_printf("%s\\certs",appdir);

	ssl_3270_ex_index = SSL_get_ex_new_index(0,NULL,NULL,NULL,NULL);

#ifdef SSL_ENABLE_CRL_CHECK

	// Enable CRL check
	X509_VERIFY_PARAM *param = X509_VERIFY_PARAM_new();
	X509_VERIFY_PARAM_set_flags(param, X509_V_FLAG_CRL_CHECK);
	X509_STORE_set1_param(store, param);
	X509_VERIFY_PARAM_free(param);
	trace_ssl(hSession,"CRL CHECK was enabled\n");

#endif // SSL_ENABLE_CRL_CHECK

	return 0;

}

#endif // HAVE_LIBSSL
