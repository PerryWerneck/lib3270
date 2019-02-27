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

#if defined(HAVE_LIBSSL)
	#include <openssl/ssl.h>
	#include <openssl/err.h>
	#include <openssl/x509_vfy.h>

	#ifndef SSL_ST_OK
		#define SSL_ST_OK 3
	#endif // !SSL_ST_OK

#endif

#include "../private.h"
#include <errno.h>
#include <lib3270.h>
#include <lib3270/internals.h>
#include <lib3270/trace.h>
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
		((SSL_ERROR_MESSAGE *) message)->title = N_( "Security error" );
		((SSL_ERROR_MESSAGE *) message)->text = N_( "Cant create a new SSL structure for current connection." );
		return -1;
	}

	SSL_set_ex_data(hSession->ssl.con,ssl_3270_ex_index,(char *) hSession);
//	SSL_set_verify(session->ssl_con, SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
	SSL_set_verify(hSession->ssl.con, 0, NULL);

	return 0;
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
	if(SSL_set_fd(hSession->ssl.con, hSession->sock) != 1)
	{
		trace_ssl(hSession,"%s","SSL_set_fd failed!\n");

		((SSL_ERROR_MESSAGE *) message)->title = N_( "Security error" );
		((SSL_ERROR_MESSAGE *) message)->text = N_( "SSL negotiation failed" );
		((SSL_ERROR_MESSAGE *) message)->description = N_( "Cant set the file descriptor for the input/output facility for the TLS/SSL (encrypted) side of ssl." );

		return -1;
	}

	trace_ssl(hSession, "%s","Running SSL_connect\n");
	rv = SSL_connect(hSession->ssl.con);
	trace_ssl(hSession, "SSL_connect exits with rc=%d\n",rv);

	if (rv != 1)
	{
		const char	* msg 		= "";

		((SSL_ERROR_MESSAGE *) message)->error = SSL_get_error(hSession->ssl.con,rv);
		if(((SSL_ERROR_MESSAGE *) message)->error == SSL_ERROR_SYSCALL && hSession->ssl.error)
			((SSL_ERROR_MESSAGE *) message)->error = hSession->ssl.error;

		msg = ERR_lib_error_string(((SSL_ERROR_MESSAGE *) message)->error);

		trace_ssl(hSession,"SSL_connect failed: %s %s\n",msg,ERR_reason_error_string(hSession->ssl.error));

		((SSL_ERROR_MESSAGE *) message)->title = N_( "Security error" );
		((SSL_ERROR_MESSAGE *) message)->text = N_( "SSL Connect failed" );

		return -1;

	}

	// Success.
	X509 * peer = NULL;
	rv = SSL_get_verify_result(hSession->ssl.con);

	switch(rv)
	{
	// https://www.openssl.org/docs/man1.0.2/crypto/X509_STORE_CTX_set_error.html
	case X509_V_OK:
		peer = SSL_get_peer_certificate(hSession->ssl.con);
		trace_ssl(hSession,"TLS/SSL negotiated connection complete. Peer certificate %s presented.\n", peer ? "was" : "was not");
		break;

	case X509_V_ERR_UNABLE_TO_GET_CRL:

		trace_ssl(hSession,"%s","The CRL of a certificate could not be found.\n" );
		((SSL_ERROR_MESSAGE *) message)->title = _( "SSL error" );
		((SSL_ERROR_MESSAGE *) message)->text = _( "Unable to get certificate CRL." );
		((SSL_ERROR_MESSAGE *) message)->description = _( "The Certificate revocation list (CRL) of a certificate could not be found." );

		return -1;

	case X509_V_ERR_CRL_NOT_YET_VALID:
		trace_ssl(hSession,"%s","The CRL of a certificate is not yet valid.\n" );

		((SSL_ERROR_MESSAGE *) message)->title = _( "SSL error" );
		((SSL_ERROR_MESSAGE *) message)->text = _( "The CRL is not yet valid." );
		((SSL_ERROR_MESSAGE *) message)->description = _( "The Certificate revocation list (CRL) is not yet valid." );
		return -1;

	case X509_V_ERR_CRL_HAS_EXPIRED:
		trace_ssl(hSession,"%s","The CRL of a certificate has expired.\n" );

#ifdef SSL_ENABLE_CRL_EXPIRATION_CHECK
		((SSL_ERROR_MESSAGE *) message)->title = _( "SSL error" );
		((SSL_ERROR_MESSAGE *) message)->text = _( "The CRL has expired." );
		((SSL_ERROR_MESSAGE *) message)->description = _( "The Certificate revocation list (CRL) has expired." );
		return -1;
#else
		break;
#endif // SSL_ENABLE_CRL_EXPIRATION_CHECK

	case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:

		peer = SSL_get_peer_certificate(hSession->ssl.con);

		debug("%s","TLS/SSL negotiated connection complete with self signed certificate in certificate chain" );
		trace_ssl(hSession,"%s","TLS/SSL negotiated connection complete with self signed certificate in certificate chain\n" );

#ifdef SSL_ENABLE_SELF_SIGNED_CERT_CHECK
		((SSL_ERROR_MESSAGE *) message)->title = _( "SSL error" );
		((SSL_ERROR_MESSAGE *) message)->text = _( "The SSL certificate for this host is not trusted." );
		((SSL_ERROR_MESSAGE *) message)->description = _( "The security certificate presented by this host was not issued by a trusted certificate authority." );
		return -1;
#else
		break;
#endif // SSL_ENABLE_SELF_SIGNED_CERT_CHECK

	default:

		trace_ssl(hSession,"Unexpected or invalid TLS/SSL verify result %d\n",rv);

#ifdef SSL_ENABLE_CRL_EXPIRATION_CHECK
		((SSL_ERROR_MESSAGE *) message)->title = _( "SSL error" );
		((SSL_ERROR_MESSAGE *) message)->text = _( "Can't verify." );
		((SSL_ERROR_MESSAGE *) message)->description = _( "Unexpected or invalid TLS/SSL verify result" );
		return -1;
#endif // SSL_ENABLE_CRL_EXPIRATION_CHECK

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

		set_ssl_state(hSession,LIB3270_SSL_SECURE);
		X509_free(peer);
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
	if(rc)
	{
		// SSL Negotiation has failed.
		host_disconnect(hSession,1); // Disconnect with "failed" status.

		if(msg.description)
			lib3270_popup_dialog(hSession, LIB3270_NOTIFY_ERROR, msg.title, msg.text, "%s", msg.description);
		else
			lib3270_popup_dialog(hSession, LIB3270_NOTIFY_ERROR, msg.title, msg.text, "%s", ERR_reason_error_string(msg.error));


	}
	else
	{
		/* Tell the world that we are (still) connected, now in secure mode. */
		lib3270_set_connected_initial(hSession);
	}

	non_blocking(hSession,True);

	return rc;
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

#ifdef DEBUG
	if(hSession != lib3270_get_default_session_handle())
	{
		trace("%s: hsession=%p, session=%p",__FUNCTION__,hSession,lib3270_get_default_session_handle());
		exit(-1);
	}
#endif // DEBUG

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

