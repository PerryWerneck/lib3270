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

#ifndef LIB3270_OPENSSL_MODULE_PRIVATE_H_INCLUDED

	#define LIB3270_OPENSSL_MODULE_PRIVATE_H_INCLUDED

	#include <config.h>

	#ifdef _WIN32
		#include <winsock.h>
		#include <windows.h>
	#else
		#include <unistd.h>
		#include <fcntl.h>
	#endif // _WIN32

	#include <lib3270.h>
 	#include <lib3270/log.h>
 	#include <internals.h>

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

	LIB3270_INTERNAL void	* lib3270_openssl_get_context(H3270 *hSession, LIB3270_NETWORK_STATE *state);
	LIB3270_INTERNAL int	  lib3270_openssl_get_ex_index(H3270 *hSession);

#endif // !LIB3270_OPENSSL_MODULE_PRIVATE_H_INCLUDED
