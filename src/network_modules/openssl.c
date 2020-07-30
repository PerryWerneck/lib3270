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
  * @brief OpenSSL based networking methods.
  *
  */

 #include "private.h"

 #include <sys/types.h>
 #include <sys/socket.h>
 #include <netdb.h>
 #include <openssl/ssl.h>
 #include <openssl/x509.h>

 struct _lib3270_net_context {

	int sock;						///< @brief Session socket.

	SSL * con;						///< @brief SSL Connection handle.

	struct {
		char			  download;	///< @brief Non zero to download CRL.
		char			* prefer;	///< @brief Prefered protocol for CRL.
		char			* url;		///< @brief URL for CRL download.
		X509_CRL 		* cert;		///< @brief Loaded CRL (can be null).
	} crl;

  };

static void crl_free(LIB3270_NET_CONTEXT *context) {
	if(context->crl.cert) {
		X509_CRL_free(context->crl.cert);
		context->crl.cert = NULL;
	}
}

static void openssl_network_finalize(H3270 *hSession) {

	debug("%s",__FUNCTION__);


	if(hSession->network.context) {

		// Cleanupp
		LIB3270_NET_CONTEXT *context = hSession->network.context;

		crl_free(context);

		// Release network context.
		lib3270_free(hSession->network.context);
		hSession->network.context = NULL;
	}

}

static int openssl_network_disconnect(H3270 *hSession) {


}

ssize_t openssl_network_send(H3270 *hSession, const void *buffer, size_t length) {

}

static ssize_t openssl_network_recv(H3270 *hSession, void *buf, size_t len) {

}

static int openssl_network_getsockname(const H3270 *hSession, struct sockaddr *addr, socklen_t *addrlen) {

}

static void * openssl_network_add_poll(H3270 *hSession, LIB3270_IO_FLAG flag, void(*call)(H3270 *, int, LIB3270_IO_FLAG, void *), void *userdata) {

}

static int openssl_network_non_blocking(H3270 *hSession, const unsigned char on) {

}

static int openssl_network_is_connected(H3270 *hSession) {

}

static int openssl_network_setsockopt(H3270 *hSession, int level, int optname, const void *optval, size_t optlen) {

}

static int openssl_network_getsockopt(H3270 *hSession, int level, int optname, void *optval, socklen_t *optlen) {
}

static int openssl_network_connect(H3270 *hSession, LIB3270_NETWORK_STATE *state) {

	set_ssl_state(hSession,LIB3270_SSL_UNDEFINED);

	SSL_CTX * ctx_context = (SSL_CTX *) lib3270_get_openssl_context(state,state);
	if(!ctx_context)
		return -1;

	//
	// Prepare for connection
	//
	LIB3270_NET_CONTEXT *context = hSession->network.context;

	if(context->crl.cert) {

		// Release CRL if expired.
		// https://stackoverflow.com/questions/23407376/testing-x509-certificate-expiry-date-with-c
		// X509_CRL_get_nextUpdate is deprecated in openssl 1.1.0

		#if OPENSSL_VERSION_NUMBER < 0x10100000L
			const ASN1_TIME * next_update = X509_CRL_get_nextUpdate(context->crl.cert);
		#else
			const ASN1_TIME * next_update = X509_CRL_get0_nextUpdate(context->crl.cert);
		#endif

		if(X509_cmp_current_time(next_update) == 1)
		{
			int day, sec;
			if(ASN1_TIME_diff(&day, &sec, NULL, next_update))
			{
				trace_ssl(hSession,"CRL is valid for %d day(s) and %d second(s)\n",day,sec);
			}
			else
			{
				trace_ssl(hSession,"Can't get CRL next update, discarding it\n");
				crl_free(context);
			}

		}
		else
		{
			trace_ssl(hSession,"CRL is no longer valid\n");
			crl_free(context);
		}

	}

	//
	// Enable SSL & Connect to host.
	//
	hSession->ssl.host = 1;
	context->sock = lib3270_network_connect(hSession, state);

	return (context->sock < 0 ? -1 : 0);

}

static int openssl_network_start_tls(H3270 *hSession, LIB3270_NETWORK_STATE *msg) {

	LIB3270_NET_CONTEXT * context = hSession->network.context;


}

void lib3270_set_openssl_network_module(H3270 *hSession) {

	static const LIB3270_NET_MODULE module = {
		.finalize = openssl_network_finalize,
		.connect = openssl_network_connect,
		.disconnect = openssl_network_disconnect,
		.start_tls = openssl_network_start_tls,
		.send = openssl_network_send,
		.recv = openssl_network_recv,
		.add_poll = openssl_network_add_poll,
		.non_blocking = openssl_network_non_blocking,
		.is_connected = openssl_network_is_connected,
		.getsockname = openssl_network_getsockname,
		.setsockopt = openssl_network_setsockopt,
		.getsockopt = openssl_network_getsockopt
	};

 	debug("%s",__FUNCTION__);

	if(hSession->network.context) {
		// Has context, finalize it.
		hSession->network.module->finalize(hSession);
	}

	hSession->ssl.host = 1;
	hSession->network.context = lib3270_malloc(sizeof(LIB3270_NET_CONTEXT));
	memset(hSession->network.context,0,sizeof(LIB3270_NET_CONTEXT));



	hSession->network.module = &module;
}
