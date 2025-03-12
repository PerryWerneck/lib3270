/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2023 Perry Werneck <perry.werneck@gmail.com>
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
 #include <internals.h>
 #include <lib3270.h>
 #include "3270ds.h"

/**
 * @brief Get field address.
 *
 * @return Negative on error(sets errno) or field address.
 *
 */
 LIB3270_EXPORT int lib3270_get_field_start(H3270 *hSession, int baddr) {
	int sbaddr;

	if(check_online_session(hSession))
		return - errno;

	if (!hSession->formatted)
		return - (errno = ENOTSUP);

	if(baddr < 0)
		baddr = hSession->cursor_addr;

	sbaddr = baddr;
	do {
		if(hSession->ea_buf[baddr].fa)
			return baddr;
		DEC_BA(baddr);
	} while (baddr != sbaddr);

	return -1;

 }

 LIB3270_EXPORT int lib3270_is_formatted(const H3270 *hSession) {
	if(check_online_session(hSession))
		return 0;
	return hSession->formatted ? 1 : 0;
 }

 LIB3270_EXPORT int lib3270_get_field_len(H3270 *hSession, int baddr) {
	int saddr;
	int addr;
	int width = 0;

	if(check_online_session(hSession))
		return - errno;

	if (!hSession->formatted)
		return - (errno = ENOTSUP);

	if(baddr < 0)
		baddr = hSession->cursor_addr;

	addr = lib3270_field_addr(hSession,baddr);
	if(addr < 0)
		return addr;

	saddr = addr;
	INC_BA(addr);
	do {
		if(hSession->ea_buf[addr].fa)
			return width;
		INC_BA(addr);
		width++;
	} while (addr != saddr);

	return -(errno = ENODATA);
 }

 LIB3270_EXPORT int lib3270_field_addr(const H3270 *hSession, int baddr) {

	int sbaddr;

	if(!lib3270_is_connected(hSession))
		return -(errno = ENOTCONN);

	if(!hSession->formatted)
		return -(errno = ENOTSUP);

	if(baddr < 0)
		baddr = lib3270_get_cursor_address(hSession);

	if( (unsigned int) baddr > lib3270_get_length(hSession))
		return -(errno = EOVERFLOW);

	sbaddr = baddr;
	do {
		if(hSession->ea_buf[baddr].fa)
			return baddr;
		DEC_BA(baddr);
	} while (baddr != sbaddr);

	return -(errno = ENODATA);
 }

 LIB3270_EXPORT LIB3270_FIELD_ATTRIBUTE lib3270_get_field_attribute(H3270 *hSession, int baddr) {
	int sbaddr;

	FAIL_IF_NOT_ONLINE(hSession);

	if(!hSession->formatted) {
		errno = ENOTCONN;
		return LIB3270_FIELD_ATTRIBUTE_INVALID;
	}

	if(baddr < 0)
		baddr = lib3270_get_cursor_address(hSession);

	sbaddr = baddr;
	do {
		if(hSession->ea_buf[baddr].fa)
			return (LIB3270_FIELD_ATTRIBUTE) hSession->ea_buf[baddr].fa;

		DEC_BA(baddr);
	} while (baddr != sbaddr);

	errno = EINVAL;
	return LIB3270_FIELD_ATTRIBUTE_INVALID;

 }

/**
 * @brief Get the length of the field at given buffer address.
 *
 * @param hSession	Session handle.
 * @param addr		Buffer address of the field.
 *
 * @return field length or negative if invalid or not connected (sets errno).
 *
 */
 int lib3270_field_length(H3270 *hSession, int baddr) {
	int saddr;
	int addr;
	int width = 0;

	addr = lib3270_field_addr(hSession,baddr);
	if(addr < 0)
		return addr;

	saddr = addr;
	INC_BA(addr);
	do {
		if(hSession->ea_buf[addr].fa)
			return width;
		INC_BA(addr);
		width++;
	} while (addr != saddr);

	return -(errno = EINVAL);

 }

/**
 * @brief Find the field attribute for the given buffer address.
 *
 * @return Field attribute.
 *
 */
 unsigned char get_field_attribute(H3270 *hSession, int baddr) {
	baddr = lib3270_field_addr(hSession,baddr);
	if(baddr < 0)
		return 0;
	return hSession->ea_buf[baddr].fa;
 }

/**
 * @brief Find the next unprotected field.
 *
 * @param hSession	Session handle.
 * @param baddr0	Search start addr (-1 to use current cursor position).
 *
 * @return address following the unprotected attribute byte, or 0 if no nonzero-width unprotected field can be found, negative if failed.
 *
 */
 LIB3270_EXPORT int lib3270_get_next_unprotected(H3270 *hSession, int baddr0) {
	register int baddr, nbaddr;

	FAIL_IF_NOT_ONLINE(hSession);

	if(!hSession->formatted)
		return -(errno = ENOTSUP);

	if(baddr0 < 0)
		baddr0 = hSession->cursor_addr;

	nbaddr = baddr0;
	do {
		baddr = nbaddr;
		INC_BA(nbaddr);
		if(hSession->ea_buf[baddr].fa &&!FA_IS_PROTECTED(hSession->ea_buf[baddr].fa) &&!hSession->ea_buf[nbaddr].fa)
			return nbaddr;
	} while (nbaddr != baddr0);

	return 0;
 }

 LIB3270_EXPORT int lib3270_get_is_protected_at(const H3270 *h, unsigned int row, unsigned int col) {
	return lib3270_get_is_protected(h, lib3270_translate_to_address(h,row,col));
 }

 LIB3270_EXPORT int lib3270_get_is_protected(const H3270 *hSession, int baddr) {

	FAIL_IF_NOT_ONLINE(hSession);

	if(baddr < 0)
		baddr = hSession->cursor_addr;

	int faddr = lib3270_field_addr(hSession,baddr);

	return FA_IS_PROTECTED(hSession->ea_buf[faddr].fa) ? 1 : 0;
 }

 LIB3270_EXPORT int lib3270_is_protected(H3270 *h, unsigned int baddr) {
	return lib3270_get_is_protected(h, baddr);
 }
