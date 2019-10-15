/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270. Registro no INPI sob o nome G3270.
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
 */

#include <lib3270-internals.h>
#include <lib3270/properties.h>

#if defined(HAVE_LIBSSL)
	#include <openssl/ssl.h>
#endif


/**
 * @brief Get SSL host option.
 *
 * @return Non zero if the host URL has SSL scheme.
 *
 */
LIB3270_EXPORT int lib3270_get_secure_host(const H3270 *hSession)
{
#ifdef HAVE_LIBSSL
	return hSession->ssl.enabled ? 1 : 0;
#else
	return 0;
#endif // HAVE_LIBSSL

}

#ifdef SSL_ENABLE_CRL_CHECK
LIB3270_EXPORT char * lib3270_get_ssl_crl_text(const H3270 *hSession)
{

	if(hSession->ssl.crl.cert)
	{

		BIO				* out = BIO_new(BIO_s_mem());
		unsigned char	* data;
		unsigned char	* text;
		int				  n;

		X509_CRL_print(out,hSession->ssl.crl.cert);

		n		= BIO_get_mem_data(out, &data);
		text	= (unsigned char *) lib3270_malloc(n+1);
		text[n]	='\0';

		memcpy(text,data,n);
		BIO_free(out);

		return (char *) text;

	}

	return NULL;

}
#else
LIB3270_EXPORT char * lib3270_get_ssl_crl_text(const H3270 GNUC_UNUSED(*hSession))
{
	return NULL;
}
#endif // SSL_ENABLE_CRL_CHECK


LIB3270_EXPORT char * lib3270_get_ssl_peer_certificate_text(const H3270 *hSession)
{
#ifdef HAVE_LIBSSL
	if(hSession->ssl.con)
	{
		X509 * peer = SSL_get_peer_certificate(hSession->ssl.con);
		if(peer)
		{
			BIO				* out	= BIO_new(BIO_s_mem());
			unsigned char	* data;
			unsigned char	* text;
			int				  n;

			X509_print(out,peer);

			n		= BIO_get_mem_data(out, &data);
			text	= (unsigned char *) lib3270_malloc(n+1);
			text[n]	='\0';
			memcpy(text,data,n);
			BIO_free(out);

			return (char *) text;
		}
	}
#endif // HAVE_LIBSSL

	return NULL;
}

 #pragma GCC diagnostic push
 #pragma GCC diagnostic ignored "-Wunused-parameter"
 const char * lib3270_get_crl_url(const H3270 *hSession)
 {
#ifdef SSL_ENABLE_CRL_CHECK
	if(hSession->ssl.crl.url)
		return hSession->ssl.crl.url;

#ifdef SSL_DEFAULT_CRL_URL
	return SSL_DEFAULT_CRL_URL;
#else
	return getenv("LIB3270_DEFAULT_CRL");
#endif // SSL_DEFAULT_CRL_URL

#else
	errno = ENOTSUP;
	return "";
#endif
 }
 #pragma GCC diagnostic pop

 #pragma GCC diagnostic push
 #pragma GCC diagnostic ignored "-Wunused-parameter"
 int lib3270_set_crl_url(H3270 *hSession, const char *crl)
 {

    FAIL_IF_ONLINE(hSession);

#ifdef SSL_ENABLE_CRL_CHECK

	if(hSession->ssl.crl.url)
	{
		free(hSession->ssl.crl.url);
		hSession->ssl.crl.url = NULL;
	}

	if(hSession->ssl.crl.cert)
	{
		X509_CRL_free(hSession->ssl.crl.cert);
		hSession->ssl.crl.cert = NULL;
	}

	if(crl)
	{
		hSession->ssl.crl.url = strdup(crl);
	}

	return 0;

#else

	return errno = ENOTSUP;

#endif // SSL_ENABLE_CRL_CHECK

 }
 #pragma GCC diagnostic pop

 #pragma GCC diagnostic push
 #pragma GCC diagnostic ignored "-Wunused-parameter"
 const char * lib3270_get_crl_prefered_protocol(const H3270 *hSession)
 {
#ifdef SSL_ENABLE_CRL_CHECK
	if(hSession->ssl.crl.prefer)
		return hSession->ssl.crl.prefer;
#endif
	errno = ENODATA;
	return "";
 }
 #pragma GCC diagnostic pop

 #pragma GCC diagnostic push
 #pragma GCC diagnostic ignored "-Wunused-parameter"
 int lib3270_set_crl_prefered_protocol(H3270 *hSession, const char *protocol)
 {

    FAIL_IF_ONLINE(hSession);

#ifdef SSL_ENABLE_CRL_CHECK

	if(hSession->ssl.crl.prefer)
	{
		lib3270_free(hSession->ssl.crl.prefer);
		hSession->ssl.crl.prefer = NULL;
	}

	if(protocol)
	{
		hSession->ssl.crl.prefer = strdup(protocol);
	}

	return 0;

#else

	return errno = ENOTSUP;

#endif // SSL_ENABLE_CRL_CHECK

 }
 #pragma GCC diagnostic pop
