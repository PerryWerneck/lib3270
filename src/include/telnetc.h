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
 * Este programa está nomeado como - e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
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

LIB3270_INTERNAL int net_connect(H3270 *session, const char *, char *, Boolean, Boolean *, Boolean *);
LIB3270_INTERNAL int net_reconnect(H3270 *hSession, int seconds);

LIB3270_INTERNAL void net_disconnect(H3270 *session);
LIB3270_INTERNAL void net_exception(H3270 *session, int fd, LIB3270_IO_FLAG flag, void *dunno);
LIB3270_INTERNAL void net_input(H3270 *session, int fd, LIB3270_IO_FLAG flag, void *dunno);
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

LIB3270_INTERNAL int net_getsockname(const H3270 *h3270, void *buf, int *len);
