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
 * Este programa está nomeado como telnet.c e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 * licinio@bb.com.br		(Licínio Luis Branco)
 * kraucer@bb.com.br		(Kraucer Fernandes Mazuco)
 *
 */

#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"


/**
 * @brief Initializes and manages a telnet socket to the given IBM host.
 */

#if defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#endif

#ifdef __MINGW32__
#include <sys/time.h>
#endif

#ifndef ANDROID
#include <stdlib.h>
#endif // !ANDROID

#include <config.h>

#include <internals.h>
#include <errno.h>

#if defined(_WIN32)
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#endif

#define TELCMDS 1
#define TELOPTS 1
#include "arpa_telnet.h"

#if !defined(_WIN32)
#include <arpa/inet.h>
#endif

#include <errno.h>
#include <fcntl.h>

#if defined(_WIN32)
#include <lib3270/os.h>
#else
#include <netdb.h>
#endif

// #include <stdarg.h>

#include "tn3270e.h"
#include "3270ds.h"

// #include "appres.h"

#include "ansic.h"
#include "ctlrc.h"
#include "hostc.h"
#include "kybdc.h"
// #include "macrosc.h"
#include "popupsc.h"
// #include "proxyc.h"
//#include "resolverc.h"
#include "statusc.h"
// #include "tablesc.h"
#include "telnetc.h"
#include "trace_dsc.h"
#include "utilc.h"
#include "w3miscc.h"
#include "xioc.h"
#include "screen.h"

#include <lib3270/internals.h>
#include <lib3270/trace.h>
#include <lib3270/log.h>
#include <lib3270/toggle.h>

#if !defined(TELOPT_NAWS) /*[*/
#define TELOPT_NAWS	31
#endif /*]*/

#if !defined(TELOPT_STARTTLS) /*[*/
#define TELOPT_STARTTLS	46
#endif /*]*/
#define TLS_FOLLOWS	1

#define BUFSZ		16384
#define TRACELINE	72

#if defined(X3270_TN3270E)
#define E_OPT(n)	(1 << (n))
#endif // X3270_TN3270E

struct _ansictl ansictl = { 0 };

static int telnet_fsm(H3270 *session, unsigned char c);
static void net_rawout(H3270 *session, unsigned const char *buf, size_t len);
static void check_in3270(H3270 *session);
static void store3270in(H3270 *hSession, unsigned char c);
static void check_linemode(H3270 *hSession, Boolean init);
static int net_connected(H3270 *session);

static void continue_tls(H3270 *hSession, unsigned char *sbbuf, int len);

#if defined(X3270_TN3270E) /*[*/
static int tn3270e_negotiate(H3270 *hSession);
#endif /*]*/
static int process_eor(H3270 *hSession);
#if defined(X3270_TN3270E) /*[*/
#if defined(X3270_TRACE) /*[*/
static const char *tn3270e_function_names(const unsigned char *, int);
#endif /*]*/
static void tn3270e_subneg_send(H3270 *hSession, unsigned char, unsigned long);
static unsigned long tn3270e_fdecode(const unsigned char *, int);
static void tn3270e_ack(H3270 *hSession);
static void tn3270e_nak(H3270 *hSession, enum pds);
#endif /*]*/

#if defined(X3270_ANSI) /*[*/
static void do_data(H3270 *hSession, char c);
static void do_intr(H3270 *hSession, char c);
static void do_quit(H3270 *hSession, char c);
static void do_cerase(H3270 *hSession, char c);
static void do_werase(H3270 *hSession, char c);
static void do_kill(H3270 *hSession, char c);
static void do_rprnt(H3270 *hSession, char c);
static void do_eof(H3270 *hSession, char c);
static void do_eol(H3270 *hSession, char c);
static void do_lnext(H3270 *hSession, char c);
// static char parse_ctlchar(char *s);
static void cooked_init(H3270 *hSession);
#endif /*]*/

#if defined(X3270_TRACE) /*[*/
static const char *cmd(int c);
static const char *opt(unsigned char c);
static const char *nnn(int c);
#else /*][*/
#if defined(__GNUC__) /*[*/
#else /*][*/
#endif /*]*/
#define cmd(x) 0
#define opt(x) 0
#define nnn(x) 0
#endif /*]*/

/* telnet states */
#define TNS_DATA	0	/* receiving data */
#define TNS_IAC		1	/* got an IAC */
#define TNS_WILL	2	/* got an IAC WILL */
#define TNS_WONT	3	/* got an IAC WONT */
#define TNS_DO		4	/* got an IAC DO */
#define TNS_DONT	5	/* got an IAC DONT */
#define TNS_SB		6	/* got an IAC SB */
#define TNS_SB_IAC	7	/* got an IAC after an IAC SB */

/* telnet predefined messages */
static unsigned char	do_opt[]	= 	{ 	IAC, DO, '_' };
static unsigned char	dont_opt[]	= 	{	IAC, DONT, '_' };
static unsigned char	will_opt[]	= 	{	IAC, WILL, '_' };
static unsigned char	wont_opt[]	= 	{	IAC, WONT, '_' };

#if defined(X3270_TN3270E) /*[*/
static const unsigned char	functions_req[] = {	IAC, SB, TELOPT_TN3270E, TN3270E_OP_FUNCTIONS };
#endif /*]*/

#if defined(X3270_TRACE) /*[*/
static const char *telquals[2] = { "IS", "SEND" };
#endif /*]*/
#if defined(X3270_TN3270E) /*[*/
#if defined(X3270_TRACE) /*[*/
static const char *reason_code[8] = { "CONN-PARTNER", "DEVICE-IN-USE",
                                      "INV-ASSOCIATE", "INV-NAME", "INV-DEVICE-TYPE", "TYPE-NAME-ERROR",
                                      "UNKNOWN-ERROR", "UNSUPPORTED-REQ"
                                    };
#define rsn(n)	(((n) <= TN3270E_REASON_UNSUPPORTED_REQ) ? \
			reason_code[(n)] : "??")
#endif /*]*/
static const char *function_name[5] = { "BIND-IMAGE", "DATA-STREAM-CTL",
                                        "RESPONSES", "SCS-CTL-CODES", "SYSREQ"
                                      };
#define fnn(n)	(((n) <= TN3270E_FUNC_SYSREQ) ? \
			function_name[(n)] : "??")
#if defined(X3270_TRACE) /*[*/
static const char *data_type[9] = { "3270-DATA", "SCS-DATA", "RESPONSE",
                                    "BIND-IMAGE", "UNBIND", "NVT-DATA", "REQUEST", "SSCP-LU-DATA",
                                    "PRINT-EOJ"
                                  };
#define e_dt(n)	(((n) <= TN3270E_DT_PRINT_EOJ) ? \
			data_type[(n)] : "??")
static const char *req_flag[1] = { " ERR-COND-CLEARED" };
#define e_rq(fn, n) (((fn) == TN3270E_DT_REQUEST) ? \
			(((n) <= TN3270E_RQF_ERR_COND_CLEARED) ? \
			req_flag[(n)] : " ??") : "")
static const char *hrsp_flag[3] = { "NO-RESPONSE", "ERROR-RESPONSE",
                                    "ALWAYS-RESPONSE"
                                  };
#define e_hrsp(n) (((n) <= TN3270E_RSF_ALWAYS_RESPONSE) ? \
			hrsp_flag[(n)] : "??")
static const char *trsp_flag[2] = { "POSITIVE-RESPONSE", "NEGATIVE-RESPONSE" };
#define e_trsp(n) (((n) <= TN3270E_RSF_NEGATIVE_RESPONSE) ? \
			trsp_flag[(n)] : "??")
#define e_rsp(fn, n) (((fn) == TN3270E_DT_RESPONSE) ? e_trsp(n) : e_hrsp(n))
#endif /*]*/
#endif /*]*/

#define XMIT_ROWS	hSession->max.rows
#define XMIT_COLS	hSession->max.cols

#if defined(_WIN32) /*[*/
#define socket_errno()	WSAGetLastError()
#define SE_EWOULDBLOCK	WSAEWOULDBLOCK
#define SE_ECONNRESET	WSAECONNRESET
#define SE_EINTR	WSAEINTR
#define SE_EAGAIN	WSAEINPROGRESS
#define SE_EPIPE	WSAECONNABORTED
#define SE_EINPROGRESS	WSAEINPROGRESS
#define SOCK_IOCTL(s, f, v)	ioctlsocket(s, f, (void *)v)
#else /*][*/
#define socket_errno()	errno
#define SE_EWOULDBLOCK	EWOULDBLOCK
#define SE_ECONNRESET	ECONNRESET
#define SE_EINTR	EINTR
#define SE_EAGAIN	EAGAIN
#define SE_EPIPE	EPIPE

#if defined(EINPROGRESS) /*[*/
#define SE_EINPROGRESS	EINPROGRESS
#endif /*]*/

#define SOCK_IOCTL	ioctl
#endif /*]*/


/*--[ Implement ]------------------------------------------------------------------------------------*/

static union {
	struct sockaddr sa;
	struct sockaddr_in sin;
#if defined(AF_INET6) /*[*/
	struct sockaddr_in6 sin6;
#endif /*]*/
} haddr;
socklen_t ha_len = sizeof(haddr);

void popup_a_sockerr(H3270 *hSession, char *fmt, ...) {
#if defined(_WIN32)
	const char *msg = lib3270_win32_strerror(socket_errno());
#else
	const char *msg = strerror(errno);
#endif // WIN32

	va_list args;
	char *text;

	va_start(args, fmt);
	text = lib3270_vsprintf(fmt, args);
	va_end(args);

	lib3270_write_log(
	    hSession,
	    "3270",
	    "Network error:\n%s\n%s",
	    text,
	    msg
	);

	lib3270_popup_dialog(
	    hSession,
	    LIB3270_NOTIFY_ERROR,
	    _( "Network error" ),
	    text,
	    "%s",
	    msg
	);

	lib3270_free(text);

}

