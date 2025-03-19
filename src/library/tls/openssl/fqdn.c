/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2025 Banco do Brasil S.A.
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
 #include <private/openssl.h>
 #include <private/intl.h>
 #include <private/trace.h>
 
 #include <arpa/inet.h>
 #include <openssl/x509v3.h>
 #include <openssl/asn1.h>
 #include <openssl/ssl.h>
 #include <openssl/err.h>

 LIB3270_INTERNAL const LIB3270_SSL_MESSAGE * openssl_check_fqdn(H3270 *hSession, X509 *server_cert, const char *hostname) {
	// 
	// Adapted from libest implementation, originally derived from curl.
	//
	// cURL file name is ./lib/ssluse.c, function: verifyhost()
	//
	// Quote from RFC2818 section 3.1 "Server Identity"
	//
	// If a subjectAltName extension of type dNSName is present, that MUST
	// be used as the identity. Otherwise, the (most specific) Common Name
	// field in the Subject field of the certificate MUST be used. Although
	// the use of the Common Name is existing practice, it is deprecated and
	// Certification Authorities are encouraged to use the dNSName instead.
	//
	// Matching is performed using the matching rules specified by
	// [RFC2459].  If more than one identity of a given type is present in
	// the certificate (e.g., more than one dNSName name, a match in any one
	// of the set is considered acceptable.) Names may contain the wildcard
	// character * which is considered to match any single domain name
	// component or component fragment. E.g., *.a.com matches foo.a.com but
	// not bar.foo.a.com. f*.com matches foo.com but not bar.com.
	//
	// In some cases, the URI is specified as an IP address rather than a
	// hostname. In this case, the iPAddress subjectAltName must be present
	// in the certificate and must exactly match the IP in the URI.
	//
    int matched = -1;     // -1 is no alternative match yet, 1 means match and 0 means mismatch
    size_t addrlen = 0;
	STACK_OF(GENERAL_NAME) * altnames;
	struct in6_addr addr_v6;
    struct in_addr addr_v4;
    int addr_is_v4 = 0;
    int addr_is_v6 = 0;
    int rv;
    int numalts;
    int i, j;
    int diff;
    const char *altptr; 
    size_t altlen; 
    unsigned char *nulstr; 
    unsigned char *peer_CN; 
    X509_NAME *name; 
    ASN1_STRING *tmp;
    const GENERAL_NAME *check; 
	const LIB3270_SSL_MESSAGE *message = NULL;

    //
    // Attempt to resolve host name to v4 address 
    //
    rv = inet_pton(AF_INET, hostname, &addr_v4);
    if (rv) {
		addr_is_v4 = 1;
        addrlen = sizeof(struct in_addr);
    } else {
		//
		// Try to see if hostname resolves to v6 address
		//
		rv = inet_pton(AF_INET6, hostname, &addr_v6);
		if (rv) {
			addr_is_v6 = 1;
			addrlen = sizeof(struct in6_addr);
		}
    }

    // get a "list" of alternative names
    altnames = X509_get_ext_d2i(server_cert, NID_subject_alt_name, NULL, NULL);

    if (altnames) {
        // get amount of alternatives, RFC2459 claims there MUST be at least
    	// one, but we don't depend on it...
        numalts = sk_GENERAL_NAME_num(altnames);
        trace_ssl(hSession,"Found %d SubjectAlternateNames\n", numalts);

        // loop through all alternatives while none has matched
        for (i = 0; (i < numalts) && (matched != 1); i++) {
            // get a handle to alternative name number i
            check = sk_GENERAL_NAME_value(altnames, i);

            // get data and length
            altptr = (char *)ASN1_STRING_get0_data(check->d.ia5);
            altlen = (size_t)ASN1_STRING_length(check->d.ia5);

            switch (check->type) {
            case GEN_DNS: // name/pattern comparison

                trace_ssl(hSession,"Checking FQDN against SAN %s\n", altptr);

                // The OpenSSL man page explicitly says: "In general it cannot be
                // assumed that the data returned by ASN1_STRING_data() is null
                // terminated or does not contain embedded nulls." But also that
                // "The actual format of the data will depend on the actual string
                // type itself: for example for and IA5String the data will be ASCII"
				//
                // Gisle researched the OpenSSL sources:
                // "I checked the 0.9.6 and 0.9.8 sources before my patch and
                // it always 0-terminates an IA5String."
                //
                if ((altlen == strnlen(altptr, 255)) &&
                    // if this isn't true, there was an embedded zero in the name
                    // string and we cannot match it.
                    cert_hostcheck(altptr, hostname)) {
                    matched = 1;
                } else{
                    matched = 0;
                }
                break;

            case GEN_IPADD: 
				// IP address comparison
                // compare alternative IP address if the data chunk is the same size
            	// our server IP address is
				trace_ssl(hSession,"Checking IP address against SAN\n");

                if (addr_is_v4) {
                    diff = memcmp(altptr, &addr_v4, altlen);
                } else if (addr_is_v6) {
                    diff = memcmp(altptr, &addr_v6, altlen);
                } else {
                	//  Should never get here...so force matched to be 0
	           		diff = -1; 
	 			}

                if ((addr_is_v4) && (altlen == addrlen) && !diff) {
                    matched = 1;
                } else if ((addr_is_v6) && (altlen == addrlen) && !diff) {
                    matched = 1;
                } else{
                    matched = 0;
                }
                break;
            }
        }
        GENERAL_NAMES_free(altnames);
    }

    if (matched == 1) {
 
		// an alternative name matched the server hostname
        trace_ssl(hSession,"subjectAltName: %s matched\n", hostname);
    
	} else if (matched == 0) {

		// an alternative name field existed, but didn't match and then we MUST fail
        trace_ssl(hSession,"subjectAltName does not match %s\n", hostname);
		message = openssl_message_from_name("SUBJECT_ALT_NAME_MISMATCH");

	} else  {

		// we have to look to the last occurrence of a commonName in the distinguished one to get the most significant one.
        i = -1;

		// The following is done because of a bug in 0.9.6b
        nulstr = (unsigned char*)"";
        peer_CN = nulstr;

        name = X509_get_subject_name(server_cert);
        if (name) {
            while ((j = X509_NAME_get_index_by_NID(name, NID_commonName, i)) >= 0) {
                i = j;
            }
        }

        // we have the name entry and we will now convert this to a string
    	// that we can use for comparison. Doing this we support BMPstring,
        // UTF8 etc.

        if (i >= 0) {

            tmp = X509_NAME_ENTRY_get_data(X509_NAME_get_entry(name, i));

            // In OpenSSL 0.9.7d and earlier, ASN1_STRING_to_UTF8 fails if the input
            // is already UTF-8 encoded. We check for this case and copy the raw
            // string manually to avoid the problem. This code can be made
            // conditional in the future when OpenSSL has been fixed. Work-around
            // brought by Alexis S. L. Carvalho.

            if (tmp) {
                if (ASN1_STRING_type(tmp) == V_ASN1_UTF8STRING) {
                    j = ASN1_STRING_length(tmp);
                    if (j >= 0) {
                        peer_CN = malloc(j + 1);
                        if (peer_CN) {
			    			memcpy(peer_CN, (char *)ASN1_STRING_get0_data(tmp), j);
                            peer_CN[j] = '\0';
                        }
                    }
                }else  { 
					// not a UTF8 name
                    j = ASN1_STRING_to_UTF8(&peer_CN, tmp);
                }
                if (peer_CN && (strnlen((char*)peer_CN, 255) != j)) {
                    // there was a terminating zero before the end of string, this cannot match and we return failure!
                    trace_ssl(hSession,"SSL: illegal cert name field\n");
    				message = openssl_message_from_name("SSL_ILLEGAL_CERT_NAME");
			
                }
            }
        }

        if (peer_CN == nulstr) {
            peer_CN = NULL;
        }
		/*
		else{
            // convert peer_CN from UTF8
        }
		*/

        if (message) {
            // error already detected, pass through

		} else if (!peer_CN) {

			trace_ssl(hSession,"unable to obtain common name from peer certificate\n");
			message = openssl_message_from_name("NO_FQDN_FROM_PEER");

		} else if (!cert_hostcheck((const char*)peer_CN, hostname)) {

			trace_ssl(hSession,"FQDN hostname mismatch in server certificate, '%s' does not match target host name '%s'\n", peer_CN, hostname);
		
			message = openssl_message_from_name("FQDN_HOSTNAME_MISMATCH");

		} else  {

			trace_ssl(hSession,"common name: %s (matched)\n", peer_CN);

		}

        if (peer_CN) {
            free(peer_CN);
        }
    }

    return message;

 }

