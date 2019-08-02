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
 * References:
 *
 * https://github.com/curl/curl/blob/curl-7_62_0/lib/ldap.c
 * http://forums.codeguru.com/showthread.php?313123-Elementary-problems-using-winldap
 * https://stackoverflow.com/questions/21501002/how-to-use-ldap-sasl-bind-in-winldap
 *
 */

#include <config.h>

#if defined(HAVE_LIBSSL) && defined(SSL_ENABLE_CRL_CHECK)

#include "private.h"
#include <winldap.h>

# ifndef LDAP_VENDOR_NAME
#  error Your Platform SDK is NOT sufficient for LDAP support! \
         Update your Platform SDK, or disable LDAP support!
# else
#  include <winber.h>
# endif

/*--[ Implement ]------------------------------------------------------------------------------------*/

static inline void lib3270_autoptr_cleanup_LDAP(LDAP **ptr)
{
	if(*ptr)
	{
		ldap_unbind(*ptr);
		*ptr = NULL;
	}

}

X509_CRL * get_crl_using_winldap(H3270 *hSession, SSL_ERROR_MESSAGE * message, const char *consturl)
{
	debug("********************************************************* %s",__FUNCTION__);

	X509_CRL * x509_crl = NULL;
	int rc = 0;

	// Strip query.

	lib3270_autoptr(char) urldup = strdup(consturl);

	char * url = urldup+7;
	char * base = strchr(url,'/');
	char * port;
	char * attrs[] = { NULL, NULL };

	if(!base)
	{
		message->error = hSession->ssl.error = 0;
		message->title = _( "Security error" );
		message->text = _( "No DN of the entry at which to start the search on the URL" );
		message->description = _( "The URL argument should be in the format ldap://[HOST]/[DN]?attribute" );
		debug("%s",message->text);
		errno = EINVAL;
		return NULL;
	}

	*(base++) = 0;
	attrs[0] = strchr(base,'?');

	if(!base)
	{
		message->error = hSession->ssl.error = 0;
		message->title = _( "Security error" );
		message->text = _( "No LDAP attribute on the URL" );
		message->description = _( "The URL argument should be in the format ldap://[HOST]/[DN]?attribute" );
		debug("%s",message->text);
		errno = EINVAL;
		return NULL;
	}

	*(attrs[0]++) = 0;

	port = strchr(url,':');
	if(port)
	{
		*(port++) = 0;
	}

	debug("host: \"%s\"",url);
	debug("port: %d", atoi(port));
	debug("Base: \"%s\"",base);
	debug("Attr: \"%s\"",attrs[0]);

	// ldap_set_option(NULL, LDAP_OPT_PROTOCOL_VERSION, &ldap_proto);

	// Do LDAP Query
	lib3270_autoptr(LDAP) ld = ldap_init(url, (port && *port ? atoi(port) : LDAP_PORT));

	if(!ld)
	{
		message->error = hSession->ssl.error = 0;
		message->title = _( "Security error" );
		message->text = _( "Can't initialize LDAP" );
		debug("%s",message->text);
		message->lasterror = GetLastError();
		message->description = NULL;
		errno = EINVAL;
		return NULL;
	}

	static const int version = LDAP_VERSION3;
	rc = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &version);
	if(rc != LDAP_SUCCESS)
	{
		message->error = hSession->ssl.error = 0;
		message->title = _( "Security error" );
		message->text = _( "Can't set LDAP protocol version" );
		message->lasterror = LdapMapErrorToWin32(rc);
		message->description = NULL;

		debug("%s (rc=%u, lasterror=%d)",ldap_err2string(rc),rc,(unsigned int) message->lasterror);

		errno = EINVAL;
		return NULL;
	}

	rc = ldap_simple_bind(ld, "", "");
	if(rc != LDAP_SUCCESS)
	{
		message->error = hSession->ssl.error = 0;
		message->title = _( "Security error" );
		message->text = _( "Can't bind to LDAP server" );
		message->lasterror = LdapMapErrorToWin32(rc);
		message->description = NULL;

		debug("%s (rc=%u, lasterror=%d)",ldap_err2string(rc),rc,(unsigned int) message->lasterror);

		errno = EINVAL;
		return NULL;
	}


	debug("********************************************************* %s",__FUNCTION__);

	return x509_crl;

}

#endif // defined(HAVE_LIBSSL) && defined(SSL_ENABLE_CRL_CHECK)