// Set up the LU list.
static void setup_lus(H3270 *hSession) {
	hSession->lu.associated = CN;
	hSession->connected_type = CN;

	if(hSession->lu.names) {
		hSession->lu.curr	= hSession->lu.names;
		hSession->lu.try	= * hSession->lu.curr;
	} else {
		hSession->lu.curr	= (char **)NULL;
		hSession->lu.try	= CN;
	}

	/*
	char *lu;
	char *comma;
	int n_lus = 1;
	int i;


	if (!hSession->lu.names[0])
	{
		Replace(hSession->lus, NULL);
		hSession->curr_lu = (char **)NULL;
		hSession->try_lu = CN;
		return;
	}

	//
	// Count the commas in the LU names.  That plus one is the
	// number of LUs to try.
	//
	lu = hSession->lu.names;
	while ((comma = strchr(lu, ',')) != CN)
	{
		n_lus++;
		lu++;
	}

	//
	// Allocate enough memory to construct an argv[] array for
	// the LUs.
	//
	Replace(hSession->lus,(char **)lib3270_malloc((n_lus+1) * sizeof(char *) + strlen(hSession->lu.names) + 1));

	// Copy each LU into the array.
	lu = (char *)(hSession->lus + n_lus + 1);
	(void) strcpy(lu, hSession->lu.names);

	i = 0;
	do
	{
		hSession->lus[i++] = lu;
		comma = strchr(lu, ',');
		if (comma != CN)
		{
			*comma = '\0';
			lu = comma + 1;
		}
	} while (comma != CN);

	hSession->lus[i]	= CN;
	*/

}

static int net_connected(H3270 *hSession) {

	// Set up SSL.
	trace_dsn(hSession,"Connected to %s%s.\n", hSession->host.current,hSession->ssl.host ? " using SSL": "");

	if(hSession->ssl.host && hSession->ssl.state == LIB3270_SSL_UNDEFINED) {
		if(lib3270_start_tls(hSession))
			return -1;
	}

	lib3270_setup_session(hSession);
	lib3270_notify_tls(hSession);

	return 0;
}

/**
 * Set up telnet options.
 *
 * Called just after a sucessfull connect to setup tn3270 state.
 *
 * @param hSession	3270 session to setup.
 *
 */
LIB3270_EXPORT void lib3270_setup_session(H3270 *hSession) {
	(void) memset((char *) hSession->myopts, 0, sizeof(hSession->myopts));
	(void) memset((char *) hSession->hisopts, 0, sizeof(hSession->hisopts));

#if defined(X3270_TN3270E)
	hSession->e_funcs = E_OPT(TN3270E_FUNC_BIND_IMAGE) | E_OPT(TN3270E_FUNC_RESPONSES) | E_OPT(TN3270E_FUNC_SYSREQ);
	hSession->e_xmit_seq = 0;
	hSession->response_required = TN3270E_RSF_NO_RESPONSE;
#endif

	hSession->need_tls_follows = 0;
	hSession->telnet_state = TNS_DATA;
	hSession->ibptr = hSession->ibuf;

	// clear statistics and flags
	time(&hSession->ns_time);
	hSession->ns_brcvd  = 0;
	hSession->ns_rrcvd  = 0;
	hSession->ns_bsent  = 0;
	hSession->ns_rsent  = 0;
	hSession->syncing   = 0;
	hSession->tn3270e_negotiated = 0;
	hSession->tn3270e_submode = E_NONE;
	hSession->tn3270e_bound = 0;

	setup_lus(hSession);

	check_linemode(hSession,True);

	// write out the passthru hostname and port nubmer
	/*
	if (hSession->passthru_host)
	{
		unsigned char *buffer = (unsigned char *) xs_buffer("%s %d\r\n", hSession->hostname, hSession->current_port);
		hSession->write(hSession, buffer, strlen((char *) buffer));
		lib3270_free(buffer);
		trace_ds(hSession,"SENT HOSTNAME %s:%d\n", hSession->hostname, hSession->current_port);
	}
	*/

	non_blocking(hSession,True);

}

///
/// @brief Connection_complete.
///
/// The connection appears to be complete (output is possible or input
/// appeared ready but recv() returned EWOULDBLOCK).  Complete the
/// connection-completion processing.
///
static void connection_complete(H3270 *session) {
	if (non_blocking(session,False) < 0) {
		host_disconnect(session,True);
		return;
	}
	lib3270_set_connected_initial(session);
	net_connected(session);
}

///	@brief Disconnect from host.
void net_disconnect(H3270 *hSession) {
	if(hSession->xio.write) {
		lib3270_remove_poll(hSession, hSession->xio.write);
		hSession->xio.write = 0;
	}

	hSession->network.module->disconnect(hSession);

	trace_dsn(hSession,"SENT disconnect\n");

	// We're not connected to an LU any more.
	hSession->lu.associated = CN;
	status_lu(hSession,CN);

}

LIB3270_EXPORT void lib3270_data_recv(H3270 *hSession, size_t nr, const unsigned char *netrbuf) {
	register const unsigned char * cp;

//	trace("%s: nr=%d",__FUNCTION__,(int) nr);

	trace_netdata(hSession, '<', netrbuf, nr);

	hSession->ns_brcvd += nr;
	for (cp = netrbuf; cp < (netrbuf + nr); cp++) {
		if(telnet_fsm(hSession,*cp)) {
			(void) ctlr_dbcs_postprocess(hSession);
			host_disconnect(hSession,True);
			return;
		}
	}

#if defined(X3270_ANSI)
	if (IN_ANSI) {
		(void) ctlr_dbcs_postprocess(hSession);
	}

	if (hSession->ansi_data) {
		trace_dsn(hSession,"\n");
		hSession->ansi_data = 0;
	}
#endif // X3270_ANSI
}

/**
 * @brief Called by the toolkit whenever there is input available on the socket.
 *
 * Called by the toolkit whenever there is input available on the
 * socket.  Reads the data, processes the special telnet commands
 * and calls process_ds to process the 3270 data stream.
 *
 * @param hSession	Session handle
 *
 */
void net_input(H3270 *hSession, int GNUC_UNUSED(fd), LIB3270_IO_FLAG GNUC_UNUSED(flag), void GNUC_UNUSED(*dunno)) {
//	register unsigned char	* cp;
	int						  nr;
	unsigned char			  buffer[BUFSZ];

	CHECK_SESSION_HANDLE(hSession);

	memset(buffer,0,BUFSZ);

#if defined(_WIN32)
	for (;;)
#endif
	{
//		if (hSession->connection.sock < 0)
//			return;

#if defined(X3270_ANSI)
		hSession->ansi_data = 0;
#endif

		/*
				if (hSession->ssl.con != NULL)
					nr = SSL_read(hSession->ssl.con, (char *) buffer, BUFSZ);
				else
					nr = hSession->network.module->recv(hSession, buffer, BUFSZ);
		*/

		nr = hSession->network.module->recv(hSession, buffer, BUFSZ);

		debug("%s: recv=%d",__FUNCTION__,nr);

		if (nr < 0) {
			if (nr == -EWOULDBLOCK) {
				return;
			}

			if(HALF_CONNECTED && nr == -EAGAIN) {
				debug("%s: Received a -EAGAIN with half-connect",__FUNCTION__);
				connection_complete(hSession);
				return;
			}

			trace_dsn(hSession,"RCVD socket error %d\n", -nr);

			host_disconnect(hSession,True);
			return;
		} else if (nr == 0) {
			// Host disconnected.
			trace_dsn(hSession,"RCVD disconnect\n");
			host_disconnect(hSession,False);
			return;
		}

		// Process the data.
		if (HALF_CONNECTED) {
			debug("%s: Received a %d with half-connect",__FUNCTION__,nr);
			if (non_blocking(hSession,False) < 0) {
				host_disconnect(hSession,True);
				return;
			}
			lib3270_set_connected_initial(hSession);
			if(net_connected(hSession))
				return;
		}

		lib3270_data_recv(hSession, nr, buffer);

	}

}

/**
 *	@brief Put a 16-bit value in a buffer.
 *	@return The number of bytes required.
 */
static int set16(char *buf, int n) {
	char *b0 = buf;

	n %= 256 * 256;
	if ((n / 256) == IAC)
		*(unsigned char *)buf++ = IAC;
	*buf++ = (n / 256);
	n %= 256;
	if (n == IAC)
		*(unsigned char *)buf++ = IAC;
	*buf++ = n;
	return buf - b0;
}

/*
 * send_naws
 *	Send a Telnet window size sub-option negotation.
 */
static void send_naws(H3270 *hSession) {
	char naws_msg[14];
	int naws_len = 0;

	(void) sprintf(naws_msg, "%c%c%c", IAC, SB, TELOPT_NAWS);
	naws_len += 3;
	naws_len += set16(naws_msg + naws_len, XMIT_COLS);
	naws_len += set16(naws_msg + naws_len, XMIT_ROWS);
	(void) sprintf(naws_msg + naws_len, "%c%c", IAC, SE);
	naws_len += 2;
	net_rawout(hSession,(unsigned char *)naws_msg, naws_len);
	trace_dsn(hSession,"SENT %s NAWS %d %d %s\n", cmd(SB), XMIT_COLS, XMIT_ROWS, cmd(SE));
}

///
/// @brief Advance 'try_lu' to the next desired LU name.
///
static void next_lu(H3270 *hSession) {
	if (hSession->lu.curr != (char **)NULL && (hSession->lu.try = *++hSession->lu.curr) == CN)
		hSession->lu.curr = (char **)NULL;
}

