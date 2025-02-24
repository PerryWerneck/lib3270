/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2025 Banco do Brasil S.A.
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
 * @file mainloop.c
 * @brief Implements the glib main loop handler for the application.
 *
 * This file contains the glib implementation of the main loop handler, which is responsible
 * for managing the primary event loop of the application. It can utilize GLib or other
 * OS-specific internal mechanisms to handle events and dispatch them appropriately.
 *
 * @note This implementation is specific to the Linux operating system.
 */

 #define _GNU_SOURCE
 #include <config.h>

 #ifndef _WIN32

 #include <lib3270/defs.h>
 #include <lib3270.h>
 #include <lib3270/memory.h>
 #include <lib3270/toggle.h>
 #include <private/mainloop.h>
 #include <private/session.h>
 #include <private/linkedlist.h>
 #include <private/intl.h>
 #include <pthread.h>
 #include <string.h>
 #include <dlfcn.h>
 #include <stdint.h>

 #define MILLION 1000000L

 static enum GlibState {
	GLIB_NOT_INITIALIZED,
	GLIB_NOT_AVAILABLE,
	GLIB_AVAILABLE
 } glibstate = GLIB_NOT_INITIALIZED;


 enum GlibMethod {
	G_TIMEOUT_ADD_FULL,
	G_SOURCE_REMOVE,
	G_IO_CHANNEL_UNIX_NEW,
	G_IO_CHANNEL_UNREF,
	G_IO_CHANNEL_SET_ENCODING,
	G_IO_ADD_WATCH_FULL,
 	G_IO_CHANNEL_SET_BUFFERED,
 	G_IO_CHANNEL_UNIX_GET_FD,
	G_MAIN_CONTEXT_GET_THREAD_DEFAULT,
	G_MAIN_LOOP_NEW,
	G_MAIN_LOOP_RUN,
	G_MAIN_LOOP_UNREF,
	G_MAIN_LOOP_QUIT,
	G_THREAD_NEW,
	G_THREAD_JOIN,
	G_IDLE_ADD_ONCE,
	G_MAIN_CONTEXT_ITERATION,

	GLIB_METHOD_COUNT
 };

 static const char * glibnames[GLIB_METHOD_COUNT] = {
	"g_timeout_add_full",
	"g_source_remove",
	"g_io_channel_unix_new",
	"g_io_channel_unref",
	"g_io_channel_set_encoding",
	"g_io_add_watch_full",
	"g_io_channel_set_buffered",
	"g_io_channel_unix_get_fd",
	"g_main_context_get_thread_default",
	"g_main_loop_new",
	"g_main_loop_run",
	"g_main_loop_unref",
	"g_main_loop_quit",
	"g_thread_new",
	"g_thread_join",
	"g_idle_add_once",
	"g_main_context_iteration"

 };

 static void * glibmethods[GLIB_METHOD_COUNT];

 typedef struct {
	H3270 *hSession;
	void *userdata;
	int (*proc)(H3270 *, void *);
 } TimerSource;

 typedef struct {
	H3270 *hSession;
	int enabled;
	void *userdata;
	void *channel;
	LIB3270_IO_FLAG	  flag;
	void (*call)(H3270 *, int, LIB3270_IO_FLAG, void *);
 } PollSource;

 static int do_timer(TimerSource *timer) {
	debug("----------------- %s: %p",__FUNCTION__,timer);
	timer->proc(timer->hSession,timer->userdata);
	return 0;
 }

 static void free_timer(TimerSource *timer) {	
	debug("----------------- %s: %p",__FUNCTION__,timer);
	lib3270_free(timer);
 }

 static void * gui_timer_add(H3270 *hSession, unsigned long interval_ms, int (*proc)(H3270 *session, void *userdata), void *userdata) {

	unsigned int (*g_timeout_add_full)(int priority, unsigned int interval, void *function, void *data, void *notify) 
		= glibmethods[G_TIMEOUT_ADD_FULL];

	TimerSource *timer = (TimerSource *) lib3270_malloc(sizeof(TimerSource));
	timer->hSession = hSession;
	timer->userdata = userdata;
	timer->proc = proc;

	intptr_t rc = 
			g_timeout_add_full(
					0, 					// G_PRIORITY_DEFAULT, 
					interval_ms,		// Interval 
					(void *) do_timer, 	// Func
					(void *) timer, 	// data
					(void *) free_timer // cleanup
			);

	debug("----------------- %s: %p",__FUNCTION__,timer);
	return (void *) rc;
 }

 static void gui_source_remove(H3270 *session, void *id) {
	debug("----------------- %s: %p",__FUNCTION__,id);
	int (*g_source_remove)(unsigned int id) = glibmethods[G_SOURCE_REMOVE];
	g_source_remove((intptr_t) id);
 }

 static void free_channel(PollSource *ps) {	
	debug("----------------- %s: %p",__FUNCTION__,ps);
	void (*g_io_channel_unref)(void *channel) 
		= glibmethods[G_IO_CHANNEL_UNREF];
	g_io_channel_unref(ps->channel);
	lib3270_free(ps);
 }

 static const struct {
	LIB3270_IO_FLAG flag;
	int condition;
 } conditions[] = {
	{ LIB3270_IO_FLAG_READ,			1|2 },	// IN|PRI
	{ LIB3270_IO_FLAG_EXCEPTION,	8|16 }, // ERR|HUP
	{ LIB3270_IO_FLAG_WRITE,		4 }		// OUT
 };

 static int do_channel(void* source, int condition, PollSource *ps) {

	debug("----------------- %s: %p (%p)",__FUNCTION__,ps,source);

	int (*g_io_channel_unix_get_fd)(void* channel) = glibmethods[G_IO_CHANNEL_UNIX_GET_FD];

	int sock = g_io_channel_unix_get_fd(ps->channel);
	LIB3270_IO_FLAG flag = 0;

	size_t ix;
	for(ix = 0; ix < (sizeof(conditions)/sizeof(conditions[0])); ix++) {
		if(condition & conditions[ix].condition) {
			flag |= conditions[ix].flag;
		}
	}

	ps->call(ps->hSession,sock,flag,ps->userdata);

	return ps->enabled;
 }

 static void * gui_poll_add(H3270 *session, int fd, LIB3270_IO_FLAG flag, void(*proc)(H3270 *, int, LIB3270_IO_FLAG, void *), void *userdata ) {
	
	void (*g_io_channel_set_buffered)(void *channel,int buffered) 
		= glibmethods[G_IO_CHANNEL_SET_BUFFERED];

	void * (*g_io_channel_unix_new)(int fd) 
		= glibmethods[G_IO_CHANNEL_UNIX_NEW];

 	int (*g_io_channel_set_encoding)(void *channel, const char *encoding, void *error) 
		= glibmethods[G_IO_CHANNEL_SET_ENCODING];

	unsigned int (*g_io_add_watch_full)(void* channel, int priority, int condition, void *func, void *user_data, void *notify)
		= glibmethods[G_IO_ADD_WATCH_FULL];

	
	PollSource *ps = (PollSource *) lib3270_malloc(sizeof(PollSource));

	ps->enabled = 1;
	ps->hSession = session;
	ps->userdata = userdata;
	ps->channel = g_io_channel_unix_new(fd);
	ps->flag = flag;
	ps->call = proc;

	g_io_channel_set_encoding(ps->channel,NULL,NULL);
	g_io_channel_set_buffered(ps->channel,0);

	int condition = 0; 

	size_t ix;
	for(ix = 0; ix < (sizeof(conditions)/sizeof(conditions[0])); ix++) {
		if(flag & conditions[ix].flag) {
			condition |= conditions[ix].condition;
		}
	}

	intptr_t id = g_io_add_watch_full(
		ps->channel,
		0,
		condition,
		(void *) do_channel,
		ps,
		(void *) free_channel
	);

	debug("----------------- %s: %p (%p)",__FUNCTION__,ps,(void *) id);
	return (void *) id;
 }

 static int gui_wait_complete(void *loop) {
	void (*g_main_loop_quit)(void *loop) =
		glibmethods[G_MAIN_LOOP_QUIT];

	g_main_loop_quit(loop);
	return 0;
 }

 static int gui_wait(H3270 *hSession, int seconds) {
	void * (*g_main_context_get_thread_default)() =
		glibmethods[G_MAIN_CONTEXT_GET_THREAD_DEFAULT];

	void * (*g_main_loop_new)(void * context, int is_running) =
		glibmethods[G_MAIN_LOOP_NEW];

	void (*g_main_loop_run)(void  * loop) =
		glibmethods[G_MAIN_LOOP_RUN];

	void (*g_main_loop_unref)(void *loop) =
		glibmethods[G_MAIN_LOOP_UNREF];

	unsigned int (*g_timeout_add_full)(int priority, unsigned int interval, void *function, void *data, void *notify) 
		= glibmethods[G_TIMEOUT_ADD_FULL];

	void *loop = g_main_loop_new(g_main_context_get_thread_default(), 0);

	g_timeout_add_full(
			0, 								// G_PRIORITY_DEFAULT, 
			seconds * 1000L,				// Interval 
			(void *) gui_wait_complete, 	// Func
			(void *) loop, 					// data
			(void *) NULL 					// cleanup
	);

	g_main_loop_run(loop);
	g_main_loop_unref(loop);

	return 0;
 }

 typedef struct {
	H3270 *hSession;
	void *mainloop;
	void *parm;
	int(*callback)(H3270 *, void *);
 } ThreadData;

 static void * gui_thread_complete(ThreadData *data) {
	void (*g_main_loop_quit)(void *loop) =
		glibmethods[G_MAIN_LOOP_QUIT];
	g_main_loop_quit(data->mainloop);
	return 0;
 }

 static void * gui_thread(ThreadData *data) {

	unsigned int (*g_idle_add_once)(void *function, void * data) =
		glibmethods[G_IDLE_ADD_ONCE];
		

	intptr_t rc = data->callback(data->hSession,data->parm);

	g_idle_add_once((void *) gui_thread_complete, data);

	return (void *) rc;
 }

 static int gui_run(H3270 *hSession, const char *name, int(*callback)(H3270 *, void *), void *parm) {

	void * (*g_thread_new)(const char *name, void *(*func)(void *), void *data) =
		glibmethods[G_THREAD_NEW];

	void * (*g_thread_join)(void *thread) =
		glibmethods[G_THREAD_JOIN];

	void * (*g_main_context_get_thread_default)() =
		glibmethods[G_MAIN_CONTEXT_GET_THREAD_DEFAULT];

	void * (*g_main_loop_new)(void * context, int is_running) =
		glibmethods[G_MAIN_LOOP_NEW];

	void (*g_main_loop_run)(void  * loop) =
		glibmethods[G_MAIN_LOOP_RUN];

	void (*g_main_loop_unref)(void *loop) =
		glibmethods[G_MAIN_LOOP_UNREF];

	ThreadData td;
	td.hSession = hSession;
	td.parm = parm;
	td.callback = callback;
	td.mainloop = g_main_loop_new(g_main_context_get_thread_default(), 0);

	void *thread = g_thread_new(name,(void *(*)(void *)) gui_thread,&td);

	g_main_loop_run(td.mainloop);
	g_main_loop_unref(td.mainloop);

	return (intptr_t) g_thread_join(thread);

 }
 
 typedef struct {
	H3270 *hSession;
	void (*callback)(H3270 *hSession, void *);
 } PostData;

 static void post_complete(PostData *pd) {
	pd->callback(pd->hSession,(pd+1));
	lib3270_free(pd);
 }

 static	void gui_post(H3270 *hSession, void(*callback)(H3270 *session, void *), void *parm, size_t parmlen) {

	unsigned int (*g_idle_add_once)(void *function, void * data) =
		glibmethods[G_IDLE_ADD_ONCE];

	PostData *pd = (PostData *) lib3270_malloc(sizeof(PostData)+parmlen+1);
	pd->hSession = hSession;
	pd->callback = callback;
	memcpy((pd+1),parm,parmlen);

	g_idle_add_once((void *) post_complete, pd);

 }

 static int gui_event_dispatcher(H3270 *hSession, int block) {

	void * (*g_main_context_get_thread_default)() =
		glibmethods[G_MAIN_CONTEXT_GET_THREAD_DEFAULT];

	int (*g_main_context_iteration)(void* context, int may_block) =
		glibmethods[G_MAIN_CONTEXT_ITERATION];
 
	return g_main_context_iteration(g_main_context_get_thread_default(),block);

 }

 LIB3270_INTERNAL int setup_glib_mainloop(H3270 *hSession) {

	if(glibstate == GLIB_NOT_AVAILABLE) {
		return ENODATA;
	}

	size_t ix;
	for(ix = 0; ix < GLIB_METHOD_COUNT; ix++) {
		dlerror();
		glibmethods[ix] = dlsym(RTLD_DEFAULT, glibnames[ix]);
		if(dlerror() != NULL) {
			glibstate = GLIB_NOT_AVAILABLE;
			debug("%s: Error loading %s", __FUNCTION__, glibnames[ix]);
			return ENODATA;
		}
	}

	// Set glib mainloop implementation.
	hSession->timer.add = gui_timer_add;
	hSession->timer.remove = gui_source_remove;

	hSession->poll.add = gui_poll_add;
	hSession->poll.remove = gui_source_remove;

	hSession->event_dispatcher = gui_event_dispatcher;
	hSession->run = gui_run;
	hSession->post = gui_post;

	hSession->wait = gui_wait;

	debug("%s: GLib mainloop implementation enabled",__FUNCTION__);

	return 0;

}

#endif // !_WIN32

