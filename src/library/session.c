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

#include <config.h>
#include <lib3270/defs.h>
#include <lib3270/popup.h>
#include <lib3270/memory.h>

#include <private/defs.h>
#include <private/session.h>
#include <private/intl.h>
#include <private/network.h>
#include <private/session.h>

#include "kybdc.h"
#include <private/ansi.h>
#include <private/toggle.h>
#include <private/screen.h>
#include "screenc.h"
#include <private/ctlr.h>
#include "ftc.h"
#include <private/3270ds.h>
#include <private/popup.h>
#include <lib3270/trace.h>
#include <lib3270/log.h>
#include <lib3270/properties.h>
#include <private/mainloop.h>


/*---[ Implement ]------------------------------------------------------------------------------------------------------------*/

LIB3270_EXPORT void lib3270_autoptr_cleanup_H3270(H3270 **ptr) {
	lib3270_session_free(*ptr);
	*ptr = NULL;
}

/**
 * @brief Closes a TN3270 session releasing resources.
 *
 * @param h handle of the session to close.
 *
 */
void lib3270_session_free(H3270 *h) {
	size_t f;

	if(!h)
		return;

	if(h->connection.context) {
		connection_close(h,-1);
	}

	if(h->timer.context) {
		h->timer.finalize(h,h->timer.context);
		h->timer.context = NULL;
	}

	if(h->poll.context) {
		h->poll.finalize(h,h->poll.context);
		h->poll.context = NULL;
	}

	// Do we have pending tasks?
	if(h->tasks) {
		lib3270_log_write(h,LIB3270_STRINGIZE_VALUE_OF(PRODUCT_NAME),"Destroying session with %u active task(s)",h->tasks);
	}

	shutdown_toggles(h);

	// Release state change callbacks
	for(f=0; f<LIB3270_STATE_USER; f++)
		lib3270_linked_list_free(&h->listeners.state[f]);

	// Release toggle change listeners.
	for(f=0; f<LIB3270_TOGGLE_COUNT; f++)
		lib3270_linked_list_free(&h->listeners.toggle[f]);

	// Release action listeners.
	for(f=0; f<LIB3270_ACTION_GROUP_CUSTOM; f++)
		lib3270_linked_list_free(&h->listeners.actions[f]);

	// Release Lu names
	if(h->lu.names) {
		lib3270_free(h->lu.names);
		h->lu.names = NULL;
	}

	// Release memory
	#define release_pointer(x) lib3270_free(x); x = NULL;

	// release_pointer(h->charset.display);
	release_pointer(h->paste_buffer);

	release_pointer(h->ibuf);
	h->ibuf_size = 0;

	for(f=0; f<(sizeof(h->buffer)/sizeof(h->buffer[0])); f++) {
		release_pointer(h->buffer[f]);
	}

	// Release hostname info
	release_pointer(h->connection.url);

	release_pointer(h->charset.host);
	release_pointer(h->charset.display);

	release_pointer(h->text);
	release_pointer(h->zero_buf);

	release_pointer(h->output.base);

	release_pointer(h->sbbuf);
	release_pointer(h->tabs);

#ifdef _WIN32
	win32_mainloop_free(h);
#endif

	// Release logfile
	lib3270_trace_close(h);
	lib3270_log_close(h);
	lib3270_free(h);

}

static void nop_void() {
}

static int default_action(H3270 GNUC_UNUSED(*hSession), const char GNUC_UNUSED(*name)) {
	return errno = ENOENT;
}

static int print(H3270 *session, LIB3270_CONTENT_OPTION GNUC_UNUSED(mode)) {
	lib3270_log_write(session, "print", "%s", "Printing is unavailable");
	lib3270_popup_dialog(session, LIB3270_NOTIFY_WARNING, _( "Can't print" ), _( "Unable to print" ), "%s", strerror(ENOTSUP));
	return errno = ENOTSUP;
}

static int save(H3270 *session, LIB3270_CONTENT_OPTION GNUC_UNUSED(mode), const char GNUC_UNUSED(*filename)) {
	lib3270_log_write(session, "save", "%s", "Saving is unavailable");
	lib3270_popup_dialog(session, LIB3270_NOTIFY_WARNING, _( "Can't save" ), _( "Unable to save" ), "%s", strerror(ENOTSUP));
	return errno = ENOTSUP;
}

static int load(H3270 *session, const char GNUC_UNUSED(*filename)) {
	lib3270_log_write(session, "load", "%s", "Loading from file is unavailable");
	lib3270_popup_dialog(session, LIB3270_NOTIFY_WARNING, _( "Can't load" ), _( "Unable to load from file" ), "%s", strerror(ENOTSUP));
	return errno = ENOTSUP;
}

