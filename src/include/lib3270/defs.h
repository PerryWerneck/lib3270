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

 #if defined (__GNUC__) || defined (__clang__)
	#define LIB3270_AUTOPTR_FUNC_NAME(TypeName) lib3270_autoptr_cleanup_##TypeName
	#define lib3270_autoptr(TypeName) TypeName * __attribute__ ((__cleanup__(LIB3270_AUTOPTR_FUNC_NAME(TypeName))))
	#define lib3270_auto_cleanup(TypeName) TypeName __attribute__ ((__cleanup__(LIB3270_AUTOPTR_FUNC_NAME(TypeName))))
 #endif // __GNUC__

 #if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1) || defined (__clang__)
	#define LIB3270_DEPRECATED(func) func __attribute__ ((deprecated))
 #elif defined(_WIN32) && !defined(_MSC_VER)
	#define LIB3270_DEPRECATED(func) __declspec(deprecated) func
 #else
	#define LIB3270_DEPRECATED(func) func
 #endif /* __GNUC__ */

 #if defined(__GNUC__)
	#define LIB3270_GNUC_FORMAT(s,f) __attribute__ ((__format__ (__printf__, s, f)))
	#define LIB3270_GNUC_NULL_TERMINATED __attribute__((__sentinel__))
 #elif defined(_WIN32) && !defined(_MSC_VER)
	#define LIB3270_GNUC_NULL_TERMINATED __attribute__((__sentinel__))
	#define LIB3270_GNUC_FORMAT(s,f)
 #else
	#define LIB3270_GNUC_NULL_TERMINATED
	#define LIB3270_GNUC_FORMAT(s,f)
 #endif

 #if defined( ANDROID )
	#define LIB3270_INTERNAL	extern __attribute__((visibility("hidden")))
	#define LIB3270_EXPORT		extern __attribute__((visibility("hidden")))
 #elif defined(_WIN32) || defined(_MSC_VER)
	#define LIB3270_INTERNAL	extern
	#define LIB3270_EXPORT		extern __declspec (dllexport)
 #elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x550)
	#define LIB3270_INTERNAL	__hidden extern
	#define LIB3270_EXPORT		extern
 #elif defined(__GNUC__) || defined(HAVE_GNUC_VISIBILITY)
	#define LIB3270_INTERNAL	__attribute__((visibility("hidden"))) extern
	#define LIB3270_EXPORT		__attribute__((visibility("default"))) extern
 #else
	#define LIB3270_INTERNAL
	#define LIB3270_EXPORT
 #endif

 typedef struct _h3270		H3270;
 typedef struct _h3270ft	H3270FT;
