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
 */

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
static void info_callback(INFO_CONST SSL *s, int where, int ret)
{
	H3270 *hSession = (H3270 *) SSL_get_ex_data(s,ssl_ex_index);
	LIB3270_NET_CONTEXT * context = hSession->network.context;

	switch(where)
	{
	case SSL_CB_CONNECT_LOOP:
		trace_ssl(hSession,"SSL_connect: %s %s\n",SSL_state_string(s), SSL_state_string_long(s));
		break;

	case SSL_CB_CONNECT_EXIT:

		trace_ssl(hSession,"%s: SSL_CB_CONNECT_EXIT\n",__FUNCTION__);

		if (ret == 0)
		{
			context->state.message = SSL_state_string_long(s);
			trace_ssl(hSession,"SSL_connect: failed in %s\n",context->state.message);
		}
		else if (ret < 0)
		{
			unsigned long e = ERR_get_error();
			context->state.message = NULL;

			char err_buf[1024];

			if(e != 0)
			{
				context->state.error = e;
				(void) ERR_error_string_n(e, err_buf, 1023);
			}
#if defined(_WIN32)
			else if (GetLastError() != 0)
			{
				strncpy(err_buf,lib3270_win32_strerror(GetLastError()),1023);
			}
#else
			else if (errno != 0)
			{
				strncpy(err_buf, strerror(errno),1023);
			}
#endif
			else
			{
				err_buf[0] = '\0';
			}

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

#ifdef DEBUG
	if(where & SSL_CB_EXIT)
	{
		trace("%s: SSL_CB_EXIT ret=%d\n",__FUNCTION__,ret);
	}
#endif

	if(where & SSL_CB_ALERT)
	{
		context->state.alert = SSL_alert_type_string_long(ret);
		trace_ssl(hSession,"SSL ALERT: %s\n",context->state.alert);
	}

	if(where & SSL_CB_HANDSHAKE_DONE)
	{
		trace_ssl(hSession,"%s: SSL_CB_HANDSHAKE_DONE state=%04x\n",__FUNCTION__,SSL_get_state(s));
		if(SSL_get_state(s) == SSL_ST_OK)
			set_ssl_state(hSession,LIB3270_SSL_NEGOTIATED);
		else
			set_ssl_state(hSession,LIB3270_SSL_UNSECURE);
	}
}

SSL_CTX * lib3270_openssl_get_context(H3270 *hSession, LIB3270_NETWORK_STATE *state) {

	static SSL_CTX * context = NULL;

	if(context)
		return context;

	trace_ssl(hSession,"Initializing SSL context.\n");

	SSL_load_error_strings();
	SSL_library_init();

	context = SSL_CTX_new(SSLv23_method());
	if(context == NULL)
	{
		static const LIB3270_NETWORK_POPUP popup = {
			.type = LIB3270_NOTIFY_SECURE,
			.icon = "dialog-error",
			.summary = N_( "Can't initialize the TLS/SSL context." ),
		};

		hSession->network.context->state.popup = state->popup = &popup;
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

	return context;

}

int lib3270_openssl_get_ex_index(H3270 GNUC_UNUSED(*hSession)) {
	return ssl_ex_index;
}
