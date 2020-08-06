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


static void openssl_network_finalize(H3270 *hSession) {

	debug("%s",__FUNCTION__);

	if(hSession->network.context) {

		// Cleanupp
		LIB3270_NET_CONTEXT *context = hSession->network.context;

		lib3270_openssl_crl_free(context);

		// Release network context.
		lib3270_free(hSession->network.context);
		hSession->network.context = NULL;
	}

}

static int openssl_network_disconnect(H3270 *hSession) {

	LIB3270_NET_CONTEXT * context = hSession->network.context;

	if(context->con) {
		SSL_shutdown(context->con);
		SSL_free(context->con);
		context->con = NULL;
	}

	if(context->sock > 0) {
		shutdown(context->sock, 2);
#ifdef _WIN32
		sockclose(context->sock);
#else
		close(context->sock);
#endif // _WIN32
		context->sock = -1;
	}

	return 0;

}

ssize_t openssl_network_send(H3270 *hSession, const void *buffer, size_t length) {

	int rc = SSL_write(hSession->network.context->con, (const char *) buffer, length);
	if(rc > 0)
		return rc;

	// https://www.openssl.org/docs/man1.0.2/man3/SSL_get_error.html
	int ssl_error = SSL_get_error(hSession->network.context->con, rc);
	switch(ssl_error) {
	case SSL_ERROR_ZERO_RETURN:

		trace_ssl(hSession,"%s","The secure connection has been closed cleanly");

		lib3270_popup_dialog(
			hSession,
			LIB3270_NOTIFY_ERROR,
			NULL,
			_("Disconnected from host"),
			"%s",
			_("The secure connection has been closed cleanly.")
		);
		return 0;

	case SSL_ERROR_WANT_READ:
	case SSL_ERROR_WANT_X509_LOOKUP:
		return -EWOULDBLOCK;	// Force a new loop.

	case SSL_ERROR_SYSCALL:
		return lib3270_socket_send_failed(hSession);

	}

	// Build error message.
	char err_buf[120];
	(void) ERR_error_string(ssl_error, err_buf);
	trace_dsn(hSession,"RCVD SSL_write error %d (%s)\n", ssl_error, err_buf);

	lib3270_autoptr(char) body = lib3270_strdup_printf(_("The SSL error message was %s"), err_buf);

	LIB3270_POPUP popup = {
		.summary = _("Error writing to host"),
		.body = body
	};

	lib3270_popup(hSession,&popup,0);

	return -1;

}

static ssize_t openssl_network_recv(H3270 *hSession, void *buf, size_t len) {

	int rc = SSL_read(hSession->network.context->con, (char *) buf, len);
	if(rc > 0) {
		return rc;
	}

	// https://www.openssl.org/docs/man1.0.2/man3/SSL_get_error.html
	int ssl_error = SSL_get_error(hSession->network.context->con, rc);
	switch(ssl_error) {
	case SSL_ERROR_ZERO_RETURN:

		trace_ssl(hSession,"%s","The secure connection has been closed cleanly");

		lib3270_popup_dialog(
			hSession,
			LIB3270_NOTIFY_ERROR,
			NULL,
			_("Disconnected from host"),
			"%s",
			_("The secure connection has been closed cleanly.")
		);
		return 0;

	case SSL_ERROR_WANT_READ:
	case SSL_ERROR_WANT_X509_LOOKUP:
		return -EWOULDBLOCK;	// Force a new loop.

	case SSL_ERROR_SYSCALL:
		return lib3270_socket_recv_failed(hSession);

	}

	// Build error message.
	char err_buf[120];
	(void) ERR_error_string(ssl_error, err_buf);
	trace_dsn(hSession,"RCVD SSL_read error %d (%s)\n", ssl_error, err_buf);

	lib3270_autoptr(char) body = lib3270_strdup_printf(_("The SSL error message was %s"), err_buf);

	LIB3270_POPUP popup = {
		.summary = _("Error reading from host"),
		.body = body
	};

	lib3270_popup(hSession,&popup,0);

	return -1;
}

static int openssl_network_getsockname(const H3270 *hSession, struct sockaddr *addr, socklen_t *addrlen) {
	return getsockname(hSession->network.context->sock, addr, addrlen);
}

static void * openssl_network_add_poll(H3270 *hSession, LIB3270_IO_FLAG flag, void(*call)(H3270 *, int, LIB3270_IO_FLAG, void *), void *userdata) {
	return lib3270_add_poll_fd(hSession,hSession->network.context->sock,flag,call,userdata);
}

