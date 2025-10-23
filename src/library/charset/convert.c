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

/**
 *	@file charset/convert.c
 *
 *	@brief This module handles ebc<->asc conversion.
 */

#include <internals.h>
#include <lib3270/charset.h>
#include <lib3270/log.h>
#include <lib3270/trace.h>
#include <private/session.h>

/*---[ Implement ]------------------------------------------------------------------------------------------------------------*/

LIB3270_EXPORT const char * lib3270_asc2ebc(H3270 *hSession, unsigned char *buffer, int sz) {
	int f;
	if(sz < 0)
		sz = strlen((const char *) buffer);

	if(sz > 0) {
		for(f=0; f<sz; f++)
			buffer[f] = asc2ebc(hSession,(char *) buffer+f);
	}

	return (const char *) buffer;
}

LIB3270_EXPORT const char * lib3270_ebc2asc(H3270 *hSession, unsigned char *buffer, int sz) {
	int f;
	if(sz < 0)
		sz = strlen((const char *) buffer);

	if(sz > 0) {
		for(f=0; f<sz; f++)
			buffer[f] = ebc2asc(hSession,buffer[f])[0];
	}

	return (const char *) buffer;
}

