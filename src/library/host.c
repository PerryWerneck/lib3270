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

/*
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

/**
 * @brief Handle connect and disconnect from hosts, and state changes on the host connection.
 */

#pragma GCC diagnostic ignored "-Wsign-compare"

#include <config.h>
#include <string.h>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif // HAVE_MALLOC_H

#include <internals.h>
#include <stdlib.h>

#include <private/host.h>
#include <private/status.h>
#include <private/popup.h>
#include "telnetc.h"
#include <private/trace.h>
#include <private/util.h>
#include "screenc.h"

#include <errno.h>
#include <lib3270/internals.h>
#include <lib3270/properties.h>
#include <lib3270/log.h>
#include <lib3270/trace.h>
#include <lib3270/toggle.h>
#include <lib3270/keyboard.h>
#include <lib3270/memory.h>
#include <private/network.h>

//
// @brief Called from timer to attempt an automatic reconnection.
//
static int check_for_auto_reconnect(H3270 *hSession, void GNUC_UNUSED(*userdata)) {

	if(hSession->auto_reconnect_inprogress && !hSession->connection.context) {

		if(hSession->cbk.reconnect_allowed(hSession) == 0) {

			debug("%s","-----------------------> Reconnecting");

			trace_event(hSession,"Reconnecting\n");
			hSession->auto_reconnect_inprogress = 0; // Reset "in-progress" to allow reconnection.
			lib3270_connect(hSession,hSession->connection.timeout);

		} else if(hSession->connection.retry) {
			hSession->timer.add(
				hSession,
				hSession->connection.retry,
				check_for_auto_reconnect, 
				NULL
			);
			
		}
	}


	return 0;
}

/**
 * @brief Activate auto-reconnect timer.
 *
 * @param hSession	TN3270 Session handle.
 * @param msec		Time to reconnect.
 *
 * @return 0 if ok or error code if not.
 *
 * @retval EBUSY	Auto reconnect is already active.
 */
int lib3270_activate_auto_reconnect(H3270 *hSession, unsigned long msec) {

	if(hSession->auto_reconnect_inprogress)
		return EBUSY;

	trace_event(hSession,"Auto-reconnect activated\n");
	hSession->auto_reconnect_inprogress = 1;
	hSession->timer.add(hSession, msec, check_for_auto_reconnect, NULL);

	return 0;
}

LIB3270_EXPORT int lib3270_disconnect(H3270 *hSession) {
	debug("%s",__FUNCTION__);
	return connection_close(hSession,0);
}

int connection_write_offline(H3270 *hSession, const void *a , size_t v, LIB3270_NET_CONTEXT *c) {
	lib3270_log_write(hSession,"3270","Attempt to write when disconnected");
	return -ENOTCONN;
}

int connection_except_offline(H3270 *hSession, LIB3270_NET_CONTEXT *) {
	lib3270_log_write(hSession,"3270","Attempt to activate exception handler on a disconnected session");
	return -ENOTCONN;
}

static void set_disconnected(H3270 *hSession) {

	set_cstate(hSession,LIB3270_NOT_CONNECTED);
	hSession->cbk.cursor(hSession,LIB3270_POINTER_LOCKED & 0x03);
	hSession->kybdlock = LIB3270_KL_NOT_CONNECTED;
	hSession->starting	= 0;
	hSession->ssl.state	= LIB3270_SSL_UNDEFINED;

	set_status(hSession,LIB3270_FLAG_UNDERA,False);

	notify_new_state(hSession,LIB3270_STATE_CONNECT, False);

	message_changed(hSession,LIB3270_MESSAGE_DISCONNECTED);

	if(hSession->cbk.update_connect)
		hSession->cbk.update_connect(hSession,0);

	hSession->cbk.update_ssl(hSession,hSession->ssl.state);

}

