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

/**
 * @brief Negotiate OpenSSL session.
 *
 */

#error deprecated
 
 
#include "private.h"
#include <lib3270/properties.h>
#include <utilc.h>

static int import_crl(H3270 *hSession, SSL_CTX * ssl_ctx, LIB3270_NET_CONTEXT * context, const char *url) {

	X509_CRL * x509_crl = NULL;

	const char *error_message = NULL;
	if(strncasecmp(url,"ldap",4) == 0) {

		// Download using LDAP
#ifdef HAVE_LDAP

		x509_crl = lib3270_crl_get_using_ldap(hSession, url, &error_message);

#else

		error_message = _("No LDAP support");

#endif // HAVE_LDAP

	} else {

		// Download with URL
		lib3270_autoptr(char) crl_text = lib3270_url_get(hSession, url, &error_message);

		lib3270_autoptr(BIO) bio = BIO_new_mem_buf(crl_text,-1);

		BIO * b64 = BIO_new(BIO_f_base64());
		bio = BIO_push(b64, bio);

		BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

		if(!d2i_X509_CRL_bio(bio, &x509_crl)) {
			trace_ssl(hSession,"Can't decode CRL data:\n%s\n",crl_text);
			error_message = _("Can't decode CRL data");
		}

	}

	if(error_message)
		trace_ssl(hSession,"Error downloading CRL from %s: %s\n",url,error_message);

	if(!x509_crl)
		return -1;

	lib3270_openssl_crl_free(context);
	context->crl.cert = x509_crl;

	if(lib3270_get_toggle(hSession,LIB3270_TOGGLE_SSL_TRACE)) {

		lib3270_autoptr(BIO) bio = BIO_new(BIO_s_mem());

		X509_CRL_print(bio,x509_crl);

		unsigned char *data = NULL;
		int n = BIO_get_mem_data(bio, &data);

		lib3270_autoptr(char) text = (char *) lib3270_malloc(n+1);
		memcpy(text,data,n);
		text[n]	='\0';

		trace_ssl(hSession,"CRL Data:\n%s\n",text);

	}

	// Add CRL in the store.
	X509_STORE *store = SSL_CTX_get_cert_store(ssl_ctx);

	if(X509_STORE_add_crl(store, x509_crl)) {
		trace_ssl(hSession,"CRL was added to context cert store\n");
		return 0;
	}

	trace_ssl(hSession,"CRL was not added to context cert store\n");

	return -1;

}

static int download_crl_from_peer(H3270 *hSession, SSL_CTX * ctx_context, LIB3270_NET_CONTEXT * context, X509 *peer) {

	debug("%s peer=%p",__FUNCTION__,(void *) peer);

	if(!peer)
		return -1;

	lib3270_autoptr(LIB3270_STRING_ARRAY) uris = lib3270_openssl_get_crls_from_peer(hSession, peer);
	if(!uris) {
		trace_ssl(hSession,"Can't get distpoints from peer certificate\n");
		return -1;
	}

	size_t ix;

	const char *prefer = lib3270_crl_get_preferred_protocol(hSession);
	if(!prefer) {

		// No preferred protocol, try all uris.
		for(ix = 0; ix < uris->length; ix++) {

			if(!import_crl(hSession,ctx_context,context,uris->str[ix])) {
				trace_ssl(hSession,"Got CRL from %s\n",uris->str[ix]);
				return 0;
			}

		}
		return -1;

	}

	// Try preferred protocol.
	trace_ssl(hSession,"CRL download protocol is set to %s\n",prefer);

	size_t length = strlen(prefer);

	for(ix = 0; ix < uris->length; ix++) {

		if(strncasecmp(prefer,uris->str[ix],length))
			continue;

		if(!import_crl(hSession,ctx_context,context,uris->str[ix])) {
			trace_ssl(hSession,"Got CRL from %s\n",uris->str[ix]);
			return 0;
		}

	}

	// Not found; try other ones
	for(ix = 0; ix < uris->length; ix++) {

		if(!strncasecmp(prefer,uris->str[ix],length))
			continue;

		if(!import_crl(hSession,ctx_context,context,uris->str[ix])) {
			trace_ssl(hSession,"Got CRL from %s\n",uris->str[ix]);
			return 0;
		}

	}

	return -1;

}