static void screen_disp(H3270 *session) {
	screen_update(session,0,session->view.rows*session->view.cols);
}

static int reconnect_allowed(H3270 *session) {
	//
	// Returns 0 to indicate that reconnection is allowed and no error code is present.
	// This function is likely part of a larger process that manages session connections.
	// Returning 0 typically signifies a successful operation or a state where no errors
	// have occurred, allowing the session to attempt reconnection.
	//
	return 0;
}

void lib3270_reset_callbacks(H3270 *hSession) {
	
	// Default calls
	memset(&hSession->cbk,0,sizeof(hSession->cbk));

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

	hSession->cbk.update_viewsize		= nop_void;
	hSession->cbk.update 				= nop_void;
	hSession->cbk.update_model			= nop_void;
	hSession->cbk.update_toggle			= nop_void;
	hSession->cbk.update_cursor			= nop_void;
	hSession->cbk.set_selection 		= nop_void;
	hSession->cbk.ctlr_done				= nop_void;
	hSession->cbk.changed				= nop_void;
	hSession->cbk.erase					= screen_disp;
	hSession->cbk.suspend				= nop_void;
	hSession->cbk.resume				= screen_disp;
	hSession->cbk.update_oia			= nop_void;
	hSession->cbk.update_selection		= nop_void;
	hSession->cbk.cursor 				= nop_void;
	hSession->cbk.update_ssl			= nop_void;
	hSession->cbk.display				= screen_disp;
	hSession->cbk.set_width				= nop_void;
	hSession->cbk.update_status			= nop_void;
	hSession->cbk.autostart				= nop_void;
	hSession->cbk.set_timer				= nop_void;
	hSession->cbk.print					= print;
	hSession->cbk.save					= save;
	hSession->cbk.load					= load;
	hSession->cbk.update_luname			= nop_void;
	hSession->cbk.update_url			= nop_void;
	hSession->cbk.action				= default_action;
	hSession->cbk.word_selected			= nop_void;
	hSession->cbk.reconnect_allowed		= reconnect_allowed;

	#pragma GCC diagnostic pop

	lib3270_set_popup_handler(hSession, NULL);
	lib3270_log_close(hSession);
	lib3270_trace_close(hSession);

}

static void lib3270_session_init(H3270 *hSession, const char *model, const char *charset) {
	int f;

	memset(hSession,0,sizeof(H3270));
	lib3270_reset_callbacks(hSession);
	hSession->ssl.download_crl = 1;

	lib3270_set_host_charset(hSession,charset);

	hSession->connection.write = connection_write_offline;
	hSession->connection.except = connection_except_offline;

	// Set the defaults.
	hSession->extended  			=  1;
	hSession->typeahead				=  1;
	hSession->oerr_lock 			=  1;
	hSession->unlock_delay			=  1;
	hSession->icrnl 				=  1;
	hSession->onlcr					=  1;
	hSession->model_num				= -1;
	hSession->connection.state		= LIB3270_NOT_CONNECTED;
	hSession->connection.timeout	= 10;
	hSession->connection.retry		= 0;
	hSession->oia.status			= LIB3270_MESSAGE_DISCONNECTED;
	hSession->kybdlock 				= KL_NOT_CONNECTED;
	hSession->aid 					= AID_NO;
	hSession->reply_mode 			= SF_SRM_FIELD;
	hSession->linemode				= 1;
	hSession->tn3270e_submode		= E_NONE;
	hSession->scroll_top 			= -1;
	hSession->scroll_bottom			= -1;
	hSession->wraparound_mode 		= 1;
	hSession->saved_wraparound_mode	= 1;
	hSession->once_cset 			= -1;
	hSession->state					= LIB3270_ANSI_STATE_DATA;
	hSession->host_type				= LIB3270_HOSTTYPE_DEFAULT;
	hSession->colors				= 16;
	hSession->m3279					= 1;

	lib3270_set_unlock_delay(hSession,UNLOCK_MS);

	// CSD
	for(f=0; f<4; f++)
		hSession->csd[f] = hSession->saved_csd[f] = LIB3270_ANSI_CSD_US;

#ifdef _WIN32
	// Get defaults from registry.
	{
		lib3270_auto_cleanup(HKEY) hKey = 0;
		DWORD disp = 0;
		LSTATUS	rc = RegCreateKeyEx(
		                 HKEY_LOCAL_MACHINE,
		                 "Software\\" LIB3270_STRINGIZE_VALUE_OF(PRODUCT_NAME) "\\" PACKAGE_NAME,
		                 0,
		                 NULL,
		                 REG_OPTION_NON_VOLATILE,
		                 KEY_QUERY_VALUE|KEY_READ,
		                 NULL,
		                 &hKey,
		                 &disp);

		if(rc == ERROR_SUCCESS) {
			size_t property;

			// Load unsigned int properties.
			const LIB3270_UINT_PROPERTY * uiProps = lib3270_get_unsigned_properties_list();

			for(property = 0; uiProps[property].name; property++) {
				if(!(uiProps[property].set && uiProps[property].group == LIB3270_ACTION_GROUP_OFFLINE))
					continue;

				DWORD val = (DWORD) uiProps[property].default_value;
				DWORD cbData = sizeof(DWORD);
				DWORD dwRet = RegQueryValueEx(hKey, uiProps[property].name, NULL, NULL, (LPBYTE) &val, &cbData);

				if(dwRet == ERROR_SUCCESS) {
					debug("%s=%u",uiProps[property].name,(unsigned int) val);
					uiProps[property].set(hSession,(unsigned int) val);
				} else {
					uiProps[property].set(hSession,uiProps[property].default_value);
				}

			}

			// TODO: load other properties, except toggles (load then in initialize_toggles).

		}
	}
#endif // _WIN32

	// Initialize toggles
	initialize_toggles(hSession);

	lib3270_set_model_name(hSession,model);

}