///
///	@brief Telnet finite-state machine.
///
///	@return 0 for okay, -1 for errors.
///
static int telnet_fsm(H3270 *hSession, unsigned char c) {
#if defined(X3270_ANSI) /*[*/
	char	*see_chr;
	int	sl;
#endif /*]*/

	switch (hSession->telnet_state) {
	case TNS_DATA:	/* normal data processing */
		if (c == IAC) {	/* got a telnet command */
			hSession->telnet_state = TNS_IAC;
#if defined(X3270_ANSI) /*[*/
			if (hSession->ansi_data) {
				trace_dsn(hSession,"\n");
				hSession->ansi_data = 0;
			}
#endif /*]*/
			break;
		}
		if (IN_NEITHER) {	/* now can assume ANSI mode */
#if defined(X3270_ANSI)/*[*/
			if (hSession->linemode)
				cooked_init(hSession);
#endif /*]*/
			host_in3270(hSession,LIB3270_CONNECTED_ANSI);
			lib3270_kybdlock_clear(hSession,KL_AWAITING_FIRST);
			status_reset(hSession);
			ps_process(hSession);
		}
		if (IN_ANSI && !IN_E) {
#if defined(X3270_ANSI) /*[*/
			if (!hSession->ansi_data) {
				trace_dsn(hSession,"<.. ");
				hSession->ansi_data = 4;
			}
			see_chr = ctl_see((int) c);
			hSession->ansi_data += (sl = strlen(see_chr));
			if (hSession->ansi_data >= TRACELINE) {
				trace_dsn(hSession," ...\n... ");
				hSession->ansi_data = 4 + sl;
			}
			trace_dsn(hSession,"%s",see_chr);
			if (!hSession->syncing) {
				if (hSession->linemode && hSession->onlcr && c == '\n')
					ansi_process(hSession,(unsigned int) '\r');
				ansi_process(hSession,(unsigned int) c);
			}
#endif /*]*/
		} else {
			store3270in(hSession,c);
		}
		break;
	case TNS_IAC:	/* process a telnet command */
		if (c != EOR && c != IAC) {
			trace_dsn(hSession,"RCVD %s ", cmd(c));
		}

		switch (c) {
		case IAC:	/* escaped IAC, insert it */
			if (IN_ANSI && !IN_E) {
#if defined(X3270_ANSI) /*[*/
				if (!hSession->ansi_data) {
					trace_dsn(hSession,"<.. ");
					hSession->ansi_data = 4;
				}
				see_chr = ctl_see((int) c);
				hSession->ansi_data += (sl = strlen(see_chr));
				if (hSession->ansi_data >= TRACELINE) {
					trace_dsn(hSession," ...\n ...");
					hSession->ansi_data = 4 + sl;
				}
				trace_dsn(hSession,"%s",see_chr);
				ansi_process(hSession,(unsigned int) c);
#endif /*]*/
			} else {
				store3270in(hSession,c);
			}
			hSession->telnet_state = TNS_DATA;
			break;

		case EOR:	/* eor, process accumulated input */

			if (IN_3270 || (IN_E && hSession->tn3270e_negotiated)) {
				hSession->ns_rrcvd++;
				if (process_eor(hSession))
					return -1;
			} else {
				lib3270_popup_dialog(
				    hSession,
				    LIB3270_NOTIFY_WARNING,_("Warning"),
				    _( "EOR received when not in 3270 mode, ignored." ),
				    NULL
				);
			}
			trace_dsn(hSession,"RCVD EOR\n");
			hSession->ibptr = hSession->ibuf;
			hSession->telnet_state = TNS_DATA;
			break;

		case WILL:
			hSession->telnet_state = TNS_WILL;
			break;

		case WONT:
			hSession->telnet_state = TNS_WONT;
			break;

		case DO:
			hSession->telnet_state = TNS_DO;
			break;

		case DONT:
			hSession->telnet_state = TNS_DONT;
			break;

		case SB:
			hSession->telnet_state = TNS_SB;
			if (hSession->sbbuf == (unsigned char *)NULL)
				hSession->sbbuf = (unsigned char *)lib3270_malloc(1024);
			hSession->sbptr = hSession->sbbuf;
			break;

		case DM:
			trace_dsn(hSession,"\n");
			if (hSession->syncing) {
				hSession->syncing = 0;
				x_except_on(hSession);
			}
			hSession->telnet_state = TNS_DATA;
			break;

		case GA:
		case NOP:
			trace_dsn(hSession,"\n");
			hSession->telnet_state = TNS_DATA;
			break;

		default:
			trace_dsn(hSession,"???\n");
			hSession->telnet_state = TNS_DATA;
			break;
		}
		break;
	case TNS_WILL:	/* telnet WILL DO OPTION command */
		trace_dsn(hSession,"%s\n", opt(c));
		switch (c) {
		case TELOPT_SGA:
		case TELOPT_BINARY:
		case TELOPT_EOR:
		case TELOPT_TTYPE:
		case TELOPT_ECHO:
#if defined(X3270_TN3270E)
		case TELOPT_TN3270E:
#endif
			if (c != TELOPT_TN3270E || !hSession->non_tn3270e_host) {
				if (!hSession->hisopts[c]) {
					hSession->hisopts[c] = 1;
					do_opt[2] = c;
					net_rawout(hSession,do_opt, sizeof(do_opt));
					trace_dsn(hSession,"SENT %s %s\n",
					          cmd(DO), opt(c));

					/*
					 * For UTS, volunteer to do EOR when
					 * they do.
					 */
					if (c == TELOPT_EOR && !hSession->myopts[c]) {
						hSession->myopts[c] = 1;
						will_opt[2] = c;
						net_rawout(hSession,will_opt,sizeof(will_opt));
						trace_dsn(hSession,"SENT %s %s\n",cmd(WILL), opt(c));
					}

					check_in3270(hSession);
					check_linemode(hSession,False);
				}
				break;
			}

		default:
			dont_opt[2] = c;
			net_rawout(hSession,dont_opt, sizeof(dont_opt));
			trace_dsn(hSession,"SENT %s %s\n", cmd(DONT), opt(c));
			break;
		}
		hSession->telnet_state = TNS_DATA;
		break;
	case TNS_WONT:	/* telnet WONT DO OPTION command */
		trace_dsn(hSession,"%s\n", opt(c));
		if (hSession->hisopts[c]) {
			hSession->hisopts[c] = 0;
			dont_opt[2] = c;
			net_rawout(hSession, dont_opt, sizeof(dont_opt));
			trace_dsn(hSession,"SENT %s %s\n", cmd(DONT), opt(c));
			check_in3270(hSession);
			check_linemode(hSession,False);
		}

		hSession->telnet_state = TNS_DATA;
		break;
	case TNS_DO:	/* telnet PLEASE DO OPTION command */
		trace_dsn(hSession,"%s\n", opt(c));
		switch (c) {
		case TELOPT_BINARY:
		case TELOPT_EOR:
		case TELOPT_TTYPE:
		case TELOPT_SGA:
		case TELOPT_NAWS:
		case TELOPT_TM:
#if defined(X3270_TN3270E) /*[*/
		case TELOPT_TN3270E:
#endif /*]*/
		case TELOPT_STARTTLS:
			if (c == TELOPT_TN3270E && hSession->non_tn3270e_host)
				goto wont;
			if (c == TELOPT_TM && !hSession->bsd_tm)
				goto wont;

			trace("hSession->myopts[c]=%d",hSession->myopts[c]);
			if (!hSession->myopts[c]) {
				if (c != TELOPT_TM)
					hSession->myopts[c] = 1;
				will_opt[2] = c;
				net_rawout(hSession, will_opt, sizeof(will_opt));
				trace_dsn(hSession,"SENT %s %s\n", cmd(WILL), opt(c));
				check_in3270(hSession);
				check_linemode(hSession,False);
			}
			if (c == TELOPT_NAWS)
				send_naws(hSession);
			if (c == TELOPT_STARTTLS) {
				static unsigned char follows_msg[] = {
					IAC, SB, TELOPT_STARTTLS,
					TLS_FOLLOWS, IAC, SE
				};

				//
				// Send IAC SB STARTTLS FOLLOWS IAC SE
				// to announce that what follows is TLS.
				//
				net_rawout(hSession, follows_msg, sizeof(follows_msg));
				trace_dsn(hSession,"SENT %s %s FOLLOWS %s\n",
				          cmd(SB),
				          opt(TELOPT_STARTTLS),
				          cmd(SE));

				debug("%s: %s requires TLS/SSL",__FUNCTION__,opt(TELOPT_STARTTLS));
				hSession->need_tls_follows = 1;
			}
			break;

		default:
wont:
			wont_opt[2] = c;
			net_rawout(hSession, wont_opt, sizeof(wont_opt));
			trace_dsn(hSession,"SENT %s %s\n", cmd(WONT), opt(c));
			break;
		}
		hSession->telnet_state = TNS_DATA;
		break;
	case TNS_DONT:	/* telnet PLEASE DON'T DO OPTION command */
		trace_dsn(hSession,"%s\n", opt(c));
		if (hSession->myopts[c]) {
			hSession->myopts[c] = 0;
			wont_opt[2] = c;
			net_rawout(hSession, wont_opt, sizeof(wont_opt));
			trace_dsn(hSession,"SENT %s %s\n", cmd(WONT), opt(c));
			check_in3270(hSession);
			check_linemode(hSession,False);
		}
		hSession->telnet_state = TNS_DATA;
		break;
	case TNS_SB:	// telnet sub-option string command
		if (c == IAC)
			hSession->telnet_state = TNS_SB_IAC;
		else
			*hSession->sbptr++ = c;
		break;
	case TNS_SB_IAC:	// telnet sub-option string command
		*hSession->sbptr++ = c;
		if (c == SE) {
			hSession->telnet_state = TNS_DATA;
			if (hSession->sbbuf[0] == TELOPT_TTYPE && hSession->sbbuf[1] == TELQUAL_SEND) {
				int tt_len, tb_len;
				char *tt_out;

				trace_dsn(hSession,"%s %s\n", opt(hSession->sbbuf[0]),telquals[hSession->sbbuf[1]]);

				if (hSession->lu.names != (char **)NULL && hSession->lu.try == CN) {
					// None of the LUs worked.
					popup_an_error(hSession, _( "Cannot connect to specified LU" ) );
					return -1;
				}

				tt_len = strlen(hSession->termtype);
				if (hSession->lu.try != CN && *hSession->lu.try) {
					tt_len += strlen(hSession->lu.try) + 1;
					hSession->lu.associated = hSession->lu.try;
				} else {
					hSession->lu.associated = CN;
				}

				status_lu(hSession,hSession->lu.associated);

				tb_len = 4 + tt_len + 2;
				tt_out = lib3270_malloc(tb_len + 1);
				(void) sprintf(tt_out, "%c%c%c%c%s%s%s%c%c",
				               IAC, SB, TELOPT_TTYPE, TELQUAL_IS,
				               hSession->termtype,
				               (hSession->lu.try != CN && *hSession->lu.try) ? "@" : "",
				               (hSession->lu.try != CN && *hSession->lu.try) ? hSession->lu.try : "",
				               IAC, SE);
				net_rawout(hSession, (unsigned char *)tt_out, tb_len);

				debug("\n\n\ntermtype=%s lu.try=[%s] - [%s]\n\n\n",
				      hSession->termtype,
				      (hSession->lu.try != CN && *hSession->lu.try) ? "@" : "",
				      (hSession->lu.try != CN && *hSession->lu.try) ? hSession->lu.try : ""
				     );

				trace_dsn(hSession,"SENT %s %s %s %.*s %s\n",
				          cmd(SB), opt(TELOPT_TTYPE),
				          telquals[TELQUAL_IS],
				          tt_len, tt_out + 4,
				          cmd(SE));
				lib3270_free(tt_out);

				/* Advance to the next LU name. */
				next_lu(hSession);
			}
#if defined(X3270_TN3270E) /*[*/
			else if (hSession->myopts[TELOPT_TN3270E] && hSession->sbbuf[0] == TELOPT_TN3270E) {
				if (tn3270e_negotiate(hSession))
					return -1;
			}
#endif /*]*/
			else if (hSession->need_tls_follows && hSession->myopts[TELOPT_STARTTLS] && hSession->sbbuf[0] == TELOPT_STARTTLS) {
				continue_tls(hSession,hSession->sbbuf, hSession->sbptr - hSession->sbbuf);
			}

		} else {
			hSession->telnet_state = TNS_SB;
		}
		break;
	}
	return 0;
}

