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
#include <lib3270-internals.h>
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

#ifdef HAVE_LIBSSL
 static const struct ssl_status_msg
 {
	long			  id;
	LIB3270_NOTIFY	  icon;
	const char		* iconName;		// Icon name from https://specifications.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html
	const char		* message;
	const char		* description;
 }
 status_msg[] =
 {
	// http://www.openssl.org/docs/apps/verify.html
	{
		X509_V_OK,
		LIB3270_NOTIFY_SECURE,
		"security-high",
		N_( "Secure connection was successful." ),
		N_( "The connection is secure and the host identity was confirmed." )
	},

	{
		X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT,
		LIB3270_NOTIFY_ERROR,
		"dialog-error",
		N_( "Unable to get issuer certificate" ),
		N_( "The issuer certificate of a looked up certificate could not be found. This normally means the list of trusted certificates is not complete." )
	},

	{
		X509_V_ERR_UNABLE_TO_GET_CRL,
		LIB3270_NOTIFY_ERROR,
		"dialog-error",
		N_( "Unable to get certificate CRL." ),
		N_( "The Certificate revocation list (CRL) of a certificate could not be found." )
	},

	{
		X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE,
		LIB3270_NOTIFY_ERROR,
		"dialog-error",
		N_( "Unable to decrypt certificate's signature" ),
		N_( "The certificate signature could not be decrypted. This means that the actual signature value could not be determined rather than it not matching the expected value, this is only meaningful for RSA keys." )
	},

	{
		X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE,
		LIB3270_NOTIFY_ERROR,
		"dialog-error",
		N_( "Unable to decrypt CRL's signature" ),
		N_( "The CRL signature could not be decrypted: this means that the actual signature value could not be determined rather than it not matching the expected value. Unused." )
	},

	{
		X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY,
		LIB3270_NOTIFY_ERROR,
		"dialog-error",
		N_( "Unable to decode issuer public key" ),
		N_( "The public key in the certificate SubjectPublicKeyInfo could not be read." )
	},

	{
		X509_V_ERR_CERT_SIGNATURE_FAILURE,
		LIB3270_NOTIFY_ERROR,
		"dialog-error",
		N_( "Certificate signature failure" ),
		N_( "The signature of the certificate is invalid." )
	},

	{
		X509_V_ERR_CRL_SIGNATURE_FAILURE,
		LIB3270_NOTIFY_ERROR,
		"dialog-error",
		N_( "CRL signature failure" ),
		N_( "The signature of the certificate is invalid." )
	},

	{
		X509_V_ERR_CERT_NOT_YET_VALID,
		LIB3270_NOTIFY_WARNING,
		"dialog-warning",
		N_( "Certificate is not yet valid" ),
		N_( "The certificate is not yet valid: the notBefore date is after the current time." )
	},

	{
		X509_V_ERR_CERT_HAS_EXPIRED,
		LIB3270_NOTIFY_ERROR,
		"dialog-error",
		N_( "Certificate has expired" ),
		N_( "The certificate has expired: that is the notAfter date is before the current time." )
	},

	{
		X509_V_ERR_CRL_NOT_YET_VALID,
		LIB3270_NOTIFY_WARNING,
		"dialog-error",
		N_( "The CRL is not yet valid." ),
		N_( "The Certificate revocation list (CRL) is not yet valid." )
	},

	{
		X509_V_ERR_CRL_HAS_EXPIRED,
#ifdef SSL_ENABLE_CRL_EXPIRATION_CHECK
		LIB3270_NOTIFY_ERROR,
#else
		LIB3270_NOTIFY_WARNING,
#endif // SSL_ENABLE_CRL_EXPIRATION_CHECK
		"security-medium",
		N_( "The CRL has expired." ),
		N_( "The Certificate revocation list (CRL) has expired.")
	},

	{
		X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD,
		LIB3270_NOTIFY_ERROR,
		"dialog-error",
		N_( "Format error in certificate's notBefore field" ),
		N_( "The certificate notBefore field contains an invalid time." )
	},

	{
		X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD,
		LIB3270_NOTIFY_ERROR,
		"dialog-error",
		N_( "Format error in certificate's notAfter field" ),
		N_( "The certificate notAfter field contains an invalid time." )
	},

	{
		X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD,
		LIB3270_NOTIFY_ERROR,
		"dialog-error",
		N_( "Format error in CRL's lastUpdate field" ),
		N_( "The CRL lastUpdate field contains an invalid time." )
	},

	{
		X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD,
		LIB3270_NOTIFY_ERROR,
		"dialog-error",
		N_( "Format error in CRL's nextUpdate field" ),
		N_( "The CRL nextUpdate field contains an invalid time." )
	},

	{
		X509_V_ERR_OUT_OF_MEM,
		LIB3270_NOTIFY_ERROR,
		"dialog-error",
		N_( "Out of memory" ),
		N_( "An error occurred trying to allocate memory. This should never happen." )
	},

	{
		X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT,
		LIB3270_NOTIFY_WARNING,
		"security-medium",
		N_( "Self signed certificate" ),
		N_( "The passed certificate is self signed and the same certificate cannot be found in the list of trusted certificates." )
	},

	{
		X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN,
#ifdef SSL_ENABLE_SELF_SIGNED_CERT_CHECK
		LIB3270_NOTIFY_ERROR,
		"security-medium",
		N_( "The SSL certificate for this host is not trusted." ),
		N_( "The security certificate presented by this host was not issued by a trusted certificate authority." )
#else
		LIB3270_NOTIFY_WARNING,
		"security-medium",
		N_( "Self signed certificate in certificate chain" ),
		N_( "The certificate chain could be built up using the untrusted certificates but the root could not be found locally." )
#endif // SSL_ENABLE_SELF_SIGNED_CERT_CHECK
	},

	{
		X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY,
		LIB3270_NOTIFY_WARNING,
		"security-low",
		N_( "Unable to get local issuer certificate" ),
		N_( "The issuer certificate could not be found: this occurs if the issuer certificate of an untrusted certificate cannot be found." )
	},

	{
		X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE,
		LIB3270_NOTIFY_ERROR,
		"security-low",
		N_( "Unable to verify the first certificate" ),
		N_( "No signatures could be verified because the chain contains only one certificate and it is not self signed." )
	},

	{
		X509_V_ERR_CERT_REVOKED,
		LIB3270_NOTIFY_ERROR,
		"security-low",
		N_( "Certificate revoked" ),
		N_( "The certificate has been revoked." )
	},

	{
		X509_V_ERR_INVALID_CA,
		LIB3270_NOTIFY_ERROR,
		"security-low",
		N_( "Invalid CA certificate" ),
		N_( "A CA certificate is invalid. Either it is not a CA or its extensions are not consistent with the supplied purpose." )
	},

	{
		X509_V_ERR_PATH_LENGTH_EXCEEDED,
		LIB3270_NOTIFY_ERROR,
		"dialog-error",
		N_( "Path length constraint exceeded" ),
		N_( "The basicConstraints pathlength parameter has been exceeded." ),
	},

	{
		X509_V_ERR_INVALID_PURPOSE,
		LIB3270_NOTIFY_ERROR,
		"dialog-error",
		N_( "Unsupported certificate purpose" ),
		N_( "The supplied certificate cannot be used for the specified purpose." )
	},

	{
		X509_V_ERR_CERT_UNTRUSTED,
		LIB3270_NOTIFY_WARNING,
		"security-low",
		N_( "Certificate not trusted" ),
		N_( "The root CA is not marked as trusted for the specified purpose." )
	},

	{
		X509_V_ERR_CERT_REJECTED,
		LIB3270_NOTIFY_ERROR,
		"security-low",
		N_( "Certificate rejected" ),
		N_( "The root CA is marked to reject the specified purpose." )
	},

	{
		X509_V_ERR_SUBJECT_ISSUER_MISMATCH,
		LIB3270_NOTIFY_ERROR,
		"security-low",
		N_( "Subject issuer mismatch" ),
		N_( "The current candidate issuer certificate was rejected because its subject name did not match the issuer name of the current certificate. Only displayed when the -issuer_checks option is set." )
	},

	{
		X509_V_ERR_AKID_SKID_MISMATCH,
		LIB3270_NOTIFY_ERROR,
		"dialog-error",
		N_( "Authority and subject key identifier mismatch" ),
		N_( "The current candidate issuer certificate was rejected because its subject key identifier was present and did not match the authority key identifier current certificate. Only displayed when the -issuer_checks option is set." )
	},

	{
		X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH,
		LIB3270_NOTIFY_ERROR,
		"dialog-error",
		N_( "Authority and issuer serial number mismatch" ),
		N_( "The current candidate issuer certificate was rejected because its issuer name and serial number was present and did not match the authority key identifier of the current certificate. Only displayed when the -issuer_checks option is set." )
	},

	{
		X509_V_ERR_KEYUSAGE_NO_CERTSIGN,
		LIB3270_NOTIFY_ERROR,
		"dialog-error",
		N_( "Key usage does not include certificate signing" ),
		N_( "The current candidate issuer certificate was rejected because its keyUsage extension does not permit certificate signing." )
	}

 };

 static const struct ssl_status_msg * get_ssl_status_msg(H3270 *hSession)
 {
 	size_t f;
	long id	= lib3270_get_SSL_verify_result(hSession);

	for(f=0;f < (sizeof(status_msg)/sizeof(status_msg[0]));f++)
	{
		if(status_msg[f].id == id)
			return status_msg+f;
	}
	return NULL;
 }

 const char	* lib3270_get_ssl_state_message(H3270 *hSession)
 {
	if(lib3270_get_secure(hSession) != LIB3270_SSL_UNSECURE)
	{
		const struct ssl_status_msg *info = get_ssl_status_msg(hSession);
		if(info)
			return gettext(info->message);
	}

	return _( "The connection is insecure" );

 }

 const char	* lib3270_get_ssl_state_icon_name(H3270 *hSession)
 {
	if(lib3270_get_secure(hSession) != LIB3270_SSL_UNSECURE)
	{
		const struct ssl_status_msg *info = get_ssl_status_msg(hSession);
		if(info)
			return info->iconName;
	}

	return "dialog-error";

 }


 const char * lib3270_get_ssl_state_description(H3270 *hSession)
 {
	if(lib3270_get_secure(hSession) != LIB3270_SSL_UNSECURE)
	{
		const struct ssl_status_msg *info = get_ssl_status_msg(hSession);
		if(info)
			return gettext(info->description);
	}
	else
	{
		return _( "The connection is insecure" );
	}

	return _( "Unexpected or unknown security status");
 }

 LIB3270_NOTIFY lib3270_get_ssl_state_icon(H3270 *hSession)
 {
	if(lib3270_get_secure(hSession) != LIB3270_SSL_UNSECURE)
	{
		const struct ssl_status_msg *info = get_ssl_status_msg(hSession);
		if(info)
			return info->icon;
	}

 	return LIB3270_NOTIFY_ERROR;
 }

#else

 const char	* lib3270_get_ssl_state_message(H3270 *hSession)
 {
	return lib3270_get_hostname(hSession);
 }

 const char * lib3270_get_ssl_state_description(H3270 *hSession)
 {
	return _( "The connection is insecure" );
 }

 LIB3270_NOTIFY lib3270_get_ssl_state_icon(H3270 *hSession)
 {
 	return LIB3270_NOTIFY_ERROR;
 }

 const char	* lib3270_get_ssl_state_icon_name(H3270 *hSession)
 {
 	return "dialog-error";
 }

#endif // HAVE_LIBSSL

