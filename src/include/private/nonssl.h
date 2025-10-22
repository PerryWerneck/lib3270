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
 #include <private/network.h>

 /// @brief Setup non SSL session
 /// @param hSession The TN3270 session.
 /// @return 0 if suceeded, non zero if failed
 /// @retval ENOTSUP OpenSSL support is not available.
 LIB3270_INTERNAL int nonssl_setup(H3270 *hSession);

 /// @brief Start TLS/SSL negotiation on the given session.
 /// @param hSession The TN3270 session.
 /// @param complete Callback to be called when negotiation is complete.
 /// @return 0 if suceeded, non zero if failed
 LIB3270_INTERNAL int nonssl_start_tls(H3270 *hSession, void (*complete)(H3270 *hSession));