/// @brief Process a STARTTLS subnegotiation.
static void continue_tls(H3270 *hSession, unsigned char *sbbuf, int len) {
	// Whatever happens, we're not expecting another SB STARTTLS.
	hSession->need_tls_follows = 0;

	// Make sure the option is FOLLOWS.
	if (len < 2 || sbbuf[1] != TLS_FOLLOWS) {
		/* Trace the junk. */
		trace_dsn(hSession,"%s ? %s\n", opt(TELOPT_STARTTLS), cmd(SE));
		popup_an_error(hSession,_( "TLS negotiation failure"));
		net_disconnect(hSession);
		return;
	}

	// Trace what we got.
	trace_dsn(hSession,"%s FOLLOWS %s\n", opt(TELOPT_STARTTLS), cmd(SE));

	hSession->ssl.host = 1;	// Set host type as SSL.
	if(lib3270_start_tls(hSession)) {
		lib3270_disconnect(hSession);
		return;
	}

	lib3270_notify_tls(hSession);

}

#if defined(X3270_TN3270E) /*[*/
/// @brief Send a TN3270E terminal type request.
static void tn3270e_request(H3270 *hSession) {
	int tt_len, tb_len;
	char *tt_out;
	char *t;

	tt_len = strlen(hSession->termtype);
	if (hSession->lu.try != CN && *hSession->lu.try)
		tt_len += strlen(hSession->lu.try) + 1;

	tb_len = 5 + tt_len + 2;
	tt_out = lib3270_malloc(tb_len + 1);
	t = tt_out;
	t += sprintf(tt_out, "%c%c%c%c%c%s",
	             IAC, SB, TELOPT_TN3270E, TN3270E_OP_DEVICE_TYPE,
	             TN3270E_OP_REQUEST, hSession->termtype);

	/* Convert 3279 to 3278, per the RFC. */
	if (tt_out[12] == '9')
		tt_out[12] = '8';

	if (hSession->lu.try != CN && *hSession->lu.try)
		t += sprintf(t, "%c%s", TN3270E_OP_CONNECT, hSession->lu.try);

	(void) sprintf(t, "%c%c", IAC, SE);

	net_rawout(hSession, (unsigned char *)tt_out, tb_len);

	trace_dsn(
	    hSession,"SENT %s %s DEVICE-TYPE REQUEST %.*s%s%s %s\n",
	    cmd(SB),
	    opt(TELOPT_TN3270E),
	    (int) strlen(hSession->termtype),
	    tt_out + 5,
	    (hSession->lu.try != CN && *hSession->lu.try) ? " CONNECT " : "",
	    (hSession->lu.try != CN && *hSession->lu.try) ? hSession->lu.try : "",
	    cmd(SE)
	);

	lib3270_free(tt_out);
}

/*
 * Back off of TN3270E.
 */
static void backoff_tn3270e(H3270 *hSession, const char *why) {
	trace_dsn(hSession,"Aborting TN3270E: %s\n", why);

	/* Tell the host 'no'. */
	wont_opt[2] = TELOPT_TN3270E;
	net_rawout(hSession, wont_opt, sizeof(wont_opt));
	trace_dsn(hSession,"SENT %s %s\n", cmd(WONT), opt(TELOPT_TN3270E));

	/* Restore the LU list; we may need to run it again in TN3270 mode. */
	setup_lus(hSession);

	/* Reset our internal state. */
	hSession->myopts[TELOPT_TN3270E] = 0;
	check_in3270(hSession);
}

/*
 * Negotiation of TN3270E options.
 * Returns 0 if okay, -1 if we have to give up altogether.
 */
static int tn3270e_negotiate(H3270 *hSession) {
//	#define LU_MAX	32
//	static char reported_lu[LU_MAX+1];
//	static char reported_type[LU_MAX+1];

	int sblen;
	unsigned long e_rcvd;

	/* Find out how long the subnegotiation buffer is. */
	for (sblen = 0; ; sblen++) {
		if (hSession->sbbuf[sblen] == SE)
			break;
	}

	trace_dsn(hSession,"TN3270E ");

	switch (hSession->sbbuf[1]) {

	case TN3270E_OP_SEND:

		if (hSession->sbbuf[2] == TN3270E_OP_DEVICE_TYPE) {

			/* Host wants us to send our device type. */
			trace_dsn(hSession,"SEND DEVICE-TYPE SE\n");

			tn3270e_request(hSession);
		} else {
			trace_dsn(hSession,"SEND ??%u SE\n", hSession->sbbuf[2]);
		}
		break;

	case TN3270E_OP_DEVICE_TYPE:

		/* Device type negotiation. */
		trace_dsn(hSession,"DEVICE-TYPE ");

		switch (hSession->sbbuf[2]) {
		case TN3270E_OP_IS: {
			int tnlen, snlen;

			/* Device type success. */

			/* Isolate the terminal type and session. */
			tnlen = 0;
			while (hSession->sbbuf[3+tnlen] != SE &&
			        hSession->sbbuf[3+tnlen] != TN3270E_OP_CONNECT)
				tnlen++;
			snlen = 0;
			if (hSession->sbbuf[3+tnlen] == TN3270E_OP_CONNECT) {
				while(hSession->sbbuf[3+tnlen+1+snlen] != SE)
					snlen++;
			}
			trace_dsn(hSession,"IS %.*s CONNECT %.*s SE\n",
			          tnlen, &hSession->sbbuf[3],
			          snlen, &hSession->sbbuf[3+tnlen+1]);

			/* Remember the LU. */
			if (tnlen) {
				if (tnlen > LIB3270_LU_MAX)
					tnlen = LIB3270_LU_MAX;
				(void)strncpy(hSession->reported_type,(char *)&hSession->sbbuf[3], tnlen);
				hSession->reported_type[tnlen] = '\0';
				hSession->connected_type = hSession->reported_type;
			}
			if (snlen) {
				if (snlen > LIB3270_LU_MAX)
					snlen = LIB3270_LU_MAX;
				(void)strncpy(hSession->lu.reported,(char *)&hSession->sbbuf[3+tnlen+1], snlen);
				hSession->lu.reported[snlen] = '\0';
				hSession->lu.associated = hSession->lu.reported;
				status_lu(hSession,hSession->lu.associated);
			}

			/* Tell them what we can do. */
			tn3270e_subneg_send(hSession, TN3270E_OP_REQUEST, hSession->e_funcs);
			break;
		}
		case TN3270E_OP_REJECT:

			/* Device type failure. */

			trace_dsn(hSession,"REJECT REASON %s SE\n", rsn(hSession->sbbuf[4]));
			if (hSession->sbbuf[4] == TN3270E_REASON_INV_DEVICE_TYPE ||
			        hSession->sbbuf[4] == TN3270E_REASON_UNSUPPORTED_REQ) {
				backoff_tn3270e(hSession,_( "Host rejected device type or request type" ));
				break;
			}

			next_lu(hSession);
			if (hSession->lu.try != CN) {
				// Try the next LU.
				tn3270e_request(hSession);
			} else if (hSession->lu.names != (char **)NULL) {
				// No more LUs to try.  Give up.
				backoff_tn3270e(hSession,_("Host rejected resource(s)"));
			} else {
				backoff_tn3270e(hSession,_("Device type rejected"));
			}

			break;
		default:
			trace_dsn(hSession,"??%u SE\n", hSession->sbbuf[2]);
			break;
		}
		break;

	case TN3270E_OP_FUNCTIONS:

		/* Functions negotiation. */
		trace_dsn(hSession,"FUNCTIONS ");

		switch (hSession->sbbuf[2]) {

		case TN3270E_OP_REQUEST:

			/* Host is telling us what functions they want. */
			trace_dsn(hSession,"REQUEST %s SE\n",tn3270e_function_names(hSession->sbbuf+3, sblen-3));

			e_rcvd = tn3270e_fdecode(hSession->sbbuf+3, sblen-3);
			if ((e_rcvd == hSession->e_funcs) || (hSession->e_funcs & ~e_rcvd)) {
				/* They want what we want, or less.  Done. */
				hSession->e_funcs = e_rcvd;
				tn3270e_subneg_send(hSession, TN3270E_OP_IS, hSession->e_funcs);
				hSession->tn3270e_negotiated = 1;
				trace_dsn(hSession,"TN3270E option negotiation complete.\n");
				check_in3270(hSession);
			} else {
				/*
				 * They want us to do something we can't.
				 * Request the common subset.
				 */
				hSession->e_funcs &= e_rcvd;
				tn3270e_subneg_send(hSession, TN3270E_OP_REQUEST,hSession->e_funcs);
			}
			break;

		case TN3270E_OP_IS:

			/* They accept our last request, or a subset thereof. */
			trace_dsn(hSession,"IS %s SE\n",tn3270e_function_names(hSession->sbbuf+3, sblen-3));
			e_rcvd = tn3270e_fdecode(hSession->sbbuf+3, sblen-3);
			if (e_rcvd != hSession->e_funcs) {
				if (hSession->e_funcs & ~e_rcvd) {
					/*
					 * They've removed something.  This is
					 * technically illegal, but we can
					 * live with it.
					 */
					hSession->e_funcs = e_rcvd;
				} else {
					/*
					 * They've added something.  Abandon
					 * TN3270E, they're brain dead.
					 */
					backoff_tn3270e(hSession,_( "Host illegally added function(s)" ));
					break;
				}
			}
			hSession->tn3270e_negotiated = 1;
			trace_dsn(hSession,"TN3270E option negotiation complete.\n");
			check_in3270(hSession);
			break;

		default:
			trace_dsn(hSession,"??%u SE\n", hSession->sbbuf[2]);
			break;
		}
		break;

	default:
		trace_dsn(hSession,"??%u SE\n", hSession->sbbuf[1]);
	}

	/* Good enough for now. */
	return 0;
}

