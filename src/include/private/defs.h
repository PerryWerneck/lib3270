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
 
//
// Compiler-specific #defines.
//
// Reference: GLIBC gmacros.h
//
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)

	#define GNUC_UNUSED	__attribute__((__unused__))

#else

	#define unused
	#define GNUC_UNUSED
	#define printflike(s, f)

#endif

/// @brief Shorthand macros
#define CN	((char *) NULL)
#define PN	((XtPointer) NULL)
#define Replace(var, value) { lib3270_free(var); var = (value); };

/* These first definitions were cribbed from X11 -- but no X code is used. */
#define False 0
#define True 1

#ifdef __APPLE__
	typedef unsigned char Boolean;
#else
	typedef char Boolean;
#endif

LIB3270_INTERNAL int check_online_session(const H3270 *hSession);
LIB3270_INTERNAL int check_offline_session(const H3270 *hSession);

/// @brief Returns -1 if the session is invalid or not online (sets errno).
#define FAIL_IF_NOT_ONLINE(x) if(check_online_session(x)) return errno;

/// @brief Returns -1 if the session is invalid or online (sets errno).
#define FAIL_IF_ONLINE(x) if(check_offline_session(x)) return errno;

