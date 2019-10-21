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
 * Este programa está nomeado como array.c e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */


/**
 * @brief Handle text arrays.
 */

 #include <lib3270.h>
 #include <lib3270/log.h>
 #include <array.h>
 #include <string.h>

/*---[ Implement ]------------------------------------------------------------------------------------------------------------*/

LIB3270_STRING_ARRAY * lib3270_string_array_new(void)
{
	LIB3270_STRING_ARRAY * array = lib3270_malloc(sizeof(LIB3270_STRING_ARRAY));
	memset(array,0,sizeof(LIB3270_STRING_ARRAY));

	return array;
}

void lib3270_string_array_free(LIB3270_STRING_ARRAY *array)
{
	size_t ix;

	if(array)
	{
		for(ix = 0; ix < array->length; ix++)
			lib3270_free((char *) array->str[ix]);

		lib3270_free(array->str);
		lib3270_free(array);
	}
}

static void lib3270_string_array_realloc(LIB3270_STRING_ARRAY *array)
{
	if(array->str)
	{
		array->str = lib3270_realloc(array->str,(array->length + 1) * sizeof(char *));
	}
	else
	{
		array->str = lib3270_malloc(sizeof(char *));
		array->length = 0; // Just in case.
	}

}

void lib3270_string_array_append(LIB3270_STRING_ARRAY *array, const char *str)
{
	lib3270_string_array_realloc(array);
	array->str[array->length++] = strdup(str);
}

void lib3270_string_array_append_with_length(LIB3270_STRING_ARRAY *array, const char *str, size_t length)
{
	lib3270_string_array_realloc(array);

	char * buffer = lib3270_malloc(length+1);
	memcpy(buffer,str,length);
	buffer[length] = 0;

	array->str[array->length++] = buffer;
}

void lib3270_autoptr_cleanup_LIB3270_STRING_ARRAY(LIB3270_STRING_ARRAY **ptr)
{
	lib3270_string_array_free(*ptr);
}

