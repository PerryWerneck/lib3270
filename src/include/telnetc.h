/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright 1995, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006 by Paul Mattes.
 * Copyright (C) 2008 Banco do Brasil S.A.
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
 * @brief Global declarations for telnet.c.
 */

/* Spelled-out tty control character. */
struct ctl_char {
	const char *name;
	char value[3];
};

LIB3270_INTERNAL void net_abort(H3270 *hSession);
LIB3270_INTERNAL void net_add_eor(unsigned char *buf, int len);
LIB3270_INTERNAL void net_break(H3270 *hSession);

// LIB3270_INTERNAL int net_connect(H3270 *session, const char *, char *, Boolean, Boolean *, Boolean *);
// LIB3270_INTERNAL int net_reconnect(H3270 *hSession, int seconds);
// LIB3270_INTERNAL void net_disconnect(H3270 *session);

// LIB3270_INTERNAL void net_exception(H3270 *session, int fd, LIB3270_IO_FLAG flag, void *dunno);
LIB3270_INTERNAL void net_interrupt(H3270 *hSession);
LIB3270_INTERNAL void net_output(H3270 *hSession);
LIB3270_INTERNAL void net_sendc(H3270 *hSession, char c);
LIB3270_INTERNAL void net_sends(H3270 *hSession, const char *s);
LIB3270_INTERNAL void net_send_erase(H3270 *hSession);
LIB3270_INTERNAL void net_send_kill(H3270 *hSession);
LIB3270_INTERNAL void net_send_werase(H3270 *hSession);
LIB3270_INTERNAL void space3270out(H3270 *hSession, int n);

#if defined(X3270_TRACE)
LIB3270_INTERNAL void trace_netdata(H3270 *hSession, char direction, unsigned const char *buf, int len);
#else
#define trace_netdata(direction, buf, len) /* */
#endif // X3270_TRACE

