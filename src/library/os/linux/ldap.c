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

#error deprecated

#include <config.h>

#if defined(HAVE_LDAP) && defined(HAVE_LIBSSL)

#include <internals.h>
#include <lib3270.h>
#include <lib3270/log.h>
#include <lib3270/trace.h>
#include <openssl/x509.h>
// #include <networking.h>

#define LDAP_DEPRECATED 1
#include <ldap.h>

typedef char LDAPPTR;

//--[ Implement ]------------------------------------------------------------------------------------

static inline void lib3270_autoptr_cleanup_LDAPMessage(LDAPMessage **message) {
	debug("%s(%p)",__FUNCTION__,*message);
	if(message)
		ldap_msgfree(*message);
	*message = NULL;
}

static inline void lib3270_autoptr_cleanup_LDAP(LDAP **ld) {
	debug("%s(%p)",__FUNCTION__,*ld);
	if(*ld)
		ldap_unbind_ext(*ld, NULL, NULL);
	*ld = NULL;
}

static inline void lib3270_autoptr_cleanup_BerElement(BerElement **ber) {
	debug("%s(%p)",__FUNCTION__,*ber);
	if(*ber)
		ber_free(*ber, 0);
	*ber = NULL;
}

static inline void lib3270_autoptr_cleanup_LDAPPTR(char **ptr) {
	debug("%s(%p)",__FUNCTION__,*ptr);
	if(*ptr)
		ldap_memfree(*ptr);
	*ptr = NULL;
}

X509_CRL * lib3270_crl_get_using_ldap(H3270 *hSession, const char *url, const char **error) {

	// Get attributes
	char * attrs[] = { NULL, NULL };
	char * base = strchr(url+7,'/');
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

	debug("host: \"%s\"",url);
	debug("Base: \"%s\"",base);
	debug("Attr: \"%s\"",attrs[0]);

	// Do LDAP Query
	lib3270_autoptr(LDAP) ld = NULL;
	lib3270_autoptr(BerElement) ber = NULL;

	int rc = ldap_initialize(&ld, url);
	if(rc != LDAP_SUCCESS) {
		*error = ldap_err2string(rc);
		return NULL;
	}

	unsigned long version = LDAP_VERSION3;
	rc = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION,(void *) &version);
	if(rc != LDAP_SUCCESS) {
		*error = ldap_err2string(rc);
		return NULL;
	}

	rc = ldap_simple_bind_s(ld, "", "");
	if(rc != LDAP_SUCCESS) {
		*error = ldap_err2string(rc);
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

	lib3270_autoptr(LDAPPTR) attr = ldap_first_attribute(ld, results, &ber);
	if(!attr) {
		*error = _("LDAP search did not produce any attributes.");
		errno = ENOENT;
		return NULL;
	}

	//
	// Load CRL
	//
	struct berval ** value = ldap_get_values_len(ld, results, attr);
	if(!value) {
		*error =_("LDAP search did not produce any values.");
		errno = ENOENT;
		return NULL;
	}

	X509_CRL * crl = NULL;

	const unsigned char *crl_data = (const unsigned char *) value[0]->bv_val;

	if(!d2i_X509_CRL(&crl, &crl_data, value[0]->bv_len)) {
		*error = _( "Can't decode certificate revocation list" );
	}

	ldap_value_free_len(value);

	return crl;

}

#endif // HAVE_LDAP