#if defined(X3270_TRACE) /*[*/
/* Expand a string of TN3270E function codes into text. */
static const char *
tn3270e_function_names(const unsigned char *buf, int len) {
	int i;
	static char text_buf[1024];
	char *s = text_buf;

	if (!len)
		return("(null)");
	for (i = 0; i < len; i++) {
		s += sprintf(s, "%s%s", (s == text_buf) ? "" : " ",
		             fnn(buf[i]));
	}
	return text_buf;
}
#endif /*]*/

/* Transmit a TN3270E FUNCTIONS REQUEST or FUNCTIONS IS message. */
static void tn3270e_subneg_send(H3270 *hSession, unsigned char op, unsigned long funcs) {
	unsigned char proto_buf[7 + 32];
	int proto_len;
	int i;

	/* Construct the buffers. */
	(void) memcpy(proto_buf, functions_req, 4);
	proto_buf[4] = op;
	proto_len = 5;
	for (i = 0; i < 32; i++) {
		if (funcs & E_OPT(i))
			proto_buf[proto_len++] = i;
	}

	/* Complete and send out the protocol message. */
	proto_buf[proto_len++] = IAC;
	proto_buf[proto_len++] = SE;
	net_rawout(hSession, proto_buf, proto_len);

	/* Complete and send out the trace text. */
	trace_dsn(hSession,"SENT %s %s FUNCTIONS %s %s %s\n",
	          cmd(SB), opt(TELOPT_TN3270E),
	          (op == TN3270E_OP_REQUEST)? "REQUEST": "IS",
	          tn3270e_function_names(proto_buf + 5, proto_len - 7),
	          cmd(SE));
}

/* Translate a string of TN3270E functions into a bit-map. */
static unsigned long
tn3270e_fdecode(const unsigned char *buf, int len) {
	unsigned long r = 0L;
	int i;

	/* Note that this code silently ignores options >= 32. */
	for (i = 0; i < len; i++) {
		if (buf[i] < 32)
			r |= E_OPT(buf[i]);
	}
	return r;
}
#endif /*]*/

#if defined(X3270_TN3270E) /*[*/
static void process_bind(H3270 *hSession, unsigned char *buf, int buflen) {
	int namelen, i;

	(void) memset(hSession->plu_name, '\0', sizeof(hSession->plu_name));

	/* Make sure it's a BIND. */
	if (buflen < 1 || buf[0] != BIND_RU) {
		return;
	}
	buf++;
	buflen--;

	/* Extract the PLU name. */
	if (buflen < BIND_OFF_PLU_NAME + BIND_PLU_NAME_MAX)
		return;
	namelen = buf[BIND_OFF_PLU_NAME_LEN];
	if (namelen > BIND_PLU_NAME_MAX)
		namelen = BIND_PLU_NAME_MAX;
	for (i = 0; i < namelen; i++) {
		hSession->plu_name[i] = hSession->charset.ebc2asc[buf[BIND_OFF_PLU_NAME + i]];
	}
}
#endif /*]*/

static int process_eor(H3270 *hSession) {

	trace("%s: syncing=%s",__FUNCTION__,hSession->syncing ? "Yes" : "No");

	if (hSession->syncing || !(hSession->ibptr - hSession->ibuf))
		return(0);

#if defined(X3270_TN3270E) /*[*/
	if (IN_E) {
		tn3270e_header *h = (tn3270e_header *) hSession->ibuf;
		unsigned char *s;
		enum pds rv;

		trace_dsn(hSession,"RCVD TN3270E(%s%s %s %u)\n",
		          e_dt(h->data_type),
		          e_rq(h->data_type, h->request_flag),
		          e_rsp(h->data_type, h->response_flag),
		          h->seq_number[0] << 8 | h->seq_number[1]);

		switch (h->data_type) {
		case TN3270E_DT_3270_DATA:
			if ((hSession->e_funcs & E_OPT(TN3270E_FUNC_BIND_IMAGE)) &&
			        !hSession->tn3270e_bound)
				return 0;
			hSession->tn3270e_submode = E_3270;
			check_in3270(hSession);
			hSession->response_required = h->response_flag;
			rv = process_ds(hSession, hSession->ibuf + EH_SIZE,(hSession->ibptr - hSession->ibuf) - EH_SIZE);
			if (rv < 0 &&
			        hSession->response_required != TN3270E_RSF_NO_RESPONSE)
				tn3270e_nak(hSession,rv);
			else if (rv == PDS_OKAY_NO_OUTPUT &&
			         hSession->response_required == TN3270E_RSF_ALWAYS_RESPONSE)
				tn3270e_ack(hSession);
			hSession->response_required = TN3270E_RSF_NO_RESPONSE;
			return 0;
		case TN3270E_DT_BIND_IMAGE:
			if (!(hSession->e_funcs & E_OPT(TN3270E_FUNC_BIND_IMAGE)))
				return 0;
			process_bind(hSession, hSession->ibuf + EH_SIZE, (hSession->ibptr - hSession->ibuf) - EH_SIZE);
			trace_dsn(hSession,"< BIND PLU-name '%s'\n", hSession->plu_name);
			hSession->tn3270e_bound = 1;
			check_in3270(hSession);
			return 0;
		case TN3270E_DT_UNBIND:
			if (!(hSession->e_funcs & E_OPT(TN3270E_FUNC_BIND_IMAGE)))
				return 0;
			hSession->tn3270e_bound = 0;
			if (hSession->tn3270e_submode == E_3270)
				hSession->tn3270e_submode = E_NONE;
			check_in3270(hSession);
			return 0;
		case TN3270E_DT_NVT_DATA:
			/* In tn3270e NVT mode */
			hSession->tn3270e_submode = E_NVT;
			check_in3270(hSession);
			for (s = hSession->ibuf; s < hSession->ibptr; s++) {
				ansi_process(hSession,*s++);
			}
			return 0;
		case TN3270E_DT_SSCP_LU_DATA:
			if (!(hSession->e_funcs & E_OPT(TN3270E_FUNC_BIND_IMAGE)))
				return 0;
			hSession->tn3270e_submode = E_SSCP;
			check_in3270(hSession);
			ctlr_write_sscp_lu(hSession, hSession->ibuf + EH_SIZE,(hSession->ibptr - hSession->ibuf) - EH_SIZE);
			return 0;
		default:
			/* Should do something more extraordinary here. */
			return 0;
		}
	} else
#endif /*]*/
	{
		(void) process_ds(hSession, hSession->ibuf, hSession->ibptr - hSession->ibuf);
	}
	return 0;
}

/// @brief Called when there is an exceptional condition on the socket.
void net_exception(H3270 *session, int GNUC_UNUSED(fd), LIB3270_IO_FLAG GNUC_UNUSED(flag), void GNUC_UNUSED(*dunno)) {
	debug("%s",__FUNCTION__);

	trace_dsn(session,"RCVD urgent data indication\n");
	if (!session->syncing) {
		session->syncing = 1;

		if(session->xio.except) {
			lib3270_remove_poll(session, session->xio.except);
			session->xio.except = NULL;
		}
	}
}

/**
 *	@brief send a 3270 record
 *
 * Flavors of Network Output:
 *
 * 3270 mode
 *
 *  ANSI mode; call each other in turn
 *	net_sendc	net_cookout for 1 byte
 *	net_sends	net_cookout for a null-terminated string
 *	net_cookout	send user data with cooked-mode processing, ANSI mode
 *	net_cookedout	send user data, ANSI mode, already cooked
 *	net_rawout	send telnet protocol data, ANSI mode
 *
 */

