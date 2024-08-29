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
  * @brief Declare lib3270's mainloop methods.
  */

 #pragma once

 #include <lib3270.h>

 typedef enum lib3270_io_event {
        LIB3270_IO_EVENT_READ		= 0x01,
        LIB3270_IO_EVENT_EXCEPTION	= 0x02,
        LIB3270_IO_EVENT_WRITE		= 0x04,
        LIB3270_IO_EVENT_HANG_UP	= 0x08,
 } LIB3270_IO_EVENT;

 #define LIB3270_TIMER_PROC (int(*)(void *))
 #define LIB3270_IO_PROC	(void(*)(int, LIB3270_IO_EVENT, void *))
 #define LIB3270_TASK		(int(*)(void *))

 typedef struct _lib3270_main_loop {

	const char *name;

	unsigned short sz;

	void	* (*add_timer)(unsigned long interval_ms, int (*proc)(void *userdata), void *userdata);
	void	  (*remove_timer)(void *timer);

	void	* (*add_poll)(int fd, LIB3270_IO_EVENT flag, void(*proc)(int, LIB3270_IO_EVENT, void *), void *userdata);
	void      (*remove_poll)(void *id);
	int       (*remove_poll_fd)(int fd);
	void      (*set_poll_state)(void *id, int enabled);

	int		  (*wait)(int seconds);
	int		  (*event_dispatcher)(unsigned long wait_ms);
	void	  (*ring_bell)();
	int		  (*run_task)(int(*callback)(void *), void *userdata);

 } lib3270_main_loop;

 /// @brief Get default (internal) main loop.
 LIB3270_EXPORT const lib3270_main_loop * lib3270_main_loop_get_default();

 ///
 /// @brief Run background task.
 ///
 /// Call task in a separate thread, keep gui main loop running until
 /// the function returns.
 ///
 /// @param hSession	TN3270 session.
 /// @param callback	Function to call.
 /// @param parm		Parameter to callback function.
 ///
 ///
 LIB3270_EXPORT int lib3270_run_task(H3270 *hSession, int(*callback)(void *), void *userdata);

 /// @brief Get main loop instance.
 LIB3270_EXPORT const lib3270_main_loop * lib3270_main_loop_get_instance();

 /// @brief Set main loop instance.
 LIB3270_EXPORT void lib3270_main_loop_set_instance(const lib3270_main_loop *instance);

 /// @brief Run iteration waiting for time
 LIB3270_EXPORT int lib3270_main_loop_iterate(unsigned long wait_ms);

 /// @brief Add timer
 LIB3270_EXPORT void * lib3270_add_timer(unsigned long interval_ms, int (*proc)(void *userdata), void *userdata);

 LIB3270_EXPORT void lib3270_remove_timer(void *timer);

 LIB3270_EXPORT void * lib3270_add_poll_fd(int fd, LIB3270_IO_EVENT flag, void(*call)(int, LIB3270_IO_EVENT, void *), void *userdata );
 LIB3270_EXPORT void   lib3270_remove_poll(void *poll);
 LIB3270_EXPORT void   lib3270_remove_poll_fd(int fd);
 LIB3270_EXPORT void   lib3270_update_poll_fd(int fd, LIB3270_IO_EVENT flag);
 LIB3270_EXPORT void   lib3270_set_poll_state(void *id, int enabled);
