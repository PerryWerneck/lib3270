/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) <2008> <Banco do Brasil S.A.>
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

/**
 *	@file popup.c
 *
 *	@brief A callback based popup dialog engine.
 *
 */

#include <config.h>
#include <winsock2.h>
#include <windows.h>

#include <lib3270/defs.h>
#include <lib3270/memory.h>
#include <private/popup.h>
#include <lib3270/win32.h>

LIB3270_INTERNAL int popup_win32_error(H3270 *hSession, int code, const LIB3270_POPUP *popup, unsigned char wait) {

	lib3270_autoptr(char) body = lib3270_win32_strerror(code);

	LIB3270_POPUP p = {
		.name		= popup->name,
		.type		= popup->type,
		.title		= popup->title,
		.summary	= popup->summary,
		.body		= body,
		.label		= popup->label
	};

	return lib3270_popup(hSession,&p,wait);

}


