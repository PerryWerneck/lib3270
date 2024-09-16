/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2024 Perry Werneck <perry.werneck@gmail.com>
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

 /**
  * @brief Implements 'glue' for mainloop and I/O.
  */

#include <internals.h>
#include <sys/time.h>
#include <sys/types.h>
#include "xioc.h"
#include "telnetc.h"
#include "utilc.h"
#include "kybdc.h"
#include <lib3270/os.h>

#if defined(_WIN32)
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#endif

#include <lib3270/log.h>
#include <lib3270/trace.h>
#include <lib3270/toggle.h>
#include <lib3270/tools/mainloop.h>

LIB3270_EXPORT void	lib3270_set_poll_state(void *id, int enabled) {
	if(id) {
		debug("%s: Polling on %p is %s",__FUNCTION__,id,(enabled ? "enabled" : "disabled"));
		lib3270_main_loop_get_instance()->set_poll_state(id, enabled);
	}
}

LIB3270_EXPORT void lib3270_remove_timer(void *timer) {
	if(timer)
		lib3270_main_loop_get_instance()->remove_timer(timer);
}

void x_except_on(H3270 *h) {
	int reading = (h->xio.read != NULL);

	debug("%s",__FUNCTION__);
	if(h->xio.except)
		return;

	if(reading)
		lib3270_remove_poll(h->xio.read);

	h->xio.except = h->network.module->add_poll(h,LIB3270_IO_EVENT_EXCEPTION,LIB3270_IO_PROC net_exception,h);

	if(reading)
		h->xio.read = h->network.module->add_poll(h,LIB3270_IO_EVENT_READ,LIB3270_IO_PROC net_input,h);

}

void remove_input_calls(H3270 *session) {
	if(session->xio.read) {
		lib3270_remove_poll(session->xio.read);
		session->xio.read = NULL;
	}
	if(session->xio.except) {
		lib3270_remove_poll(session->xio.except);
		session->xio.except = NULL;
	}
	if(session->xio.write) {
		lib3270_remove_poll(session->xio.write);
		session->xio.write = NULL;
	}
}

LIB3270_EXPORT void lib3270_main_iterate(H3270 *hSession, int block) {
	lib3270_main_loop_get_instance()->event_dispatcher(block ? 1000 : 0);
}

LIB3270_EXPORT int lib3270_wait(H3270 *hSession, int seconds) {

	time_t end = time(0) + seconds;

	while(time(0) < end) {
		lib3270_main_loop_iterate(1000);
	}

	return 0;
}

LIB3270_EXPORT void lib3270_ring_bell(H3270 *session) {
	CHECK_SESSION_HANDLE(session);
	if(lib3270_get_toggle(session,LIB3270_TOGGLE_BEEP))
		lib3270_main_loop_get_instance()->ring_bell();
}

/**
 * @brief Run background task.
 *
 * Call task in a separate thread, keep gui main loop running until
 * the function returns.
 *
 * @param hSession	TN3270 session.
 * @param callback	Function to call.
 * @param parm		Parameter to callback function.
 *
 */
LIB3270_EXPORT int lib3270_run_task(H3270 *hSession, int(*callback)(void *), void *userdata) {
	int rc;

	CHECK_SESSION_HANDLE(hSession);

	hSession->cbk.set_timer(hSession,1);
	hSession->tasks++;
	rc = lib3270_main_loop_get_instance()->run_task(callback,userdata);
	hSession->cbk.set_timer(hSession,0);
	hSession->tasks--;
	return rc;

}

int non_blocking(H3270 *hSession, Boolean on) {

	if(hSession->network.module->non_blocking(hSession,on))
		return 0;

	lib3270_set_poll_state(hSession->xio.read, on);
	lib3270_set_poll_state(hSession->xio.write, on);
	lib3270_set_poll_state(hSession->xio.except, on);

	return 0;
}

LIB3270_EXPORT void * lib3270_add_timer(unsigned long interval_ms, int (*proc)(void *userdata), void *userdata) {
	return lib3270_main_loop_get_instance()->add_timer(interval_ms,proc,userdata);
}

LIB3270_EXPORT void * lib3270_add_poll_fd(int fd, LIB3270_IO_EVENT flag, void(*call)(int, LIB3270_IO_EVENT, void *), void *userdata ) {
	return lib3270_main_loop_get_instance()->add_poll(fd,flag,call,userdata);
}

LIB3270_EXPORT void lib3270_remove_poll(void *poll) {
	lib3270_main_loop_get_instance()->remove_poll(poll);
}

LIB3270_EXPORT void lib3270_remove_poll_fd(int fd) {
	lib3270_main_loop_get_instance()->remove_poll_fd(fd);
}

LIB3270_EXPORT int lib3270_main_loop_iterate(unsigned long wait_ms) {
	return lib3270_main_loop_get_instance()->event_dispatcher(wait_ms);
}



