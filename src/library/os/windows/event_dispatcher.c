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
 * @brief Implements the default event dispatcher for windows.
 *
 */

#include <internals.h>
#include <sys/time.h>
#include <sys/types.h>
#include <lib3270/log.h>
#include <lib3270/os.h>

#define TN	(timeout_t *)NULL

/*---[ Implement ]------------------------------------------------------------------------------------------*/

static void ms_ts(unsigned long long *u) {
	FILETIME t;

	/* Get the system time, in 100ns units. */
	GetSystemTimeAsFileTime(&t);
	memcpy(u, &t, sizeof(unsigned long long));

	/* Divide by 10,000 to get ms. */
	*u /= 10000ULL;
}

/**
 * @brief lib3270's default event dispatcher.
 *
 * @param hSession	TN3270 session to process.
 * @param block		If non zero, the method blocks waiting for event.
 *
 */
int lib3270_default_event_dispatcher(H3270 *hSession, int block) {
	unsigned long long now;
	int maxSock;
	DWORD tmo;

	fd_set rfds, wfds, xfds;

	input_t *ip;
	int processed_any = 0;

retry:

	hSession->input.changed = 0;

	// If we've processed any input, then don't block again.
	if(processed_any)
		block = 0;

	maxSock = 0;

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_ZERO(&xfds);

	for (ip = (input_t *) hSession->input.list.first; ip != (input_t *)NULL; ip = (input_t *) ip->next) {
		if(!ip->enabled) {
			debug("Socket %d is disabled",ip->fd);
			continue;
		}

		if(ip->flag & LIB3270_IO_FLAG_READ) {
			FD_SET(ip->fd, &rfds);
			maxSock = max(ip->fd,maxSock);
		}

		if(ip->flag & LIB3270_IO_FLAG_WRITE) {
			FD_SET(ip->fd, &wfds);
			maxSock = max(ip->fd,maxSock);
		}

		if(ip->flag & LIB3270_IO_FLAG_EXCEPTION) {
			FD_SET(ip->fd, &xfds);
			maxSock = max(ip->fd,maxSock);
		}
	}

	if (block) {
		if (hSession->timeouts.first) {
			ms_ts(&now);
			if (now > ((timeout_t *) hSession->timeouts.first)->ts)
				tmo = 0;
			else
				tmo = ((timeout_t *) hSession->timeouts.first)->ts - now;
		} else {
			// Block for 1 second (at maximal)
			tmo = 1000;
		}
	} else {
		tmo = 1000;
	}

	if(maxSock) {
		struct timeval tm;

		tm.tv_sec 	= 0;
		tm.tv_usec	= tmo;

		int ns = select(maxSock+1, &rfds, &wfds, &xfds, &tm);

		if (ns < 0 && errno != EINTR) {
			lib3270_popup_dialog(	hSession,
			                        LIB3270_NOTIFY_ERROR,
			                        _( "Network error" ),
			                        _( "Select() failed when processing for events." ),
			                        lib3270_win32_strerror(WSAGetLastError()));
		} else {
			for (ip = (input_t *) hSession->input.list.first; ip != (input_t *)NULL; ip = (input_t *) ip->next) {
				if((ip->flag & LIB3270_IO_FLAG_READ) && FD_ISSET(ip->fd, &rfds)) {
					(*ip->call)(hSession,ip->fd,LIB3270_IO_FLAG_READ,ip->userdata);
					processed_any = True;
					if (hSession->input.changed)
						goto retry;
				}

				if ((ip->flag & LIB3270_IO_FLAG_WRITE) && FD_ISSET(ip->fd, &wfds)) {
					(*ip->call)(hSession,ip->fd,LIB3270_IO_FLAG_WRITE,ip->userdata);
					processed_any = True;
					if (hSession->input.changed)
						goto retry;
				}

				if ((ip->flag & LIB3270_IO_FLAG_EXCEPTION) && FD_ISSET(ip->fd, &xfds)) {
					(*ip->call)(hSession,ip->fd,LIB3270_IO_FLAG_EXCEPTION,ip->userdata);
					processed_any = True;
					if (hSession->input.changed)
						goto retry;
				}
			}
		}
	} else if(block) {
		Sleep(tmo);
	}

	// See what's expired.
	if (hSession->timeouts.first) {
		struct timeout *t;
		ms_ts(&now);

		while(hSession->timeouts.first) {
			t = (struct timeout *) hSession->timeouts.first;

			if (t->ts <= now) {

				t->in_play = True;
				(*t->proc)(hSession,t->userdata);
				processed_any = True;

				lib3270_linked_list_delete_node(&hSession->timeouts,t);

			} else {
				break;
			}
		}
	}

	if (hSession->input.changed)
		goto retry;

	return processed_any;

}

