/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2025 Perry Werneck <perry.werneck@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
 
 #include <config.h>
 #include <lib3270/defs.h>
 #include <private/intl.h>
 #include <private/network.h>
 #include <private/openssl.h>

 static const struct {

	long code;
	LIB3270_SSL_MESSAGE message;

 } messages[] = {

	// http://www.openssl.org/docs/apps/verify.html
	{
		.code = X509_V_OK,
		.message = {
			.name = "X509_V_OK",
			.title = N_("The connection is secure"),
			.type = LIB3270_NOTIFY_SECURITY_HIGH,
			.icon = "security-high",
			.summary = N_( "Secure connection was successful." ),
			.body = N_( "The connection is secure and the host identity was confirmed." )
		}
	},

	{
		.code = X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT,
		.message = {
			.name = "X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "dialog-error",
			.summary = N_( "Unable to get issuer certificate" ),
			.body = N_( "The issuer certificate of a looked up certificate could not be found. This normally means the list of trusted certificates is not complete." )
		}
	},

	{
		.code = X509_V_ERR_UNABLE_TO_GET_CRL,
		.message = {
			.name = "X509_V_ERR_UNABLE_TO_GET_CRL",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "security-low",
			.summary = N_( "Unable to get certificate CRL." ),
			.body = N_( "The Certificate revocation list (CRL) of a certificate could not be found." ),
		}
	},

	{
		.code = X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE,
		.message = {
			.name = "X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "dialog-error",
			.summary = N_( "Unable to decrypt certificate's signature" ),
			.body = N_( "The certificate signature could not be decrypted. This means that the actual signature value could not be determined rather than it not matching the expected value, this is only meaningful for RSA keys." )
		}
	},

	{
		.code = X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE,
		.message = {
			.name = "X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "dialog-error",
			.summary = N_( "Unable to decrypt CRL's signature" ),
			.body = N_( "The CRL signature could not be decrypted: this means that the actual signature value could not be determined rather than it not matching the expected value. Unused." )
		}
	},

	{
		.code = X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY,
		.message = {
			.name = "X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "dialog-error",
			.summary = N_( "Unable to decode issuer public key" ),
			.body = N_( "The public key in the certificate SubjectPublicKeyInfo could not be read." )
		}
	},

	{
		.code = X509_V_ERR_CERT_SIGNATURE_FAILURE,
		.message = {
			.name = "X509_V_ERR_CERT_SIGNATURE_FAILURE",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "dialog-error",
			.summary = N_( "Certificate signature failure" ),
			.body = N_( "The signature of the certificate is invalid." )
		}
	},

	{
		.code = X509_V_ERR_CRL_SIGNATURE_FAILURE,
		.message = {
			.name = "X509_V_ERR_CRL_SIGNATURE_FAILURE",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "dialog-error",
			.summary = N_( "CRL signature failure" ),
			.body = N_( "The signature of the certificate is invalid." )
		}
	},

	{
		.code = X509_V_ERR_CERT_NOT_YET_VALID,
		.message = {
			.name = "X509_V_ERR_CERT_NOT_YET_VALID",
			.type = LIB3270_NOTIFY_SECURITY_MEDIUM,
			.icon = "security-medium",
			.summary = N_( "Certificate is not yet valid" ),
			.body = N_( "The certificate is not yet valid: the notBefore date is after the current time." )
		}
	},

	{
		.code = X509_V_ERR_CERT_HAS_EXPIRED,
		.message = {
			.name = "X509_V_ERR_CERT_HAS_EXPIRED",
			.type = LIB3270_NOTIFY_SECURITY_MEDIUM,
			.icon = "security-medium",
			.summary = N_( "Certificate has expired" ),
			.body = N_( "The certificate has expired: that is the notAfter date is before the current time." )
		}
	},

	{
		.code = X509_V_ERR_CRL_NOT_YET_VALID,
		.message = {
			.name = "X509_V_ERR_CRL_NOT_YET_VALID",
			.type = LIB3270_NOTIFY_SECURITY_MEDIUM,
			.icon = "security-medium",
			.summary = N_( "The CRL is not yet valid." ),
			.body = N_( "The Certificate revocation list (CRL) is not yet valid." )
		}
	},

	{
		.code = X509_V_ERR_CRL_HAS_EXPIRED,
		.message = {
			.name = "X509_V_ERR_CRL_HAS_EXPIRED",
			.type = LIB3270_NOTIFY_SECURITY_MEDIUM,
			.icon = "security-medium",
			.summary = N_( "The CRL has expired." ),
			.body = N_( "The Certificate revocation list (CRL) has expired.")
		}
	},

	{
		.code = X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD,
		.message = {
			.name = "X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "dialog-error",
			.summary = N_( "Format error in certificate's notBefore field" ),
			.body = N_( "The certificate notBefore field contains an invalid time." )
		}
	},

	{
		.code = X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD,
		.message = {
			.name = "X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "dialog-error",
			.summary = N_( "Format error in certificate's notAfter field" ),
			.body = N_( "The certificate notAfter field contains an invalid time." )
		}
	},

	{
		.code = X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD,
		.message = {
			.name = "X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "dialog-error",
			.summary = N_( "Format error in CRL's lastUpdate field" ),
			.body = N_( "The CRL lastUpdate field contains an invalid time." )
		}
	},

	{
		.code = X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD,
		.message = {
			.name = "X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "dialog-error",
			.summary = N_( "Format error in CRL's nextUpdate field" ),
			.body = N_( "The CRL nextUpdate field contains an invalid time." )
		}
	},

	{
		.code = X509_V_ERR_OUT_OF_MEM,
		.message = {
			.name = "X509_V_ERR_OUT_OF_MEM",
			.type = LIB3270_NOTIFY_ERROR,
			.icon = "dialog-error",
			.summary = N_( "Out of memory" ),
			.body = N_( "An error occurred trying to allocate memory. This should never happen." )
		}
	},

	{
		.code = X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT,
		.message = {
			.name = "X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT",
			.type = LIB3270_NOTIFY_SECURITY_MEDIUM,
			.icon = "security-medium",
			.summary = N_( "Self signed certificate" ),
			.body = N_( "The passed certificate is self signed and the same certificate cannot be found in the list of trusted certificates." )
		}
	},

	{
		.code = X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN,
		.message = {
			.name = "X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "security-low",
			.summary = N_( "Self signed certificate in certificate chain" ),
			.body = N_( "The certificate chain could be built up using the untrusted certificates but the root could not be found locally." )
		}
	},

	{
		.code = X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY,
		.message = {
			.name = "X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "security-low",
			.summary = N_( "Unable to get local issuer certificate" ),
			.body = N_( "The issuer certificate could not be found: this occurs if the issuer certificate of an untrusted certificate cannot be found." )
		}
	},

	{
		.code = X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE,
		.message = {
			.name = "X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "security-low",
			.summary = N_( "Unable to verify the first certificate" ),
			.body = N_( "No signatures could be verified because the chain contains only one certificate and it is not self signed." )
		}
	},

	{
		.code = X509_V_ERR_CERT_REVOKED,
		.message = {
			.name = "X509_V_ERR_CERT_REVOKED",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "security-low",
			.summary = N_( "Certificate revoked" ),
			.body = N_( "The certificate has been revoked." )
		}
	},

	{
		.code = X509_V_ERR_INVALID_CA,
		.message = {
			.name = "X509_V_ERR_INVALID_CA",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "security-low",
			.summary = N_( "Invalid CA certificate" ),
			.body = N_( "A CA certificate is invalid. Either it is not a CA or its extensions are not consistent with the supplied purpose." )
		}
	},

	{
		.code = X509_V_ERR_PATH_LENGTH_EXCEEDED,
		.message = {
			.name = "X509_V_ERR_PATH_LENGTH_EXCEEDED",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "dialog-error",
			.summary = N_( "Path length constraint exceeded" ),
			.body = N_( "The basicConstraints pathlength parameter has been exceeded." ),
		}
	},

	{
		.code = X509_V_ERR_INVALID_PURPOSE,
		.message = {
			.name = "X509_V_ERR_INVALID_PURPOSE",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "dialog-error",
			.summary = N_( "Unsupported certificate purpose" ),
			.body = N_( "The supplied certificate cannot be used for the specified purpose." )
		}
	},

	{
		.code = X509_V_ERR_CERT_UNTRUSTED,
		.message = {
			.name = "X509_V_ERR_CERT_UNTRUSTED",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "security-low",
			.summary = N_( "Certificate not trusted" ),
			.body = N_( "The root CA is not marked as trusted for the specified purpose." )
		}
	},

	{
		.code = X509_V_ERR_CERT_REJECTED,
		.message = {
			.name = "X509_V_ERR_CERT_REJECTED",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "security-low",
			.summary = N_( "Certificate rejected" ),
			.body = N_( "The root CA is marked to reject the specified purpose." )
		}
	},

	{
		.code = X509_V_ERR_SUBJECT_ISSUER_MISMATCH,
		.message = {
			.name = "X509_V_ERR_SUBJECT_ISSUER_MISMATCH",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "security-low",
			.summary = N_( "Subject issuer mismatch" ),
			.body = N_( "The current candidate issuer certificate was rejected because its subject name did not match the issuer name of the current certificate. Only displayed when the -issuer_checks option is set." )
		}
	},

	{
		.code = X509_V_ERR_AKID_SKID_MISMATCH,
		.message = {
			.name = "X509_V_ERR_AKID_SKID_MISMATCH",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "dialog-error",
			.summary = N_( "Authority and subject key identifier mismatch" ),
			.body = N_( "The current candidate issuer certificate was rejected because its subject key identifier was present and did not match the authority key identifier current certificate. Only displayed when the -issuer_checks option is set." )
		}
	},

	{
		.code = X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH,
		.message = {
			.name = "X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "dialog-error",
			.summary = N_( "Authority and issuer serial number mismatch" ),
			.body = N_( "The current candidate issuer certificate was rejected because its issuer name and serial number was present and did not match the authority key identifier of the current certificate. Only displayed when the -issuer_checks option is set." )
		}
	},

	{
		.code = X509_V_ERR_KEYUSAGE_NO_CERTSIGN,
		.message = {
			.name = "X509_V_ERR_KEYUSAGE_NO_CERTSIGN",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "dialog-error",
			.summary = N_( "Key usage does not include certificate signing" ),
			.body = N_( "The current candidate issuer certificate was rejected because its keyUsage extension does not permit certificate signing." )
		}
	},

	{
		.code = -1,	
		.message = {
			.name = "FQDN_MISMATCH",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "security-low",
			.summary = N_( "Fully Qualified Domain Name (FQDN) mismatch" ),
			.body = N_( "The domain name in the SSL/TLS certificate does not match the host name requested." )
		},
	},

	{
		.code = -1,	
		.message = {
			.name = "NO_PEER_CERTIFICATE",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "dialog-error",
			.summary = N_( "No peer certificate" ),
			.body = N_( "Unable to get peer certificate for SSL/TLS connection." )
		},
	},

	{
		.code = -1,	
		.message = {
			.name = "SUBJECT_ALT_NAME_MISMATCH",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "security-low",
			.summary = N_( "Hostname from TLS/SSL certificate does not match" ),
			.body = N_( "The subjectAltName field in the TLS/SSL certificate does not match the requested hostname." )

		},
	},

	{
		.code = -1,	
		.message = {
			.name = "SSL_ILLEGAL_CERT_NAME",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "dialog-error",
			.summary = N_( "The TLS/SSL certificate name is ilegal" ),
			.body = N_( "There was a terminating zero before the end of TLS/SSL certificate name, this cannot match." )
		},
	},

	{
		.code = -1,	
		.message = {
			.name = "NO_FQDN_FROM_PEER",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "dialog-error",
			.summary = N_( "Unable to obtain common name from peer certificate" ),
			.body = N_( "The SSL/TLS certificate does not contains a common name." )

		},
	},

	{
		.code = -1,	
		.message = {
			.name = "FQDN_HOSTNAME_MISMATCH",
			.type = LIB3270_NOTIFY_SECURITY_LOW,
			.icon = "security-low",
			.summary = N_( "FQDN hostname mismatch in server certificate" ),
			.body = N_( "The common name from SSL/TLS certificate does not match the target hostname." )

		},
	}

 };

 LIB3270_INTERNAL const LIB3270_SSL_MESSAGE * openssl_message_from_code(long code) {

	size_t ix;

	if(code == -1) {

		static const LIB3270_SSL_MESSAGE invalid = {
			.name = "SSL_INVALID_CODE",
			.type = LIB3270_NOTIFY_CRITICAL,
			.icon = "dialog-error",
			.summary = N_( "The SSL error code was invalid" ),
			.body = N_( "Trying to get SSL message from invalid code '-1', this is an internal logic error" )
		};

		return &invalid;
	}

	for(ix = 0; ix < (sizeof(messages)/sizeof(messages[0])); ix++) {

		if(messages[ix].code == code) {
			return &messages[ix].message;
		}

	}

	return NULL;
 }

 LIB3270_INTERNAL const LIB3270_SSL_MESSAGE * openssl_message_from_name(const char *name) {
 
	size_t ix;

	for(ix = 0; ix < (sizeof(messages)/sizeof(messages[0])); ix++) {

		if(!strcasecmp(messages[ix].message.name,name)) {
			return &messages[ix].message;
		}

	}

	static const LIB3270_SSL_MESSAGE invalid = {
		.name = "SSL_INVALID_NAME",
		.type = LIB3270_NOTIFY_CRITICAL,
		.icon = "dialog-error",
		.summary = N_( "The SSL error name was invalid" ),
		.body = N_( "Trying to get SSL message using an invalid name, this is an internal logic error" )
	};

	return &invalid;
}
