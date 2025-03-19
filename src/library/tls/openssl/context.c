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
 #include <private/openssl.h>
 #include <private/trace.h>
 #include <private/session.h>

 #include <openssl/ssl.h>
 #include <openssl/x509v3.h>
 #include <openssl/err.h>
 #include <pthread.h>

 #define TLS_VERIFY_DEPTH 6

 int e_ctx_ssl_exdata_index = SSL_EXDATA_INDEX_INVALID;
 pthread_mutex_t ssl_guard = PTHREAD_MUTEX_INITIALIZER;

 ///
 /// @brief Callback function for SSL certificate verification.
 ///
 /// This function is registered with SSL to be called during the verification
 /// of each certificate in the server's identity cert chain.
 ///
 /// @param approve	The status of this certificate from the SSL verify code.
 /// @param x_ctx	Ptr to the X509 certificate store structure  
 ///
 /// @return The potentially modified status after processing this certificate.
 ///
 static int cert_verify_cb(int approve, X509_STORE_CTX *x_ctx) {

	H3270 * hSession;

	{
		if (x_ctx == NULL) {
			// Invalid X509 context pointer
			return (approve);
		}    
		
		//
		// Retrieve the pointer to the SSL structure for this connection and then
		// the application specific data stored into the SSL object.
		//
		if (e_ctx_ssl_exdata_index == SSL_EXDATA_INDEX_INVALID) {
			// Invalid SSL exdata index for context value
			return (approve);
		}
			
		SSL *ssl = X509_STORE_CTX_get_ex_data(x_ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
		if (!ssl) {
			// NULL pointer retrieved for SSL session pointer from X509 ctx ex_data
			return (approve);
		}        
		
		hSession = (H3270 *) SSL_get_ex_data(ssl, e_ctx_ssl_exdata_index);
		if (!hSession) {
			// NULL pointer retrieved for context from SSL ex_data
			return (approve);
		}        

	}

	if(approve) {
		trace_ssl(hSession,"Certificate verified (state=%d)\n",((Context *) hSession->connection.context)->cert_error);
        return (approve);
	}

    int cert_error = X509_STORE_CTX_get_error(x_ctx);

	trace_ssl(
		hSession,
		"Certificate verify failed (reason = %d) (%s)\n", 
			cert_error, 
			X509_verify_cert_error_string(cert_error)
	);	

	debug("------------> cert_error=%d (%s) approve=%d",cert_error,X509_verify_cert_error_string(cert_error),approve);

	const LIB3270_SSL_MESSAGE *ssl_message = NULL;
	if(cert_error) {
		ssl_message = openssl_message_from_code(cert_error);
	}

	if(ssl_message) {

		if(set_ssl_message(hSession,ssl_message)) {
			((Context *) hSession->connection.context)->cert_error = cert_error;
		}

		debug("msg=%s (%s)",ssl_message->name,ssl_message->summary);

		// Check if ssl_message->name is authorized by policy.
		if(!hSession->cbk.check_policy(hSession,ssl_message->name,EINVAL)) {
			trace_ssl(
				hSession,
				"Aproving '%s' by policy '%s'\n",
					X509_verify_cert_error_string(cert_error),
					ssl_message->name
			);
			approve = 1;
		}

	} else {

		trace_ssl(
			hSession,
			"Cant find description for '%s', policy was not verified\n",
				X509_verify_cert_error_string(cert_error)
		);

	}

	return (approve);
 
 }

 static void info_callback(const SSL *ssl, int where, int ret) {

	H3270 *hSession = (H3270 *) SSL_get_ex_data(ssl,e_ctx_ssl_exdata_index);
	Context * context = (Context *) hSession->connection.context;

	switch(where) {
	case SSL_CB_CONNECT_LOOP:

		trace_ssl(
			hSession,
			"SSL_connect: %s %s\n",
			SSL_state_string(ssl), 
			SSL_state_string_long(ssl
		));

		break;

	case SSL_CB_CONNECT_EXIT:

		trace_ssl(hSession,"%s: SSL_CB_CONNECT_EXIT\n",__FUNCTION__);

		if (ret == 0) {

			trace_ssl(hSession,"SSL_connect: failed in %s\n",SSL_state_string_long(ssl));

		} else if (ret < 0) {

			lib3270_autoptr(char) errors = openssl_errors(context);

			trace_ssl(hSession,"SSL Connect error %d\n%s\n",
				ret,
				errors
			);

		}
		break;

	default:
		trace_ssl(hSession,"SSL Current state is \"%s\"\n",SSL_state_string_long(ssl));
	}

	if(where & SSL_CB_EXIT) {
		trace_ssl(hSession,"SSL_CB_EXIT ret=%d\n",ret);
	}

	if(where & SSL_CB_ALERT) {
		trace_ssl(hSession,"SSL ALERT: %s\n",SSL_alert_type_string_long(ret));
	}

	if(where & SSL_CB_HANDSHAKE_DONE) {
		trace_ssl(
			hSession,
			"%s: SSL_CB_HANDSHAKE_DONE state=%04x (%s)\n",
				__FUNCTION__,
				SSL_get_state(ssl),
				SSL_state_string_long(ssl)
			);

		if(SSL_get_state(ssl) == TLS_ST_OK)
			set_ssl_state(hSession,LIB3270_SSL_NEGOTIATED);
		else
			set_ssl_state(hSession,LIB3270_SSL_UNSECURE);
	}
 
 }

 LIB3270_INTERNAL SSL_CTX * openssl_context(H3270 *hSession) {

	pthread_mutex_lock(&ssl_guard);

	static SSL_CTX *context = NULL;
	if(context) {
		SSL_CTX_up_ref(context);
		pthread_mutex_unlock(&ssl_guard);
		return context;
	}

	ERR_load_crypto_strings();
	SSL_load_error_strings();

	context = SSL_CTX_new(TLS_client_method());
	if(!context) {
		trace_ssl(hSession,"Failed to create SSL context\n");
		pthread_mutex_unlock(&ssl_guard);
		return NULL;
	}

	SSL_CTX_set_info_callback(context, info_callback);

	// Make sure we're verifying the server
	SSL_CTX_set_verify(context, SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT, cert_verify_cb);

   	//
    // Set up X509 params and assign them to the SSL ctx
    // - Enable CRL checks
    // - Max # of untrusted CA certs that can exist in a chain
    // - ensure that the cert is being used as intended, if
    //   it contains the X509 KeyUsage extension
    //
	{
		// https://linux.die.net/man/3/x509_verify_param_set_flags

		X509_VERIFY_PARAM *vpm = X509_VERIFY_PARAM_new();
		if (vpm == NULL) {
			trace_ssl(hSession,"Failed to create X509 verify params\n");
			SSL_CTX_free(context);
			context = NULL;
			pthread_mutex_unlock(&ssl_guard);
			return NULL;
		}
		
		X509_VERIFY_PARAM_set_depth(vpm, TLS_VERIFY_DEPTH);

		X509_VERIFY_PARAM_set_purpose(vpm, X509_PURPOSE_SSL_SERVER);

		// https://stackoverflow.com/questions/26218495/openssl-c-api-crl-check
		// X509_VERIFY_PARAM_set_flags(vpm, X509_V_FLAG_CRL_CHECK|X509_V_FLAG_CRL_CHECK_ALL);
	
		SSL_CTX_set1_param(context, vpm);
		X509_VERIFY_PARAM_free(vpm);
	}

    if (e_ctx_ssl_exdata_index == SSL_EXDATA_INDEX_INVALID) {
        e_ctx_ssl_exdata_index = SSL_get_ex_new_index(0, PACKAGE_NAME, NULL, NULL, NULL);    
    }

	pthread_mutex_unlock(&ssl_guard);

	return context;
}
 
