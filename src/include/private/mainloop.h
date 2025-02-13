/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2025 Perry Werneck <perry.werneck@gmail.com>
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

 #include <config.h>
 #include <lib3270/defs.h>
 #include <lib3270.h>

 /// @brief Set the mainloop methods for the session.
 /// @param hSession The session to be set.
 /// @param gui Non-zero if the session is running in GUI mode.
 LIB3270_INTERNAL int lib3270_setup_mainloop(H3270 *hSession, int gui);

 // LIB3270_INTERNAL void	* lib3270_add_timer(unsigned long msec, H3270 *session, int (*fn)(H3270 *session, void *userdata), void *userdata);
 // LIB3270_INTERNAL void	  lib3270_remove_timer(H3270 *session, void *cookie);