LIB3270_INTERNAL int lib3270_sock_send(H3270 *hSession, unsigned const char *buf, int len) {
	int rc = hSession->network.module->send(hSession, buf, len);

	if(rc > 0)
		return rc;

	// Send error, notify
	trace_dsn(hSession,"SND socket error %d\n", -rc);

	return -1;
}

/**
 * @brief Send out raw telnet data.
 *
 * We assume that there will always be enough space to buffer what we want to transmit,
 * so we don't handle EAGAIN or EWOULDBLOCK.
 *
 * @param hSession	Session handle.
 * @param buf		Buffer to send.
 * @param len		Buffer length
 *
 */
static void net_rawout(H3270 *hSession, unsigned const char *buf, size_t len) {
	trace_netdata(hSession, '>', buf, len);

	while (len) {
		int nw = lib3270_sock_send(hSession,buf,len);

		if (nw > 0) {
			// Data sent
			hSession->ns_bsent += nw;
			len -= nw;
			buf += nw;
		} else if(nw < 0) {
			host_disconnect(hSession,True);
			return;
		}
	}
}

#if defined(X3270_ANSI)

/**
 * @brief Send user data out in ANSI mode, without cooked-mode processing.
 */
static void net_cookedout(H3270 *hSession, const char *buf, int len) {

	if (lib3270_get_toggle(hSession,LIB3270_TOGGLE_DS_TRACE)) {
		int i;

		trace_dsn(hSession,">");
		for (i = 0; i < len; i++)
			trace_dsn(hSession," %s", ctl_see((int) *(buf+i)));
		trace_dsn(hSession,"\n");
	}

	net_rawout(hSession,(unsigned const char *) buf, len);
}

/***
 * @brief Send output in ANSI mode, including cooked-mode processing if appropriate.
 */
static void net_cookout(H3270 *hSession, const char *buf, int len) {

	if (!IN_ANSI || (hSession->kybdlock & KL_AWAITING_FIRST))
		return;

	if (hSession->linemode) {
		register int	i;
		char	c;

		for (i = 0; i < len; i++) {
			c = buf[i];

			/* Input conversions. */
			if (!hSession->lnext && c == '\r' && hSession->icrnl)
				c = '\n';
			else if (!hSession->lnext && c == '\n' && hSession->inlcr)
				c = '\r';

			/* Backslashes. */
			if (c == '\\' && !hSession->backslashed)
				hSession->backslashed = 1;
			else
				hSession->backslashed = 0;

			/* Control chars. */
			if (c == '\n')
				do_eol(hSession,c);
			else if (c == ansictl.vintr)
				do_intr(hSession, c);
			else if (c == ansictl.vquit)
				do_quit(hSession,c);
			else if (c == ansictl.verase)
				do_cerase(hSession,c);
			else if (c == ansictl.vkill)
				do_kill(hSession,c);
			else if (c == ansictl.vwerase)
				do_werase(hSession,c);
			else if (c == ansictl.vrprnt)
				do_rprnt(hSession,c);
			else if (c == ansictl.veof)
				do_eof(hSession,c);
			else if (c == ansictl.vlnext)
				do_lnext(hSession,c);
			else if (c == 0x08 || c == 0x7f) /* Yes, a hack. */
				do_cerase(hSession,c);
			else
				do_data(hSession,c);
		}
		return;
	} else
		net_cookedout(hSession, buf, len);
}

/**
 * @brief Cooked mode input processing.
 */

static void cooked_init(H3270 *hSession) {
	if (hSession->lbuf == (unsigned char *)NULL)
		hSession->lbuf = (unsigned char *)lib3270_malloc(BUFSZ);
	hSession->lbptr = hSession->lbuf;
	hSession->lnext = 0;
	hSession->backslashed = 0;
}

static void ansi_process_s(H3270 *hSession, const char *data) {
	while (*data)
		ansi_process(hSession,(unsigned int) *data++);
}

static void forward_data(H3270 *hSession) {
	net_cookedout(hSession, (char *) hSession->lbuf, hSession->lbptr - hSession->lbuf);
	cooked_init(hSession);
}

static void do_data(H3270 *hSession, char c) {
	if (hSession->lbptr+1 < hSession->lbuf + BUFSZ) {
		*hSession->lbptr++ = c;
		if (c == '\r')
			*hSession->lbptr++ = '\0';
		if (c == '\t')
			ansi_process(hSession,(unsigned int) c);
		else
			ansi_process_s(hSession,ctl_see((int) c));
	} else
		ansi_process_s(hSession,"\007");

	hSession->lnext = 0;
	hSession->backslashed = 0;
}

static void do_intr(H3270 *hSession, char c) {
	if (hSession->lnext) {
		do_data(hSession,c);
		return;
	}
	ansi_process_s(hSession,ctl_see((int) c));
	cooked_init(hSession);
	net_interrupt(hSession);
}

static void do_quit(H3270 *hSession, char c) {
	if (hSession->lnext) {
		do_data(hSession,c);
		return;
	}
	ansi_process_s(hSession,ctl_see((int) c));
	cooked_init(hSession);
	net_break(hSession);
}

static void do_cerase(H3270 *hSession, char c) {
	int len;

	if (hSession->backslashed) {
		hSession->lbptr--;
		ansi_process_s(hSession,"\b");
		do_data(hSession,c);
		return;
	}

	if (hSession->lnext) {
		do_data(hSession,c);
		return;
	}

	if (hSession->lbptr > hSession->lbuf) {
		len = strlen(ctl_see((int) *--hSession->lbptr));

		while (len--)
			ansi_process_s(hSession,"\b \b");
	}
}

static void do_werase(H3270 *hSession, char c) {
	int any = 0;
	int len;

	if (hSession->lnext) {
		do_data(hSession,c);
		return;
	}

	while (hSession->lbptr > hSession->lbuf) {
		char ch;

		ch = *--hSession->lbptr;

		if (ch == ' ' || ch == '\t') {
			if (any) {
				++hSession->lbptr;
				break;
			}
		} else
			any = 1;
		len = strlen(ctl_see((int) ch));

		while (len--)
			ansi_process_s(hSession,"\b \b");
	}
}

static void do_kill(H3270 *hSession, char c) {
	int i, len;

	if (hSession->backslashed) {
		hSession->lbptr--;
		ansi_process_s(hSession,"\b");
		do_data(hSession,c);
		return;
	}

	if (hSession->lnext) {
		do_data(hSession,c);
		return;
	}

	while (hSession->lbptr > hSession->lbuf) {
		len = strlen(ctl_see((int) *--hSession->lbptr));

		for (i = 0; i < len; i++)
			ansi_process_s(hSession,"\b \b");
	}
}

static void do_rprnt(H3270 *hSession, char c) {
	unsigned char *p;

	if (hSession->lnext) {
		do_data(hSession,c);
		return;
	}

	ansi_process_s(hSession,ctl_see((int) c));
	ansi_process_s(hSession,"\r\n");
	for (p = hSession->lbuf; p < hSession->lbptr; p++)
		ansi_process_s(hSession,ctl_see((int) *p));
}

static void do_eof(H3270 *hSession, char c) {
	if (hSession->backslashed) {
		hSession->lbptr--;
		ansi_process_s(hSession,"\b");
		do_data(hSession,c);
		return;
	}

	if (hSession->lnext) {
		do_data(hSession,c);
		return;
	}

	do_data(hSession,c);
	forward_data(hSession);
}

static void do_eol(H3270 *hSession, char c) {
	if (hSession->lnext) {
		do_data(hSession,c);
		return;
	}

	if (hSession->lbptr+2 >= hSession->lbuf + BUFSZ) {
		ansi_process_s(hSession,"\007");
		return;
	}

	*hSession->lbptr++ = '\r';
	*hSession->lbptr++ = '\n';
	ansi_process_s(hSession,"\r\n");
	forward_data(hSession);
}

static void do_lnext(H3270 *hSession, char c) {
	if (hSession->lnext) {
		do_data(hSession,c);
		return;
	}
	hSession->lnext = 1;
	ansi_process_s(hSession,"^\b");
}
#endif /*]*/


const char * lib3270_connection_state_get_name(const LIB3270_CSTATE cstate) {
	static const char *state_names[] = {
		N_("Unconnected"),
		N_("Resolving"),
		N_("Pending"),
		N_("Connected initial"),
		N_("TN3270 NVT"),
		N_("TN3270 3270"),
		N_("TN3270E"),
		N_("TN3270E NVT"),
		N_("TN3270E SSCP-LU"),
		N_("TN3270E 3270")
	};

	if(cstate > (sizeof(state_names)/sizeof(state_names[0])))
		return _("Unknown");

	return dgettext(GETTEXT_PACKAGE,state_names[cstate]);
}

LIB3270_EXPORT const char * lib3270_state_get_name(const LIB3270_STATE state) {
	static const char *state_names[] = {
		N_("Resolving"),
		N_("Connecting"),
		N_("Half connect"),
		N_("Connect"),
		N_("3270 Mode"),
		N_("Line mode"),
		N_("Remodel"),
		N_("Printer"),
		N_("Exiting"),
		N_("Charset"),
	};

	if(state > (sizeof(state_names)/sizeof(state_names[0])))
		return _("Unknown");

	return dgettext(GETTEXT_PACKAGE,state_names[state]);

}


/**
 * Check for switches between NVT, SSCP-LU and 3270 modes.
 *
 * @param hSession	Session handle.
 *
 */
