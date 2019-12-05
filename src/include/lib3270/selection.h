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
 * Este programa está nomeado como selection.h e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

 /**
  * @brief LIB3270 calls for managing selected area.
  *
  * @author perry.werneck@gmail.com
  *
  */

 #ifndef LIB3270_SELECTION_H_INCLUDED

	#define LIB3270_SELECTION_H_INCLUDED 1

	/**
	 * @brief Selection element
	 *
	 */
	typedef struct _lib3270_selection_element
	{
		unsigned char chr;			///< @brief Element character.

		struct
		{
			unsigned short	visual;	///< @brief Element colors & visual state. @see LIB3270_ATTR
			unsigned char	field;	///< @brief Field attribute. @see LIB3270_FIELD_ATTRIBUTE
		} attribute;

	} lib3270_selection_element;

	/**
	 * @brief A rectangle with informations about the selected area.
	 *
	 */
	typedef struct _lib3270_selection
	{
		/// @brief Cursor address.
		unsigned int cursor_address;

		/// @brief Clipboard rectangle.
		struct {
			unsigned int row;
			unsigned int col;
			unsigned int width;
			unsigned int height;
		} bounds;

		/// @brief Selection contents.
		lib3270_selection_element contents[1];

	} lib3270_selection;

	LIB3270_EXPORT int	  lib3270_unselect(H3270 *session);
	LIB3270_EXPORT void	  lib3270_select_to(H3270 *session, int baddr);
	LIB3270_EXPORT int	  lib3270_select_word_at(H3270 *session, int baddr);
	LIB3270_EXPORT int	  lib3270_select_field_at(H3270 *session, int baddr);
	LIB3270_EXPORT int	  lib3270_select_field(H3270 *session);
	LIB3270_EXPORT int	  lib3270_select_all(H3270 *session);

	/**
	 * @brief Get selection options.
	 *
	 * @see lib3270_get_selection_as_text
	 *
	 */
	typedef enum _LIB3270_SELECTION_OPTIONS {

		LIB3270_SELECTION_CUT				= 0x0001,		///< @brief Cut selected data (if available).
		LIB3270_SELECTION_ALL				= 0x0002,		///< @brief Get all data (the default is get only selected data).
//		LIB3270_SELECTION_UNPROTECTED_ONLY	= 0x0004,		///< @brief Get only unprotected contents.

	} LIB3270_SELECTION_OPTIONS;

	/**
	 * @brief "Paste" supplied string.
	 *
	 * @param h		Session handle.
	 * @param str	String to paste.
	 *
	 * @see lib3270_paste_next.
	 *
	 * @return 0 if suceeded, negative if faile, > 0 if there's more data.
	 *
	 * @retval 0		The entire string was pasted.
	 * @retval -EINVAL	Invalid argument.
	 * @retval -EPERM	Keyboard is locked.
	 *
	 */
	 LIB3270_EXPORT int lib3270_paste_text(H3270 *hSession, const unsigned char *str);

	/**
	 * @brief Paste remaining string.
	 *
	 * @param hSession	Session handle.
	 *
	 * @see lib3270_paste_text.
	 *
	 * @return Non 0 if there's more to paste.
	 *
	 */
	 LIB3270_EXPORT int lib3270_paste_next(H3270 *hSession);

	/**
	 * @brief Check if can paste next.
	 *
	 * @param hSession	Session handle.
	 *
	 * @see lib3270_paste_next.
	 *
	 * @return Non 0 if there's more to paste.
	 *
	 */
	LIB3270_EXPORT int lib3270_can_paste_next(const H3270 *hSession);

	/**
	 * @brief Move selected box 1 char in the selected direction.
	 *
	 * @param h		Session handle.
	 * @param dir	Direction to move
	 *
	 * @return 0 if the movement can be done, non zero if failed.
	 */
	 LIB3270_EXPORT int lib3270_move_selection(H3270 *h, LIB3270_DIRECTION dir);

	/**
	 * @brief Move selected box.
	 *
	 * @param h		Session handle.
	 * @param from	Address of origin position inside the selected buffer.
	 * @param to	Address of the new origin position.
	 *
	 * @return The new origin position.
	 *
	 */
	 LIB3270_EXPORT int lib3270_move_selected_area(H3270 *h, int from, int to);

	/**
	 * @brief Drag selected region.
	 *
	 * Move or resize selected box according to the selection flags.
	 *
	 * @param h			Session handle.
	 * @param flag		Selection flag.
	 * @param origin	Reference position (got from mouse button down or other move action).
	 * @param baddr		New position.
	 *
	 * @return The new reference position.
	 *
	 */
	 LIB3270_EXPORT int lib3270_drag_selection(H3270 *h, unsigned char flag, int origin, int baddr);

	/**
	 * @brief Gets the selected range of characters in the screen
	 *
	 * @param h		Session handle.
	 * @param start	return location for start of selection, as a character offset.
	 * @param end	return location for end of selection, as a character offset.
	 *
	 * @return Non 0 if selection is non-empty
	 *
	 */
	 LIB3270_EXPORT int lib3270_get_selection_bounds(H3270 *hSession, int *start, int *end);

	/**
	 * @brief Get the coordinates of a rectangle containing the selected region.
	 *
	 * @param hSession	Session handle.
	 * @param col		Pointer to last row.
	 * @param row		Pointer to first row.
	 * @param width		Pointer to first col.
	 * @param height	Pointer to last col.
	 *
	 * @return 0 if suceeds, error code if not (sets errno).
	 *
	 */
	LIB3270_EXPORT int lib3270_get_selection_rectangle(H3270 *hSession, unsigned int *row, unsigned int *col, unsigned int *width, unsigned int *height);

	/**
	 * @brief Create a new selection block.
	 *
	 * @param hSession	Session handle.
	 * @param cut		Non zero to clear selected contents.
	 * @param all		Non zero to get entire terminal, zero to get only the selected rectangle.
	 *
	 * @return NULL on error (sets errno), pointer to a rectangle containing the selected area (release it with lib3270_free).
	 *
	 */
	LIB3270_EXPORT lib3270_selection * lib3270_selection_new(H3270 *hSession, int cut, int all);

	LIB3270_EXPORT lib3270_selection * LIB3270_DEPRECATED(lib3270_get_selection(H3270 *hSession, int cut, int all));

	/**
	 * @brief Get the length of the selection block.
	 *
	 * @param selection	Selection block.
	 *
	 * @return The length of the selection block.
	 *
	 */
	LIB3270_EXPORT size_t lib3270_selection_get_length(const lib3270_selection *selection);

	/**
	 * @brief Get bitmasked flag for the current selection.
	 *
	 * Calculate flags to help drawing of the correct mouse pointer over a selection.
	 *
	 * @param h		Session handle.
	 * @param baddr	Position.
	 *
	 * @return bitmask for mouse pointer.
	 */
	 LIB3270_EXPORT unsigned char lib3270_get_selection_flags(H3270 *h, int baddr);

	/**
	 * @brief Get a string from required region.
	 *
	 * @param h			Session handle.
	 * @param start_pos	First char to get.
	 * @param end_pos	Last char to get.
	 * @param all		zero to get only selected chars.
	 *
	 * @return String with selected region (release it with free()
	 *
	 */
	 LIB3270_EXPORT char * lib3270_get_region(H3270 *h, int start_pos, int end_pos, unsigned char all);


	/**
	 * @brief Selects a range of characters in the screen.
	 *
	 * @param h				Session handle.
	 * @param start_offset	Start offset.
	 * @param end_offset :	End offset.
	 *
	 */
	 LIB3270_EXPORT int lib3270_select_region(H3270 *h, int start, int end);

	/**
	 * @brief Erase selected inputs.
	 *
	 * @param hSession	Session handle.
	 *
	 */
	LIB3270_EXPORT int lib3270_erase_selected(H3270 *hSession);

 #endif // LIB3270_SELECTION_H_INCLUDED
