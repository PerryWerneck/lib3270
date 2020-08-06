/*
 * "Software PW3270, desenvolvido com base nos códigos fontes do WC3270  e  X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270.
 *
 * Copyright (C) <2008> <Banco do Brasil S.A.>
 *
 * Este programa é software livre. Você pode redistribuí-lo e/ou modificá-lo sob
 * os termos da GPL v.2 - Licença Pública Geral  ',  conforme  publicado  pela
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
 * Este programa está nomeado como openssl.c e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas de Mendonça)
 *
 */

 /**
  * @brief Negotiate OpenSSL session.
  *
  */

 #include "private.h"
 #include <lib3270/properties.h>

 static int import_crl(H3270 *hSession, SSL_CTX * ssl_ctx, LIB3270_NET_CONTEXT * context, const char *crl) {

	X509_CRL * x509_crl = NULL;

	// Import CRL
	{
		lib3270_autoptr(BIO) bio = BIO_new_mem_buf(crl,-1);

		BIO * b64 = BIO_new(BIO_f_base64());
		bio = BIO_push(b64, bio);

		BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

		if(!d2i_X509_CRL_bio(bio, &x509_crl)) {
			trace_ssl(hSession,"Can't decode CRL data:\n%s\n",crl);
			return -1;
		}

		lib3270_openssl_crl_free(context);
		context->crl.cert = x509_crl;

	}

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
	} else {
		trace_ssl(hSession,"CRL was not added to context cert store\n");
	}

	return 0;

 }

 int openssl_network_start_tls(H3270 *hSession) {

	SSL_CTX * ctx_context = (SSL_CTX *) lib3270_openssl_get_context(hSession);
	if(!ctx_context)
		return -1;

	LIB3270_NET_CONTEXT * context = hSession->network.context;

	debug("%s",__FUNCTION__);

	context->con = SSL_new(ctx_context);
	if(context->con == NULL)
	{
		static const LIB3270_SSL_MESSAGE message = {
			.type = LIB3270_NOTIFY_SECURE,
			.summary = N_( "Cant create a new SSL structure for current connection." )
		};

		hSession->ssl.message = &message;
		return -1;
	}

	SSL_set_ex_data(context->con,lib3270_openssl_get_ex_index(hSession),(char *) hSession);
//	SSL_set_verify(context->con, SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
	SSL_set_verify(context->con, 0, NULL);

	if(SSL_set_fd(context->con, context->sock) != 1)
	{
		trace_ssl(hSession,"%s","SSL_set_fd failed!\n");

		static const LIB3270_SSL_MESSAGE message = {
			.summary = N_( "SSL negotiation failed" ),
			.body = N_( "Cant set the file descriptor for the input/output facility for the TLS/SSL (encrypted) side of ssl." )
		};

		hSession->ssl.message = &message;
		return -1;

	}

	trace_ssl(hSession, "%s","Running SSL_connect\n");
	int rv = SSL_connect(context->con);
	trace_ssl(hSession, "SSL_connect exits with rc=%d\n",rv);

	if (rv != 1)
	{
		int code = SSL_get_error(context->con,rv);

		if(code == SSL_ERROR_SYSCALL && hSession->ssl.error)
			code = hSession->ssl.error;
		else
			hSession->ssl.error = code;

		trace_ssl(hSession,"SSL_connect failed: %s\n",ERR_reason_error_string(code));

		static const LIB3270_SSL_MESSAGE message = {
			.summary = N_( "SSL Connect failed" ),
			.body = N_("The client was unable to negotiate a secure connection with the host")
		};

		hSession->ssl.message = &message;
		return -1;

	}

	// Get peer certificate, notify application before validation.
	lib3270_autoptr(X509) peer = SSL_get_peer_certificate(context->con);

	if(peer) {

		if(lib3270_get_toggle(hSession,LIB3270_TOGGLE_SSL_TRACE))
		{
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

		lib3270_autoptr(char) crl_text = NULL;
		if(context->crl.url) {

			// There's a pre-defined URL, use it.
			const char *error_message = NULL;
			crl_text = lib3270_url_get(hSession, context->crl.url,&error_message);

			if(error_message) {
				trace_ssl(hSession,"Error downloading CRL from %s: %s\n",context->crl.url,error_message);
			} else {
				import_crl(hSession, ctx_context, context, crl_text);
			}


		} else if(peer) {

			// There's no pre-defined URL, get them from peer.
			lib3270_autoptr(LIB3270_STRING_ARRAY) uris = lib3270_openssl_get_crls_from_peer(hSession, peer);

			if(uris) {

				size_t ix;
				for(ix = 0; ix < uris->length; ix++) {

					const char * error_message = NULL;
					crl_text = lib3270_url_get(hSession, uris->str[ix], &error_message);

					if(error_message) {
						trace_ssl(hSession,"Error downloading CRL from %s: %s\n",uris->str[ix],error_message);
					} else if(!import_crl(hSession, ctx_context, context, crl_text)) {
						break;
					}

				}
			}

		}

	}

	//
	// Validate SSL state.
	//
	long verify_result = SSL_get_verify_result(context->con);

	// Get validation message.
	hSession->ssl.message = lib3270_openssl_message_from_id(verify_result);

	// Trace cypher
	if(lib3270_get_toggle(hSession,LIB3270_TOGGLE_SSL_TRACE))
	{
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
	/*
	switch(verify_result) {
	case X509_V_OK:
		trace_ssl(hSession,"TLS/SSL negotiated connection complete. Peer certificate %s presented.\n", peer ? "was" : "was not");
		break;

#ifdef SSL_ENABLE_SELF_SIGNED_CERT_CHECK
	case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
		trace_ssl(hSession,"TLS/SSL negotiated connection complete with self signed certificate in certificate chain\n");
		set_ssl_state(hSession,LIB3270_SSL_NEGOTIATED);
		return EACCES;
#endif

	default:
		set_ssl_state(hSession,LIB3270_SSL_NEGOTIATED);
	}
	*/

	if(hSession->ssl.message)
		trace_ssl(hSession,"%s",hSession->ssl.message->summary);
	else
		trace_ssl(hSession,"TLS/SSL verify result was %ld\n", verify_result);

	return 0;

}
