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
 *
 */


#include <config.h>
#include <internals.h>

#if defined(HAVE_LIBSSL)

	#include <openssl/ssl.h>
	#include <openssl/err.h>
	#include <openssl/x509_vfy.h>
	#include <openssl/x509v3.h>

	#ifndef SSL_ST_OK
		#define SSL_ST_OK 3
	#endif // !SSL_ST_OK

	#include "crl.h"

#endif

#include <errno.h>
#include <lib3270.h>
#include <lib3270/internals.h>
#include <lib3270/trace.h>
#include <lib3270/log.h>
#include <lib3270/toggle.h>
#include "hostc.h" // host_disconnect
#include "trace_dsc.h"

/*--[ Implement ]------------------------------------------------------------------------------------*/

#if defined(HAVE_LIBSSL)

 /**
  * @brief Index of h3270 handle in SSL session.
  *
  */
 int ssl_3270_ex_index = -1;

/**
 * @brief Global SSL_CTX object as framework to establish TLS/SSL or DTLS enabled connections.
 *
 */
 SSL_CTX * ssl_ctx = NULL;

/**
 * @brief Initialize openssl session.
 *
 * @param hSession lib3270 session handle.
 *
 * @return 0 if ok, non zero if fails.
 *
 */
static int background_ssl_init(H3270 *hSession, void *message)
{
	set_ssl_state(hSession,LIB3270_SSL_UNDEFINED);
	hSession->ssl.error = 0;
	hSession->ssl.host = False;

	if(ssl_ctx_init(hSession, (SSL_ERROR_MESSAGE *) message)) {
		debug("%s has failed","ssl_ctx_init");
		set_ssl_state(hSession,LIB3270_SSL_UNDEFINED);
		hSession->ssl.host = False;
		return -1;
	}

	if(hSession->ssl.con)
		SSL_free(hSession->ssl.con);

	hSession->ssl.con = SSL_new(ssl_ctx);
	if(hSession->ssl.con == NULL)
	{
		static const LIB3270_POPUP popup = {
			.type = LIB3270_NOTIFY_SECURE,
			.summary = N_( "Cant create a new SSL structure for current connection." )
		};

		((SSL_ERROR_MESSAGE *) message)->code = hSession->ssl.error = ERR_get_error();
		((SSL_ERROR_MESSAGE *) message)->popup = &popup;
		return -1;
	}

	SSL_set_ex_data(hSession->ssl.con,ssl_3270_ex_index,(char *) hSession);
//	SSL_set_verify(session->ssl_con, SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
	SSL_set_verify(hSession->ssl.con, 0, NULL);

	return 0;
}

