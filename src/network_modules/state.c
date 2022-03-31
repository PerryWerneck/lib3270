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
 */

#include <config.h>
#include <internals.h>
#include <errno.h>
#include <lib3270.h>
#include <lib3270/popup.h>
#include <lib3270/trace.h>
#include <trace_dsc.h>
#include <lib3270/log.h>
#include <lib3270/trace.h>
#include <networking.h>

#ifdef HAVE_LIBSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif // HAVE_LIBSSL

/*--[ Implement ]------------------------------------------------------------------------------------*/

LIB3270_EXPORT int lib3270_is_secure(const H3270 *hSession) {
	return lib3270_get_ssl_state(hSession) == LIB3270_SSL_SECURE;
}

LIB3270_EXPORT LIB3270_SSL_STATE lib3270_get_ssl_state(const H3270 *hSession) {
	return hSession->ssl.state;
}

void set_ssl_state(H3270 *hSession, LIB3270_SSL_STATE state) {
	if(state == hSession->ssl.state)
		return;

	hSession->ssl.state = state;
	trace_dsn(hSession,"SSL state changes to %d\n",(int) state);
	debug("SSL state changes to %d\n",(int) state);

	hSession->cbk.update_ssl(hSession,hSession->ssl.state);
}

LIB3270_EXPORT const char * lib3270_get_ssl_state_message(const H3270 *hSession) {

	if(hSession->ssl.message) {

		if(hSession->ssl.message->summary)
			return dgettext(GETTEXT_PACKAGE,hSession->ssl.message->summary);

		return "";
	}

	return _( "The connection is insecure" );

}

LIB3270_EXPORT const char * lib3270_get_ssl_state_icon_name(const H3270 *hSession) {

	if(hSession->ssl.message && hSession->ssl.message->icon)
		return hSession->ssl.message->icon;

	return "dialog-error";
}

LIB3270_EXPORT const char * lib3270_get_ssl_state_description(const H3270 *hSession) {

	if(hSession->ssl.message && hSession->ssl.message->body) {
		return dgettext(GETTEXT_PACKAGE,hSession->ssl.message->body);
	}

	return "";

}

LIB3270_EXPORT char * lib3270_get_ssl_crl_text(const H3270 *hSession) {

	if(hSession->network.module && hSession->network.module->getcrl)
		return hSession->network.module->getcrl(hSession);

	errno = ENOTSUP;
	return NULL;
}

LIB3270_EXPORT char * lib3270_get_ssl_peer_certificate_text(const H3270 *hSession) {

	if(hSession->network.module && hSession->network.module->getcert)
		return hSession->network.module->getcert(hSession);

	errno = ENOTSUP;
	return NULL;
}
