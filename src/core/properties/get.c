/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270. Registro no INPI sob o nome G3270.
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
#include <string.h>
#include <lib3270.h>
#include <lib3270/properties.h>
#include <utilc.h>

LIB3270_EXPORT const char * lib3270_property_get_name(const LIB3270_PROPERTY * property) {

	if(property && property->name)
		return property->name;

	return "";

}

LIB3270_EXPORT const char * lib3270_property_get_tooltip(const LIB3270_PROPERTY * property) {

	if(property) {

		if(property->description && *property->description)
			return dgettext(GETTEXT_PACKAGE,property->description);

		if(property->summary && *property->summary)
			return dgettext(GETTEXT_PACKAGE,property->summary);

	}

	return "";
}

LIB3270_EXPORT const char * lib3270_property_get_label(const LIB3270_PROPERTY * property) {

	if(property && property->label)
		return dgettext(GETTEXT_PACKAGE,property->label);

	return "";

}

LIB3270_EXPORT const char * lib3270_property_get_description(const LIB3270_PROPERTY * property) {

	if(property && property->description)
		return dgettext(GETTEXT_PACKAGE,property->description);

	return "";

}

LIB3270_EXPORT const char * lib3270_property_get_summary(const LIB3270_PROPERTY * property) {

	if(property && property->summary)
		return dgettext(GETTEXT_PACKAGE,property->summary);

	return "";

}

LIB3270_EXPORT const LIB3270_PROPERTY * lib3270_property_get_by_name(const char *name) {

	// Search string properties
	{
		const LIB3270_STRING_PROPERTY * property = lib3270_get_string_properties_list();

		while(property->name) {

			if(!lib3270_compare_alnum(name,property->name))
				return (const LIB3270_PROPERTY *) property;

			property++;

		}

	}

	// Search unsigned int properties.
	{
		const LIB3270_UINT_PROPERTY * property = lib3270_get_unsigned_properties_list();

		while(property->name) {

			if(!lib3270_compare_alnum(name,property->name))
				return (const LIB3270_PROPERTY *) property;

			property++;

		}

	}

	// Not found!
	errno = ENOENT;
	return NULL;
}
