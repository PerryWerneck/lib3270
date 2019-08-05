/*
 * "Software G3270, desenvolvido com base nos códigos fontes do WC3270  e  X3270
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
 * Este programa está nomeado como toggle.h e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas de Mendonça)
 *
 */

#ifndef LIB3270_TOGGLE_H_INCLUDED

	#define LIB3270_TOGGLE_H_INCLUDED 1

	#include <lib3270.h>

	/**
	 * @brief get toggle state.
	 *
	 * @param h		Session handle.
	 * @param ix	Toggle id.
	 *
	 * @return 0 if the toggle is disabled, non zero if enabled.
	 *
	 */
	LIB3270_EXPORT unsigned char lib3270_get_toggle(H3270 *h, LIB3270_TOGGLE ix);

	/**
	 * @brief Set toggle state.
	 *
	 * @param h		Session handle.
	 * @param ix	Toggle id.
	 * @param value	New toggle state (non zero for true).
	 *
	 * @returns 0 if the toggle is already at the state, 1 if the toggle was changed; < 0 on error (sets errno).
	 */
	LIB3270_EXPORT int lib3270_set_toggle(H3270 *h, LIB3270_TOGGLE ix, int value);

	/**
	 * @brief Translate a string toggle name to the corresponding value.
	 *
	 * @param name	Toggle name.
	 *
	 * @return Toggle ID or -1 if it's invalid.
	 *
	 */
	LIB3270_EXPORT LIB3270_TOGGLE lib3270_get_toggle_id(const char *name);

	/**
	 * @brief Get the toggle name as string.
	 *
	 * @param id	Toggle id
	 *
	 * @return Constant string with the toggle name or "" if invalid.
	 *
	 */
	LIB3270_EXPORT const char * lib3270_get_toggle_name(LIB3270_TOGGLE ix);

	/**
	 * @brief Get a long description of the toggle.
	 *
	 * @return Constant string with the toggle description.
	 *
	 */
	LIB3270_EXPORT const char * lib3270_get_toggle_description(LIB3270_TOGGLE ix);

	/**
	 * @brief Get a short description of the toggle (for menus).
	 *
	 * @return Constant string with the toggle label.
	 *
	 */
	LIB3270_EXPORT const char * lib3270_get_toggle_label(LIB3270_TOGGLE ix);

	/**
	 * @brief Revert toggle status.
	 *
	 * @param h		Session handle.
	 * @param ix	Toggle id.
	 *
	 * @return Toggle status.
	 */
	LIB3270_EXPORT int lib3270_toggle(H3270 *h, LIB3270_TOGGLE ix);

	LIB3270_EXPORT void lib3270_set_session_id(H3270 *hSession, char id);
	LIB3270_EXPORT char lib3270_get_session_id(H3270 *hSession);

#endif /* LIB3270_TOGGLE_H_INCLUDED */
