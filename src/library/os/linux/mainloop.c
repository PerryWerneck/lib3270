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
 * @brief Implements the default main loop handler for the application.
 *
 * This file contains the default implementation of the main loop handler, which is responsible
 * for managing the primary event loop of the application. It can utilize GLib or other
 * OS-specific internal mechanisms to handle events and dispatch them appropriately.
 *
 * @note This implementation is specific to the Linux operating system.
 */

 #define _GNU_SOURCE
 #include <config.h>
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

	timeout_t *t_new = lib3270_new(timeout_t);
	timeout_t *t = NULL;
	timeout_t *prev = NULL;

	t_new->call = proc;
	t_new->userdata = userdata;
	t_new->in_play = 0;

	debug("%s: timer=%p session=%p interval=%lu", __FUNCTION__, t_new, session, interval_ms);

	pthread_mutex_lock(&guard);

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
		t = (struct timeout *) hSession->timer.context->first;
		pthread_mutex_unlock(&guard);

		while(t) {

			if (t->tv.tv_sec < now.tv_sec ||(t->tv.tv_sec == now.tv_sec && t->tv.tv_usec < now.tv_usec)) {

				t->in_play = 1;
				debug("do_timer: timer=%p session=%p", t, hSession);
				t->in_play = 1;
				(*t->call)(hSession,t->userdata);
				t->in_play = 0;

				pthread_mutex_lock(&guard);
				lib3270_linked_list_delete_node((lib3270_linked_list *) hSession->timer.context,t);
				pthread_mutex_unlock(&guard);

				processed_any = 1;

			} else {

				break;

			}

			pthread_mutex_lock(&guard);
			t = (struct timeout *) hSession->timer.context->first;
			pthread_mutex_unlock(&guard);
	
		}

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

 static void timer_finalize(H3270 *session, LIB3270_TIMER_CONTEXT * context) {
	lib3270_linked_list_free((lib3270_linked_list *) context);
	lib3270_free(context);
 }

 static	void poll_finalize(H3270 *session, LIB3270_POLL_CONTEXT * context) {
	lib3270_linked_list_free((lib3270_linked_list *) context);
	lib3270_free(context);
 }

 /// @brief Set default mainloop implementation.
 LIB3270_INTERNAL void setup_default_mainloop(H3270 *hSession) {

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

