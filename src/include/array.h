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

/**
 *	@file array.h
 *	@brief Global declarations for array.c.
 */

#ifndef LIB3270_ARRAY_H_INCLUDED

	#define LIB3270_ARRAY_H_INCLUDED

	#include <stddef.h>
	#include <lib3270.h>

	typedef struct _lib3270_string_array
	{
		size_t length;	///< @brief Number of elements.
		const char **str;
	} LIB3270_STRING_ARRAY;

	LIB3270_INTERNAL LIB3270_STRING_ARRAY * lib3270_string_array_new(void);
	LIB3270_INTERNAL void lib3270_string_array_free(LIB3270_STRING_ARRAY *object);
	LIB3270_INTERNAL void lib3270_string_array_append(LIB3270_STRING_ARRAY *object, const char *str);
	LIB3270_INTERNAL void lib3270_string_array_append_with_length(LIB3270_STRING_ARRAY *array, const char *str, size_t length);

	LIB3270_INTERNAL void lib3270_autoptr_cleanup_LIB3270_STRING_ARRAY(LIB3270_STRING_ARRAY **ptr);

#endif // LIB3270_ARRAY_H_INCLUDED
