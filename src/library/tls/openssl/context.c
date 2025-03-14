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
 /// @brief Callback function for SSL certificate verification
 ///
 /// This function is registered with SSL to be called during the verification
 /// of each certificate in the server's identity cert chain.
 ///
 /// @param ok		The status of this certificate from the SSL verify code.
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
		trace_ssl(hSession,"Certificate verified\n");
        return (approve);
	}

    int cert_error = X509_STORE_CTX_get_error(x_ctx);
    // X509 *current_cert = X509_STORE_CTX_get_current_cert(x_ctx);

	((Context *) hSession->connection.context)->cert_error = cert_error;

	trace_ssl(
		hSession,
		"Certificate verify failed (reason = %d) (%s)\n", 
			cert_error, 
			X509_verify_cert_error_string(cert_error)
	);

	return (approve);
 
 }

 LIB3270_INTERNAL SSL_CTX * get_openssl_context(H3270 *hSession) {

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
 
