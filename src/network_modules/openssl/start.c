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

 int openssl_network_start_tls(H3270 *hSession, LIB3270_NETWORK_STATE *state) {

	SSL_CTX * ctx_context = (SSL_CTX *) lib3270_openssl_get_context(hSession,state);
	if(!ctx_context)
		return -1;

	LIB3270_NET_CONTEXT * context = hSession->network.context;

	debug("%s",__FUNCTION__);

	set_ssl_state(hSession,LIB3270_SSL_NEGOTIATING);
	context->con = SSL_new(ctx_context);
	if(context->con == NULL)
	{
		static const LIB3270_NETWORK_POPUP popup = {
			.type = LIB3270_NOTIFY_SECURE,
			.summary = N_( "Cant create a new SSL structure for current connection." )
		};

		state->popup = &popup;
		return -1;
	}

	SSL_set_ex_data(context->con,lib3270_openssl_get_ex_index(hSession),(char *) hSession);
//	SSL_set_verify(context->con, SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
	SSL_set_verify(context->con, 0, NULL);

	if(SSL_set_fd(context->con, context->sock) != 1)
	{
		trace_ssl(hSession,"%s","SSL_set_fd failed!\n");

		static const LIB3270_NETWORK_POPUP popup = {
			.summary = N_( "SSL negotiation failed" ),
			.body = N_( "Cant set the file descriptor for the input/output facility for the TLS/SSL (encrypted) side of ssl." )
		};

		state->popup = &popup;
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

		state->error_message = ERR_lib_error_string(code);

		trace_ssl(hSession,"SSL_connect failed: %s\n",ERR_reason_error_string(code));

		static const LIB3270_NETWORK_POPUP popup = {
			.summary = N_( "SSL Connect failed" ),
		};

		state->popup = &popup;
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

		trace_ssl(hSession,"CRL Validation has failed, requesting download\n");

		lib3270_autoptr(char) crl_text = NULL;
		if(context->crl.url) {

			// There's a pre-defined URL, use it.
			const LIB3270_POPUP * popup = NULL;
			crl_text = lib3270_url_get(hSession, context->crl.url,&popup);

			if(popup) {
				state->popup = popup;
				trace_ssl(hSession,"Error downloading CRL from %s: %s\n",context->crl.url,popup->summary);
			}

#ifndef DEBUG
			#error TODO: Import crl_text;
#endif // DEBUG

		} else if(peer) {

			// There's no pre-defined URL, get them from peer.
			lib3270_autoptr(LIB3270_STRING_ARRAY) uris = lib3270_openssl_get_crls_from_peer(hSession, peer);

			if(uris) {

				size_t ix;
				for(ix = 0; ix < uris->length; ix++) {

					LIB3270_POPUP * popup = NULL;
					crl_text = lib3270_url_get(hSession, uris->str[ix], &popup);

					if(popup) {
						trace_ssl(hSession,"Error downloading CRL from %s: %s\n",uris[ix],popup->summary);
					}

#ifndef DEBUG
					#error TODO: Import crl_text;
#endif // DEBUG

				}
			}

		}

	}



	return 0;
}
