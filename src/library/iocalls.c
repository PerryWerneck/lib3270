/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270.
 *
 * Copyright (C) <2008> <Banco do Brasil S.A.>
 *
 * Este programa é software livre. Você pode redistribuí-lo e/ou modificá-lo sob
 * os termos da GPL v.2 - Licença Pública Geral  GNU,  conforme  publicado  pela
 * Free Software Foundation.
 *
 * Este programa é distribuído na expectativa de  ser  útil,  mas  SEM  QUALQUER
 * GARANTIA; sem mesmo a garantia implícita de COMERCIALIZAÇÃO ou  de  ADEQUAÇÃO
 * A QUALQUER PROPÓSITO EM PARTICULAR. Consulte a Licença Pública Geral GNU para
 * obter mais detalhes.
 *
 * Você deve ter recebido uma cópia da Licença Pública Geral GNU junto com este
 * programa; se não, escreva para a Free Software Foundation, Inc., 51 Franklin
 * St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Este programa está nomeado como iocalls.c e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 * licinio@bb.com.br		(Licínio Luis Branco)
 * kraucer@bb.com.br		(Kraucer Fernandes Mazuco)
 *
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
#include <private/mainloop.h>
#include <lib3270/mainloop.h>

#define MILLION			1000000L

/*---[ Standard calls ]-------------------------------------------------------------------------------------*/

// Timeout calls
static void   internal_remove_timer(H3270 *session, void *timer);
static void	* internal_add_timer(H3270 *session, unsigned long interval_ms, int (*proc)(H3270 *session, void *userdata), void *userdata);
static void	* internal_add_poll(H3270 *session, int fd, LIB3270_IO_FLAG flag, void(*proc)(H3270 *, int, LIB3270_IO_FLAG, void *), void *userdata );
static void	  internal_remove_poll(H3270 *session, void *id);
static void	  internal_set_poll_state(H3270 *session, void *id, int enabled);
static int    internal_wait(H3270 *session, int seconds);
static void	  internal_ring_bell(H3270 *session);
static int    internal_run_task(H3270 *session, const char *name, int(*callback)(H3270 *, void *), void *parm);

/*---[ Default callbacks ]----------------------------------------------------------------------------------*/

static struct {
	void	* (*add_timer)(H3270 *session, unsigned long interval_ms, int (*proc)(H3270 *session, void *userdata), void *userdata);
	void	  (*remove_timer)(H3270 *session, void *timer);
	void	* (*add_poll)(H3270 *session, int fd, LIB3270_IO_FLAG flag, void(*proc)(H3270 *, int, LIB3270_IO_FLAG, void *), void *userdata);
	void	  (*remove_poll)(H3270 *session, void *id);
	void	  (*set_poll_state)(H3270 *session, void *id, int enabled);
	int	  	  (*wait_callback)(H3270 *session, int seconds);
	int 	  (*event_dispatcher)(H3270 *session,int wait);
	void	  (*ring_bell)(H3270 *);
	int		  (*run_task)(H3270 *hSession, const char *name, int(*callback)(H3270 *h, void *), void *parm);
} defs = {
	internal_add_timer,
	internal_remove_timer,
	internal_add_poll,
	internal_remove_poll,
	internal_set_poll_state,
	internal_wait,
	lib3270_default_event_dispatcher,
	internal_ring_bell,
	internal_run_task
} ;


/*---[ Typedefs ]-------------------------------------------------------------------------------------------*/

#define TN	(timeout_t *)NULL

/*---[ Implement ]------------------------------------------------------------------------------------------*/

/*
void lib3270_setup_mainloop(H3270 *hSession) {
	hSession->timer.add = defs.add_timer;
	hSession->timer.remove = defs.remove_timer;
	hSession->poll.add = defs.add_poll;
	hSession->poll.remove = defs.remove_poll;
	hSession->poll.set_state = defs.set_poll_state;
	hSession->wait = defs.wait_callback;
	hSession->run = defs.run_task;
	hSession->event_dispatcher = defs.event_dispatcher;
}
*/

/* Timeouts */

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

