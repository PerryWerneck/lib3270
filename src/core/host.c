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
 * @brief Handle connect and disconnect from hosts, and state changes on the host connection.
 */

#pragma GCC diagnostic ignored "-Wsign-compare"

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif // HAVE_MALLOC_H

#include <internals.h>
#include <stdlib.h>
//#include "resources.h"

#include "hostc.h"
#include "statusc.h"
#include "popupsc.h"
#include "telnetc.h"
#include "trace_dsc.h"
#include "utilc.h"
#include "xioc.h"
#include "screenc.h"

#include <errno.h>
#include <lib3270/internals.h>
#include <lib3270/os.h>
#include <lib3270/properties.h>
#include <lib3270/log.h>
#include <lib3270/trace.h>
#include <lib3270/toggle.h>
#include <lib3270/keyboard.h>
#include <networking.h>

/**
 * @brief Called from timer to attempt an automatic reconnection.
 */
static int check_for_auto_reconnect(H3270 *hSession, void GNUC_UNUSED(*userdata)) {

	if(hSession->auto_reconnect_inprogress) {
		lib3270_write_log(hSession,"3270","Starting auto-reconnect on %s",lib3270_get_url(hSession));
		hSession->auto_reconnect_inprogress = 0; // Reset "in-progress" to allow reconnection.
		if(hSession->cbk.reconnect(hSession,0))
			lib3270_write_log(hSession,"3270","Auto-reconnect fails: %s",strerror(errno));
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

	hSession->auto_reconnect_inprogress = 1;
	(void) AddTimer(msec, hSession, check_for_auto_reconnect, NULL);

	return 0;
}

LIB3270_EXPORT int lib3270_disconnect(H3270 *h) {
	debug("%s",__FUNCTION__);
	return host_disconnect(h,0);
}

/// @brief Do disconnect.
/// @param hSession Session handle.
/// @param failed	Non zero if it was a failure.
int host_disconnect(H3270 *hSession, int failed) {

	CHECK_SESSION_HANDLE(hSession);

	debug("%s: connected=%s half connected=%s network=%s",
	      __FUNCTION__,
	      (CONNECTED ? "Yes" : "No"),
	      (HALF_CONNECTED ? "Yes" : "No"),
	      (hSession->network.module->is_connected(hSession) ? "Active" : "Inactive")
	     );

	if (CONNECTED || HALF_CONNECTED) {
		// Disconecting, disable input
		remove_input_calls(hSession);
		net_disconnect(hSession);

		trace("Disconnected (Failed: %d Reconnect: %d in_progress: %d)",failed,lib3270_get_toggle(hSession,LIB3270_TOGGLE_RECONNECT),hSession->auto_reconnect_inprogress);

		//
		// Remember a disconnect from ANSI mode, to keep screen tracing in sync.
		//
		if (IN_ANSI && lib3270_get_toggle(hSession,LIB3270_TOGGLE_SCREEN_TRACE))
			trace_ansi_disc(hSession);

		lib3270_set_disconnected(hSession);

		if(hSession->connection.error) {

			// TODO: Add 'reconnect' option in the popup dialog for optional auto reconnect.
			lib3270_popup(hSession,hSession->connection.error,!hSession->auto_reconnect_inprogress);

			lib3270_free(hSession->connection.error);
			hSession->connection.error = NULL;

		}

		if(failed && hSession->connection.retry && lib3270_get_toggle(hSession,LIB3270_TOGGLE_RECONNECT))
			lib3270_activate_auto_reconnect(hSession,hSession->connection.retry);

		return 0;

	}

	if(hSession->network.module->is_connected(hSession)) {

		debug("%s: Disconnecting socket", __FUNCTION__);

	}

	return errno = ENOTCONN;

}

int lib3270_set_cstate(H3270 *hSession, LIB3270_CSTATE cstate) {
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

	lib3270_set_cstate(hSession,new_cstate);
	hSession->ever_3270 = now3270;
	lib3270_st_changed(hSession, LIB3270_STATE_3270_MODE, now3270);
}

void lib3270_set_connected_initial(H3270 *hSession) {
	lib3270_set_cstate(hSession,LIB3270_CONNECTED_INITIAL);

	hSession->starting	= 1;	// Enable autostart

	lib3270_st_changed(hSession, LIB3270_STATE_CONNECT, True);
	if(hSession->cbk.update_connect)
		hSession->cbk.update_connect(hSession,1);
}

void lib3270_set_disconnected(H3270 *hSession) {
	CHECK_SESSION_HANDLE(hSession);

	lib3270_set_cstate(hSession,LIB3270_NOT_CONNECTED);
	mcursor_set(hSession,LIB3270_POINTER_LOCKED);

	hSession->kybdlock = LIB3270_KL_NOT_CONNECTED;
	hSession->starting	= 0;
	hSession->ssl.state	= LIB3270_SSL_UNDEFINED;

	set_status(hSession,LIB3270_FLAG_UNDERA,False);

	lib3270_st_changed(hSession,LIB3270_STATE_CONNECT, False);

	status_changed(hSession,LIB3270_MESSAGE_DISCONNECTED);

	if(hSession->cbk.update_connect)
		hSession->cbk.update_connect(hSession,0);

	hSession->cbk.update_ssl(hSession,hSession->ssl.state);

}

/**
 * @brief Signal a state change.
 */
void lib3270_st_changed(H3270 *hSession, LIB3270_STATE tx, int mode) {
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

	lib3270_write_event_trace(hSession,"Host URL was changed\nFrom: %s\nTo: %s\n",hSession->host.url,url);
	lib3270_free(hSession->host.url);
	hSession->host.url = url;
	hSession->cbk.update_url(hSession, hSession->host.url);

	hSession->network.module->reset(hSession);

}

LIB3270_EXPORT const char * lib3270_get_associated_luname(const H3270 *hSession) {
	if(check_online_session(hSession))
		return NULL;

	return hSession->lu.associated;
}

LIB3270_EXPORT const char * lib3270_get_url(const H3270 *hSession) {
	if(hSession->host.url)
		return hSession->host.url;

	return lib3270_get_default_host(hSession);
}

LIB3270_EXPORT const char * lib3270_get_default_host(const H3270 GNUC_UNUSED(*hSession)) {
#ifdef _WIN32
	{
		lib3270_auto_cleanup(HKEY) hKey;
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

#ifdef LIB3270_DEFAULT_HOST
	return LIB3270_DEFAULT_HOST;
#else
	return getenv("LIB3270_DEFAULT_HOST");
#endif // LIB3270_DEFAULT_HOST
}

LIB3270_EXPORT int lib3270_set_url(H3270 *h, const char *n) {
	FAIL_IF_ONLINE(h);

	if(!n)
		n = lib3270_get_default_host(h);

	if(!n)
		return errno = ENOENT;

	lib3270_autoptr(char)	  str 		= strdup(n);
	char					* hostname 	= lib3270_set_network_module_from_url(h,str);
	const char 				* srvc;
	char					* ptr;
	char					* query		= "";

	trace("%s(%s)",__FUNCTION__,str);

	if(!(hostname && *hostname))
		return 0;

	srvc = h->network.module->service;

	ptr = strchr(hostname,':');
	if(ptr) {
		*(ptr++) = 0;
		srvc  = ptr;
		query = strchr(ptr,'?');

	} else {
		srvc = "3270";
		query = strchr(hostname,'?');
	}

	if(query)
		*(query++) = 0;
	else
		query = "";

	trace("SRVC=[%s]",srvc);

	Replace(h->host.current,strdup(hostname));
	Replace(h->host.srvc,strdup(srvc));

	// Verifica parâmetros
	if(query && *query) {
		lib3270_autoptr(char) str = strdup(query);
		char *ptr;

#ifdef HAVE_STRTOK_R
		char *saveptr	= NULL;
		for(ptr = strtok_r(str,"&",&saveptr); ptr; ptr=strtok_r(NULL,"&",&saveptr))
#else
		for(ptr = strtok(str,"&"); ptr; ptr=strtok(NULL,"&"))
#endif
		{
			char *var = ptr;
			char *val = strchr(ptr,'=');
			if(val) {
				*(val++) = 0;

				if(lib3270_set_string_property(h, var, val, 0) == 0)
					continue;

				lib3270_write_log(h,"","Can't set attribute \"%s\": %s",var,strerror(errno));

			} else {
				if(lib3270_set_int_property(h,var,1,0))
					continue;

				lib3270_write_log(h,"","Can't set attribute \"%s\": %s",var,strerror(errno));
			}

		}

	}

	// Notifica atualização
	update_url(h);

	// The "reconnect" action is now available.
	lib3270_action_group_notify(h, LIB3270_ACTION_GROUP_OFFLINE);

	return 0;
}

LIB3270_EXPORT const char * lib3270_get_host(const H3270 *h) {
	return h->host.url;
}

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