/// @brief Disconnect from host.
/// @param hSession The tn3270 session
/// @param failed Non zero if it was a failure.
/// @return 0 if ok or error code if not.
int connection_close(H3270 *hSession, int failed) {

	debug("%s: connected=%s half connected=%s context=%p failed=%d",
	      __FUNCTION__,
	      (CONNECTED ? "Yes" : "No"),
	      (HALF_CONNECTED ? "Yes" : "No"),
	      hSession->connection.context,
		  failed
	     );

	hSession->connection.write = connection_write_offline;
	hSession->connection.except = connection_except_offline;

	if(hSession->connection.context) {
		int rc = hSession->connection.context->disconnect(hSession,hSession->connection.context);
		if(rc) {
			lib3270_log_write(hSession, "3270", "Network context disconnection returned %d", rc);
		}
		free(hSession->connection.context);
		hSession->connection.context = NULL;
	}

	if (CONNECTED || HALF_CONNECTED) {

		trace_dsn(hSession,"SENT disconnect\n");

		// We're not connected to an LU any more.
		hSession->lu.associated = CN;
		hSession->cbk.update_luname(hSession,"");

		debug("Disconnected (Failed: %d Reconnect: %d in_progress: %d)",failed,lib3270_get_toggle(hSession,LIB3270_TOGGLE_RECONNECT),hSession->auto_reconnect_inprogress);

		//
		// Remember a disconnect from ANSI mode, to keep screen tracing in sync.
		//
		if (IN_ANSI && lib3270_get_toggle(hSession,LIB3270_TOGGLE_SCREEN_TRACE))
			trace_ansi_disc(hSession);

		set_disconnected(hSession);

		if(failed && hSession->connection.retry && lib3270_get_toggle(hSession,LIB3270_TOGGLE_RECONNECT))
			lib3270_activate_auto_reconnect(hSession,hSession->connection.retry);

		return 0;

	}

	return errno = ENOTCONN;

}

int set_cstate(H3270 *hSession, LIB3270_CSTATE cstate) {
	debug("%s(%s,%d)",__FUNCTION__,lib3270_connection_state_get_name(cstate),(int) cstate);

	if(hSession->connection.state != cstate) {
		trace_dsn(
		    hSession,
		    "Connection state changes from %s to %s.\n",
		    lib3270_connection_state_get_name(hSession->connection.state),
		    lib3270_connection_state_get_name(cstate)
		);

		// Salve old states.
		int connected = lib3270_is_connected(hSession);
		int disconnected = lib3270_is_disconnected(hSession);

		// Cstate has changed.
		hSession->connection.state = cstate;

		// Do I need to send notifications?

		if(connected != lib3270_is_connected(hSession)) {
			// Online state has changed, fire LIB3270_ACTION_GROUP_ONLINE
			lib3270_action_group_notify(hSession, LIB3270_ACTION_GROUP_ONLINE);
		}

		if(disconnected != lib3270_is_disconnected(hSession)) {
			// Offline state has changed, fire LIB3270_ACTION_GROUP_OFFLINE
			lib3270_action_group_notify(hSession, LIB3270_ACTION_GROUP_OFFLINE);
		}

		return 1;
	}

	return 0;

}

/**
 * @brief The host has entered 3270 or ANSI mode, or switched between them.
 */
void host_in3270(H3270 *hSession, LIB3270_CSTATE new_cstate) {
	Boolean now3270 = (new_cstate == LIB3270_CONNECTED_3270 ||
	                   new_cstate == LIB3270_CONNECTED_SSCP ||
	                   new_cstate == LIB3270_CONNECTED_TN3270E);

	set_cstate(hSession,new_cstate);
	hSession->ever_3270 = now3270;
	notify_new_state(hSession, LIB3270_STATE_3270_MODE, now3270);
}

void set_connected_initial(H3270 *hSession) {
	set_cstate(hSession,LIB3270_CONNECTED_INITIAL);

	hSession->starting	= 1;	// Enable autostart

	notify_new_state(hSession, LIB3270_STATE_CONNECT, True);
	if(hSession->cbk.update_connect)
		hSession->cbk.update_connect(hSession,1);
}

/**
 * @brief Signal a state change.
 */
void notify_new_state(H3270 *hSession, LIB3270_STATE tx, int mode) {
	struct lib3270_linked_list_node * node;

	debug("%s(%s,%d)",__FUNCTION__,lib3270_state_get_name(tx),mode);
	trace_dsn(
	    hSession,
	    "Notifying state %s with mode %d.\n",
	    lib3270_state_get_name(tx),
	    mode
	);

	for(node = hSession->listeners.state[tx].first; node; node = node->next) {
		((struct lib3270_state_callback *) node)->func(hSession,mode,node->userdata);
	}

}

