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
  * @brief Implements the default event dispatcher for linux.
  *
  */

#include <lib3270-internals.h>
#include <sys/time.h>
#include <sys/types.h>
#include <lib3270/log.h>
#include <lib3270/trace.h>

#define MILLION			1000000L
#define TN	(timeout_t *)NULL

/*---[ Implement ]------------------------------------------------------------------------------------------*/

/**
 * @brief lib3270's default event dispatcher.
 *
 * @param hSession	TN3270 session to process.
 * @param block		If non zero, the method blocks waiting for event.
 *
 */
int lib3270_default_event_dispatcher(H3270 *hSession, int block)
{
	int ns;
	struct timeval now, twait, *tp;
	int events;

	fd_set rfds, wfds, xfds;

	input_t *ip;
	int processed_any = 0;

retry:

	hSession->inputs_changed = 0;

	// If we've processed any input, then don't block again.
	if(processed_any)
		block = 0;

	events = 0;

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_ZERO(&xfds);

	for (ip = hSession->inputs; ip != (input_t *)NULL; ip = (input_t *) ip->next)
	{
		if(!ip->enabled)
		{
			debug("Socket %d is disabled",ip->fd);
			continue;
		}

		if(ip->flag & LIB3270_IO_FLAG_READ)
		{
			FD_SET(ip->fd, &rfds);
			events++;
		}

		if(ip->flag & LIB3270_IO_FLAG_WRITE)
		{
			FD_SET(ip->fd, &wfds);
			events++;
		}

		if(ip->flag & LIB3270_IO_FLAG_EXCEPTION)
		{
			FD_SET(ip->fd, &xfds);
			events++;
		}
	}

	if (block)
	{
		if (hSession->timeouts != TN)
		{
			(void) gettimeofday(&now, (void *)NULL);
			twait.tv_sec = hSession->timeouts->tv.tv_sec - now.tv_sec;
			twait.tv_usec = hSession->timeouts->tv.tv_usec - now.tv_usec;
			if (twait.tv_usec < 0L) {
				twait.tv_sec--;
				twait.tv_usec += MILLION;
			}
			if (twait.tv_sec < 0L)
				twait.tv_sec = twait.tv_usec = 0L;
			tp = &twait;
		}
		else
		{
			twait.tv_sec = 1;
			twait.tv_usec = 0L;
			tp = &twait;
		}
	}
	else
	{
		twait.tv_sec  = 1;
		twait.tv_usec = 0L;
		tp = &twait;

		if(!events)
			return processed_any;
	}

	ns = select(FD_SETSIZE, &rfds, &wfds, &xfds, tp);

	if (ns < 0 && errno != EINTR)
	{
		lib3270_popup_dialog(	hSession,
								LIB3270_NOTIFY_ERROR,
								_( "Network error" ),
								_( "Select() failed when processing for events." ),
								"%s",
								strerror(errno));
	}
	else
	{
		for (ip = hSession->inputs; ip != (input_t *) NULL; ip = (input_t *) ip->next)
		{
			if((ip->flag & LIB3270_IO_FLAG_READ) && FD_ISSET(ip->fd, &rfds))
			{
				(*ip->call)(ip->session,ip->fd,LIB3270_IO_FLAG_READ,ip->userdata);
				processed_any = True;
				if (hSession->inputs_changed)
					goto retry;
			}

			if((ip->flag & LIB3270_IO_FLAG_WRITE) && FD_ISSET(ip->fd, &wfds))
			{
				(*ip->call)(ip->session,ip->fd,LIB3270_IO_FLAG_WRITE,ip->userdata);
				processed_any = True;
				if (hSession->inputs_changed)
					goto retry;
			}

			if((ip->flag & LIB3270_IO_FLAG_EXCEPTION) && FD_ISSET(ip->fd, &xfds))
			{
				(*ip->call)(ip->session,ip->fd,LIB3270_IO_FLAG_EXCEPTION,ip->userdata);
				processed_any = True;
				if (hSession->inputs_changed)
					goto retry;
			}
		}
	}

	// See what's expired.
	if (hSession->timeouts != TN)
	{
		struct timeout *t;
		(void) gettimeofday(&now, (void *)NULL);

		while ((t = hSession->timeouts) != TN)
		{
			if (t->tv.tv_sec < now.tv_sec ||(t->tv.tv_sec == now.tv_sec && t->tv.tv_usec < now.tv_usec))
			{
				hSession->timeouts = t->next;
				t->in_play = True;
				(*t->proc)(t->session);
				processed_any = True;
				lib3270_free(t);
			} else
				break;
		}
	}

	if (hSession->inputs_changed)
		goto retry;

	return processed_any;

}
