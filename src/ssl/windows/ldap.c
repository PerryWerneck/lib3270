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
 * https://docs.microsoft.com/en-us/windows/win32/api/winldap/
 * https://github.com/curl/curl/blob/curl-7_62_0/lib/ldap.c
 * http://forums.codeguru.com/showthread.php?313123-Elementary-problems-using-winldap
 * https://stackoverflow.com/questions/21501002/how-to-use-ldap-sasl-bind-in-winldap
 *
 */

#include <config.h>

#if defined(HAVE_LIBSSL) && defined(SSL_ENABLE_CRL_CHECK) && defined(HAVE_LDAP)

#include "private.h"
#include <winldap.h>
#include <utilc.h>
#include <lib3270/toggle.h>

# ifndef LDAP_VENDOR_NAME
#  error Your Platform SDK is NOT sufficient for LDAP support! \
         Update your Platform SDK, or disable LDAP support!
# else
#  include <winber.h>
# endif

/*--[ Implement ]------------------------------------------------------------------------------------*/

static inline void lib3270_autoptr_cleanup_LDAP(LDAP **ptr)
{
	debug("%s(%p)",__FUNCTION__,*ptr);
	if(*ptr)
	{
		ldap_unbind(*ptr);
		*ptr = NULL;
	}

}

static inline void lib3270_autoptr_cleanup_LDAPMessage(LDAPMessage **message)
{
	debug("%s(%p)",__FUNCTION__,*message);
	if(message)
		ldap_msgfree(*message);
	*message = NULL;
}

static inline void lib3270_autoptr_cleanup_LDAPPTR(char **ptr)
{
	debug("%s(%p)",__FUNCTION__,*ptr);
	if(*ptr)
		ldap_memfree(*ptr);
	*ptr = NULL;
}

static inline void lib3270_autoptr_cleanup_BerElement(BerElement **ber)
{
	debug("%s(%p)",__FUNCTION__,*ber);
	if(*ber)
		ber_free(*ber, 0);
	*ber = NULL;
}


