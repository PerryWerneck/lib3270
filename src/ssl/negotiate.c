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
		((SSL_ERROR_MESSAGE *) message)->error = hSession->ssl.error = ERR_get_error();
		((SSL_ERROR_MESSAGE *) message)->title = _( "Security error" );
		((SSL_ERROR_MESSAGE *) message)->text = _( "Cant create a new SSL structure for current connection." );
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
		.description = "SSLv3"
	},
	{
		.id = TLS1_VERSION,
		.description = "TLSv1"
	},
	{
		.id = TLS1_1_VERSION,
		.description = "TLSv1.1"
	},
	{
		.id = TLS1_2_VERSION,
		.description = "TLSv1.2"
	},
	{
		.id = DTLS1_VERSION,
		.description = "DTLSv1"
	},
	{
		.id = DTLS1_2_VERSION,
		.description = "DTLSv2"
	}

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
		trace_ssl(hSession,"Minimum protocol version set to %s\n",protocol->description);
		SSL_set_min_proto_version(hSession->ssl.con,protocol->id);
#else
		trace_ssl(hSession,"Can't set minimum protocol version to %s\n",protocol->description);
#endif // OPENSSL_VERSION_NUMBER
	}

	if( (protocol = get_protocol_from_id(hSession->ssl.protocol.max_version)) != NULL )
	{
#if (OPENSSL_VERSION_NUMBER >= 0x1010009fL)
		trace_ssl(hSession,"Maximum protocol version set to %s\n",protocol->description);
		SSL_set_max_proto_version(hSession->ssl.con,protocol->id);
#else
		trace_ssl(hSession,"Can't set maximum protocol version to %s\n",protocol->description);
#endif // OPENSSL_VERSION_NUMBER
	}

	if(SSL_set_fd(hSession->ssl.con, hSession->connection.sock) != 1)
	{
		trace_ssl(hSession,"%s","SSL_set_fd failed!\n");

		((SSL_ERROR_MESSAGE *) message)->title = _( "Security error" );
		((SSL_ERROR_MESSAGE *) message)->text = _( "SSL negotiation failed" );
		((SSL_ERROR_MESSAGE *) message)->description = _( "Cant set the file descriptor for the input/output facility for the TLS/SSL (encrypted) side of ssl." );

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
		const char * msg = "";

		((SSL_ERROR_MESSAGE *) message)->error = SSL_get_error(hSession->ssl.con,rv);
		if(((SSL_ERROR_MESSAGE *) message)->error == SSL_ERROR_SYSCALL && hSession->ssl.error)
			((SSL_ERROR_MESSAGE *) message)->error = hSession->ssl.error;

		msg = ERR_lib_error_string(((SSL_ERROR_MESSAGE *) message)->error);

		trace_ssl(hSession,"SSL_connect failed: %s %s\n",msg,ERR_reason_error_string(hSession->ssl.error));

		((SSL_ERROR_MESSAGE *) message)->title = _( "Security error" );
		((SSL_ERROR_MESSAGE *) message)->text = _( "SSL Connect failed" );

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

		((SSL_ERROR_MESSAGE *) message)->title = _( "Security error" );
		((SSL_ERROR_MESSAGE *) message)->text = _( "Can't verify." );
		((SSL_ERROR_MESSAGE *) message)->description = _( "Unexpected or invalid TLS/SSL verify result" );
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
			((SSL_ERROR_MESSAGE *) message)->title = _( "Security error" );
			((SSL_ERROR_MESSAGE *) message)->text = _( "The SSL certificate for this host is not trusted." );
			((SSL_ERROR_MESSAGE *) message)->description = _( "The security certificate presented by this host was not issued by a trusted certificate authority." );
			return EACCES;
#else
			break;
#endif // SSL_ENABLE_SELF_SIGNED_CERT_CHECK

		default:
			trace_ssl(hSession,"TLS/SSL verify result was %d (%s)\n", rv, msg->description);

			debug("message: %s",msg->message);
			debug("description: %s",msg->description);

			((SSL_ERROR_MESSAGE *) message)->text = gettext(msg->message);
			((SSL_ERROR_MESSAGE *) message)->description = gettext(msg->description);

			set_ssl_state(hSession,LIB3270_SSL_NEGOTIATED);

			if(msg->icon == LIB3270_NOTIFY_ERROR)
			{
				((SSL_ERROR_MESSAGE *) message)->title = _( "Security error" );
				return EACCES;
			}

			((SSL_ERROR_MESSAGE *) message)->title = _( "Security warning" );

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

	if(rc == EACCES)
	{
		// SSL validation has failed

		int abort = -1;

		if(msg.description)
			abort = popup_ssl_error(hSession,rc,msg.title,msg.text,msg.description);
		else
			abort = popup_ssl_error(hSession,rc,msg.title,msg.text,ERR_reason_error_string(msg.error));

		if(abort)
		{
			host_disconnect(hSession,1); // Disconnect with "failed" status.
			return rc;
		}

	}
	else if(rc)
	{
		// SSL Negotiation has failed.
		host_disconnect(hSession,1); // Disconnect with "failed" status.

		if(msg.description)
			lib3270_popup_dialog(hSession, LIB3270_NOTIFY_ERROR, msg.title, msg.text, "%s", msg.description);
		else
			lib3270_popup_dialog(hSession, LIB3270_NOTIFY_ERROR, msg.title, msg.text, "%s", ERR_reason_error_string(msg.error));

		return rc;

	}

	/* Tell the world that we are (still) connected, now in secure mode. */
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

		if(msg.description)
			lib3270_popup_dialog(hSession, LIB3270_NOTIFY_ERROR, msg.title, msg.text, "%s", msg.description);
		else
			lib3270_popup_dialog(hSession, LIB3270_NOTIFY_ERROR, msg.title, msg.text, "%s", ERR_reason_error_string(msg.error));

	}

	non_blocking(hSession,True);

	return rc;

}


