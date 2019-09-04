/*
 * Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
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
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

#ifndef LIB3270_ACTIONS_H_INCLUDED

	#define LIB3270_ACTIONS_H_INCLUDED 1

#ifdef __cplusplus
	extern "C" {
#endif

 typedef struct _lib3270_action_entry
 {
    const char *name;					///< @brief Action name.
    const char *key;					///< @brief Default key (or NULL if no default).
	const char *icon;					///< @brief Icon name (from https://standards.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html)
    const char *label;					///< @brief Label (or NULL).
    const char *tooltip;				///< @brief Description (or NULL).
    int (*call)(H3270 *hSession);		///< @brief lib3270 associated method.
 } LIB3270_ACTION_ENTRY;

/**
 *
 * @brief Send an "Enter" action.
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_enter(H3270 *hSession);

/**
 *
 * @brief
 *
 * @param hSession	TN3270 Session handle.
 * @param keycode	Number of the pfkey to activate.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_pfkey(H3270 *hSession, int keycode);

/**
 *
 * @brief
 *
 * @param hSession	TN3270 Session handle.
 * @param keycode	Number of the pakey to activate.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_pakey(H3270 *hSession, int keycode);

/**
 *
 * @brief Cursor up 1 position.
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_cursor_up(H3270 *hSession);

/**
 *
 * @brief Cursor down 1 position.
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_cursor_down(H3270 *hSession);

/**
 *
 * @brief Cursor left 1 position.
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_cursor_left(H3270 *hSession);

/**
 *
 * @brief Cursor right 1 position.
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_cursor_right(H3270 *hSession);

/**
 *
 * @brief Cursor to first field on next line or any lines after that.
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_newline(H3270 *hSession);

/**
 *
 * @brief
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_kybdreset(H3270 *hSession);

/**
 *
 * @brief Clear AID key
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_clear(H3270 *hSession);

/**
 *
 * @brief
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_eraseinput(H3270 *hSession);

/**
 *
 * @brief
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_select_field(H3270 *hSession);

/**
 *
 * @brief
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_select_all(H3270 *hSession);

/**
 *
 * @brief
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_unselect(H3270 *hSession);

/**
 *
 * @brief
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_reselect(H3270 *hSession);

/**
 *
 * @brief Erase End Of Field Key.
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_eraseeof(H3270 *hSession);

/**
 *
 * @brief Erase End Of Line Key.
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_eraseeol(H3270 *hSession);

/**
 *
 * @brief
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_erase(H3270 *hSession);

/**
 *
 * @brief
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_delete(H3270 *hSession);

/**
 *
 * @brief DUP key
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_dup(H3270 *hSession);

/**
 *
 * @brief FM key
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_fieldmark(H3270 *hSession);

/**
 *
 * @brief 3270-style backspace.
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_backspace(H3270 *hSession);

/**
 *
 * @brief Cursor to previous word.
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_previousword(H3270 *hSession);

/**
 *
 * @brief Cursor to next unprotected word.
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_nextword(H3270 *hSession);

/**
 *
 * @brief Move the cursor to the first blank after the last nonblank in the field.
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_fieldend(H3270 *hSession);

/**
 *
 * @brief Move to first unprotected field on screen.
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_firstfield(H3270 *hSession);

/**
 *
 * @brief
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_nextfield(H3270 *hSession);

/**
 *
 * @brief Tab backward to previous field.
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_previousfield(H3270 *hSession);

/**
 *
 * @brief ATTN key, per RFC 2355.  Sends IP, regardless.
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_attn(H3270 *hSession);

/**
 *
 * @brief
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_break(H3270 *hSession);

/**
 *
 * @brief
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_paste_next(H3270 *hSession);

/**
 *
 * @brief Backspaces the cursor until it hits the front of a word (does a ^W).
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_deleteword(H3270 *hSession);

/**
 *
 * @brief Delete field key (does a ^U).
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_deletefield(H3270 *hSession);

/**
 *
 * @brief
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_sysreq(H3270 *hSession);

/**
 *
 * @brief
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_testpattern(H3270 *hSession);

/**
 *
 * @brief
 *
 * @param hSession	TN3270 Session handle.
 *
 * @return 0 if Ok, non zero if not (sets errno)
 *
 */
 LIB3270_EXPORT int lib3270_charsettable(H3270 *hSession);


/**
 *
 * @brief Get lib3270 action table.
 *
 * @return Array with all the supported actions.
 */
 LIB3270_EXPORT const LIB3270_ACTION_ENTRY * lib3270_get_action_table();

/**
 *
 * @brief Call lib3270 action by name.
 *
 * @param hSession	TN3270 Session handle.
 * @param name	Name of the action to call.
 *
 */
 LIB3270_EXPORT int lib3270_action(H3270 *hSession, const char *name);

#ifdef __cplusplus
	}
#endif

#endif // LIB3270_ACTIONS_H_INCLUDED
