/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2008 Banco do Brasil S.A.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

 #pragma once

 #include <lib3270/defs.h>

 /// @brief Get all text inside the terminal.
 /// @param h			Session Handle.
 /// @param offset	Start position (-1 to current cursor position).
 /// @param len		Text length or -1 to all text.
 /// @param lf		Line break char (0 to disable line breaks).
 /// @return Contents at position if available, or NULL if error (sets errno). Release it with lib3270_free()
 /// @exception ENOTCONN	Not connected to host.
 /// @exception EOVERFLOW	Invalid offset.
 ///
 LIB3270_EXPORT char * lib3270_get_string_at_address(H3270 *h, int offset, int len, char lf);

 /// @brief Get text at requested position
 /// @param h			Session Handle.
 /// @param row		Desired row.
 /// @param col		Desired col.
 /// @param len		Text length or -1 to all text.
 /// @param lf		Line break char (0 to disable line breaks).
 /// @return Contents at position if available, or NULL if error (sets errno). Release it with lib3270_free()
 /// @exception ENOTCONN	Not connected to host.
 /// @exception EOVERFLOW	Invalid position.
 LIB3270_EXPORT char * lib3270_get_string_at(H3270 *h, unsigned int row, unsigned int col, int len, char lf);

 /// @brief Check for text at requested position
 /// @param h			Session Handle.
 /// @param row		Desired row.
 /// @param col		Desired col.
 /// @param text		Text to check.
 /// @param lf		Line break char (0 to disable line breaks).
 /// @return Test result from strcmp
 LIB3270_EXPORT int lib3270_cmp_string_at(H3270 *h, unsigned int row, unsigned int col, const char *text, char lf);

 LIB3270_EXPORT int lib3270_cmp_string_at_address(H3270 *h, int baddr, const char *text, char lf);

 /// @brief Get contents of the field at position.
 /// @param h			Session Handle.
 /// @param baddr		Reference position.
 /// @return NULL if failed (sets errno), contents of the entire field if suceeds (release it with lib3270_free()).
 /// @exception ENOTCONN	Not connected to host.
 /// @exception EOVERFLOW	Invalid position.
 LIB3270_EXPORT char * lib3270_get_field_string_at(H3270 *h, int baddr);