#if defined(SSL_ENABLE_CRL_CHECK)
int x509_store_ctx_error_callback(int ok, X509_STORE_CTX GNUC_UNUSED(*ctx))
{
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
#endif // SSL_ENABLE_CRL_CHECK

static const struct ssl_protocol {
	int id;
	const char * description;
} ssl_protocols[] = {

	{
		.id = SSL3_VERSION,
		.description = SSL_TXT_SSLV3
	},
	{
		.id = TLS1_VERSION,
		.description = SSL_TXT_TLSV1
	},
	{
		.id = TLS1_1_VERSION,
		.description = SSL_TXT_TLSV1_1
	},
	{
		.id = TLS1_2_VERSION,
		.description = SSL_TXT_TLSV1_2
	},
#ifdef DTLS1_VERSION
	{
		.id = DTLS1_VERSION,
		.description = "DTLSv1"
	},
#endif // DTLS1_VERSION
#ifdef DTLS1_2_VERSION
	{
		.id = DTLS1_2_VERSION,
		.description = "DTLSv2"
	},
#endif // DTLS1_2_VERSION

};

static const struct ssl_protocol * get_protocol_from_id(int id) {

	if(id < 1)
		return NULL;

	id--;

	if( ((size_t) id) > (sizeof(ssl_protocols)/sizeof(ssl_protocols[0])))
		return NULL;

	return ssl_protocols + id;

}

static int background_ssl_negotiation(H3270 *hSession, void *message)
{
	int rv;

	trace("%s",__FUNCTION__);

	/* Initialize the SSL library. */
	if(background_ssl_init(hSession,message))
	{
		return -1;
	}

	/* Set up the TLS/SSL connection. */
	const struct ssl_protocol * protocol;

	if( (protocol = get_protocol_from_id(hSession->ssl.protocol.min_version)) != NULL )
	{
#if (OPENSSL_VERSION_NUMBER >= 0x1010009fL)
		if(SSL_set_min_proto_version(hSession->ssl.con,protocol->id) == 1)
		{
			trace_ssl(hSession,"Minimum protocol version set to %s\n",protocol->description);
		}
		else
		{
			lib3270_write_log(hSession,"ssl","Can't set minimum protocol version to %s",protocol->description);
		}
#else
		trace_ssl(hSession,"Can't set minimum protocol version to %s\n",protocol->description);
#endif // OPENSSL_VERSION_NUMBER
	}

	if( (protocol = get_protocol_from_id(hSession->ssl.protocol.max_version)) != NULL )
	{
#if (OPENSSL_VERSION_NUMBER >= 0x1010009fL)
		if(SSL_set_max_proto_version(hSession->ssl.con,protocol->id) == 1)
		{
			trace_ssl(hSession,"Maximum protocol version set to %s\n",protocol->description);
		}
		else
		{
			lib3270_write_log(hSession,"ssl","Can't set maximum protocol version to %s",protocol->description);
		}
#else
		trace_ssl(hSession,"Can't set maximum protocol version to %s\n",protocol->description);
#endif // OPENSSL_VERSION_NUMBER
	}

	if(SSL_set_fd(hSession->ssl.con, hSession->connection.sock) != 1)
	{
		trace_ssl(hSession,"%s","SSL_set_fd failed!\n");

		static const LIB3270_POPUP popup = {
			.summary = N_( "SSL negotiation failed" ),
			.body = N_( "Cant set the file descriptor for the input/output facility for the TLS/SSL (encrypted) side of ssl." )
		};

		((SSL_ERROR_MESSAGE *) message)->popup = &popup;

		return -1;
	}

#ifdef SSL_CRL_URL

	// Load CRL from pre-defined URL
	if(!hSession->ssl.crl.cert)
	{
		if(lib3270_crl_new_from_url(hSession, message, SSL_CRL_URL))
			return EACCES;
	}

#endif // SSL_CRL_URL

	trace_ssl(hSession, "%s","Running SSL_connect\n");
	rv = SSL_connect(hSession->ssl.con);
	trace_ssl(hSession, "SSL_connect exits with rc=%d\n",rv);

	if (rv != 1)
	{
		((SSL_ERROR_MESSAGE *) message)->code = SSL_get_error(hSession->ssl.con,rv);
		if(((SSL_ERROR_MESSAGE *) message)->code == SSL_ERROR_SYSCALL && hSession->ssl.error)
			((SSL_ERROR_MESSAGE *) message)->code = hSession->ssl.error;

		const char * msg = ERR_lib_error_string(((SSL_ERROR_MESSAGE *) message)->code);

		trace_ssl(hSession,"SSL_connect failed: %s %s\n",msg,ERR_reason_error_string(hSession->ssl.error));

		static const LIB3270_POPUP popup = {
			.type = LIB3270_NOTIFY_ERROR,
			.summary = N_( "SSL Connect failed" ),
		};

		((SSL_ERROR_MESSAGE *) message)->popup = &popup;

		return -1;

	}

	//
	// Success.
	//

	// Get peer certificate, notify application before validation.
	lib3270_autoptr(X509) peer = SSL_get_peer_certificate(hSession->ssl.con);

	if(peer)
	{
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

		hSession->cbk.set_peer_certificate(peer);

#ifdef SSL_ENABLE_CRL_CHECK

		if(!hSession->ssl.crl.cert)
		{
			if(lib3270_crl_new_from_x509(hSession, message, peer))
				return EACCES;
		}

#endif // SSL_ENABLE_CRL_CHECK

	}

#ifdef SSL_ENABLE_CRL_CHECK
	if(SSL_get_verify_result(hSession->ssl.con) == X509_V_ERR_UNABLE_TO_GET_CRL && hSession->ssl.crl.cert && peer)
	{
		//
		// Verify CRL
		//
		// References:
		//
		// http://www.zedwood.com/article/cpp-check-crl-for-revocation
		//

		trace_ssl(hSession,"Doing CRL check using %s\n",hSession->ssl.crl.url);

		// Got CRL, verify it!
		// Reference: https://stackoverflow.com/questions/10510850/how-to-verify-the-certificate-for-the-ongoing-ssl-session
		X509_STORE_CTX *csc = X509_STORE_CTX_new();
		X509_STORE_CTX_set_verify_cb(csc, x509_store_ctx_error_callback);
		X509_STORE_CTX_init(csc, SSL_CTX_get_cert_store(ssl_ctx), peer, NULL);

		if(X509_verify_cert(csc) != 1)
			rv = X509_STORE_CTX_get_error(csc);
		else
			rv = X509_V_OK;

		trace_ssl(hSession, "X509_verify_cert error code was %d", rv);

		SSL_set_verify_result(hSession->ssl.con, rv);

		X509_STORE_CTX_free(csc);

	}
#endif // SSL_ENABLE_CRL_CHECK

	// Check validation state.
	rv = SSL_get_verify_result(hSession->ssl.con);
	debug("SSL Verify result was %d", rv);
	const struct ssl_status_msg * msg = ssl_get_status_from_error_code((long) rv);

	if(!msg)
	{
		trace_ssl(hSession,"Unexpected or invalid TLS/SSL verify result %d\n",rv);
		set_ssl_state(hSession,LIB3270_SSL_UNSECURE);

		static LIB3270_POPUP popup = {
			.summary = N_( "Can't verify." ),
			.body = N_( "Unexpected or invalid TLS/SSL verify result" )
		};

		((SSL_ERROR_MESSAGE *) message)->popup = &popup;
		return EACCES;

	}
	else
	{
		switch(rv)
		{
		case X509_V_OK:
			trace_ssl(hSession,"TLS/SSL negotiated connection complete. Peer certificate %s presented.\n", peer ? "was" : "was not");
			set_ssl_state(hSession,LIB3270_SSL_SECURE);
			break;

		case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:

			trace_ssl(hSession,"TLS/SSL negotiated connection complete with self signed certificate in certificate chain (rc=%d)\n",rv);

			set_ssl_state(hSession,LIB3270_SSL_NEGOTIATED);

#ifdef SSL_ENABLE_SELF_SIGNED_CERT_CHECK
			static const LIB3270_POPUP popup = {
				.name = "SelfSignedCert",
				.type = LIB3270_NOTIFY_SECURE,
				.summary = N_( "The SSL certificate for this host is not trusted." ),
				.body = N_( "The security certificate presented by this host was not issued by a trusted certificate authority." )
			}
			((SSL_ERROR_MESSAGE *) message)->popup = &popup;
			return EACCES;
#else
			break;
#endif // SSL_ENABLE_SELF_SIGNED_CERT_CHECK

		default:
			trace_ssl(hSession,"TLS/SSL verify result was %d (%s)\n", rv, msg->body);

			((SSL_ERROR_MESSAGE *) message)->popup = (LIB3270_POPUP *) msg;

			debug("message: %s",((SSL_ERROR_MESSAGE *) message)->popup->summary);
			debug("description: %s",((SSL_ERROR_MESSAGE *) message)->popup->body);

			set_ssl_state(hSession,LIB3270_SSL_NEGOTIATED);

			if(msg->type == LIB3270_NOTIFY_ERROR)
				return EACCES;

		}

	}

	if(lib3270_get_toggle(hSession,LIB3270_TOGGLE_SSL_TRACE))
	{
		char				  buffer[4096];
		int 				  alg_bits		= 0;
		const SSL_CIPHER	* cipher		= SSL_get_current_cipher(hSession->ssl.con);

		trace_ssl(hSession,"TLS/SSL cipher description: %s",SSL_CIPHER_description((SSL_CIPHER *) cipher, buffer, 4095));
		SSL_CIPHER_get_bits(cipher, &alg_bits);
		trace_ssl(hSession,"%s version %s with %d bits\n",
						SSL_CIPHER_get_name(cipher),
						SSL_CIPHER_get_version(cipher),
						alg_bits);
	}

	return 0;
}

int ssl_negotiate(H3270 *hSession)
{
	int rc;
	SSL_ERROR_MESSAGE msg;

	memset(&msg,0,sizeof(msg));

	set_ssl_state(hSession,LIB3270_SSL_NEGOTIATING);
	non_blocking(hSession,False);

	rc = lib3270_run_task(hSession, background_ssl_negotiation, &msg);

	debug("background_ssl_negotiation exits with rc=%d",rc);
	if(rc && msg.popup)
	{
		// SSL Negotiation has failed.
		if(popup_ssl_error(hSession,rc,&msg))
		{
			host_disconnect(hSession,1); // Disconnect with "failed" status.
			return rc;
		}

	} else if(rc) {

		// SSL Negotiation has failed, no popup to present.
		const LIB3270_POPUP popup = {
			.summary = N_("SSL negotiation has failed")
		};

		msg.popup = &popup;
		if(popup_ssl_error(hSession,rc,&msg))
		{
			host_disconnect(hSession,1); // Disconnect with "failed" status.
			return rc;
		}

	}

	// Tell the world that we are (still) connected, now in secure mode.
	lib3270_set_connected_initial(hSession);
	non_blocking(hSession,True);

	return 0;
}


int	ssl_init(H3270 *hSession) {

	int rc;
	SSL_ERROR_MESSAGE msg;

	memset(&msg,0,sizeof(msg));

	non_blocking(hSession,False);

	rc = lib3270_run_task(hSession, background_ssl_init, &msg);
	if(rc)
	{
		// SSL init has failed.
		host_disconnect(hSession,1); // Disconnect with "failed" status.

		if(msg.popup)
		{
			ssl_popup_message(hSession,&msg);
		}
		else
		{
			LIB3270_POPUP popup = {
				.summary = N_("Unexpected error on SSL initialization")
			};

			lib3270_autoptr(char) body = lib3270_strdup_printf("%s (rc=%d)",strerror(rc),rc);
			popup.body = body;

			msg.popup = &popup;
			ssl_popup_message(hSession,&msg);
			msg.popup = NULL;

		}


	}

	non_blocking(hSession,True);

	return rc;

}

#endif /*]*/

