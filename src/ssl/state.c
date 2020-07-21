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
#include <lib3270/internals.h>
#include <lib3270/popup.h>
#include <lib3270/trace.h>
#include <trace_dsc.h>
#include <lib3270/log.h>
#include <lib3270/trace.h>

#ifdef HAVE_LIBSSL
	#include <openssl/ssl.h>
	#include <openssl/err.h>
#endif // HAVE_LIBSSL

/*--[ Implement ]------------------------------------------------------------------------------------*/

LIB3270_EXPORT int lib3270_is_secure(const H3270 *hSession)
{
	return lib3270_get_ssl_state(hSession) == LIB3270_SSL_SECURE;
}

LIB3270_EXPORT long lib3270_get_SSL_verify_result(const H3270 *hSession)
{
#if defined(HAVE_LIBSSL)
	if(hSession->ssl.con)
		return SSL_get_verify_result(hSession->ssl.con);
#else
	errno = ENOTSUP;
#endif // HAVE_LIBSSL
	return -1;
}

LIB3270_EXPORT LIB3270_SSL_STATE lib3270_get_ssl_state(const H3270 *hSession)
{
#if defined(HAVE_LIBSSL)
	return hSession->ssl.state;
#else
	return LIB3270_SSL_UNDEFINED;
#endif // HAVE_LIBSSL
}

#if defined(HAVE_LIBSSL)
void set_ssl_state(H3270 *hSession, LIB3270_SSL_STATE state)
{
	if(state == hSession->ssl.state)
		return;

	hSession->ssl.state = state;
	trace_dsn(hSession,"SSL state changes to %d\n",(int) state);

	hSession->cbk.update_ssl(hSession,hSession->ssl.state);
}