static void check_in3270(H3270 *hSession) {
	LIB3270_CSTATE new_cstate = LIB3270_NOT_CONNECTED;


#if defined(X3270_TN3270E) /*[*/
	if (hSession->myopts[TELOPT_TN3270E]) {
		if (!hSession->tn3270e_negotiated)
			new_cstate = LIB3270_CONNECTED_INITIAL_E;
		else switch (hSession->tn3270e_submode) {
			case E_NONE:
				new_cstate = LIB3270_CONNECTED_INITIAL_E;
				break;
			case E_NVT:
				new_cstate = LIB3270_CONNECTED_NVT;
				break;
			case E_3270:
				new_cstate = LIB3270_CONNECTED_TN3270E;
				break;
			case E_SSCP:
				new_cstate = LIB3270_CONNECTED_SSCP;
				break;
			}
	} else
#endif /*]*/
		if (hSession->myopts[TELOPT_BINARY] &&
		        hSession->myopts[TELOPT_EOR] &&
		        hSession->myopts[TELOPT_TTYPE] &&
		        hSession->hisopts[TELOPT_BINARY] &&
		        hSession->hisopts[TELOPT_EOR]) {
			new_cstate = LIB3270_CONNECTED_3270;
		} else if (hSession->connection.state == LIB3270_CONNECTED_INITIAL) {
			/* Nothing has happened, yet. */
			return;
		} else {
			new_cstate = LIB3270_CONNECTED_ANSI;
		}

	if (new_cstate != hSession->connection.state) {
#if defined(X3270_TN3270E) /*[*/
		int was_in_e = IN_E;
#endif /*]*/

#if defined(X3270_TN3270E)
		//
		// If we've now switched between non-TN3270E mode and
		// TN3270E mode, reset the LU list so we can try again
		// in the new mode.
		//
		if (hSession->lu.names != (char **)NULL && was_in_e != IN_E) {
			hSession->lu.curr = hSession->lu.names;
			hSession->lu.try = *hSession->lu.curr;
		}
#endif

		// Allocate the initial 3270 input buffer.
		if(new_cstate >= LIB3270_CONNECTED_INITIAL && !(hSession->ibuf_size && hSession->ibuf)) {
			hSession->ibuf 		= (unsigned char *) lib3270_malloc(BUFSIZ);
			hSession->ibuf_size	= BUFSIZ;
			hSession->ibptr		= hSession->ibuf;
		}

#if defined(X3270_ANSI)
		// Reinitialize line mode.
		if ((new_cstate == LIB3270_CONNECTED_ANSI && hSession->linemode) ||
		        new_cstate == LIB3270_CONNECTED_NVT)
			cooked_init(hSession);
#endif

#if defined(X3270_TN3270E)
		// If we fell out of TN3270E, remove the state.
		if (!hSession->myopts[TELOPT_TN3270E]) {
			hSession->tn3270e_negotiated = 0;
			hSession->tn3270e_submode = E_NONE;
			hSession->tn3270e_bound = 0;
		}
#endif
		trace_dsn(hSession,"Now operating in %s mode.\n",lib3270_connection_state_get_name(new_cstate));
		host_in3270(hSession,new_cstate);
	}
}

/*
 * store3270in
 *	Store a character in the 3270 input buffer, checking for buffer
 *	overflow and reallocating ibuf if necessary.
 */
static void store3270in(H3270 *hSession, unsigned char c) {
	if(hSession->ibptr - hSession->ibuf >= hSession->ibuf_size) {
		hSession->ibuf_size += BUFSIZ;
		hSession->ibuf = (unsigned char *) lib3270_realloc((char *) hSession->ibuf, hSession->ibuf_size);
		hSession->ibptr = hSession->ibuf + hSession->ibuf_size - BUFSIZ;
	}
	*hSession->ibptr++ = c;
}

/**
 * Ensure that <n> more characters will fit in the 3270 output buffer.
 *
 * Allocates the buffer in BUFSIZ chunks.
 * Allocates hidden space at the front of the buffer for TN3270E.
 *
 * @param hSession	3270 session handle.
 * @param n			Number of characters to set.
 */
void space3270out(H3270 *hSession, int n) {
	unsigned nc = 0;	/* amount of data currently in obuf */
	unsigned more = 0;

	if (hSession->output.length)
		nc = hSession->output.ptr - hSession->output.buf;

	while ((nc + n + EH_SIZE) > (hSession->output.length + more)) {
		more += BUFSIZ;
	}

	if (more) {
		hSession->output.length += more;
		hSession->output.base = (unsigned char *)Realloc((char *) hSession->output.base,hSession->output.length);
		hSession->output.buf = hSession->output.base + EH_SIZE;
		hSession->output.ptr = hSession->output.buf + nc;
	}
}


/**
 *	Set the session variable 'linemode', which says whether we are in
 *	character-by-character mode or line mode.
 */
static void check_linemode(H3270 *hSession, Boolean init) {
	int wasline = hSession->linemode;

	/*
	 * The next line is a deliberate kluge to effectively ignore the SGA
	 * option.  If the host will echo for us, we assume
	 * character-at-a-time; otherwise we assume fully cooked by us.
	 *
	 * This allows certain IBM hosts which volunteer SGA but refuse
	 * ECHO to operate more-or-less normally, at the expense of
	 * implementing the (hopefully useless) "character-at-a-time, local
	 * echo" mode.
	 *
	 * We still implement "switch to line mode" and "switch to character
	 * mode" properly by asking for both SGA and ECHO to be off or on, but
	 * we basically ignore the reply for SGA.
	 */
	hSession->linemode = hSession->hisopts[TELOPT_ECHO] ? 0 : 1 /* && !hisopts[TELOPT_SGA] */;

	if (init || hSession->linemode != wasline) {
		lib3270_st_changed(hSession,LIB3270_STATE_LINE_MODE, hSession->linemode);
		if (!init) {
			trace_dsn(hSession,"Operating in %s mode.\n",hSession->linemode ? "line" : "character-at-a-time");
		}
#if defined(X3270_ANSI) /*[*/
		if (IN_ANSI && hSession->linemode)
			cooked_init(hSession);
#endif /*]*/
	}
}

#if defined(X3270_TRACE)

///
/// @brief Expands a number to a character string, for displaying unknown telnet commands and options.
///
static const char *
nnn(int c) {
	static char	buf[64];
	memset(buf,0,sizeof(buf));
	(void) snprintf(buf, 63, "%d", c);
	return buf;
}

///
/// @brief Expands a TELNET command into a character string.
///
static const char * cmd(int c) {
	if (TELCMD_OK(c))
		return TELCMD(c);
	else
		return nnn(c);
}

///
/// @brief Expands a TELNET option into a character string.
///
static const char * opt(unsigned char c) {
	if (c == TELOPT_TN3270E)
		return "TN3270E";
	else if (c == TELOPT_STARTTLS)
		return "START-TLS";
	else if (TELOPT_OK(c))
		return TELOPT(c);
	else
		return nnn((int)c);
}

#define LINEDUMP_MAX	32

void trace_netdata(H3270 *hSession, char direction, unsigned const char *buf, int len) {
#define NETDUMP_MAX 121

	if (lib3270_get_toggle(hSession,LIB3270_TOGGLE_NETWORK_TRACE)) {
		char l1[NETDUMP_MAX+2];
		char l2[NETDUMP_MAX+2];
		char l3[NETDUMP_MAX+2];

		int offset;
		int col = 0;

		{
			time_t ltime;
			time(&ltime);

#ifdef HAVE_LOCALTIME_R
			struct tm tm;
			strftime(l1, 81, "%x %X", localtime_r(&ltime,&tm));
#else
			strftime(l1, 81, "%x %X", localtime(&ltime));
#endif // HAVE_LOCALTIME_R
		}

		lib3270_write_nettrace(hSession,"%c %s %s data len=%d\n\n",direction,l1,direction == '>' ? "SEND" : "RECV", len);

		for (offset = 0; offset < len; offset++) {
			unsigned char text[4];

			text[0] = hSession->charset.ebc2asc[buf[offset]];
			l1[col] = (text[0] >= ' ' ? text[0] : '.');

			snprintf((char *) text,4,"%02x",buf[offset]);
			l2[col] = text[0];
			l3[col] = text[1];

			if(++col >= NETDUMP_MAX) {
				l1[col] = l2[col] = l3[col] = 0;
				lib3270_write_nettrace(hSession,"\t%s\n\t%s\n\t%s\n\n",l1,l2,l3);
				col = 0;
			}
		}

		if(col) {
			l1[col] = l2[col] = l3[col] = 0;
			lib3270_write_nettrace(hSession,"\t%s\n\t%s\n\t%s\n\n",l1,l2,l3);
		}

	} else if (lib3270_get_toggle(hSession,LIB3270_TOGGLE_DS_TRACE)) {
		int offset;
		struct timeval ts;
		double tdiff;

		(void) gettimeofday(&ts, (struct timezone *)NULL);

		if (IN_3270) {
			tdiff = ((1.0e6 * (double)(ts.tv_sec - hSession->ds_ts.tv_sec)) +
			         (double)(ts.tv_usec - hSession->ds_ts.tv_usec)) / 1.0e6;
			trace_dsn(hSession,"%c +%gs\n", direction, tdiff);
		}

		hSession->ds_ts = ts;
		for (offset = 0; offset < len; offset++) {
			if (!(offset % LINEDUMP_MAX))
				trace_dsn(hSession,"%s%c 0x%-3x ",(offset ? "\n" : ""), direction, offset);
			trace_dsn(hSession,"%02x", buf[offset]);
		}
		trace_dsn(hSession,"\n");
	}

}
#endif // X3270_TRACE


/**
 * Send 3270 output over the network.
 *
 * Send 3270 output over the network:
 *	- Prepend TN3270E header
 *	- Expand IAC to IAC IAC
 *	- Append IAC EOR
 *
 * @param hSession Session handle
 *
 */