H3270 * lib3270_session_new(const char *model, int gui) {

	H3270 * hSession;

	debug("%s - gui=%s",__FUNCTION__,gui ? "Yes" : "No");

	hSession = lib3270_malloc(sizeof(H3270));
	lib3270_session_init(hSession, model, "bracket" );
	
	lib3270_trace_close(hSession); // Initialize trace callbacks.
	lib3270_log_close(hSession); // Initialize log callbacks.

#ifdef _WIN32
	// Win32 mainloop already support gui.
	win32_mainloop_new(hSession);
#else
	if(!gui || setup_glib_mainloop(hSession)) {
		setup_default_mainloop(hSession);
	}
#endif // _WIN32

	if(screen_init(hSession)) {
		lib3270_free(hSession);
		return NULL;
	}

	debug("%s: Initializing KYBD",__FUNCTION__);
	lib3270_register_schange(hSession,LIB3270_STATE_CONNECT,kybd_connect,NULL);
	lib3270_register_schange(hSession,LIB3270_STATE_3270_MODE,kybd_in3270,NULL);

#if defined(X3270_ANSI)
	debug("%s: Initializing ANSI",__FUNCTION__);
	lib3270_register_schange(hSession,LIB3270_STATE_3270_MODE,ansi_in3270,NULL);
#endif // X3270_ANSI


#if defined(X3270_FT)
	ft_init(hSession);
#endif

	lib3270_set_url(hSession,NULL);	// Set default URL (if available).

	debug("%s finished",__FUNCTION__);

	errno = 0;
	return hSession;
}

LIB3270_INTERNAL int check_online_session(const H3270 *hSession) {

	// Is the session valid?
	if(!hSession)
		return errno = EINVAL;

	// Is it connected?
	if((int) hSession->connection.state < (int)LIB3270_CONNECTED_INITIAL)
		return errno = ENOTCONN;

	return 0;
}

LIB3270_INTERNAL int check_offline_session(const H3270 *hSession) {

	// Is the session valid?
	if(!hSession)
		return errno = EINVAL;

	// Is it connected?
	if((int) hSession->connection.state >= (int)LIB3270_CONNECTED_INITIAL)
		return errno = EISCONN;

	return 0;
}

LIB3270_EXPORT void lib3270_set_user_data(H3270 *h, void *ptr) {
	h->user_data = ptr;
}

LIB3270_EXPORT void * lib3270_get_user_data(H3270 *h) {
	return h->user_data;
}

LIB3270_EXPORT void lib3270_set_session_id(H3270 *hSession, char id) {
	hSession->id = id;
}

LIB3270_EXPORT char lib3270_get_session_id(H3270 *hSession) {
	return hSession->id;
}

struct lib3270_session_callbacks * lib3270_get_session_callbacks(H3270 *hSession, const char *revision, unsigned short sz) {

	#define REQUIRED_REVISION "20211118"

	if(revision && strcasecmp(revision,REQUIRED_REVISION) < 0) {
		errno = EINVAL;
		lib3270_log_write(hSession,PACKAGE_NAME,"Invalid revision %s when setting callback table",revision);
		return NULL;
	}

	if(sz != sizeof(struct lib3270_session_callbacks)) {

		lib3270_log_write(hSession,PACKAGE_NAME,"Invalid callback table (sz=%u expected=%u)",sz,(unsigned int) sizeof(struct lib3270_session_callbacks));
		errno = EINVAL;
		return NULL;
	}

	return &hSession->cbk;
}


