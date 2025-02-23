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

/*
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

/**
 * @brief Win32 Registry functions.
 */

#include <config.h>
#include <winsock2.h>
#include <windows.h>
#include <lib3270.h>
#include <lib3270/memory.h>

LIB3270_EXPORT LSTATUS lib3270_win32_create_regkey(LPCSTR lpSubKey, REGSAM samDesired, PHKEY phkResult) {

	LSTATUS	rc;
	DWORD   disp;
	char	* path;

	if(lpSubKey)
		path = lib3270_strdup_printf(LIB3270_STRINGIZE_VALUE_OF(PRODUCT_NAME) "\\%s",(const char *) lpSubKey);
	else
		path = strdup(LIB3270_STRINGIZE_VALUE_OF(PRODUCT_NAME));

	rc = RegCreateKeyEx(HKEY_CURRENT_USER,path,0,NULL,REG_OPTION_NON_VOLATILE,samDesired,NULL,phkResult,&disp);
	if(rc != ERROR_SUCCESS)
		rc = RegCreateKeyEx(HKEY_LOCAL_MACHINE,path,0,NULL,REG_OPTION_NON_VOLATILE,samDesired,NULL,phkResult,&disp);

	lib3270_free(path);

	return rc;
}

LIB3270_EXPORT DWORD lib3270_win32_get_dword(HKEY hKey, const char *name, DWORD def) {
	DWORD val = def;
	DWORD cbData = sizeof(DWORD);

	DWORD dwRet = RegQueryValueEx(hKey, name, NULL, NULL, (LPBYTE) &val, &cbData);

	if(dwRet != ERROR_SUCCESS)
		return def;

	return val;
}

LIB3270_EXPORT LSTATUS lib3270_win32_set_string(LPCSTR module, LPCSTR keyname, LPCSTR value) {

	HKEY hKey = 0;
	LSTATUS status = lib3270_win32_create_regkey(module, KEY_CREATE_SUB_KEY|KEY_SET_VALUE, &hKey);

	if(status != ERROR_SUCCESS)
		return status;

	status = RegSetValueEx(hKey,keyname,0,REG_SZ,(const BYTE *) value,strlen(value)+1);

	RegCloseKey(hKey);

	return status;
}
