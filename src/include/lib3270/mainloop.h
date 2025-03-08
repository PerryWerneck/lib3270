/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2025 Perry Werneck <perry.werneck@gmail.com>
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

 #error deprecated 
 
 #pragma once

 #include <lib3270.h>

// LIB3270_EXPORT void		* lib3270_add_poll_fd(H3270 *session, int fd, LIB3270_IO_FLAG flag, void(*call)(H3270 *, int, LIB3270_IO_FLAG, void *), void *userdata );
// LIB3270_EXPORT void		  lib3270_remove_poll(H3270 *session, void *id);
// LIB3270_EXPORT void		  lib3270_set_poll_state(H3270 *session, void *id, int enabled);

// LIB3270_EXPORT void		  lib3270_remove_poll_fd(H3270 *session, int fd);
// LIB3270_EXPORT void		  lib3270_update_poll_fd(H3270 *session, int fd, LIB3270_IO_FLAG flag);

/**
 * @brief Set default I/O Handlers.
 *
 * @param cbk	Structure with the application I/O handles to set.
 *
 * @return 0 if ok, error code if not.
 *
 */
// LIB3270_EXPORT int lib3270_register_io_controller(const LIB3270_IO_CONTROLLER *cbk);

/**
 * @brief Register session io handlers.
 */
// LIB3270_EXPORT int lib3270_session_set_handlers(H3270 *session, const LIB3270_IO_CONTROLLER *cntrl);

/**
 * Register time handlers.
 *
 * @param add	Callback for adding a timeout
 * @param rm	Callback for removing a timeout
 *
 */
// LIB3270_EXPORT void lib3270_register_timer_handlers(void * (*add)(H3270 *session, unsigned long interval_ms, int (*proc)(H3270 *session, void *userdata), void *userdata), void (*rm)(H3270 *session, void *timer));

// LIB3270_EXPORT void lib3270_register_fd_handlers(void * (*add)(H3270 *session, int fd, LIB3270_IO_FLAG flag, void(*proc)(H3270 *, int, LIB3270_IO_FLAG, void *), void *userdata), void (*rm)(H3270 *, void *id));

