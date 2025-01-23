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

 #include <config.h>
 #include <lib3270.h>
 #include <lib3270/log.h>
 #include <glib-object.h>
 #include <glib/gmain.h>
 #include <glib-tn3270.h>
 #include <private/intl.h>
 #include <private/gobject.h>
 #include <private/mainloop.h>
 #include <lib3270/mainloop.h>
 #include <lib3270/popup.h>
 #include <string.h>

 typedef struct _timer
 {
	unsigned char remove;
	void	* userdata;
	int		(*call)(H3270 *session, void *userdata);
	H3270 	* session;
 } TIMER;

 static void * source_add(H3270 *session, int fd, LIB3270_IO_FLAG flag, void(*proc)(H3270 *, int, LIB3270_IO_FLAG, void *), void *userdata)
 {
	return (void *) IO_source_new(session, fd, flag, proc, userdata);
 }

 static void source_remove(H3270 *session, void *id)
 {
	if(id)
		g_source_destroy((GSource *) id);
 }

 static void source_set_state(H3270 *session, void *id, int enabled)
 {
	if(id)
		IO_source_set_state( (GSource *) id, (gboolean) (enabled != 0));
 }

 static gboolean do_timer(TIMER *t)
 {
	if(!t->remove)
		return t->call(t->session,t->userdata);
	return FALSE;
 }

 static void * timer_add(H3270 *session, unsigned long interval_ms, int (*proc)(H3270 *session, void *userdata), void *userdata)
 {
	TIMER *t = g_malloc0(sizeof(TIMER));

	t->call		= proc;
	t->session	= session;
	t->userdata	= userdata;

	guint id = g_timeout_add_full(G_PRIORITY_DEFAULT, (guint) interval_ms, (GSourceFunc) do_timer, t, g_free);
	debug("timer-id=%u",id);

	return t;
 }

 static void timer_remove(H3270 *session, void * timer)
 {
	((TIMER *) timer)->remove++;
 }

 static int sleep(H3270 *hSession, int seconds)
 {
	time_t end = time(0) + seconds;
	GMainContext *context = g_main_context_default();

	while(time(0) < end)
		g_main_context_iteration(context,TRUE);

	return 0;

 }

 static int run_events(H3270 *hSession, int wait)
 {
	int rc = 0;
	GMainContext *context = g_main_context_default();

	while(g_main_context_pending(context))
	{
		rc = 1;
		g_main_context_iteration(context,TRUE);
	}

	if(wait)
		g_main_context_iteration(context,TRUE);

	return rc;
 }


 static gboolean tn3270_session_ring_bell(TN3270Session *session)
 {	
	TN3270_SESSION_GET_CLASS(session)->ring_bell(session);
	return FALSE;
 }

 static void ring_bell(H3270 *session)
 {
	g_idle_add((GSourceFunc) tn3270_session_ring_bell,lib3270_get_user_data(session));
 }

 struct bgParameter
 {
	gboolean	  running;
	H3270 		* hSession;
	int			  rc;
	int			  (*callback)(H3270 *session, void *parm);
	void		* parm;
 };

 gpointer BgCall(struct bgParameter *p)
 {
	p->rc = p->callback(p->hSession,p->parm);
	p->running = FALSE;
	return 0;
 }

 static	int run_task(H3270 *hSession, const char *name, int(*callback)(H3270 *, void *), void *parm)
 {
	struct bgParameter p = { TRUE, hSession, -1, callback, parm };

	p.running = TRUE;

	GThread	*thread = g_thread_new( 
		((name && *name) ? name : PACKAGE_NAME), 
		(GThreadFunc) BgCall, 
		&p
	);

	if(!thread)
	{
		g_error("Can't start background thread");
		return -1;
	}

	GMainContext *context = g_main_context_default();
	while(p.running)
	{
		g_main_context_iteration(context,TRUE);
	}

	g_thread_join(thread);

	return p.rc;

 }

 int tn3270_session_setup_mainloop(TN3270SessionPrivate *self)
 {
	static const LIB3270_IO_CONTROLLER hdl =
	{
		.sz = sizeof(LIB3270_IO_CONTROLLER),

		.AddTimer = timer_add,
		.RemoveTimer = timer_remove,

		.add_poll = source_add,
		.remove_poll = source_remove,
		.set_poll_state = source_set_state,

		.Wait = sleep,
		.event_dispatcher = run_events,
		.ring_bell = ring_bell,
		.run_task = run_task
	};

	int rc = lib3270_session_set_handlers(self->handler,&hdl);

	if(rc)
	{
		lib3270_popup_dialog(
			self->handler, 
			LIB3270_NOTIFY_CRITICAL, 
			_("Initialization error"), 
			_("I/O setup failed"), 
			_("Unable to set I/O controller, the release %s of %s can't be used"),
				lib3270_get_revision(),
				PACKAGE_NAME
		);

		g_error(
			_("Unable to set I/O controller, the release %s of %s can't be used (the system error code was '%s')"),
			lib3270_get_revision(),
			PACKAGE_NAME,
			strerror(rc)
		);
	}

	return rc;
 }
