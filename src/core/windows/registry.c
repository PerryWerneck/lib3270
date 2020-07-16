/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270. Registro no INPI sob
 * o nome G3270.
 *
 * Copyright (C) <2008> <Banco do Brasil S.A.>
 *
 * Este programa é software livre. Você pode redistribuí-lo e/ou modificá-lo sob
 * os termos da GPL v.2 - Licença Pública Geral  GNU,  conforme  publicado  pela
 * Free Software Foundation.
 *
 * Este programa é distribuído na expectativa de  ser  útil,  mas  SEM  QUALQUER
 * GARANTIA; sem mesmo a garantia implícita de COMERCIALIZAÇÃO ou  de  ADEQUAÇÃO
 * A QUALQUER PROPÓSITO EM PARTICULAR. Consulte a Licença Pública Geral GNU para
 * obter mais detalhes.
 *
 * Você deve ter recebido uma cópia da Licença Pública Geral GNU junto com este
 * programa; se não, escreva para a Free Software Foundation, Inc., 51 Franklin
 * St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Este programa está nomeado como - e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

/**
 * @brief Win32 Registry functions.
 */

#include <winsock2.h>
#include <windows.h>
#include <lib3270.h>

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

LIB3270_EXPORT DWORD lib3270_win32_get_dword(HKEY hKey, const char *name, DWORD def)
{
	DWORD val = def;
	DWORD cbData = sizeof(DWORD);

	DWORD dwRet = RegQueryValueEx(hKey, name, NULL, NULL, (LPBYTE) &val, &cbData);

	if(dwRet != ERROR_SUCCESS)
		return def;

	return val;
}
