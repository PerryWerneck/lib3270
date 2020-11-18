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

/// @brief Get CRL info from X509 cert.
///
/// References:
///
/// http://www.zedwood.com/article/cpp-check-crl-for-revocation


#include "private.h"
#include <utilc.h>

/*--[ Implement ]------------------------------------------------------------------------------------*/

LIB3270_STRING_ARRAY * lib3270_openssl_get_crls_from_peer(H3270 *hSession, X509 *cert) {

	//
	// Get Distribution points.
	//
	lib3270_autoptr(CRL_DIST_POINTS) dist_points = (CRL_DIST_POINTS *) X509_get_ext_d2i(cert, NID_crl_distribution_points, NULL, NULL);

	if(!dist_points) {
		trace_ssl(hSession,"The host certificate doesn't have CRL distribution points\n");
		return NULL;
	}

	LIB3270_STRING_ARRAY * uris = lib3270_string_array_new();

	size_t ix;
	for(ix = 0; ix < (size_t) sk_DIST_POINT_num(dist_points); ix++) {

		DIST_POINT *dp = sk_DIST_POINT_value(dist_points, ix);

		if(!dp->distpoint || dp->distpoint->type != 0)
			continue;

		GENERAL_NAMES *gens = dp->distpoint->name.fullname;

		int i;
		for (i = 0; i < sk_GENERAL_NAME_num(gens); i++) {
			int gtype;
			GENERAL_NAME *gen = sk_GENERAL_NAME_value(gens, i);
			ASN1_STRING *uri = GENERAL_NAME_get0_value(gen, &gtype);

			if(uri && gtype == GEN_URI)
			{
				int length = ASN1_STRING_length(uri);

#if (OPENSSL_VERSION_NUMBER >= 0x10100000L) // OpenSSL 1.1.0+
				const unsigned char * data = ASN1_STRING_get0_data(uri);
#else
				const unsigned char * data = ASN1_STRING_data(uri);
#endif // OpenSSL 1.1.0+

				if(data && length > 0)
				{
					lib3270_autoptr(char) uri = lib3270_malloc( ((size_t) length) + 1);
					strncpy(uri,(char *) data, (size_t) length);

					lib3270_autoptr(char) unescaped = lib3270_unescape(uri);
					lib3270_string_array_append(uris,unescaped);
				}

			}

		}

	}

	return uris;

}

