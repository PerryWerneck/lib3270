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
 *
 * References:
 *
 * http://www.openssl.org/docs/ssl/
 * https://stackoverflow.com/questions/4389954/does-openssl-automatically-handle-crls-certificate-revocation-lists-now
 *
 */

#include <config.h>

#if defined(HAVE_LIBSSL) && defined(SSL_ENABLE_CRL_CHECK) && defined(HAVE_LDAP)

#define LDAP_DEPRECATED 1
#include <ldap.h>

/*--[ Implement ]------------------------------------------------------------------------------------*/

static inline void lib3270_autoptr_cleanup_LDAPMessage(LDAPMessage **message)
{
	debug("%s(%p)",__FUNCTION__,*message);
	if(message)
		ldap_msgfree(*message);
	*message = NULL;
}

static inline void lib3270_autoptr_cleanup_LDAP(LDAP **ld)
{
	debug("%s(%p)",__FUNCTION__,*ld);
	if(*ld)
		ldap_unbind_ext(*ld, NULL, NULL);
	*ld = NULL;
}

static inline void lib3270_autoptr_cleanup_BerElement(BerElement **ber)
{
	debug("%s(%p)",__FUNCTION__,*ber);
	if(*ber)
		ber_free(*ber, 0);
	*ber = NULL;
}

static inline void lib3270_autoptr_cleanup_LDAPPTR(char **ptr)
{
	debug("%s(%p)",__FUNCTION__,*ptr);
	if(*ptr)
		ldap_memfree(*ptr);
	*ptr = NULL;
}

LIB3270_INTERNAL X509_CRL * get_crl_using_ldap(H3270 *hSession, SSL_ERROR_MESSAGE * message, const char *consturl)
{
	X509_CRL * x509_crl = NULL;

	int	rc;
	lib3270_autoptr(char) url = strdup(consturl);
	char * base = strchr(url+7,'/');
	char * attrs[] = { NULL, NULL };

	if(!base)
	{
		message->error = hSession->ssl.error = 0;
		message->title = _( "Security error" );
		message->text = _( "No DN of the entry at which to start the search on the URL" );
		message->description = _( "The URL argument should be in the format ldap://[HOST]/[DN]?attribute" );
		return errno = EINVAL;
	}

	*(base++) = 0;
	attrs[0] = strchr(base,'?');

	if(!base)
	{
		message->error = hSession->ssl.error = 0;
		message->title = _( "Security error" );
		message->text = _( "No LDAP attribute on the URL" );
		message->description = _( "The URL argument should be in the format ldap://[HOST]/[DN]?attribute" );
		return errno = EINVAL;
	}

	*(attrs[0]++) = 0;

	debug("host: \"%s\"",url);
	debug("Base: \"%s\"",base);
	debug("Attr: \"%s\"",attrs[0]);

	// Do LDAP Query
	LDAP __attribute__ ((__cleanup__(lib3270_autoptr_cleanup_LDAP))) *ld = NULL;
	BerElement __attribute__ ((__cleanup__(lib3270_autoptr_cleanup_BerElement))) * ber = NULL;

	rc = ldap_initialize(&ld, url);
	if(rc != LDAP_SUCCESS)
	{
		message->error = hSession->ssl.error = 0;
		message->title = _( "Security error" );
		message->text = _( "Can't initialize LDAP" );
		message->description = ldap_err2string(rc);
		lib3270_write_log(hSession,"ssl","%s: %s",url, message->description);
		return -1;
	}

	unsigned long version = LDAP_VERSION3;
	rc = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION,(void *) &version);
	if(rc != LDAP_SUCCESS) {
		message->error = hSession->ssl.error = 0;
		message->title = _( "Security error" );
		message->text = _( "Can't set LDAP version" );
		message->description = ldap_err2string(rc);
		lib3270_write_log(hSession,"ssl","%s: %s",url, message->description);
		return NULL;
	}

	rc = ldap_simple_bind_s(ld, "", "");
	if(rc != LDAP_SUCCESS)
	{
		message->error = hSession->ssl.error = 0;
		message->title = _( "Security error" );
		message->text = _( "Can't bind to LDAP server" );
		message->description = ldap_err2string(rc);
		lib3270_write_log(hSession,"ssl","%s: %s",url, message->description);
		return -1;
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
		message->error = hSession->ssl.error = 0;
		message->title = _( "Security error" );
		message->text = _( "Can't search LDAP server" );
		message->description = ldap_err2string(rc);
		lib3270_write_log(hSession,"ssl","%s: %s",url, message->description);
		return NULL;
	}

	char __attribute__ ((__cleanup__(lib3270_autoptr_cleanup_LDAPPTR))) *attr = ldap_first_attribute(ld, results, &ber);
	if(!attr)
	{
		message->error = hSession->ssl.error = 0;
		message->title = _( "Security error" );
		message->text = _( "Can't get LDAP attribute" );
		message->description = _("Search did not produce any attributes.");
		lib3270_write_log(hSession,"ssl","%s: %s",url, message->description);
		errno = ENOENT;
		return NULL;
	}

	struct berval ** value = ldap_get_values_len(ld, results, attr);
	if(!value)
	{
		message->error = hSession->ssl.error = 0;
		message->title = _( "Security error" );
		message->text = _( "Can't get LDAP attribute" );
		message->description = _("Search did not produce any values.");
		lib3270_write_log(hSession,"ssl","%s: %s",url, message->description);
		errno = ENOENT;
		return NULL;
	}

	if(lib3270_get_toggle(hSession,LIB3270_TOGGLE_SSL_TRACE))
	{
		lib3270_trace_data(
			hSession,
			"CRL Data received from LDAP server",
			(const char *) value[0]->bv_val,
			value[0]->bv_len
		);
	}

	// Precisa salvar uma cópia porque d2i_X509_CRL modifica o ponteiro.
	const unsigned char *crl_data = (const unsigned char *) value[0]->bv_val;

	if(!d2i_X509_CRL(&x509_crl, &crl_data, value[0]->bv_len))
	{
		message->error = hSession->ssl.error = ERR_get_error();
		message->title = _( "Security error" );
		message->text = _( "Can't decode certificate revocation list" );
		lib3270_write_log(hSession,"ssl","%s: %s",url, message->text);
		ldap_value_free_len(value);
		return NULL;
	}

	ldap_value_free_len(value);

	return x509_crl;

}

#endif // HAVE_LIBSSL && SSL_ENABLE_CRL_CHECK && HAVE_LDAP
