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
#include "../private.h"
#include <errno.h>
#include <lib3270.h>
#include <lib3270/internals.h>
#include <lib3270/trace.h>
#include <trace_dsc.h>

/*--[ Implement ]------------------------------------------------------------------------------------*/

LIB3270_EXPORT int lib3270_is_secure(H3270 *hSession)
{
	return lib3270_get_secure(hSession) == LIB3270_SSL_SECURE;
}

LIB3270_EXPORT long lib3270_get_SSL_verify_result(H3270 *hSession)
{
	CHECK_SESSION_HANDLE(hSession);
#if defined(HAVE_LIBSSL)
	if(hSession->ssl.con)
		return SSL_get_verify_result(hSession->ssl.con);
#endif // HAVE_LIBSSL
	return -1;
}

LIB3270_EXPORT LIB3270_SSL_STATE lib3270_get_secure(H3270 *hSession)
{
	CHECK_SESSION_HANDLE(hSession);
	return hSession->ssl.state;
}

void set_ssl_state(H3270 *hSession, LIB3270_SSL_STATE state)
{
	CHECK_SESSION_HANDLE(hSession);

	debug("%s: %d -> %d",__FUNCTION__,hSession->ssl.state,state);

	if(state == hSession->ssl.state)
		return;

	hSession->ssl.state = state;
	trace_dsn(hSession,"SSL state changes to %d\n",(int) state);

	hSession->cbk.update_ssl(hSession,hSession->ssl.state);
}
