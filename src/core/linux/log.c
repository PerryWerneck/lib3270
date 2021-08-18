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

#include <internals.h>
#include <stdio.h>
#include <stdarg.h>
#include <config.h>
#include <lib3270.h>
#include <lib3270/log.h>

#ifdef HAVE_SYSLOG
#include <syslog.h>
#endif // HAVE_SYSLOG

/*---[ Implementacao ]--------------------------------------------------------------------------------------*/

int use_syslog = 0;

int default_log_writer(H3270 GNUC_UNUSED(*session), void GNUC_UNUSED(*userdata), const char *module, int GNUC_UNUSED(rc), const char *message) {
#ifdef HAVE_SYSLOG
	if(use_syslog) {
		syslog(LOG_INFO, "%s: %s", module, message);
	} else {
		printf("%s %s\n", module, message);
		fflush(stdout);
	}
#else
	printf("%s %s\n", module, message);
	fflush(stdout);
#endif
	return 0;
}

LIB3270_EXPORT int lib3270_set_syslog(int flag) {
#ifdef HAVE_SYSLOG
	if(flag) {
		if(!use_syslog) {
			openlog(LIB3270_STRINGIZE_VALUE_OF(LIB3270_NAME), LOG_CONS, LOG_USER);
			use_syslog = 1;
		}
	} else {
		if(use_syslog) {
			closelog();
			use_syslog = 0;
		}
	}

	return 0;

#else
	return errno  = ENOENT;
#endif // HAVE_SYSLOG
}
