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


/**
 *	@file	toggles/init.c
 *	@brief	Initialize toggles.
 */

#include <errno.h>
#include <sys/types.h>

#ifdef WIN32
	#include <winsock2.h>
	#include <windows.h>
	#include <ws2tcpip.h>
#else
	#include <sys/socket.h>
#endif // !WIN32

#include <config.h>
#include <lib3270/toggle.h>
#include <internals.h>

#include "ansic.h"
#include "ctlrc.h"
#include "popupsc.h"
#include "screenc.h"
#include "trace_dsc.h"
#include "togglesc.h"
#include "utilc.h"
#include <lib3270/log.h>

/*---[ Implement ]------------------------------------------------------------------------------------------------------------*/

static void toggle_altscreen(H3270 *session, const struct lib3270_toggle *t, LIB3270_TOGGLE_TYPE GNUC_UNUSED(tt))
{
	if(!session->screen_alt)
		set_viewsize(session,t->value ? 24 : session->max.rows,80);
}

static void toggle_redraw(H3270 *session, const struct lib3270_toggle GNUC_UNUSED(*t), LIB3270_TOGGLE_TYPE GNUC_UNUSED(tt))
{
	session->cbk.display(session);
}

/**
 * @brief No-op toggle.
 */
static void toggle_nop(H3270 GNUC_UNUSED(*session), const struct lib3270_toggle GNUC_UNUSED(*t), LIB3270_TOGGLE_TYPE GNUC_UNUSED(tt))
{
}

static void toggle_keepalive(H3270 *session, const struct lib3270_toggle GNUC_UNUSED(*t), LIB3270_TOGGLE_TYPE GNUC_UNUSED(tt))
{
	if(session->connection.sock > 0)
	{
		// Update keep-alive option
		int optval = t->value ? 1 : 0;

		if (setsockopt(session->connection.sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&optval, sizeof(optval)) < 0)
		{
			popup_a_sockerr(session, N_( "Can't %s network keep-alive" ), optval ? _( "enable" ) : _( "disable" ));
		}
		else
		{
			trace_dsn(session,"Network keep-alive is %s\n",optval ? "enabled" : "disabled" );
		}

	}
}

static void toggle_connect(H3270 *hSession, const struct lib3270_toggle *toggle, LIB3270_TOGGLE_TYPE tt)
{
	if(tt != LIB3270_TOGGLE_TYPE_INITIAL && lib3270_is_disconnected(hSession) && !hSession->popups && toggle->value)
	{
		if(lib3270_reconnect(hSession,0))
			lib3270_write_log(hSession,"3270","Auto-connect fails: %s",strerror(errno));
	}

}

/**
 * @brief Called from system initialization code to handle initial toggle settings.
 */
void initialize_toggles(H3270 *session)
{
	static const struct _upcalls
	{
		LIB3270_TOGGLE_ID	id;
		void (*upcall)(H3270 *session, const struct lib3270_toggle *t, LIB3270_TOGGLE_TYPE tt);
	}
	upcalls[] =
	{
		{
			LIB3270_TOGGLE_RECTANGLE_SELECT,
			toggle_rectselect

		},
		{
			LIB3270_TOGGLE_MONOCASE,
			toggle_redraw
		},
		{
			LIB3270_TOGGLE_UNDERLINE,
			toggle_redraw

		},
		{
			LIB3270_TOGGLE_ALTSCREEN,
			toggle_altscreen

		},
		{
			LIB3270_TOGGLE_KEEP_ALIVE,
			toggle_keepalive
		},
		{
			LIB3270_TOGGLE_CONNECT_ON_STARTUP,
			toggle_connect
		}

	};

	unsigned int f;

	for(f=0;f<LIB3270_TOGGLE_COUNT;f++)
		session->toggle[f].upcall = toggle_nop;

	for(f=0;f<(sizeof(upcalls)/sizeof(upcalls[0]));f++)
		session->toggle[upcalls[f].id].upcall = upcalls[f].upcall;

	for(f=0;f<LIB3270_TOGGLE_COUNT;f++)
	{
		session->toggle[f].value = toggle_descriptor[f].def;
		if(session->toggle[f].value)
			session->toggle[f].upcall(session,&session->toggle[f],LIB3270_TOGGLE_TYPE_INITIAL);
	}

}

/**
 * @brief Called from system exit code to handle toggles.
 */
void shutdown_toggles(H3270 *session)
{
#if defined(X3270_TRACE)
	static const LIB3270_TOGGLE_ID disable_on_shutdown[] = {LIB3270_TOGGLE_DS_TRACE, LIB3270_TOGGLE_EVENT_TRACE, LIB3270_TOGGLE_SCREEN_TRACE};

	size_t f;

	for(f=0;f< (sizeof(disable_on_shutdown)/sizeof(disable_on_shutdown[0])); f++)
		lib3270_set_toggle(session,disable_on_shutdown[f],0);

#endif
}
