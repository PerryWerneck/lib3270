/*
 * "Software PW3270, desenvolvido com base nos códigos fontes do WC3270  e  X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270.
 *
 * Copyright (C) <2008> <Banco do Brasil S.A.>
 *
 * Este programa é software livre. Você pode redistribuí-lo e/ou modificá-lo sob
 * os termos da GPL v.2 - Licença Pública Geral  ',  conforme  publicado  pela
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
 * erico.mendonca@gmail.com	(Erico Mascarenhas de Mendonça)
 *
 */

/**
 * @brief Select network methods.
 *
 */

#error deprecated

#include <config.h>
#include <lib3270.h>
#include <lib3270/log.h>
#include <lib3270/trace.h>
// #include <networking.h>
#include <string.h>

/*--[ Implement ]------------------------------------------------------------------------------------*/

/*
char * lib3270_set_network_module_from_url(H3270 *hSession, char *url) {

	static const struct {
		const char *scheme;					///< @brief URL scheme for module.
		void (*activate)(H3270 *hSession);	///< @brief Selection method.
	} modules[] = {

		{ "tn3270://",	lib3270_set_default_network_module	},

#ifdef HAVE_LIBSSL

		{ "tn3270s://",	lib3270_set_libssl_network_module		},

		// Compatibility schemes.
		{ "L://",		lib3270_set_libssl_network_module	},
		{ "L:",			lib3270_set_libssl_network_module	},

#endif // HAVE_LIBSSL

	};

	size_t ix;

	for(ix=0; ix < (sizeof(modules)/sizeof(modules[0])); ix++) {

		size_t len = strlen(modules[ix].scheme);
		if(!strncasecmp(url,modules[ix].scheme,len)) {
			modules[ix].activate(hSession);
			return url+len;
		}

	}

	debug("Unable to parse '%s'",url);
	return NULL;
}
*/
