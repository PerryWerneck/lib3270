/*
 * "Software G3270, desenvolvido com base nos códigos fontes do WC3270  e  X3270
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
 * Este programa está nomeado como private.h e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas de Mendonça)
 *
 */

#ifdef WIN32
	#include <winsock2.h>
	#include <windows.h>
#endif // WIN32

#include <config.h>				/* autoconf settings */
#include <lib3270.h>			/* lib3270 API calls and defs */

#if defined(HAVE_LIBSSLx)

	#include <openssl/ssl.h>
	#include <openssl/err.h>

	/**
	 * @brief X509 auto-cleanup.
	 */
	static inline void lib3270_autoptr_cleanup_X509(X509 **ptr)
	{
		if(*ptr)
			X509_free(*ptr);
	}

	/**
	 * @brief Dist points auto-cleanup.
	 */
	static inline void lib3270_autoptr_cleanup_CRL_DIST_POINTS(CRL_DIST_POINTS **ptr)
	{
		if(*ptr)
			CRL_DIST_POINTS_free(*ptr);
	}


#endif // HAVE_LIBSSL

#if defined(SSL_ENABLE_CRL_CHECK) && defined(HAVE_LIBSSLx)

	/// @brief Unconditional release of the session CRL.
	LIB3270_INTERNAL void lib3270_crl_free(H3270 *hSession);

	/// @brief Load CRL from URL.
	LIB3270_INTERNAL int lib3270_crl_new_from_url(H3270 *hSession, void *ssl_error, const char *url);

	/// @brief Load CRL from X509 certificate.
	LIB3270_INTERNAL int lib3270_crl_new_from_x509(H3270 *hSession, void *ssl_error, X509 *cert);

	/// @brief Load CRL from distribution points.
	LIB3270_INTERNAL int lib3270_crl_new_from_dist_points(H3270 *hSession, void *ssl_error, CRL_DIST_POINTS * dist_points);

	LIB3270_INTERNAL X509_CRL * lib3270_download_crl(H3270 *hSession, SSL_ERROR_MESSAGE * message, const char *url);


#endif // SSL_ENABLE_CRL_CHECK && HAVE_LIBSSL