static void * internal_add_timer(H3270 *session, unsigned long interval_ms, int (*proc)(H3270 *session, void *userdata), void *userdata) {
	timeout_t *t_new;
	timeout_t *t;
	timeout_t *prev = TN;

	trace("%s session=%p proc=%p interval=%ld",__FUNCTION__,session,proc,interval_ms);

	t_new = (timeout_t *) lib3270_malloc(sizeof(timeout_t));

	t_new->proc = proc;
	t_new->userdata = userdata;
	t_new->in_play = False;

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
	for (t = (timeout_t *) session->timeouts.first; t != TN; t = (timeout_t *) t->next) {
#if defined(_WIN32)
		if (t->ts > t_new->ts)
#else
		if (t->tv.tv_sec > t_new->tv.tv_sec || (t->tv.tv_sec == t_new->tv.tv_sec && t->tv.tv_usec > t_new->tv.tv_usec))
#endif
			break;

		prev = t;
	}

	// Insert it.
	if (prev == TN) {
		// t_new is Front.
		t_new->next = session->timeouts.first;
		session->timeouts.first = (struct lib3270_linked_list_node *) t_new;
	} else if (t == TN) {
		// t_new is Rear.
		t_new->next = NULL;
		prev->next = (struct lib3270_linked_list_node *) t_new;
		session->timeouts.last = (struct lib3270_linked_list_node *) t_new;
	} else {
		// t_new is Middle.
		t_new->next = (struct lib3270_linked_list_node *) t;
		prev->next = (struct lib3270_linked_list_node *) t_new;
	}

	trace("Timer %p added with value %ld",t_new,interval_ms);

	return t_new;
}

static void internal_remove_timer(H3270 *session, void * timer) {
	timeout_t *st = (timeout_t *)timer;

	trace("Removing timeout: %p",st);

	if(!st->in_play)
		lib3270_linked_list_delete_node(&session->timeouts,timer);

}

/* I/O events. */

static void * internal_add_poll(H3270 *session, int fd, LIB3270_IO_FLAG flag, void(*call)(H3270 *, int, LIB3270_IO_FLAG, void *), void *userdata ) {
	input_t *ip = (input_t *) lib3270_linked_list_append_node(&session->input.list,sizeof(input_t), userdata);

	ip->enabled					= 1;
	ip->fd						= fd;
	ip->flag					= flag;
	ip->call					= call;

	session->input.changed = 1;

	return ip;
}

static void internal_remove_poll(H3270 *session, void *id) {
	lib3270_linked_list_delete_node(&session->input.list,id);
	session->input.changed = 1;
}

static void internal_set_poll_state(H3270 *session, void *id, int enabled) {
	input_t *ip;

	for (ip = (input_t *) session->input.list.first; ip; ip = (input_t *) ip->next) {
		if (ip == (input_t *)id) {
			ip->enabled = enabled ? 1 : 0;
			session->input.changed = 1;
			break;
		}

	}

}

/*
LIB3270_EXPORT void	 lib3270_remove_poll(H3270 *session, void *id) {
	session->poll.remove(session, id);
}
*/

LIB3270_EXPORT void	lib3270_set_poll_state(H3270 *session, void *id, int enabled) {
	if(id) {
		debug("%s: Polling on %p is %s",__FUNCTION__,id,(enabled ? "enabled" : "disabled"))
		session->poll.set_state(session, id, enabled);
	}
}

/*
LIB3270_EXPORT void lib3270_remove_poll_fd(H3270 *session, int fd) {
	input_t *ip;

	for (ip = (input_t *) session->input.list.first; ip; ip = (input_t *) ip->next) {
		if(ip->fd == fd) {
			session->poll.remove(session, ip);
			return;
		}
	}

	lib3270_write_log(session,"iocalls","Invalid or unexpected FD on %s(%d)",__FUNCTION__,fd);

}
*/

LIB3270_EXPORT void lib3270_update_poll_fd(H3270 *session, int fd, LIB3270_IO_FLAG flag) {
	input_t *ip;

	for (ip = (input_t *) session->input.list.first; ip; ip = (input_t *) ip->next) {
		if(ip->fd == fd) {
			ip->flag = flag;
			return;
		}
	}

	lib3270_write_log(session,"iocalls","Invalid or unexpected FD on %s(%d)",__FUNCTION__,fd);

}

/*
LIB3270_EXPORT void	 * lib3270_add_poll_fd(H3270 *session, int fd, LIB3270_IO_FLAG flag, void(*call)(H3270 *, int, LIB3270_IO_FLAG, void *), void *userdata ) {
	debug("%s(%d)",__FUNCTION__,fd);
	return session->poll.add(session,fd,flag,call,userdata);
}
*/

static int internal_wait(H3270 *hSession, int seconds) {
	time_t end = time(0) + seconds;

	while(time(0) < end) {
		hSession->event_dispatcher(hSession,1);
	}

	return 0;
}

static void internal_ring_bell(H3270 GNUC_UNUSED(*session)) {
	return;
}

/* External entry points */

