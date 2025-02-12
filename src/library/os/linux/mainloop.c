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

 #define _GNU_SOURCE
 #include <config.h>
 #include <lib3270/defs.h>
 #include <lib3270.h>
 #include <private/mainloop.h>
 #include <private/session.h>
 #include <private/linkedlist.h>
 #include <private/intl.h>
 #include <pthread.h>
 #include <string.h>

 #define MILLION 1000000L

 //
 // Default mainloop implementation.
 //
 static pthread_mutex_t guard = PTHREAD_MUTEX_INITIALIZER;

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

	debug("%s: session=%p interval=%u", __FUNCTION__, session, interval_ms);

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
	for (t = (timeout_t *) session->timeouts.first; t != NULL; t = (timeout_t *) t->next) {
		if (t->tv.tv_sec > t_new->tv.tv_sec || (t->tv.tv_sec == t_new->tv.tv_sec && t->tv.tv_usec > t_new->tv.tv_usec))
			break;
		prev = t;
	}

	// Insert it.
	if (prev == NULL) {
		// t_new is Front.
		t_new->next = session->timeouts.first;
		session->timeouts.first = (struct lib3270_linked_list_node *) t_new;
	} else if (t == NULL) {
		// t_new is Rear.
		t_new->next = NULL;
		prev->next = (struct lib3270_linked_list_node *) t_new;
		session->timeouts.last = (struct lib3270_linked_list_node *) t_new;
	} else {
		// t_new is Middle.
		t_new->next = (struct lib3270_linked_list_node *) t;
		prev->next = (struct lib3270_linked_list_node *) t_new;
	}

	pthread_mutex_unlock(&guard);

	debug("%s: session=%p timer=%u", __FUNCTION__, session, t_new);

	return t_new;

 }

 static void default_timer_remove(H3270 *session, void *timer) {

	debug("%s: session=%p timer=%p", __FUNCTION__, session, timer);

	if(timer) {
		pthread_mutex_lock(&guard);
		if(!((timeout_t *)timer)->in_play)
			lib3270_linked_list_delete_node(&session->timeouts,timer);
		pthread_mutex_unlock(&guard);
	}

 }

 static void * default_poll_add(H3270 *session, int fd, LIB3270_IO_FLAG flag, void(*proc)(H3270 *, int, LIB3270_IO_FLAG, void *), void *userdata ) {

	pthread_mutex_lock(&guard);

	input_t *ip = (input_t *) lib3270_linked_list_append_node(&session->input.list,sizeof(input_t), userdata);

	ip->enabled					= 1;
	ip->fd						= fd;
	ip->flag					= flag;
	ip->call					= proc;
	session->input.changed = 1;

	pthread_mutex_unlock(&guard);

	return ip;

 }

 static void default_poll_remove(H3270 *hSession, void *id) {

	if(id) {
		pthread_mutex_lock(&guard);
		lib3270_linked_list_delete_node(&hSession->input.list,id);
		hSession->input.changed = 1;
		pthread_mutex_unlock(&guard);
	}

 }

 static void default_poll_set_state(H3270 *session, void *id, int enabled) {

	input_t *ip;

	pthread_mutex_lock(&guard);
	for (ip = (input_t *) session->input.list.first; ip; ip = (input_t *) ip->next) {
		if (ip == (input_t *)id) {
			ip->enabled = enabled ? 1 : 0;
			session->input.changed = 1;
			break;
		}
	}
	pthread_mutex_unlock(&guard);

 }

 static int default_event_dispatcher(H3270 *hSession, int block) {
	int ns;
	struct timeval now, twait, *tp;
	int events;

	fd_set rfds, wfds, xfds;

	input_t *ip;
	int processed_any = 0;

 retry:

	hSession->input.changed = 0;

	// If we've processed any input, then don't block again.
	if(processed_any)
		block = 0;

	events = 0;

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_ZERO(&xfds);

	pthread_mutex_lock(&guard);
	for (ip = (input_t *) hSession->input.list.first; ip != (input_t *)NULL; ip = (input_t *) ip->next) {
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
		if (hSession->timeouts.first) {
			(void) gettimeofday(&now, (void *)NULL);
			twait.tv_sec = ((timeout_t *) hSession->timeouts.first)->tv.tv_sec - now.tv_sec;
			twait.tv_usec = ((timeout_t *) hSession->timeouts.first)->tv.tv_usec - now.tv_usec;
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

		for (ip = (input_t *) hSession->input.list.first; ip != (input_t *) NULL; ip = (input_t *) ip->next) {
			if((ip->flag & LIB3270_IO_FLAG_READ) && FD_ISSET(ip->fd, &rfds)) {
				(*ip->call)(hSession,ip->fd,LIB3270_IO_FLAG_READ,ip->userdata);
				processed_any = 1;
				if (hSession->input.changed)
					goto retry;
			}

			if((ip->flag & LIB3270_IO_FLAG_WRITE) && FD_ISSET(ip->fd, &wfds)) {
				(*ip->call)(hSession,ip->fd,LIB3270_IO_FLAG_WRITE,ip->userdata);
				processed_any = 1;
				if (hSession->input.changed)
					goto retry;
			}

			if((ip->flag & LIB3270_IO_FLAG_EXCEPTION) && FD_ISSET(ip->fd, &xfds)) {
				(*ip->call)(hSession,ip->fd,LIB3270_IO_FLAG_EXCEPTION,ip->userdata);
				processed_any = 1;
				if (hSession->input.changed)
					goto retry;
			}
		}

	}

	// See what's expired.
	if (hSession->timeouts.first) {
		struct timeout *t;
		(void) gettimeofday(&now, (void *)NULL);

		pthread_mutex_lock(&guard);
		while(hSession->timeouts.first) {
			t = (struct timeout *) hSession->timeouts.first;

			if (t->tv.tv_sec < now.tv_sec ||(t->tv.tv_sec == now.tv_sec && t->tv.tv_usec < now.tv_usec)) {
				t->in_play = 1;

				(*t->call)(hSession,t->userdata);
				lib3270_linked_list_delete_node(&hSession->timeouts,t);

				processed_any = 1;


			} else {
				break;
			}

		}
		pthread_mutex_unlock(&guard);

	}

	if (hSession->input.changed)
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

 static void default_ring_bell(H3270 *hSession) {

 }

 static int default_run(H3270 *hSession, const char *name, int(*callback)(H3270 *, void *), void *parm) {
	return callback(hSession,parm);
 }

 //	Setup mainloop implementation for the session.
 void lib3270_setup_mainloop(H3270 *hSession, int gui) {

	// Set default mainloop implementation.
 	hSession->io.timer.add = default_timer_add;
 	hSession->io.timer.remove = default_timer_remove;
 	hSession->io.poll.add = default_poll_add;
 	hSession->io.poll.remove = default_poll_remove;
 	hSession->io.poll.set_state = default_poll_set_state;
 	hSession->event_dispatcher = default_event_dispatcher;
 	hSession->wait = default_wait;
 	hSession->ring_bell = default_ring_bell;
 	hSession->run = default_run;

	// TODO: Implement GUI mainloop.

 }

