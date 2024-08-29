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
  * @brief Implements default main loop.
  */

 #include <config.h>
 #include <stdio.h>
 #include <poll.h>
 #include <assert.h>
 #include <time.h>
 #include <string.h>
 #include <sys/time.h>
 #include <lib3270/tools/mainloop.h>

 #ifdef HAVE_MALLOC_H
	#include <malloc.h>
 #endif // HAVE_MALLOC_H

 #define MILLION 1000000L

 //
 // Timers
 //
 /// @brief Timeout control structure.
 typedef struct timeout {
	struct timeout * prev;
	struct timeout * next;
	void * userdata;
 	unsigned char in_play;
#if defined(_WIN32)
	unsigned long long ts;
#else
	struct timeval tv;
#endif
	int (*proc)(void *userdata);
 } timeout_t;

 static struct _timeouts {
	timeout_t * first; \
	timeout_t * last; \
 } timeouts = { NULL, NULL };

 #if defined(_WIN32)
 static void ms_ts(unsigned long long *u) {
	FILETIME t;

	/* Get the system time, in 100ns units. */
	GetSystemTimeAsFileTime(&t);
	memcpy(u, &t, sizeof(unsigned long long));

	/* Divide by 10,000 to get ms. */
	*u /= 10000ULL;
 }
 #endif

 static void * add_timer(unsigned long interval_ms, int (*proc)(void *userdata), void *userdata) {

	timeout_t *t_new;
	timeout_t *t;
	timeout_t *prev = NULL;

	t_new = (timeout_t *) malloc(sizeof(timeout_t));
	memset(t_new,0,sizeof(timeout_t));

	t_new->proc = proc;
	t_new->userdata = userdata;
	t_new->in_play = 0;

#if defined(_WIN32)

	ms_ts(&t_new->ts);
	t_new->ts += interval_ms;

#else

	gettimeofday(&t_new->tv, NULL);
	t_new->tv.tv_sec += interval_ms / 1000L;
	t_new->tv.tv_usec += (interval_ms % 1000L) * 1000L;

	if (t_new->tv.tv_usec > MILLION) {
		t_new->tv.tv_sec += t_new->tv.tv_usec / MILLION;
		t_new->tv.tv_usec %= MILLION;
	}

#endif /*]*/

	/* Find where to insert this item. */
	for (t = timeouts.first; t; t = t->next) {
#if defined(_WIN32)
		if (t->ts > t_new->ts)
#else
		if (t->tv.tv_sec > t_new->tv.tv_sec || (t->tv.tv_sec == t_new->tv.tv_sec && t->tv.tv_usec > t_new->tv.tv_usec))
#endif
			break;

		prev = t;
	}

	// Insert it.
	if (!prev) {
		// t_new is Front.
		t_new->next = timeouts.first;
		timeouts.first = t_new;
	} else if (!t) {
		// t_new is Rear.
		t_new->next = NULL;
		prev->next = t_new;
		timeouts.last = t_new;
	} else {
		// t_new is Middle.
		t_new->next = t;
		prev->next = t_new;
	}

	return t_new;
 }

 static void remove_timer(void * timer) {
	timeout_t *st = (timeout_t *)timer;
	if(!st->in_play) {
		if(st->prev)
			st->prev->next = st->next;
		else
			timeouts.first = st->next;

		if(st->next)
			st->next->prev = st->prev;
		else
			timeouts.last = st->prev;
		free(st);
	}
 }

 //
 // Main loop
 //
 static const lib3270_main_loop *mainloop = NULL;

 LIB3270_EXPORT const lib3270_main_loop * lib3270_main_loop_get_instance() {
	if(mainloop) {
		return mainloop;
	}
	return mainloop = lib3270_main_loop_get_default();
 }

 static void wakeup() {

 }

 //
 // Input Events.
 //

 /// @brief I/O events.
 typedef struct input {
	struct input * prev;
	struct input * next;
	void * userdata;
	unsigned char enabled;
	int fd;
	int events;
	void (*call)(int, LIB3270_IO_EVENT, void *);
 } input_t;

 static struct _inputs {
	input_t * first; \
	input_t * last; \
 } inputs = { NULL, NULL };

 void * add_poll(int fd, LIB3270_IO_EVENT flag, void(*call)(int, LIB3270_IO_EVENT, void *), void *userdata ) {

	input_t *ip = (input_t *) malloc(sizeof(input_t));
	memset(ip,0,sizeof(input_t));

	ip->prev 		= inputs.last;
	inputs.last		= ip;

	ip->enabled		= 1;
	ip->userdata	= userdata;
	ip->fd			= fd;
	ip->call		= call;

	ip->events		= 0;
	if(flag & LIB3270_IO_EVENT_READ) {
		ip->events |= POLLIN;
	}

	if(flag & LIB3270_IO_EVENT_EXCEPTION) {
		ip->events |= POLLERR;
	}

	if(flag & LIB3270_IO_EVENT_WRITE) {
		ip->events |= POLLOUT;
	}

	if(flag & LIB3270_IO_EVENT_HANG_UP) {
		ip->events |= POLLHUP;
	}

	wakeup();

	return ip;
 }

 static void remove_poll(void *id) {

	input_t *ip = (input_t *) id;

	if(ip->prev)
		ip->prev->next = ip->next;
	else
		inputs.first = ip->next;

	if(ip->next)
		ip->next->prev = ip->prev;
	else
		inputs.last = ip->prev;

	free(ip);

	wakeup();

 }

 static int remove_poll_fd(int fd) {

	input_t *ip = inputs.first;

	while(ip) {
		if(ip->fd == fd) {

			if(ip->prev)
				ip->prev->next = ip->next;
			else
				inputs.first = ip->next;

			if(ip->next)
				ip->next->prev = ip->prev;
			else
				inputs.last = ip->prev;

			free(ip);

			wakeup();

			return 0;
		}
		ip = ip->next;
	}

	return ENOENT;

 }

 void set_poll_state(void *id, int enabled) {

	input_t *ip;
	for (ip = (input_t *) inputs.first; ip; ip = (input_t *) ip->next) {
		if (ip == (input_t *)id) {
			ip->enabled = enabled ? 1 : 0;
			wakeup();
			break;
		}

	}

 }

 static int wait(int seconds) {
	time_t end = time(0) + seconds;
	while(time(0) < end) {
		lib3270_main_loop_get_instance()->event_dispatcher((end - time(0)) * 1000);
	}
	return 0;
 }

 static void ring_bell() {
	return;
 }

 static int event_dispatcher(unsigned long ms) {

	int processed = 0;

	//
	// Get socket list.
	//
 	nfds_t nfds = 0;
	input_t *ip;
	for (ip = (input_t *) inputs.first; ip; ip = (input_t *) ip->next) {
		if(ip->enabled) {
			nfds++;
		}
	}

	struct pollfd fds[nfds];
	{
		nfds_t item = 0;
		for (ip = (input_t *) inputs.first; ip; ip = (input_t *) ip->next) {
			if(ip->enabled) {
				assert(item >= nfds);

				fds[item].fd = ip->fd;
				fds[item].revents = 0;
				fds[item].events = ip->events;

				item++;
			}
		}
	};

	// Check timeouts.
	{
		struct timeval now, twait;
		(void) gettimeofday(&now, (void *)NULL);

		while(timeouts.first) {
			if (timeouts.first->tv.tv_sec < now.tv_sec || (timeouts.first->tv.tv_sec == now.tv_sec && timeouts.first->tv.tv_usec < now.tv_usec)) {
				timeouts.first->in_play = 1;
				processed++;
				(*timeouts.first->proc)(timeouts.first->userdata);
				remove_timer(timeouts.first);
			} else {
				break;
			}
		}

		if(ms && timeouts.first) {

			// Check next timer.
			(void) gettimeofday(&now, (void *)NULL);
			twait.tv_sec = timeouts.first->tv.tv_sec - now.tv_sec;
			twait.tv_usec = timeouts.first->tv.tv_usec - now.tv_usec;
			if (twait.tv_usec < 0L) {
				twait.tv_sec--;
				twait.tv_usec += MILLION;
			}

			if (twait.tv_sec < 0L) {
				twait.tv_sec = twait.tv_usec = 0L;
			} else {
				twait.tv_sec = 1;
				twait.tv_usec = 0L;
			}

			unsigned long wait = (twait.tv_sec * 1000) + (twait.tv_usec/1000);
			if(wait < ms) {
				ms = wait;
			}

		}
	}

	// Wait for events.
	{
		int rc = poll(fds,nfds,ms);
		if(rc < 0) {
			return -errno;
		} else if(rc > 0) {

			size_t nfd;
			for(nfd = 0; nfd < nfds && rc; nfd++) {

				if(fds[nfd].revents) {
					input_t *ix;
					for(ix = inputs.first;ix;ix = ix->next) {
						if(ix->fd == fds[nfd].fd && ix->events == fds[nfd].events) {
							// Found event, execute it.
							LIB3270_IO_EVENT event = 0;
							if(fds[nfd].revents & POLLIN) {
								event |= LIB3270_IO_EVENT_READ;
							}

							if(fds[nfd].revents & POLLERR) {
								event |= LIB3270_IO_EVENT_EXCEPTION;
							}

							if(fds[nfd].revents & POLLOUT) {
								event |= LIB3270_IO_EVENT_WRITE;
							}

							if(fds[nfd].revents & POLLHUP) {
								event |= LIB3270_IO_EVENT_HANG_UP;
							}

							ix->call(fds[nfd].fd,event,ix->userdata);
							processed++;

							break;
						}
					}
					rc--;
				}

			}

		}
	}

	return processed;

 }

 static int run_task(int(*callback)(void *), void *userdata) {
	return callback(userdata);
 }

 LIB3270_EXPORT const lib3270_main_loop * lib3270_main_loop_get_default() {

	static lib3270_main_loop controller = {
		.sz = sizeof(lib3270_main_loop),
		.name = "internal",
		.add_timer = add_timer,
		.remove_timer = remove_timer,
		.add_poll = add_poll,
		.remove_poll = remove_poll,
		.remove_poll_fd = remove_poll_fd,
		.set_poll_state = set_poll_state,
		.wait = wait,
		.event_dispatcher = event_dispatcher,
		.ring_bell = ring_bell,
		.run_task = run_task
	};

	return &controller;
 }