/*
static void update_url(H3270 *hSession) {
	char * url =
	    lib3270_strdup_printf(
	        "%s://%s:%s",
	        hSession->network.module->name,
	        hSession->host.current,
	        hSession->host.srvc
	    );

	if(hSession->host.url && !strcmp(hSession->host.url,url)) {
		debug("%s: Same url, ignoring",__FUNCTION__);
		lib3270_free(url);
		return;
	}

	debug("URL %s -> %s",hSession->host.url,url);

	trace_event(hSession,"Host URL was changed\nFrom: %s\nTo: %s\n",hSession->host.url,url);
	lib3270_free(hSession->host.url);
	hSession->host.url = url;
	hSession->cbk.update_url(hSession, hSession->host.url);

	hSession->network.module->reset(hSession);

}
*/

LIB3270_EXPORT const char * lib3270_get_associated_luname(const H3270 *hSession) {
	if(check_online_session(hSession))
		return NULL;

	return hSession->lu.associated;
}

LIB3270_EXPORT const char * lib3270_get_url(const H3270 *hSession) {
	if(hSession->connection.url)
		return hSession->connection.url;
	return lib3270_get_default_host(hSession);
}

LIB3270_EXPORT const char * lib3270_get_default_host(const H3270 GNUC_UNUSED(*hSession)) {
#ifdef _WIN32
	{
		lib3270_auto_cleanup(HKEY) hKey = 0;
		DWORD disp = 0;
		LSTATUS	rc = RegCreateKeyEx(
		    HKEY_LOCAL_MACHINE,
		    "Software\\" LIB3270_STRINGIZE_VALUE_OF(PRODUCT_NAME),
		    0,
		    NULL,
		    REG_OPTION_NON_VOLATILE,
		    KEY_QUERY_VALUE|KEY_READ,
		    NULL,
		    &hKey,
		    &disp);

		if(rc == ERROR_SUCCESS) {
			static char * default_host = NULL;
			DWORD cbData = 4096;

			if(!default_host) {
				default_host = (char *) malloc(cbData+1);
			} else {
				default_host = (char *) realloc(default_host,cbData+1);
			}

			DWORD dwRet = RegQueryValueEx(hKey,"host",NULL,NULL,(LPBYTE) default_host, &cbData);

			if(dwRet == ERROR_SUCCESS) {
				default_host = (char *) realloc(default_host,cbData+1);
				default_host[cbData] = 0;
				return default_host;
			}

			free(default_host);
			default_host = NULL;

		}
	}
#endif // _WIN32

	return getenv("LIB3270_DEFAULT_HOST");

}

LIB3270_EXPORT int lib3270_set_url(H3270 *hSession, const char *url) {

	FAIL_IF_ONLINE(hSession);

	if(!url) {
		url = lib3270_get_default_host(hSession);
	}

	if(!(url && *url)) {
		return EINVAL;
	}

	if(hSession->connection.url) {
		if(!strcmp(url,hSession->connection.url)) {
			return 0;
		}
		free(hSession->connection.url);
	}

	hSession->connection.url = strdup(url);
	hSession->cbk.update_url(hSession, hSession->connection.url);

	// The "reconnect" action is now available.
	lib3270_action_group_notify(hSession, LIB3270_ACTION_GROUP_OFFLINE);

	return 0;
}

/*
LIB3270_EXPORT const char * lib3270_get_host(const H3270 *h) {
	return h->host.url;
}
*/

LIB3270_EXPORT int lib3270_has_active_script(const H3270 *h) {
	return (h->oia.flag[LIB3270_FLAG_SCRIPT] != 0);
}

LIB3270_EXPORT int lib3270_get_typeahead(const H3270 *h) {
	return (h->oia.flag[LIB3270_FLAG_TYPEAHEAD] != 0);
}

LIB3270_EXPORT int lib3270_get_undera(const H3270 *h) {
	return (h->oia.flag[LIB3270_FLAG_UNDERA] != 0);
}

LIB3270_EXPORT int lib3270_get_oia_box_solid(const H3270 *h) {
	return (h->oia.flag[LIB3270_FLAG_BOXSOLID] != 0);
}