void net_output(H3270 *hSession) {
	static unsigned char *xobuf = NULL;
	static int xobuf_len = 0;
	int need_resize = 0;
	unsigned char *nxoptr, *xoptr;

#if defined(X3270_TN3270E)
#define BSTART	((IN_TN3270E || IN_SSCP) ? hSession->output.base : hSession->output.buf)
#else
#define BSTART	obuf
#endif

#if defined(X3270_TN3270E) /*[*/
	/* Set the TN3720E header. */
	if (IN_TN3270E || IN_SSCP) {
		tn3270e_header *h = (tn3270e_header *) hSession->output.base;

		/* Check for sending a TN3270E response. */
		if (hSession->response_required == TN3270E_RSF_ALWAYS_RESPONSE) {
			tn3270e_ack(hSession);
			hSession->response_required = TN3270E_RSF_NO_RESPONSE;
		}

		/* Set the outbound TN3270E header. */
		h->data_type = IN_TN3270E ? TN3270E_DT_3270_DATA : TN3270E_DT_SSCP_LU_DATA;
		h->request_flag = 0;
		h->response_flag = 0;
		h->seq_number[0] = (hSession->e_xmit_seq >> 8) & 0xff;
		h->seq_number[1] = hSession->e_xmit_seq & 0xff;

		trace_dsn(hSession,"SENT TN3270E(%s NO-RESPONSE %u)\n",IN_TN3270E ? "3270-DATA" : "SSCP-LU-DATA", hSession->e_xmit_seq);
		if (hSession->e_funcs & E_OPT(TN3270E_FUNC_RESPONSES))
			hSession->e_xmit_seq = (hSession->e_xmit_seq + 1) & 0x7fff;
	}
#endif /*]*/

	/* Reallocate the expanded output buffer. */
	while (xobuf_len <  (hSession->output.ptr - BSTART + 1) * 2) {
		xobuf_len += BUFSZ;
		need_resize++;
	}

	if (need_resize) {
		Replace(xobuf, (unsigned char *)lib3270_malloc(xobuf_len));
	}

	/* Copy and expand IACs. */
	xoptr = xobuf;
	nxoptr = BSTART;
	while (nxoptr < hSession->output.ptr) {
		if ((*xoptr++ = *nxoptr++) == IAC) {
			*xoptr++ = IAC;
		}
	}

	/* Append the IAC EOR and transmit. */
	*xoptr++ = IAC;
	*xoptr++ = EOR;
	net_rawout(hSession,xobuf, xoptr - xobuf);

	trace_dsn(hSession,"SENT EOR\n");
	hSession->ns_rsent++;
#undef BSTART
}

#if defined(X3270_TN3270E) /*[*/
/* Send a TN3270E positive response to the server. */
static void tn3270e_ack(H3270 *hSession) {
	unsigned char rsp_buf[10];
	tn3270e_header *h_in = (tn3270e_header *) hSession->ibuf;
	int rsp_len = 0;

	rsp_len = 0;
	rsp_buf[rsp_len++] = TN3270E_DT_RESPONSE;	    /* data_type */
	rsp_buf[rsp_len++] = 0;				    /* request_flag */
	rsp_buf[rsp_len++] = TN3270E_RSF_POSITIVE_RESPONSE; /* response_flag */
	rsp_buf[rsp_len++] = h_in->seq_number[0];	    /* seq_number[0] */

	if (h_in->seq_number[0] == IAC)
		rsp_buf[rsp_len++] = IAC;

	rsp_buf[rsp_len++] = h_in->seq_number[1];	    /* seq_number[1] */

	if (h_in->seq_number[1] == IAC)
		rsp_buf[rsp_len++] = IAC;

	rsp_buf[rsp_len++] = TN3270E_POS_DEVICE_END;
	rsp_buf[rsp_len++] = IAC;
	rsp_buf[rsp_len++] = EOR;
	trace_dsn(hSession,"SENT TN3270E(RESPONSE POSITIVE-RESPONSE %u) DEVICE-END\n",h_in->seq_number[0] << 8 | h_in->seq_number[1]);
	net_rawout(hSession, rsp_buf, rsp_len);
}

/* Send a TN3270E negative response to the server. */
static void tn3270e_nak(H3270 *hSession, enum pds rv) {
	unsigned char rsp_buf[10];
	tn3270e_header *h_in = (tn3270e_header *) hSession->ibuf;
	int rsp_len = 0;
	char *neg = NULL;

	rsp_buf[rsp_len++] = TN3270E_DT_RESPONSE;	    /* data_type */
	rsp_buf[rsp_len++] = 0;				    /* request_flag */
	rsp_buf[rsp_len++] = TN3270E_RSF_NEGATIVE_RESPONSE; /* response_flag */
	rsp_buf[rsp_len++] = h_in->seq_number[0];	    /* seq_number[0] */
	if (h_in->seq_number[0] == IAC)
		rsp_buf[rsp_len++] = IAC;
	rsp_buf[rsp_len++] = h_in->seq_number[1];	    /* seq_number[1] */
	if (h_in->seq_number[1] == IAC)
		rsp_buf[rsp_len++] = IAC;
	switch (rv) {
	default:
	case PDS_BAD_CMD:
		rsp_buf[rsp_len++] = TN3270E_NEG_COMMAND_REJECT;
		neg = "COMMAND-REJECT";
		break;
	case PDS_BAD_ADDR:
		rsp_buf[rsp_len++] = TN3270E_NEG_OPERATION_CHECK;
		neg = "OPERATION-CHECK";
		break;
	}
	rsp_buf[rsp_len++] = IAC;
	rsp_buf[rsp_len++] = EOR;
	trace_dsn(hSession,"SENT TN3270E(RESPONSE NEGATIVE-RESPONSE %u) %s\n",h_in->seq_number[0] << 8 | h_in->seq_number[1], neg);
	net_rawout(hSession, rsp_buf, rsp_len);
}
#endif /*]*/

#if defined(X3270_TRACE) /*[*/
/*
 * Add IAC EOR to a buffer.
 */
void
net_add_eor(unsigned char *buf, int len) {
	buf[len++] = IAC;
	buf[len++] = EOR;
}
#endif /*]*/


#if defined(X3270_ANSI) /*[*/
/**
 * Send a character of user data over the network in ANSI mode.
 *
 * @param hSession	Session handle.
 * @param c			Character to send.
 *
 */
void
net_sendc(H3270 *hSession, char c) {
	if (c == '\r' && !hSession->linemode) {
		/* CR must be quoted */
		net_cookout(hSession,"\r\0", 2);
	} else {
		net_cookout(hSession,&c, 1);
	}
}

/**
 * Send a null-terminated string of user data in ANSI mode.
 *
 * @param hSession	Session handle.
 * @param s			String to send.
 */
void net_sends(H3270 *hSession,const char *s) {
	net_cookout(hSession, s, strlen(s));
}

/**
 * Sends the ERASE character in ANSI mode.
 *
 */
void net_send_erase(H3270 *hSession) {
	net_cookout(hSession, &ansictl.verase, 1);
}

/**
 *	Sends the KILL character in ANSI mode.
 */
void net_send_kill(H3270 *hSession) {
	net_cookout(hSession, &ansictl.vkill, 1);
}

/**
 * Sends the WERASE character in ANSI mode.
 */
void net_send_werase(H3270 *hSession) {
	net_cookout(hSession, &ansictl.vwerase, 1);
}
#endif /*]*/

/*
#if defined(X3270_MENUS)

void net_charmode(H3270 *hSession)
{
	if (!CONNECTED)
		return;

	if (!hisopts[TELOPT_ECHO])
	{
		do_opt[2] = TELOPT_ECHO;
		net_rawout(do_opt, sizeof(do_opt));
		trace_dsn(hSession,"SENT %s %s\n", cmd(DO), opt(TELOPT_ECHO));
	}

	if (!hisopts[TELOPT_SGA])
	{
		do_opt[2] = TELOPT_SGA;
		net_rawout(do_opt, sizeof(do_opt));
		trace_dsn(hSession,"SENT %s %s\n", cmd(DO), opt(TELOPT_SGA));
	}
}
#endif
*/

/*
 * net_break
 *	Send telnet break, which is used to implement 3270 ATTN.
 *
 */
void net_break(H3270 *hSession) {
	static const unsigned char buf[] = { IAC, BREAK };

	/* I don't know if we should first send TELNET synch ? */
	net_rawout(hSession, buf, sizeof(buf));
	trace_dsn(hSession,"SENT BREAK\n");
}

/*
 * net_interrupt
 *	Send telnet IP.
 *
 */
void net_interrupt(H3270 *hSession) {
	static const unsigned char buf[] = { IAC, IP };

	/* I don't know if we should first send TELNET synch ? */
	net_rawout(hSession, buf, sizeof(buf));
	trace_dsn(hSession,"SENT IP\n");
}

/*
 * net_abort
 *	Send telnet AO.
 *
 */
#if defined(X3270_TN3270E) /*[*/
void net_abort(H3270 *hSession) {
	static const unsigned char buf[] = { IAC, AO };

	if (hSession->e_funcs & E_OPT(TN3270E_FUNC_SYSREQ)) {
		/*
		 * I'm not sure yet what to do here.  Should the host respond
		 * to the AO by sending us SSCP-LU data (and putting us into
		 * SSCP-LU mode), or should we put ourselves in it?
		 * Time, and testers, will tell.
		 */
		switch (hSession->tn3270e_submode) {
		case E_NONE:
		case E_NVT:
			break;

		case E_SSCP:
			net_rawout(hSession, buf, sizeof(buf));
			trace_dsn(hSession,"SENT AO\n");
			if (hSession->tn3270e_bound || !(hSession->e_funcs & E_OPT(TN3270E_FUNC_BIND_IMAGE))) {
				hSession->tn3270e_submode = E_3270;
				check_in3270(hSession);
			}
			break;

		case E_3270:
			net_rawout(hSession, buf, sizeof(buf));
			trace_dsn(hSession,"SENT AO\n");
			hSession->tn3270e_submode = E_SSCP;
			check_in3270(hSession);
			break;
		}
	}
}
#endif /*]*/

/* Return the local address for the socket. */

/*
int net_getsockname(const H3270 *session, void *buf, int *len)
{
	if (session->connection.sock < 0)
		return -1;
	return getsockname(session->connection.sock, buf, (socklen_t *)(void *)len);
}
*/