/* Callback for tracing protocol negotiation. */
void ssl_info_callback(INFO_CONST SSL *s, int where, int ret)
{
	H3270 *hSession = (H3270 *) SSL_get_ex_data(s,ssl_3270_ex_index);

	switch(where)
	{
	case SSL_CB_CONNECT_LOOP:
		trace_ssl(hSession,"SSL_connect: %s %s\n",SSL_state_string(s), SSL_state_string_long(s));
		break;

	case SSL_CB_CONNECT_EXIT:

		trace_ssl(hSession,"%s: SSL_CB_CONNECT_EXIT\n",__FUNCTION__);

		if (ret == 0)
		{
			trace_ssl(hSession,"SSL_connect: failed in %s\n",SSL_state_string_long(s));
		}
		else if (ret < 0)
		{
			unsigned long e = ERR_get_error();
			char err_buf[1024];

			if(e != 0)
			{
				hSession->ssl.error = e;
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
		trace_ssl(hSession,"SSL Current state is \"%s\"\n",SSL_state_string_long(s));
	}

#ifdef DEBUG
	if(where & SSL_CB_EXIT)
	{
		trace("%s: SSL_CB_EXIT ret=%d\n",__FUNCTION__,ret);
	}
#endif

	if(where & SSL_CB_ALERT)
		trace_ssl(hSession,"SSL ALERT: %s\n",SSL_alert_type_string_long(ret));

	if(where & SSL_CB_HANDSHAKE_DONE)
	{
		trace_ssl(hSession,"%s: SSL_CB_HANDSHAKE_DONE state=%04x\n",__FUNCTION__,SSL_get_state(s));
		if(SSL_get_state(s) == SSL_ST_OK)
			set_ssl_state(hSession,LIB3270_SSL_NEGOTIATED);
		else
			set_ssl_state(hSession,LIB3270_SSL_UNSECURE);
	}
}

#endif /*]*/

