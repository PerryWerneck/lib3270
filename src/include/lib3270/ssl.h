/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes paul.mattes@case.edu), de emulação de terminal 3270 para acesso a
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
 * Este programa está nomeado como ssl.h e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

/**
 * @brief TN3270 SSL definitions.
 *
 * @author perry.werneck@gmail.com
 *
 */

#ifndef LIB3270_SSL_H_INCLUDED

#define LIB3270_SSL_H_INCLUDED 1

#include <lib3270.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief SSL state
typedef enum lib3270_ssl_state {
	LIB3270_SSL_UNSECURE,			/**< @brief No secure connection */
	LIB3270_SSL_SECURE,				/**< @brief Connection secure with CA check */
	LIB3270_SSL_NEGOTIATED,			/**< @brief Connection secure, no CA, self-signed or expired CRL */
	LIB3270_SSL_NEGOTIATING,		/**< @brief Negotiating SSL */
	LIB3270_SSL_VERIFYING,			/**< @brief Verifying SSL (Getting CRL) */
	LIB3270_SSL_UNDEFINED			/**< @brief Undefined */
} LIB3270_SSL_STATE;

/**
 * @brief Set URL for the certificate revocation list.
 *
 * @param hSession	Session handle.
 * @param crl		URL for the certificate revocation list.
 *
 * @return 0 on sucess, non zero on error (sets errno).
 *
 */
LIB3270_EXPORT int lib3270_crl_set_url(H3270 *hSession, const char *crl);
LIB3270_EXPORT const char * lib3270_crl_get_url(const H3270 *hSession);

LIB3270_EXPORT int lib3270_crl_set_preferred_protocol(H3270 *hSession, const char *protocol);
LIB3270_EXPORT const char * lib3270_crl_get_preferred_protocol(const H3270 *hSession);

/**
 * @brief Get the available protocols for CRL download.
 *
 */
LIB3270_EXPORT const char ** lib3270_get_available_crl_protocols(void);

/**
 * @brief Get SSL host option.
 *
 * @return Non zero if the host URL has SSL scheme.
 *
 */
LIB3270_EXPORT int lib3270_get_secure_host(const H3270 *hSession);

/**
 * @brief Get security state.
 *
 */
LIB3270_EXPORT LIB3270_SSL_STATE lib3270_get_ssl_state(const H3270 *session);

/**
 * @brief Get security state as text.
 *
 */
LIB3270_EXPORT const char * lib3270_get_ssl_state_message(const H3270 *hSession);

LIB3270_EXPORT const char * lib3270_get_ssl_state_icon_name(const H3270 *hSession);

/**
 * @brief Get security state message.
 *
 */
LIB3270_EXPORT const char * lib3270_get_ssl_state_description(const H3270 *hSession);

LIB3270_EXPORT char * lib3270_get_ssl_crl_text(const H3270 *hSession);
LIB3270_EXPORT char * lib3270_get_ssl_peer_certificate_text(const H3270 *hSession);

/**
 * @brief Disable automatic download of the CRL.
 *
 * @param hSession	Session handle.
 * @param Value		Non zero to enable automatic download of CRL.
 *
 * @return 0 if ok or error code if not (Sets errno).
 *
 * @retval	0 		Success, the property was set.
 * @retval	ENOTSUP	No SSL/TLS support.
 */
LIB3270_EXPORT int lib3270_ssl_set_crl_download(H3270 *hSession, int enabled);

LIB3270_EXPORT int lib3270_ssl_get_crl_download(const H3270 *hSession);


#ifdef __cplusplus
}
#endif

#endif // LIB3270_SSL_H_INCLUDED
