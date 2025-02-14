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
 * @brief Implements the main loop handler for the application.
 *
 * This file contains the implementation of the main loop handler, which is responsible
 * for managing the primary event loop of the application. It can utilize GLib or other
 * OS-specific internal mechanisms to handle events and dispatch them appropriately.
 *
 * @note This implementation is specific to the Linux operating system.
 */

 #define _GNU_SOURCE
 #include <config.h>
 #include <lib3270/defs.h>
 #include <lib3270.h>
 #include <lib3270/toggle.h>
 #include <private/mainloop.h>
 #include <private/session.h>
 #include <private/linkedlist.h>
 #include <private/intl.h>
 #include <pthread.h>
 #include <string.h>
 #include <dlfcn.h>

 #define MILLION 1000000L

 //
 // Default mainloop implementation.
 //
 static pthread_mutex_t guard = PTHREAD_MUTEX_INITIALIZER;

 struct _lib3270_poll_context {
	LIB3270_LINKED_LIST
	unsigned int changed;
 };

 struct _lib3270_timer_context {
	LIB3270_LINKED_LIST
 };

 typedef struct timeout {
	LIB3270_LINKED_LIST_HEAD
	unsigned char in_play;
	struct timeval tv;
	int (*call)(H3270 *, void *);
 } timeout_t;

 typedef struct _input_t {
	LIB3270_LINKED_LIST_HEAD
	unsigned char	  enabled;
	int 			  fd;
	LIB3270_IO_FLAG	  flag;
	void (*call)(H3270 *, int, LIB3270_IO_FLAG, void *);
 } input_t;

 static void * default_timer_add(H3270 *session, unsigned long interval_ms, int (*proc)(H3270 *session, void *userdata), void *userdata) {

	debug("%s: session=%p interval=%lu", __FUNCTION__, session, interval_ms);

	pthread_mutex_lock(&guard);

	timeout_t *t_new = (timeout_t *) lib3270_malloc(sizeof(timeout_t));
	timeout_t *t = NULL;
	timeout_t *prev = NULL;

	t_new->call = proc;
	t_new->userdata = userdata;
	t_new->in_play = 0;

	gettimeofday(&t_new->tv, NULL);
	t_new->tv.tv_sec += interval_ms / 1000L;
	t_new->tv.tv_usec += (interval_ms % 1000L) * 1000L;

	if (t_new->tv.tv_usec > MILLION) {
		t_new->tv.tv_sec += t_new->tv.tv_usec / MILLION;
		t_new->tv.tv_usec %= MILLION;
	}

	// Find where to insert this item.
	for (t = (timeout_t *) session->timer.context->first; t != NULL; t = (timeout_t *) t->next) {
		if (t->tv.tv_sec > t_new->tv.tv_sec || (t->tv.tv_sec == t_new->tv.tv_sec && t->tv.tv_usec > t_new->tv.tv_usec))
			break;
		prev = t;
	}

	// Insert it.
	if (prev == NULL) {
		// t_new is Front.
		t_new->next = session->timer.context->first;
		session->timer.context->first = (struct lib3270_linked_list_node *) t_new;
	} else if (t == NULL) {
		// t_new is Rear.
		t_new->next = NULL;
		prev->next = (struct lib3270_linked_list_node *) t_new;
		session->timer.context->last = (struct lib3270_linked_list_node *) t_new;
	} else {
		// t_new is Middle.
		t_new->next = (struct lib3270_linked_list_node *) t;
		prev->next = (struct lib3270_linked_list_node *) t_new;
	}

	pthread_mutex_unlock(&guard);

	debug("%s: session=%p timer=%p", __FUNCTION__, session, t_new);

	return t_new;

 }

 static void default_timer_remove(H3270 *session, void *timer) {

	debug("%s: session=%p timer=%p", __FUNCTION__, session, timer);

	if(timer) {
		pthread_mutex_lock(&guard);
		if(!((timeout_t *)timer)->in_play)
			lib3270_linked_list_delete_node( (lib3270_linked_list *) session->timer.context,timer);
		pthread_mutex_unlock(&guard);
	}

 }

 static void * default_poll_add(H3270 *session, int fd, LIB3270_IO_FLAG flag, void(*proc)(H3270 *, int, LIB3270_IO_FLAG, void *), void *userdata ) {

	pthread_mutex_lock(&guard);

	input_t *ip = (input_t *) lib3270_linked_list_append_node(
									(lib3270_linked_list *) session->poll.context,
									sizeof(input_t), 
									userdata
								);

	ip->enabled					= 1;
	ip->fd						= fd;
	ip->flag					= flag;
	ip->call					= proc;
	session->poll.context->changed = 1;

	pthread_mutex_unlock(&guard);

	return ip;

 }

 static void default_poll_remove(H3270 *hSession, void *id) {

	if(id) {
		pthread_mutex_lock(&guard);
		lib3270_linked_list_delete_node( (lib3270_linked_list *) hSession->poll.context,id);
		hSession->poll.context->changed = 1;
		pthread_mutex_unlock(&guard);
	}

 }

 static int default_event_dispatcher(H3270 *hSession, int block) {
	int ns;
	struct timeval now, twait, *tp;
	int events;

	fd_set rfds, wfds, xfds;

	input_t *ip;
	int processed_any = 0;

 retry:

	hSession->poll.context->changed = 0;

	// If we've processed any input, then don't block again.
	if(processed_any)
		block = 0;

	events = 0;

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_ZERO(&xfds);

	pthread_mutex_lock(&guard);
	for (ip = (input_t *) hSession->poll.context->first; ip != (input_t *)NULL; ip = (input_t *) ip->next) {
		if(!ip->enabled) {
			debug("Socket %d is disabled",ip->fd);
			continue;
		}

		if(ip->flag & LIB3270_IO_FLAG_READ) {
			FD_SET(ip->fd, &rfds);
			events++;
		}

		if(ip->flag & LIB3270_IO_FLAG_WRITE) {
			FD_SET(ip->fd, &wfds);
			events++;
		}

		if(ip->flag & LIB3270_IO_FLAG_EXCEPTION) {
			FD_SET(ip->fd, &xfds);
			events++;
		}
	}
	pthread_mutex_unlock(&guard);

	if (block) {
		if (hSession->timer.context->first) {
			(void) gettimeofday(&now, (void *)NULL);
			twait.tv_sec = ((timeout_t *) hSession->timer.context->first)->tv.tv_sec - now.tv_sec;
			twait.tv_usec = ((timeout_t *) hSession->timer.context->first)->tv.tv_usec - now.tv_usec;
			if (twait.tv_usec < 0L) {
				twait.tv_sec--;
				twait.tv_usec += MILLION;
			}
			if (twait.tv_sec < 0L)
				twait.tv_sec = twait.tv_usec = 0L;
			tp = &twait;
		} else {
			twait.tv_sec = 1;
			twait.tv_usec = 0L;
			tp = &twait;
		}
	} else {
		twait.tv_sec  = 0;
		twait.tv_usec = 10L;
		tp = &twait;

		if(!events)
			return processed_any;
	}

	ns = select(FD_SETSIZE, &rfds, &wfds, &xfds, tp);

	if (ns < 0 && errno != EINTR) {
		lib3270_popup_dialog(	hSession,
		                        LIB3270_NOTIFY_ERROR,
		                        _( "Network error" ),
		                        _( "Select() failed when processing for events." ),
		                        "%s",
		                        strerror(errno));
	} else {

		for (ip = (input_t *) hSession->poll.context->first; ip != (input_t *) NULL; ip = (input_t *) ip->next) {
			if((ip->flag & LIB3270_IO_FLAG_READ) && FD_ISSET(ip->fd, &rfds)) {
				(*ip->call)(hSession,ip->fd,LIB3270_IO_FLAG_READ,ip->userdata);
				processed_any = 1;
				if (hSession->poll.context->changed)
					goto retry;
			}

			if((ip->flag & LIB3270_IO_FLAG_WRITE) && FD_ISSET(ip->fd, &wfds)) {
				(*ip->call)(hSession,ip->fd,LIB3270_IO_FLAG_WRITE,ip->userdata);
				processed_any = 1;
				if (hSession->poll.context->changed)
					goto retry;
			}

			if((ip->flag & LIB3270_IO_FLAG_EXCEPTION) && FD_ISSET(ip->fd, &xfds)) {
				(*ip->call)(hSession,ip->fd,LIB3270_IO_FLAG_EXCEPTION,ip->userdata);
				processed_any = 1;
				if (hSession->poll.context->changed)
					goto retry;
			}
		}

	}

	// See what's expired.
	if (hSession->timer.context->first) {
		struct timeout *t;
		(void) gettimeofday(&now, (void *)NULL);

		pthread_mutex_lock(&guard);
		while(hSession->timer.context->first) {
			t = (struct timeout *) hSession->timer.context->first;

			if (t->tv.tv_sec < now.tv_sec ||(t->tv.tv_sec == now.tv_sec && t->tv.tv_usec < now.tv_usec)) {
				t->in_play = 1;

				(*t->call)(hSession,t->userdata);
				lib3270_linked_list_delete_node((lib3270_linked_list *) hSession->timer.context,t);

				processed_any = 1;


			} else {
				break;
			}

		}
		pthread_mutex_unlock(&guard);

	}

	if (hSession->poll.context->changed)
		goto retry;

	return processed_any;

 }

 static int default_wait(H3270 *hSession, int seconds) {
	time_t end = time(0) + seconds;
	while(time(0) < end) {
		hSession->event_dispatcher(hSession,1);
	}
	return 0;
 }

 static int default_run(H3270 *hSession, const char *name, int(*callback)(H3270 *, void *), void *parm) {
	return callback(hSession,parm);
 }

 static	void default_post(void(*callback)(void *), void *parm, size_t len) {
	callback(parm);
 }

 //
 //	Setup mainloop implementation for the session.
 //
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
	void *userdata;
	void *channel;
	LIB3270_IO_FLAG	  flag;
	void (*call)(H3270 *, int, LIB3270_IO_FLAG, void *);
 } PollSource;

 static int do_timer(TimerSource *timer) {
	timer->proc(timer->hSession,timer->userdata);
	return 0;
 }

 static void free_timer(TimerSource *timer) {	
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

	return (void *) rc;
 }

 static void gui_source_remove(H3270 *session, void *id) {
	int (*g_source_remove)(unsigned int id) = glibmethods[G_SOURCE_REMOVE];
	g_source_remove((intptr_t) id);
 }

 static void free_channel(PollSource *ps) {	
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

	return 0;
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

 void * gui_thread_complete(ThreadData *data) {
	void (*g_main_loop_quit)(void *loop) =
		glibmethods[G_MAIN_LOOP_QUIT];
	g_main_loop_quit(data->mainloop);
	return 0;
 }

 void * gui_thread(ThreadData *data) {

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
	void (*callback)(void *);
	u_int8_t parm[0];
 } PostData;

 static void post_complete(PostData *pd) {
	pd->callback(pd->parm);
	lib3270_free(pd);
 }

 static	void gui_post(void(*callback)(void *), void *parm, size_t parmlen) {

	unsigned int (*g_idle_add_once)(void *function, void * data) =
		glibmethods[G_IDLE_ADD_ONCE];

	PostData *pd = (PostData *) lib3270_malloc(sizeof(PostData)+parmlen+1);
	pd->callback = callback;
	memcpy(pd->parm,parm,parmlen);

	g_idle_add_once((void *) post_complete, pd);

 }

 static int gui_event_dispatcher(H3270 *hSession, int block) {

	void * (*g_main_context_get_thread_default)() =
		glibmethods[G_MAIN_CONTEXT_GET_THREAD_DEFAULT];

	int (*g_main_context_iteration)(void* context, int may_block) =
		glibmethods[G_MAIN_CONTEXT_ITERATION];
 
	return g_main_context_iteration(g_main_context_get_thread_default(),block);

 }

 static void timer_finalize(H3270 *session, LIB3270_TIMER_CONTEXT * context) {
	lib3270_linked_list_free((lib3270_linked_list *) context);
	lib3270_free(context);
 }

 static	void poll_finalize(H3270 *session, LIB3270_POLL_CONTEXT * context) {
	lib3270_linked_list_free((lib3270_linked_list *) context);
	lib3270_free(context);
 }

 /// @brief Set default mainloop implementation.
 static void lib3270_setup_internal_mainloop(H3270 *hSession) {

	debug("%s: Internal mainloop implementation enabled",__FUNCTION__);

 	hSession->timer.add = default_timer_add;
 	hSession->timer.remove = default_timer_remove;
	hSession->timer.context = lib3270_malloc(sizeof(struct _lib3270_timer_context));
	hSession->timer.finalize = timer_finalize;
	memset(hSession->timer.context,0,sizeof(struct _lib3270_timer_context));

 	hSession->poll.add = default_poll_add;
 	hSession->poll.remove = default_poll_remove;
	hSession->poll.context = lib3270_malloc(sizeof(struct _lib3270_poll_context));
	hSession->poll.finalize = poll_finalize;
	memset(hSession->poll.context,0,sizeof(struct _lib3270_poll_context));

 	hSession->event_dispatcher = default_event_dispatcher;
 	hSession->run = default_run;
	hSession->post = default_post;

 	hSession->wait = default_wait;

 }

 int lib3270_setup_mainloop(H3270 *hSession, int glib) {

	if(!glib || glibstate == GLIB_NOT_AVAILABLE) {
		lib3270_setup_internal_mainloop(hSession);
		return 0;
	}

	size_t ix;
	for(ix = 0; ix < GLIB_METHOD_COUNT; ix++) {
		dlerror();
		glibmethods[ix] = dlsym(RTLD_DEFAULT, glibnames[ix]);
		if(dlerror() != NULL) {
			glibstate = GLIB_NOT_AVAILABLE;
			debug("%s: Error loading %s", __FUNCTION__, glibnames[ix]);
			lib3270_setup_internal_mainloop(hSession);
			return -1;
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

	return 1;
 }

