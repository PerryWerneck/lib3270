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

#if defined(HAVE_LDAP) && defined(HAVE_LIBSSL)

#include "private.h"
#include <winldap.h>
#include <private/util.h>
#include <lib3270/toggle.h>
#include <lib3270/trace.h>

# ifndef LDAP_VENDOR_NAME
#  error Your Platform SDK is NOT sufficient for LDAP support! \
Update your Platform SDK, or disable LDAP support!
# else
#  include <winber.h>
# endif

//--[ Implement ]------------------------------------------------------------------------------------

static inline void lib3270_autoptr_cleanup_LDAP(LDAP **ptr) {
	debug("%s(%p)",__FUNCTION__,*ptr);
	if(*ptr) {
		ldap_unbind(*ptr);
		*ptr = NULL;
	}

}

static inline void lib3270_autoptr_cleanup_LDAPMessage(LDAPMessage **message) {
	debug("%s(%p)",__FUNCTION__,*message);
	if(message)
		ldap_msgfree(*message);
	*message = NULL;
}

static inline void lib3270_autoptr_cleanup_LDAPPTR(char **ptr) {
	debug("%s(%p)",__FUNCTION__,*ptr);
	if(*ptr)
		ldap_memfree(*ptr);
	*ptr = NULL;
}

static inline void lib3270_autoptr_cleanup_BerElement(BerElement **ber) {
	debug("%s(%p)",__FUNCTION__,*ber);
	if(*ber)
		ber_free(*ber, 0);
	*ber = NULL;
}


X509_CRL * lib3270_crl_get_using_ldap(H3270 *hSession, const char *consturl, const char **error) {
	X509_CRL * x509_crl = NULL;
	int rc = 0;

	// Strip query.

	lib3270_autoptr(char) urldup = unescape(consturl);

	char * url = urldup+7;
	char * base = strchr(url,'/');
	char * port;
	char * attrs[] = { NULL, NULL };

	if(!base) {
		*error = _( "The URL argument should be in the format ldap://[HOST]/[DN]?attribute" );
		errno = EINVAL;
		return NULL;
	}

	*(base++) = 0;
	attrs[0] = strchr(base,'?');

	if(!base) {
		*error = _( "The URL argument should be in the format ldap://[HOST]/[DN]?attribute" );
		errno = EINVAL;
		return NULL;
	}

	*(attrs[0]++) = 0;

	port = strchr(url,':');
	if(port) {
		*(port++) = 0;
	}

	debug("host: \"%s\"",url);
	debug("port: %d", atoi(port));
	debug("Base: \"%s\"",base);
	debug("Attr: \"%s\"",attrs[0]);

	// ldap_set_option(NULL, LDAP_OPT_PROTOCOL_VERSION, &ldap_proto);

	// Do LDAP Query
	lib3270_autoptr(LDAP) ld = ldap_init(url, (port && *port ? atoi(port) : LDAP_PORT));

	if(!ld) {
		*error = _( "Can't initialize LDAP" );
		errno = -1;
		return NULL;
	}

	static const int version = LDAP_VERSION3;
	rc = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &version);
	if(rc != LDAP_SUCCESS) {
		*error = _( "Can't set LDAP protocol version" );
		SetLastError(LdapMapErrorToWin32(rc));
		errno = EINVAL;
		return NULL;
	}

	rc = ldap_simple_bind_s(ld, NULL, NULL);
	if(rc != LDAP_SUCCESS) {
		*error =_( "Can't bind to LDAP server" );
		SetLastError(LdapMapErrorToWin32(rc));
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


	if(rc != LDAP_SUCCESS) {
		*error = ldap_err2string(rc);
		return NULL;
	}

	lib3270_autoptr(BerElement) ber = NULL;
	char __attribute__ ((__cleanup__(lib3270_autoptr_cleanup_LDAPPTR))) *attr = ldap_first_attribute(ld, results, &ber);
	if(!attr) {
		*error = _("LDAP Search did not produce any attributes.");
		errno = ENOENT;
		return NULL;
	}

	struct berval ** value = ldap_get_values_len(ld, results, attr);
	if(!value) {
		*error = _("LDAPSearch did not produce any values.");
		errno = ENOENT;
		return NULL;
	}

	if(lib3270_get_toggle(hSession,LIB3270_TOGGLE_SSL_TRACE)) {
		trace_data(
		    hSession,
		    "CRL Data received from LDAP server",
		    (const unsigned char *) value[0]->bv_val,
		    value[0]->bv_len
		);
	}

	// Precisa salvar uma cópia porque d2i_X509_CRL modifica o ponteiro.
	const unsigned char *crl_data = (const unsigned char *) value[0]->bv_val;

	if(!d2i_X509_CRL(&x509_crl, &crl_data, value[0]->bv_len)) {
		*error =_( "Can't decode certificate revocation list" );
		ldap_value_free_len(value);
		return NULL;
	}

	ldap_value_free_len(value);

	return x509_crl;

}

#endif //  defined(HAVE_LIBSSL) && defined(HAVE_LDAP)
