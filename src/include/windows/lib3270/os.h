/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270.
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
 * Este programa está nomeado como lib3270.h e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

#error deprecetaed

/**
 * @brief TN3270 WIN32 API definitions.
 *
 * @author perry.werneck@gmail.com
 *
 */


#pragma once

#include <winsock2.h>
#include <windows.h>
#include <lib3270.h>

#ifdef __cplusplus
extern "C" {
#endif

LIB3270_EXPORT void lib3270_autoptr_cleanup_HKEY(HKEY *hKey);

LIB3270_EXPORT const char	* lib3270_win32_strerror(int e);
LIB3270_EXPORT const char	* lib3270_win32_local_charset(void);
LIB3270_EXPORT LSTATUS		  lib3270_win32_create_regkey(LPCSTR lpSubKey, REGSAM samDesired, PHKEY phkResult);
LIB3270_EXPORT DWORD		  lib3270_win32_get_dword(HKEY hKey, const char *name, DWORD def);
LIB3270_EXPORT LSTATUS		  lib3270_win32_set_registry(LPCSTR module, LPCSTR keyname, LPCSTR value);
LIB3270_EXPORT LSTATUS		  lib3270_win32_set_string(LPCSTR module, LPCSTR keyname, LPCSTR value);

/**
 * @brief Translate windows error code.
 *
 * @param lasterror	Windows error code (from GetLastError()).
 *
 * @return String with translated message (release it with lib3270_free).
 *
 */
LIB3270_EXPORT char 		* lib3270_win32_translate_error_code(int lasterror);

/**
 * @brief Get lib3270's installation path.
 *
 * @return Full path for the lib3270 installation path (release it with lib3270_free)
 *
 */
LIB3270_EXPORT char		* lib3270_get_installation_path();

#ifdef __cplusplus
}
#endif