X509_CRL * get_crl_using_ldap(H3270 *hSession, SSL_ERROR_MESSAGE * message, const char *consturl)
{
	X509_CRL * x509_crl = NULL;
	int rc = 0;

	// Strip query.

	lib3270_autoptr(char) urldup = lib3270_unescape(consturl);

	char * url = urldup+7;
	char * base = strchr(url,'/');
	char * port;
	char * attrs[] = { NULL, NULL };

	if(!base)
	{
		static const LIB3270_POPUP popup = {
			.summary = N_( "No DN of the entry at which to start the search on the URL" ),
			.body = N_( "The URL argument should be in the format ldap://[HOST]/[DN]?attribute" )
		};

		message->code = hSession->ssl.error = 0;
		message->popup = &popup;
		debug("%s",message->popup->summary);
		errno = EINVAL;
		return NULL;
	}

	*(base++) = 0;
	attrs[0] = strchr(base,'?');

	if(!base)
	{
		static const LIB3270_POPUP popup = {
			.summary = N_( "No LDAP attribute on the URL" ),
			.body = N_( "The URL argument should be in the format ldap://[HOST]/[DN]?attribute" )
		};

		message->code = hSession->ssl.error = 0;
		message->popup = &popup;
		debug("%s",message->popup->summary);
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
		static const LIB3270_POPUP popup = {
			.summary = N_( "Can't initialize LDAP" )
		};

		message->code = hSession->ssl.error = 0;
		message->popup = &popup;

		debug("%s",message->popup->summary);
		message->lasterror = GetLastError();
		errno = EINVAL;
		return NULL;
	}

	static const int version = LDAP_VERSION3;
	rc = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &version);
	if(rc != LDAP_SUCCESS)
	{
		static const LIB3270_POPUP popup = {
			.summary = N_( "Can't set LDAP protocol version" )
		};

		message->code = hSession->ssl.error = 0;
		message->popup = &popup;
		message->lasterror = LdapMapErrorToWin32(rc);

		debug("%s (rc=%u, lasterror=%d)",ldap_err2string(rc),rc,(unsigned int) message->lasterror);

		errno = EINVAL;
		return NULL;
	}

	rc = ldap_simple_bind_s(ld, NULL, NULL);
	if(rc != LDAP_SUCCESS)
	{
		static const LIB3270_POPUP popup = {
			.summary = N_( "Can't bind to LDAP server" )
		};

		message->code = hSession->ssl.error = 0;
		message->popup = &popup;
		message->lasterror = LdapMapErrorToWin32(rc);

		debug("%s (rc=%u, lasterror=%d)",ldap_err2string(rc),rc,(unsigned int) message->lasterror);

		errno = EINVAL;
		return NULL;
	}

	lib3270_autoptr(LDAPMessage) results = NULL;
	rc = ldap_search_ext_s(
				ld,						// Specifies the LDAP pointer returned by a previous call to ldap_init(), ldap_ssl_init(), or ldap_open().
				base,					// Specifies the DN of the entry at which to start the search.
				LDAP_SCOPE_BASE,		// Specifies the scope of the search.
				NULL,					// Specifies a string representation of the filter to apply in the search.
				(char **)  &attrs,		// Specifies a null-terminated array of character string attribute types to return from entries that match filter.
				0,						// Should be set to 1 to request attribute types only. Set to 0 to request both attributes types and attribute values.
				NULL,
				NULL,
				NULL,
				0,
				&results
			);


	if(rc != LDAP_SUCCESS)
	{
		static const LIB3270_POPUP popup = {
			.summary = N_( "Can't search LDAP server" )
		};
		message->body = ldap_err2string(rc);
		message->popup = &popup;
		lib3270_write_log(hSession,"ssl","%s: %s",url, message->body);
		return NULL;
	}

	lib3270_autoptr(BerElement) ber = NULL;
	char __attribute__ ((__cleanup__(lib3270_autoptr_cleanup_LDAPPTR))) *attr = ldap_first_attribute(ld, results, &ber);
	if(!attr)
	{
		static const LIB3270_POPUP popup = {
			.summary = N_( "Can't get LDAP attribute" ),
			.body  = N_("Search did not produce any attributes.")
		};

		message->code = hSession->ssl.error = 0;
		message->popup = &popup;
		lib3270_write_log(hSession,"ssl","%s: %s",url, message->popup->body);
		errno = ENOENT;
		return NULL;
	}

	struct berval ** value = ldap_get_values_len(ld, results, attr);
	if(!value)
	{
		static const LIB3270_POPUP popup = {
			.summary = N_( "Can't get LDAP attribute" ),
			.body = N_("Search did not produce any values.")
		};
		message->code = hSession->ssl.error = 0;
		message->popup = &popup;
		lib3270_write_log(hSession,"ssl","%s: %s",url, message->popup->body);
		errno = ENOENT;
		return NULL;
	}

	if(lib3270_get_toggle(hSession,LIB3270_TOGGLE_SSL_TRACE))
	{
		lib3270_trace_data(
			hSession,
			"CRL Data received from LDAP server",
			(const unsigned char *) value[0]->bv_val,
			value[0]->bv_len
		);
	}

	// Precisa salvar uma cópia porque d2i_X509_CRL modifica o ponteiro.
	const unsigned char *crl_data = (const unsigned char *) value[0]->bv_val;

	if(!d2i_X509_CRL(&x509_crl, &crl_data, value[0]->bv_len))
	{
		static const LIB3270_POPUP popup = {
			.summary = N_( "Can't decode certificate revocation list" )
		};

		message->code = hSession->ssl.error = ERR_get_error();
		message->popup = &popup;

		lib3270_write_log(hSession,"ssl","%s: %s",url, message->popup->summary);
		ldap_value_free_len(value);
		return NULL;
	}

	ldap_value_free_len(value);

	return x509_crl;

}

#endif //  defined(HAVE_LIBSSL) && defined(SSL_ENABLE_CRL_CHECK) && defined(HAVE_LDAP)
