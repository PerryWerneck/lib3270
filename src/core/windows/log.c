/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270. Registro no INPI sob o nome G3270.
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

#include <winsock2.h>
#include <windows.h>
#include <wtsapi32.h>
#include <lmcons.h>

#include "../private.h"
#include <stdio.h>
#include <stdarg.h>
#include <config.h>
#include <lib3270.h>
#include <lib3270/log.h>

/*---[ Implement ]------------------------------------------------------------------------------------------*/

 void default_log_writer(H3270 GNUC_UNUSED(*session), const char *module, int rc, const char *fmt, va_list arg_ptr)
 {
	char	username[UNLEN + 1];
	DWORD	szName = sizeof(username);

	memset(username,0,szName);

	if(!GetUserName(username, &szName)) {
		strncpy(username,"?",UNLEN);
	}

	lib3270_autoptr(char) msg = lib3270_vsprintf(fmt,arg_ptr);

	const char *outMsg[] = {
		username,
		module,
		msg
	};

#ifdef DEBUG
	fprintf(stderr,"LOG(%s): %s\n",module,msg);
	fflush(stderr);
#endif // DEBUG

	ReportEvent(
		hEventLog,
		(rc == 0 ? EVENTLOG_INFORMATION_TYPE : EVENTLOG_ERROR_TYPE),
		1,
		0,
		NULL,
		3,
		0,
		outMsg,
		NULL
	);

 }

 LIB3270_EXPORT int lib3270_set_syslog(int GNUC_UNUSED(flag))
 {
 	return errno  = ENOENT;
 }