int x509_store_ctx_error_callback(int ok, X509_STORE_CTX GNUC_UNUSED(*ctx)) {
	debug("%s(%d)",__FUNCTION__,ok);

	/*
	  55     {
	  56         if (!ok) {
	  57             Category::getInstance("OpenSSL").error(
	  58                 "path validation failure at depth(%d): %s",
	  59                 X509_STORE_CTX_get_error_depth(ctx),
	  60                 X509_verify_cert_error_string(X509_STORE_CTX_get_error(ctx))
	  61                 );
	  62         }
	  63         return ok;
	  64     }
	*/
	return ok;
}

int openssl_network_start_tls(H3270 *hSession) {

	SSL_CTX * ctx_context = (SSL_CTX *) lib3270_openssl_get_context(hSession);
	if(!ctx_context) {

		if(!hSession->ssl.message) {
			static const LIB3270_SSL_MESSAGE message = {
				.type = LIB3270_NOTIFY_SECURE,
				.summary = N_( "Cant get SSL context for current connection." )
			};
			hSession->ssl.message = &message;
		}
		return -1;
	}

	LIB3270_NET_CONTEXT * context = hSession->network.context;

	debug("%s",__FUNCTION__);

	context->con = SSL_new(ctx_context);
	if(context->con == NULL) {
		static const LIB3270_SSL_MESSAGE message = {
			.type = LIB3270_NOTIFY_SECURE,
			.summary = N_( "Cant create a new SSL structure for current connection." )
		};
		hSession->ssl.message = &message;
		return -1;
	}

	SSL_set_ex_data(context->con,lib3270_openssl_get_ex_index(hSession),(char *) hSession);
//	SSL_set_verify(context->con, SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
//	SSL_set_verify(context->con, SSL_VERIFY_PEER, NULL);
	SSL_set_verify(context->con, SSL_VERIFY_NONE, NULL);

	if(SSL_set_fd(context->con, context->sock) != 1) {
		trace_ssl(hSession,"%s","SSL_set_fd failed!\n");

		static const LIB3270_SSL_MESSAGE message = {
			.summary = N_( "SSL negotiation failed" ),
			.body = N_( "Cant set the file descriptor for the input/output facility for the TLS/SSL (encrypted) side of ssl." )
		};

		hSession->ssl.message = &message;
		return -1;

	}

	trace_ssl(hSession, "%s","Running SSL_connect\n");
	hSession->ssl.error = 0;
	int rv = SSL_connect(context->con);
	trace_ssl(hSession, "SSL_connect exits with rc=%d\n",rv);

	if (rv != 1) {

		LIB3270_SSL_MESSAGE message = {
			.type = LIB3270_NOTIFY_ERROR,
			.title = N_( "Connection failed" ),
			.summary = N_("Unable to negotiate a secure connection with the host"),
		};

		if(!hSession->ssl.error)
			hSession->ssl.error = SSL_get_error(context->con,rv);

		if(hSession->ssl.error == SSL_ERROR_SYSCALL) {

			// Some I/O error occurred.
			// The OpenSSL error queue may contain more information on the error.
			// If the error queue is empty (i.e. ERR_get_error() returns 0), ret
			// can be used to find out more about the error:
			// If ret == 0, an EOF was observed that violates the protocol.
			// If ret == -1, the underlying BIO reported an I/O error
			// (for socket I/O on Unix systems, consult errno for details).

			if(rv == 0) {
				message.body = N_("An EOF was observed that violates the protocol");
			} else if(errno)
				message.body = strerror(errno);
			else
				message.body = N_("Unexpected I/O error");

		} else {

			message.body = ERR_reason_error_string(hSession->ssl.error);

		}

		debug("SSL_connect failed: %s (rc=%d)\n",message.body ? message.body : message.summary, hSession->ssl.error);
		trace_ssl(hSession,"SSL_connect failed: %s (rc=%d)\n",message.body ? message.body : message.summary, hSession->ssl.error);

		hSession->ssl.message = &message;
		return -1;

	}

	// Get peer certificate, notify application before validation.
	lib3270_autoptr(X509) peer = SSL_get_peer_certificate(context->con);

	if(peer) {

		if(lib3270_get_toggle(hSession,LIB3270_TOGGLE_SSL_TRACE)) {
			BIO				* out	= BIO_new(BIO_s_mem());
			unsigned char	* data;
			unsigned char	* text;
			int				  n;

			X509_print(out,peer);

			n		= BIO_get_mem_data(out, &data);
			text	= (unsigned char *) malloc (n+1);
			text[n]	='\0';
			memcpy(text,data,n);

			trace_ssl(hSession,"TLS/SSL peer certificate:\n%s\n",text);

			free(text);
			BIO_free(out);

		}

	}

	// Do we really need to download a new CRL?
	if(lib3270_ssl_get_crl_download(hSession) && SSL_get_verify_result(context->con) == X509_V_ERR_UNABLE_TO_GET_CRL) {

		// CRL download is enabled and verification has failed; look for CRL file.

		trace_ssl(hSession,"CRL Validation has failed, requesting CRL download\n");
		set_ssl_state(hSession,LIB3270_SSL_VERIFYING);

		int rc_download = -1;

		if(context->crl.url) {
			rc_download = import_crl(hSession, ctx_context,context,context->crl.url);
		} else {
			rc_download = download_crl_from_peer(hSession, ctx_context, context, peer);
		}

		debug("Download rc=%d",rc_download);

		if(!rc_download) {
			// Got CRL, verify it!
			// Reference: https://stackoverflow.com/questions/10510850/how-to-verify-the-certificate-for-the-ongoing-ssl-session

			X509_STORE_CTX *csc = X509_STORE_CTX_new();
			X509_STORE_CTX_set_verify_cb(csc, x509_store_ctx_error_callback);
			X509_STORE_CTX_init(csc, SSL_CTX_get_cert_store(ctx_context), peer, NULL);

			if(X509_verify_cert(csc) != 1)
				rv = X509_STORE_CTX_get_error(csc);
			else
				rv = X509_V_OK;

			trace_ssl(hSession, "X509_verify_cert error code was %d\n", rv);

			SSL_set_verify_result(context->con, rv);

			X509_STORE_CTX_free(csc);

		}

	}

	//
	// Validate SSL state.
	//
	long verify_result = SSL_get_verify_result(context->con);

	// Get validation message.
	hSession->ssl.message = lib3270_openssl_message_from_id(verify_result);
	debug("Verify message: %s",hSession->ssl.message->summary);

	// Trace cypher
	if(lib3270_get_toggle(hSession,LIB3270_TOGGLE_SSL_TRACE)) {
		char				  buffer[4096];
		int 				  alg_bits		= 0;
		const SSL_CIPHER	* cipher		= SSL_get_current_cipher(context->con);

		trace_ssl(hSession,"TLS/SSL cipher description: %s",SSL_CIPHER_description((SSL_CIPHER *) cipher, buffer, 4095));
		SSL_CIPHER_get_bits(cipher, &alg_bits);
		trace_ssl(hSession,"%s version %s with %d bits\n",
		          SSL_CIPHER_get_name(cipher),
		          SSL_CIPHER_get_version(cipher),
		          alg_bits);
	}

	// Check results.
	if(hSession->ssl.message)
		trace_ssl(hSession,"%s\n",hSession->ssl.message->summary);
	else
		trace_ssl(hSession,"TLS/SSL verify result was %ld\n", verify_result);

	set_ssl_state(hSession,LIB3270_SSL_NEGOTIATED);

	return 0;

}