/*
void * lib3270_add_timer(unsigned long interval_ms, H3270 *hSession, int (*proc)(H3270 *session, void *userdata), void *userdata) {

	void *timer = hSession->timer.add(
		hSession,
		interval_ms ? interval_ms : 100,	// Prevents a zero-value timer.
		proc,
		userdata
	);

	trace("Timeout %p created with %ld ms",timer,interval_ms);

	return timer;
}
*/

/*
void lib3270_remove_timer(H3270 *hSession, void * timer) {
	if(!timer)
		return;
	trace("Removing timeout %p",timer);
	return hSession->timer.remove(hSession, timer);
}
*/

/*
void x_except_on(H3270 *h) {

	debug("%s: Need refactor!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!",__FUNCTION__);
	int reading = (h->xio.read != NULL);

	debug("%s",__FUNCTION__);
	if(h->xio.except)
		return;

	if(reading)
		h->poll.remove(h,h->xio.read);

	h->xio.except = h->poll.add(h,LIB3270_IO_FLAG_EXCEPTION,net_exception,0);

	if(reading)
		h->xio.read = h->poll.add(h,LIB3270_IO_FLAG_READ,net_input,0);
}
*/

/*
void remove_input_calls(H3270 *session) {
	if(session->xio.read) {
		session->poll.remove(session,session->xio.read);
		session->xio.read = NULL;
	}
	if(session->xio.except) {
		session->poll.remove(session,session->xio.except);
		session->xio.except = NULL;
	}
	if(session->xio.write) {
		session->poll.remove(session,session->xio.write);
		session->xio.write = NULL;
	}
}
*/

LIB3270_EXPORT void lib3270_register_timer_handlers(void * (*add)(H3270 *session, unsigned long interval_ms, int (*proc)(H3270 *session,void *userdata), void *userdata), void (*rm)(H3270 *session, void *timer)) {
	if(add)
		defs.add_timer = add;

	if(rm)
		defs.remove_timer = rm;

}

LIB3270_EXPORT void lib3270_register_fd_handlers(void * (*add)(H3270 *session, int fd, LIB3270_IO_FLAG flag, void(*proc)(H3270 *, int, LIB3270_IO_FLAG, void *), void *userdata), void (*rm)(H3270 *, void *id)) {
	if(add)
		defs.add_poll = add;

	if(rm)
		defs.remove_poll = rm;
}

LIB3270_EXPORT int lib3270_register_io_controller(const LIB3270_IO_CONTROLLER *cbk) {
	if(!cbk || cbk->sz != sizeof(LIB3270_IO_CONTROLLER))
		return errno = EINVAL;

	lib3270_register_timer_handlers(cbk->AddTimer,cbk->RemoveTimer);
	lib3270_register_fd_handlers(cbk->add_poll,cbk->remove_poll);

	if(cbk->Wait)
		defs.wait_callback = cbk->Wait;

	if(cbk->event_dispatcher)
		defs.event_dispatcher = cbk->event_dispatcher;

	if(cbk->ring_bell)
		defs.ring_bell = cbk->ring_bell;

	if(cbk->run_task)
		defs.run_task = cbk->run_task;

	if(cbk->set_poll_state)
		defs.set_poll_state = cbk->set_poll_state;

	return 0;

}

LIB3270_EXPORT int lib3270_wait(H3270 *hSession, int seconds) {
	hSession->wait(hSession,seconds);
	return 0;
}

int internal_run_task(H3270 *hSession, const char *name, int(*callback)(H3270 *, void *), void *parm) {
	return callback(hSession,parm);
}

/**
 * @brief Run background task.
 *
 * Call task in a separate thread, keep gui main loop running until
 * the function returns.
 *
 * @param hSession	TN3270 session.
 * @param name		Task name.
 * @param callback	Function to call.
 * @param parm		Parameter to callback function.
 *
 */
LIB3270_EXPORT int lib3270_run_task(H3270 *hSession, const char *name, int(*callback)(H3270 *h, void *), void *parm) {
	int rc;

	hSession->cbk.set_timer(hSession,1);
	hSession->tasks++;
	rc = hSession->run(
			hSession,
			((name && *name) ? name : PACKAGE_NAME),
			callback,
			parm
		);
	hSession->cbk.set_timer(hSession,0);
	hSession->tasks--;
	return rc;

}

/*
int non_blocking(H3270 *hSession, Boolean on) {
	if(hSession->network.module->non_blocking(hSession,on))
		return 0;

	lib3270_set_poll_state(hSession,hSession->xio.read, on);
	lib3270_set_poll_state(hSession,hSession->xio.write, on);
	lib3270_set_poll_state(hSession,hSession->xio.except, on);

	return 0;
}
*/