static const struct ssl_status_msg status_msg[] =
{
	// http://www.openssl.org/docs/apps/verify.html
	{
		.id = X509_V_OK,
		.type = LIB3270_NOTIFY_SECURE,
		.iconName = "security-high",
		.summary = N_( "Secure connection was successful." ),
		.body = N_( "The connection is secure and the host identity was confirmed." )
	},

	{
		.id = X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT,
		.type = LIB3270_NOTIFY_ERROR,
		.iconName = "dialog-error",
		.summary = N_( "Unable to get issuer certificate" ),
		.body = N_( "The issuer certificate of a looked up certificate could not be found. This normally means the list of trusted certificates is not complete." )
	},

	{
		.id = X509_V_ERR_UNABLE_TO_GET_CRL,
		.type = LIB3270_NOTIFY_ERROR,
		.iconName = "dialog-error",
		.summary = N_( "Unable to get certificate CRL." ),
		.body = N_( "The Certificate revocation list (CRL) of a certificate could not be found." )
	},

	{
		.id = X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE,
		.type = LIB3270_NOTIFY_ERROR,
		.iconName = "dialog-error",
		.summary = N_( "Unable to decrypt certificate's signature" ),
		.body = N_( "The certificate signature could not be decrypted. This means that the actual signature value could not be determined rather than it not matching the expected value, this is only meaningful for RSA keys." )
	},

	{
		.id = X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE,
		.type = LIB3270_NOTIFY_ERROR,
		.iconName = "dialog-error",
		.summary = N_( "Unable to decrypt CRL's signature" ),
		.body = N_( "The CRL signature could not be decrypted: this means that the actual signature value could not be determined rather than it not matching the expected value. Unused." )
	},

	{
		.id = X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY,
		.type = LIB3270_NOTIFY_ERROR,
		.iconName = "dialog-error",
		.summary = N_( "Unable to decode issuer public key" ),
		.body = N_( "The public key in the certificate SubjectPublicKeyInfo could not be read." )
	},

	{
		.id = X509_V_ERR_CERT_SIGNATURE_FAILURE,
		.type = LIB3270_NOTIFY_ERROR,
		.iconName = "dialog-error",
		.summary = N_( "Certificate signature failure" ),
		.body = N_( "The signature of the certificate is invalid." )
	},

	{
		.id = X509_V_ERR_CRL_SIGNATURE_FAILURE,
		.type = LIB3270_NOTIFY_ERROR,
		.iconName = "dialog-error",
		.summary = N_( "CRL signature failure" ),
		.body = N_( "The signature of the certificate is invalid." )
	},

	{
		.id = X509_V_ERR_CERT_NOT_YET_VALID,
		.type = LIB3270_NOTIFY_WARNING,
		.iconName = "dialog-warning",
		.summary = N_( "Certificate is not yet valid" ),
		.body = N_( "The certificate is not yet valid: the notBefore date is after the current time." )
	},

	{
		.id = X509_V_ERR_CERT_HAS_EXPIRED,
		.type = LIB3270_NOTIFY_ERROR,
		.iconName = "dialog-error",
		.summary = N_( "Certificate has expired" ),
		.body = N_( "The certificate has expired: that is the notAfter date is before the current time." )
	},

	{
		.id = X509_V_ERR_CRL_NOT_YET_VALID,
		.type = LIB3270_NOTIFY_WARNING,
		.iconName = "dialog-error",
		.summary = N_( "The CRL is not yet valid." ),
		.body = N_( "The Certificate revocation list (CRL) is not yet valid." )
	},

	{
		.id = X509_V_ERR_CRL_HAS_EXPIRED,
#ifdef SSL_ENABLE_CRL_EXPIRATION_CHECK
		.type = LIB3270_NOTIFY_ERROR,
#else
		.type = LIB3270_NOTIFY_WARNING,
#endif // SSL_ENABLE_CRL_EXPIRATION_CHECK
		.iconName = "security-medium",
		.summary = N_( "The CRL has expired." ),
		.body = N_( "The Certificate revocation list (CRL) has expired.")
	},

	{
		.id = X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD,
		.type = LIB3270_NOTIFY_ERROR,
		.iconName = "dialog-error",
		.summary = N_( "Format error in certificate's notBefore field" ),
		.body = N_( "The certificate notBefore field contains an invalid time." )
	},

	{
		.id = X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD,
		.type = LIB3270_NOTIFY_ERROR,
		.iconName = "dialog-error",
		.summary = N_( "Format error in certificate's notAfter field" ),
		.body = N_( "The certificate notAfter field contains an invalid time." )
	},

	{
		.id = X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD,
		.type = LIB3270_NOTIFY_ERROR,
		.iconName = "dialog-error",
		.summary = N_( "Format error in CRL's lastUpdate field" ),
		.body = N_( "The CRL lastUpdate field contains an invalid time." )
	},

	{
		.id = X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD,
		.type = LIB3270_NOTIFY_ERROR,
		.iconName = "dialog-error",
		.summary = N_( "Format error in CRL's nextUpdate field" ),
		.body = N_( "The CRL nextUpdate field contains an invalid time." )
	},

	{
		.id = X509_V_ERR_OUT_OF_MEM,
		.type = LIB3270_NOTIFY_ERROR,
		.iconName = "dialog-error",
		.summary = N_( "Out of memory" ),
		.body = N_( "An error occurred trying to allocate memory. This should never happen." )
	},

	{
		.id = X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT,
		.type = LIB3270_NOTIFY_WARNING,
		.iconName = "security-medium",
		.summary = N_( "Self signed certificate" ),
		.body = N_( "The passed certificate is self signed and the same certificate cannot be found in the list of trusted certificates." )
	},

	{
		.id = X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN,
#ifdef SSL_ENABLE_SELF_SIGNED_CERT_CHECK
		.type = LIB3270_NOTIFY_ERROR,
#else
		.type = LIB3270_NOTIFY_WARNING,
#endif // SSL_ENABLE_SELF_SIGNED_CERT_CHECK
		.iconName = "security-medium",
		.summary = N_( "Self signed certificate in certificate chain" ),
		.body = N_( "The certificate chain could be built up using the untrusted certificates but the root could not be found locally." )
	},

	{
		.id = X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY,
		.type = LIB3270_NOTIFY_WARNING,
		.iconName = "security-low",
		.summary = N_( "Unable to get local issuer certificate" ),
		.body = N_( "The issuer certificate could not be found: this occurs if the issuer certificate of an untrusted certificate cannot be found." )
	},

	{
		.id = X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE,
		.type = LIB3270_NOTIFY_ERROR,
		.iconName = "security-low",
		.summary = N_( "Unable to verify the first certificate" ),
		.body = N_( "No signatures could be verified because the chain contains only one certificate and it is not self signed." )
	},

	{
		.id = X509_V_ERR_CERT_REVOKED,
		.type = LIB3270_NOTIFY_ERROR,
		.iconName = "security-low",
		.summary = N_( "Certificate revoked" ),
		.body = N_( "The certificate has been revoked." )
	},

	{
		.id = X509_V_ERR_INVALID_CA,
		.type = LIB3270_NOTIFY_ERROR,
		.iconName = "security-low",
		.summary = N_( "Invalid CA certificate" ),
		.body = N_( "A CA certificate is invalid. Either it is not a CA or its extensions are not consistent with the supplied purpose." )
	},

	{
		.id = X509_V_ERR_PATH_LENGTH_EXCEEDED,
		.type = LIB3270_NOTIFY_ERROR,
		.iconName = "dialog-error",
		.summary = N_( "Path length constraint exceeded" ),
		.body = N_( "The basicConstraints pathlength parameter has been exceeded." ),
	},

	{
		.id = X509_V_ERR_INVALID_PURPOSE,
		.type = LIB3270_NOTIFY_ERROR,
		.iconName = "dialog-error",
		.summary = N_( "Unsupported certificate purpose" ),
		.body = N_( "The supplied certificate cannot be used for the specified purpose." )
	},

	{
		.id = X509_V_ERR_CERT_UNTRUSTED,
		.type = LIB3270_NOTIFY_WARNING,
		.iconName = "security-low",
		.summary = N_( "Certificate not trusted" ),
		.body = N_( "The root CA is not marked as trusted for the specified purpose." )
	},

	{
		.id = X509_V_ERR_CERT_REJECTED,
		.type = LIB3270_NOTIFY_ERROR,
		.iconName = "security-low",
		.summary = N_( "Certificate rejected" ),
		.body = N_( "The root CA is marked to reject the specified purpose." )
	},

	{
		.id = X509_V_ERR_SUBJECT_ISSUER_MISMATCH,
		.type = LIB3270_NOTIFY_ERROR,
		.iconName = "security-low",
		.summary = N_( "Subject issuer mismatch" ),
		.body = N_( "The current candidate issuer certificate was rejected because its subject name did not match the issuer name of the current certificate. Only displayed when the -issuer_checks option is set." )
	},

	{
		.id = X509_V_ERR_AKID_SKID_MISMATCH,
		.type = LIB3270_NOTIFY_ERROR,
		.iconName = "dialog-error",
		.summary = N_( "Authority and subject key identifier mismatch" ),
		.body = N_( "The current candidate issuer certificate was rejected because its subject key identifier was present and did not match the authority key identifier current certificate. Only displayed when the -issuer_checks option is set." )
	},

	{
		.id = X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH,
		.type = LIB3270_NOTIFY_ERROR,
		.iconName = "dialog-error",
		.summary = N_( "Authority and issuer serial number mismatch" ),
		.body = N_( "The current candidate issuer certificate was rejected because its issuer name and serial number was present and did not match the authority key identifier of the current certificate. Only displayed when the -issuer_checks option is set." )
	},

	{
		.id = X509_V_ERR_KEYUSAGE_NO_CERTSIGN,
		.type = LIB3270_NOTIFY_ERROR,
		.iconName = "dialog-error",
		.summary = N_( "Key usage does not include certificate signing" ),
		.body = N_( "The current candidate issuer certificate was rejected because its keyUsage extension does not permit certificate signing." )
	}

 };

 const struct ssl_status_msg * ssl_get_status_from_error_code(long id)
 {
 	size_t f;

	for(f=0;f < (sizeof(status_msg)/sizeof(status_msg[0]));f++)
	{
		if(status_msg[f].id == id)
			return status_msg+f;
	}
	return NULL;
 }

 static const struct ssl_status_msg * get_ssl_status_msg(const H3270 *hSession)
 {
 	return ssl_get_status_from_error_code(lib3270_get_SSL_verify_result(hSession));
 }

 const char	* lib3270_get_ssl_state_message(const H3270 *hSession)
 {
	if(lib3270_get_ssl_state(hSession) != LIB3270_SSL_UNSECURE)
	{
		const struct ssl_status_msg *info = get_ssl_status_msg(hSession);
		if(info)
			return gettext(info->summary);
	}

	return _( "The connection is insecure" );

 }

 const char	* lib3270_get_ssl_state_icon_name(const H3270 *hSession)
 {
	if(lib3270_get_ssl_state(hSession) != LIB3270_SSL_UNSECURE)
	{
		const struct ssl_status_msg *info = get_ssl_status_msg(hSession);
		if(info)
			return info->iconName;
	}

	return "dialog-error";

 }


 const char * lib3270_get_ssl_state_description(const H3270 *hSession)
 {
	if(lib3270_get_ssl_state(hSession) != LIB3270_SSL_UNSECURE)
	{
		const struct ssl_status_msg *info = get_ssl_status_msg(hSession);
		if(info)
			return gettext(info->body);
	}
	else
	{
		return _( "The connection is insecure" );
	}

	return _( "Unexpected or unknown security status");
 }

 LIB3270_NOTIFY lib3270_get_ssl_state_icon(const H3270 *hSession)
 {
	if(lib3270_get_ssl_state(hSession) != LIB3270_SSL_UNSECURE)
	{
		const struct ssl_status_msg *info = get_ssl_status_msg(hSession);
		if(info)
			return info->type;
	}

 	return LIB3270_NOTIFY_ERROR;
 }

#else

 const char	* lib3270_get_ssl_state_message(const H3270 *hSession)
 {
	return lib3270_get_hostname(hSession);
 }

 const char * lib3270_get_ssl_state_description(const H3270 *hSession)
 {
	return _( "The connection is insecure" );
 }

 LIB3270_NOTIFY lib3270_get_ssl_state_icon(const H3270 *hSession)
 {
 	return LIB3270_NOTIFY_ERROR;
 }

 const char	* lib3270_get_ssl_state_icon_name(const H3270 *hSession)
 {
 	return "dialog-error";
 }

#endif // HAVE_LIBSSL

