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

#ifndef LIB3270_TOGGLE_H_INCLUDED

#define LIB3270_TOGGLE_H_INCLUDED 1

#include <lib3270.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Toogles.
 */
typedef enum _lib3270_toggle_id {
	LIB3270_TOGGLE_MONOCASE,
	LIB3270_TOGGLE_CURSOR_BLINK,
	LIB3270_TOGGLE_SHOW_TIMING,
	LIB3270_TOGGLE_CURSOR_POS,
	LIB3270_TOGGLE_DS_TRACE,
	LIB3270_TOGGLE_LINE_WRAP,
	LIB3270_TOGGLE_BLANK_FILL,
	LIB3270_TOGGLE_SCREEN_TRACE,
	LIB3270_TOGGLE_EVENT_TRACE,
	LIB3270_TOGGLE_MARGINED_PASTE,
	LIB3270_TOGGLE_RECTANGLE_SELECT,
	LIB3270_TOGGLE_CROSSHAIR,
	LIB3270_TOGGLE_FULL_SCREEN,
	LIB3270_TOGGLE_INSERT,
	LIB3270_TOGGLE_SMART_PASTE,
	LIB3270_TOGGLE_BOLD,
	LIB3270_TOGGLE_KEEP_SELECTED,
	LIB3270_TOGGLE_UNDERLINE,					/**< @brief Show underline ? */
	LIB3270_TOGGLE_CONNECT_ON_STARTUP,
	LIB3270_TOGGLE_KP_ALTERNATIVE,              /**< @brief Keypad +/- move to next/previous field */
	LIB3270_TOGGLE_BEEP,						/**< @brief Beep on errors */
	LIB3270_TOGGLE_VIEW_FIELD,					/**< @brief View Field attribute */
	LIB3270_TOGGLE_ALTSCREEN,					/**< @brief auto resize on altscreen */
	LIB3270_TOGGLE_KEEP_ALIVE,					/**< @brief Enable network keep-alive with SO_KEEPALIVE */
	LIB3270_TOGGLE_NETWORK_TRACE,				/**< @brief Enable network in/out trace */
	LIB3270_TOGGLE_SSL_TRACE,					/**< @brief Enable security traces */

	// Deprecated.

	LIB3270_TOGGLE_RECONNECT,					/**< @brief Auto reconnect */

	LIB3270_TOGGLE_COUNT

} LIB3270_TOGGLE_ID;

/**
 * @brief Toggle types.
 *
 */
typedef enum _LIB3270_TOGGLE_TYPE {
	LIB3270_TOGGLE_TYPE_INITIAL,
	LIB3270_TOGGLE_TYPE_INTERACTIVE,
	LIB3270_TOGGLE_TYPE_ACTION,
	LIB3270_TOGGLE_TYPE_FINAL,
	LIB3270_TOGGLE_TYPE_UPDATE,

	LIB3270_TOGGLE_TYPE_USER

} LIB3270_TOGGLE_TYPE;


typedef struct _lib3270_toggle {
	LIB3270_PROPERTY_HEAD

	LIB3270_TOGGLE_ID	  id;			///< @brief Toggle ID.
	const char			  def;			///< @brief Default value.
	const char			* key;			///< @brief Default key (or NULL if no default).

} LIB3270_TOGGLE;

/**
 * @brief Get the toggle by name.
 *
 */
LIB3270_EXPORT const LIB3270_TOGGLE * lib3270_toggle_get_by_name(const char *name);

/**
 * @brief Get the toggle descriptors.
 *
 * @return Pointer to all available toggles.
 *
 */
LIB3270_EXPORT const LIB3270_TOGGLE * lib3270_get_toggles();

LIB3270_EXPORT const LIB3270_TOGGLE * LIB3270_DEPRECATED(lib3270_get_toggle_list());

/**
 * @brief get toggle state.
 *
 * @param hSession		Session handle.
 * @param ix			Toggle id.
 *
 * @return 0 if the toggle is disabled, non zero if enabled.
 *
 */
LIB3270_EXPORT unsigned char lib3270_get_toggle(const H3270 *hSession, LIB3270_TOGGLE_ID ix);

/**
 * @brief Set toggle state.
 *
 * @param hSession	Session handle.
 * @param ix		Toggle id.
 * @param value		New toggle state (non zero for true).
 *
 * @returns 0 if the toggle is already at the state, 1 if the toggle was changed; < 0 on error (sets errno).
 *
 * @retval -EINVAL	Invalid toggle id.
 *
 */
LIB3270_EXPORT int lib3270_set_toggle(H3270 *hSession, LIB3270_TOGGLE_ID ix, int value);

/**
 * @brief Translate a string toggle name to the corresponding value.
 *
 * @param name	Toggle name.
 *
 * @return Toggle ID or negative if it's invalid.
 *
 */
LIB3270_EXPORT LIB3270_TOGGLE_ID lib3270_get_toggle_id(const char *name);

/**
 * @brief Get the toggle name as string.
 *
 * @param id	Toggle id
 *
 * @return Constant string with the toggle name or "" if invalid.
 *
 */
LIB3270_EXPORT const char * lib3270_get_toggle_name(LIB3270_TOGGLE_ID ix);



/**
 * @brief Get a long description of the toggle.
 *
 * @return Constant string with the toggle description.
 *
 */
LIB3270_EXPORT const char * lib3270_get_toggle_description(LIB3270_TOGGLE_ID ix);

/**
 * @brief Get a summary description of the toggle (for menus).
 *
 * @return Constant string with the toggle summary.
 *
 */
LIB3270_EXPORT const char * lib3270_get_toggle_summary(LIB3270_TOGGLE_ID ix);

/**
 * @brief Get a short description of the toggle (for buttons).
 *
 * @return Constant string with the toggle label.
 *
 */
LIB3270_EXPORT const char * lib3270_get_toggle_label(LIB3270_TOGGLE_ID ix);

/**
 * @brief Revert toggle status.
 *
 * @param hSession	Session handle.
 * @param ix		Toggle id.
 *
 * @return 0 if the toggle is already at the state, 1 if the toggle was changed; < 0 on error (sets errno).
 *
 * @retval -EINVAL	Invalid toggle id.
 */
LIB3270_EXPORT int lib3270_toggle(H3270 *hSession, LIB3270_TOGGLE_ID ix);

LIB3270_EXPORT const void * lib3270_register_toggle_listener(H3270 *hSession, LIB3270_TOGGLE_ID tx, void (*func)(H3270 *, LIB3270_TOGGLE_ID, char, void *),void *data);
LIB3270_EXPORT int lib3270_unregister_toggle_listener(H3270 *hSession, LIB3270_TOGGLE_ID tx, const void *id);


/**
 * @brief Get toggle descriptor from ID.
 *
 * @param id	Toggle.id.
 *
 * @return The toggle descriptor or NULL if the ID is invalid.
 *
 */
LIB3270_EXPORT const LIB3270_TOGGLE * lib3270_toggle_get_from_id(LIB3270_TOGGLE_ID id);

LIB3270_EXPORT const char * lib3270_toggle_get_name(const LIB3270_TOGGLE *toggle);
LIB3270_EXPORT const char * lib3270_toggle_get_label(const LIB3270_TOGGLE *toggle);
LIB3270_EXPORT const char * lib3270_toggle_get_summary(const LIB3270_TOGGLE *toggle);
LIB3270_EXPORT const char * lib3270_toggle_get_description(const LIB3270_TOGGLE *toggle);


#ifdef __cplusplus
}
#endif

#endif /* LIB3270_TOGGLE_H_INCLUDED */
