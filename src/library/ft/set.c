/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright 1995, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006 by Paul Mattes.
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

#include <config.h>
#include <lib3270.h>
#include <private/filetransfer.h>
#include <lib3270/log.h>
#include <internals.h>

/*---[ Implement ]-------------------------------------------------------------------------------------------------------*/

LIB3270_EXPORT int	lib3270_ft_set_lrecl(H3270 *hSession, int lrecl) {

	if(!hSession->ft)
		return errno = EINVAL;

	hSession->ft->lrecl	= lrecl;

	return 0;
}

LIB3270_EXPORT int	lib3270_ft_set_blksize(H3270 *hSession, int blksize) {

	if(!hSession->ft)
		return errno = EINVAL;

	hSession->ft->blksize = blksize;

	return 0;
}

LIB3270_EXPORT int	lib3270_ft_set_primspace(H3270 *hSession, int primspace) {

	if(!hSession->ft)
		return errno = EINVAL;

	hSession->ft->primspace	= primspace;

	return 0;
}

LIB3270_EXPORT int	lib3270_ft_set_secspace(H3270 *hSession, int secspace) {

	if(!hSession->ft)
		return errno = EINVAL;

	hSession->ft->secspace = secspace;

	return 0;
}

LIB3270_EXPORT int lib3270_ft_set_options(H3270 *hSession, LIB3270_FT_OPTION options) {

	if(!hSession->ft)
		return errno = EINVAL;

	hSession->ft->ascii_flag	= (options & LIB3270_FT_OPTION_ASCII)	? 1 : 0;
	hSession->ft->cr_flag   	= (options & LIB3270_FT_OPTION_CRLF)	? 1 : 0;
	hSession->ft->remap_flag	= (options & LIB3270_FT_OPTION_REMAP)	? 1 : 0;
	hSession->ft->unix_text		= (options & LIB3270_FT_OPTION_UNIX)	? 1 : 0;
	hSession->ft->flags			|= options;

	return 0;
}
