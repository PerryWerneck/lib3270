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
 * Este programa está nomeado como mkfb.c e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

/**
 * @brief Utility to create the actions definition files.
 *
 */

#include <getopt.h>
#include <stdio.h>

#define DECLARE_LIB3270_ACTION( name, description ) \
		{ \
			description, \
			NULL, \
			"LIB3270_EXPORT int lib3270_" # name "(H3270 *hSession);" \
		},

#define DECLARE_LIB3270_CLEAR_SELECTION_ACTION( name, description ) \
		{ \
			description, \
			NULL, \
			"LIB3270_EXPORT int lib3270_" # name "(H3270 *hSession);" \
		},

#define DECLARE_LIB3270_KEY_ACTION( name, description ) \
		{ \
			description, \
			NULL, \
			"LIB3270_EXPORT int lib3270_" # name "(H3270 *hSession);" \
		},

#define DECLARE_LIB3270_CURSOR_ACTION( name, description ) \
		{ \
			description, \
			NULL, \
			"LIB3270_EXPORT int lib3270_cursor_" # name "(H3270 *hSession);" \
		},

#define DECLARE_LIB3270_FKEY_ACTION( name, description ) \
		{ \
			description, \
			"keycode\tNumber of the " #name " to activate.", \
			"LIB3270_EXPORT int lib3270_" # name "(H3270 *hSession, int keycode);" \
		},


static struct {
	const char *description;
	const char *args;
	const char *prototype;
} actions[] = {
	#include <lib3270/action_table.h>
};


int main(int argc, char *argv[]) {

	enum _format {
		FORMAT_HEADER
	} format = FORMAT_HEADER;

	size_t ix;

	char * outfile = "actions.h";

	//#pragma GCC diagnostic push
	//#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
	static struct option options[] = {
		{ "outfile",	required_argument,	0,	'o' },
		{ 0, 0, 0, 0}

	};
	//#pragma GCC diagnostic pop

	int long_index =0;
	int opt;
	while((opt = getopt_long(argc, argv, "o:", options, &long_index )) != -1) {

		switch(opt) {
		case 'o':	// Pidfile
			outfile = optarg;
			break;
		}

	}

	FILE *out = fopen(outfile,"w");

	fprintf(out,"%s\n",
		"/*\n"
		" * Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270\n"
		" * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a\n"
		" * aplicativos mainframe. Registro no INPI sob o nome G3270. Registro no INPI sob o nome G3270.\n"
		" *\n"
		" * Copyright (C) <2008> <Banco do Brasil S.A.>\n"
		" *\n"
		" * Este programa é software livre. Você pode redistribuí-lo e/ou modificá-lo sob\n"
		" * os termos da GPL v.2 - Licença Pública Geral  GNU,  conforme  publicado  pela\n"
		" * Free Software Foundation.\n"
		" *\n"
		" * Este programa é distribuído na expectativa de  ser  útil,  mas  SEM  QUALQUER\n"
		" * GARANTIA; sem mesmo a garantia implícita de COMERCIALIZAÇÃO ou  de  ADEQUAÇÃO\n"
		" * A QUALQUER PROPÓSITO EM PARTICULAR. Consulte a Licença Pública Geral GNU para\n"
		" * obter mais detalhes.\n"
		" *\n"
		" * Você deve ter recebido uma cópia da Licença Pública Geral GNU junto com este\n"
		" * programa; se não, escreva para a Free Software Foundation, Inc., 51 Franklin\n"
		" * St, Fifth Floor, Boston, MA  02110-1301  USA\n"
		" *\n"
		" * Contatos:\n"
		" *\n"
		" * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)\n"
		" * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)\n"
		" *\n"
		" */\n\n"
		"#ifndef LIB3270_ACTIONS_H_INCLUDED\n"
		"\n"
		"\n	#define LIB3270_ACTIONS_H_INCLUDED 1\n\n"
		"#ifdef __cplusplus\n"
		"	extern \"C\" {\n"
		"#endif\n\n"
	);

	if(format == FORMAT_HEADER)
	{

		for(ix = 0; ix < (sizeof(actions)/sizeof(actions[0])); ix++)
		{
			fprintf(out,
				"/**\n"
				" *\n"
				" * @brief %s\n"
				" *\n"
				" * @param hSession\tTN3270 Session handle.\n",
				actions[ix].description
			);

			if(actions[ix].args) {
				fprintf(out," * @param %s\n", actions[ix].args);
			}

			fprintf(out,
				" *\n"
				" * @return 0 if Ok, non zero if not (sets errno)\n"
			);

            fprintf(out," *\n */\n %s\n\n",actions[ix].prototype);
		}

		fprintf(out,
			"\n"
			"\n typedef struct _lib3270_action_entry"
			"\n {"
			"\n     const char *name;"
			"\n     const char *description;"
			"\n     int (*call)(H3270 *hSession);"
			"\n } LIB3270_ACTION_ENTRY;"
			"\n\n"
			"/**\n"
			" *\n"
			" * @brief Get lib3270 action table.\n"
			" *\n"
			" * @return Array with all the supported actions.\n"
			" */\n"
			" LIB3270_EXPORT const LIB3270_ACTION_ENTRY * lib3270_get_action_table();\n"
			"\n"
			"/**\n"
			" *\n"
			" * @brief Call lib3270 action by name.\n"
			" *\n"
			" * @param hSession\tTN3270 Session handle.\n"
			" * @param name\tName of the action to call.\n"
			" *\n"
			" */\n"
			" LIB3270_EXPORT int lib3270_action(H3270 *hSession, const char *name);\n\n"
			"#ifdef __cplusplus\n"
			"	}\n"
			"#endif\n"
			"\n#endif // LIB3270_ACTIONS_H_INCLUDED"
		);
	}

	fclose(out);

	return 0;
}