static int openssl_network_non_blocking(H3270 *hSession, const unsigned char on) {
	return lib3270_socket_set_non_blocking(hSession, hSession->network.context->sock, on);
}

static int openssl_network_is_connected(const H3270 *hSession) {
	return hSession->network.context->sock > 0;
}

static int openssl_network_setsockopt(H3270 *hSession, int level, int optname, const void *optval, size_t optlen) {
	debug("%s(%d)",__FUNCTION__,hSession->network.context->sock);
	return setsockopt(hSession->network.context->sock, level, optname, optval, optlen);
}

static int openssl_network_getsockopt(H3270 *hSession, int level, int optname, void *optval, socklen_t *optlen) {
	debug("%s(%d)",__FUNCTION__,hSession->network.context->sock);
	return getsockopt(hSession->network.context->sock, level, optname, optval, optlen);
}

static char * openssl_network_getcert(const H3270 *hSession) {

	LIB3270_NET_CONTEXT * context = hSession->network.context;

	if(context && context->con) {
		lib3270_autoptr(X509) peer = SSL_get_peer_certificate(context->con);

		if(peer) {

			lib3270_autoptr(BIO)	  out = BIO_new(BIO_s_mem());
			unsigned char			* data;
			unsigned char			* text;
			int						  n;

			X509_print(out,peer);

			n		= BIO_get_mem_data(out, &data);
			text	= (unsigned char *) lib3270_malloc(n+1);
			memcpy(text,data,n);
			text[n]	='\0';

			return (char *) text;
		}
	}

	errno = ENOTCONN;
	return NULL;
}

static char * openssl_network_getcrl(const H3270 *hSession) {

	LIB3270_NET_CONTEXT * context = hSession->network.context;

	if(context->crl.cert) {

		lib3270_autoptr(BIO)	  out = BIO_new(BIO_s_mem());
		unsigned char			* data;
		unsigned char			* text;
		int						  n;

		X509_print(out,context->crl.cert);

		n		= BIO_get_mem_data(out, &data);
		text	= (unsigned char *) lib3270_malloc(n+1);
		memcpy(text,data,n);
		text[n]	='\0';

		return (char *) text;

	}

	errno = ENOENT;
	return NULL;

}

static int openssl_network_init(H3270 *hSession) {

	set_ssl_state(hSession,LIB3270_SSL_UNDEFINED);

	SSL_CTX * ctx_context = (SSL_CTX *) lib3270_openssl_get_context(hSession);
	if(!ctx_context)
		return -1;

	return 0;
}

static int openssl_network_connect(H3270 *hSession, LIB3270_NETWORK_STATE *state) {

	LIB3270_NET_CONTEXT * context = hSession->network.context;

	if(context->crl.cert) {

		// Has CRL, release if expired.
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
				lib3270_openssl_crl_free(context);
			}

		}
		else
		{
			trace_ssl(hSession,"CRL is no longer valid\n");
			lib3270_openssl_crl_free(context);
		}

	}

	//
	// Enable SSL & Connect to host.
	//
	set_ssl_state(hSession,LIB3270_SSL_UNDEFINED);

	hSession->ssl.host = 1;
	context->sock = lib3270_network_connect(hSession, state);

	debug("%s: sock=%d",__FUNCTION__,context->sock);

	return (context->sock < 0 ? -1 : 0);

}

void lib3270_set_libssl_network_module(H3270 *hSession) {

	static const LIB3270_NET_MODULE module = {
		.name = "tn3270s",
		.service = "tn3270s",
		.init = openssl_network_init,
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
		.getsockopt = openssl_network_getsockopt,
		.getcert = openssl_network_getcert,
		.getcrl	= openssl_network_getcrl
	};

 	debug("%s",__FUNCTION__);

	if(hSession->network.context) {
		// Has context, finalize it.
		hSession->network.module->finalize(hSession);
	}

	hSession->ssl.host = 1;
	hSession->network.context = lib3270_malloc(sizeof(LIB3270_NET_CONTEXT));
	memset(hSession->network.context,0,sizeof(LIB3270_NET_CONTEXT));

	hSession->network.context->sock = -1;

	hSession->network.module = &module;
}

int lib3270_activate_ssl_network_module(H3270 *hSession, int sock) {

	lib3270_set_libssl_network_module(hSession);

	int rc = openssl_network_init(hSession);

	hSession->network.context->sock = sock;

	return rc;

}

void lib3270_openssl_crl_free(LIB3270_NET_CONTEXT *context) {
	if(context->crl.cert) {
		X509_CRL_free(context->crl.cert);
		context->crl.cert = NULL;
	}
}



