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

 #include <private/network.h>
 #include <private/intl.h>
 #include <private/mainloop.h>
 
 #include <sys/signalfd.h>
 #include <signal.h>           // Definition of SIG_* constants
 #include <sys/syscall.h>      // Definition of SYS_* constants
 #include <unistd.h>
 #include <sys/types.h>
 #include <sys/socket.h>
 #include <netdb.h>
 #include <string.h>
 #include <arpa/inet.h>

 #include <lib3270.h>
 #include <lib3270/mainloop.h>
 #include <lib3270/log.h>
 #include <lib3270/popup.h>
 #include <trace_dsc.h>
 #include <malloc.h>

 #include <internals.h>

 typedef struct {
	LIB3270_NET_CONTEXT parent;
	int sock;
	sigset_t sigs;
	struct addrinfo hints;
	struct gaicb *list[1];

	void *timer;
	void *resolved;

	char buffer[0];
 } Context;

 static int finalize(H3270 *hSession, Context *context) {	

	debug("%s: Cleaning resolver context %p",__FUNCTION__,context);

	int rc = 0;

	if(context->sock != -1) {
		rc = gai_cancel(context->list[0]);
		context->sock = -1;
	}

	if(context->timer) {
		lib3270_remove_timer(hSession,context->timer);
		context->timer = NULL;
	}
	
	if(context->resolved) {
		lib3270_remove_poll(hSession,context->resolved);
		context->resolved = NULL;
	}

	if(context->list[0]) {
		free(context->list[0]);
		context->list[0] = NULL;
	}

	debug("%s: DISCONNECT rc=%d",__FUNCTION__,rc);

	return rc;
 }

 static void failed(H3270 *hSession, Context *context) {
	if(context->sock != -1) {
		close(context->sock);
		context->sock = -1;
	}
	lib3270_connection_close(hSession,-1);
 }

 static void net_response(H3270 *hSession, int sock, LIB3270_IO_FLAG flag, Context *context) {

	debug("%s: GOT response on context %p socket %d",__FUNCTION__,context,context->sock);

    struct signalfd_siginfo ssi;
    ssize_t rret;

	while(1) {

 		while ((rret = read(sock, &ssi, sizeof(ssi))) == -1 && errno == EINTR);

    	if (rret == -1) {

        	if (errno == EAGAIN || errno == EWOULDBLOCK)
            	return;

	    } else if (rret != sizeof(ssi)) {

			LIB3270_POPUP popup = {
				.name		= "dns-error",
				.type		= LIB3270_NOTIFY_ERROR,
				.title		= _("DNS error"),
				.summary	= _("Error resolving host name"),
				.body		= _("Unexpected number of bytes read from signalfd"),
				.label		= _("OK")
			};

			failed(hSession,context);
			lib3270_popup(hSession, &popup, 0);

			return;

		}  else if (ssi.ssi_code != SI_ASYNCNL) {

			LIB3270_POPUP template = {
				.name		= "dns-error",
				.type		= LIB3270_NOTIFY_ERROR,
				.title		= _("DNS error"),
				.summary	= _("Error resolving host name"),
				.body		= "",
				.label		= _("OK")
			};

			lib3270_autoptr(LIB3270_POPUP) popup =
				lib3270_popup_clone_printf(
					&template,
					_("Received signal with unexpected si_code '%d'"),
					ssi.ssi_code,
					hSession->host.url
				);

			failed(hSession,context);
			lib3270_popup(hSession, popup, 0);

			return;

		}

		// Handle DNS response
		struct gaicb *req = (void *) ssi.ssi_ptr;
		{
			int rc = gai_error(req);
			if(rc != 0) {

				if(rc == EAI_INPROGRESS) {
					trace_dsn(hSession,"DNS request still in progress\n");
					continue;
				}

				LIB3270_POPUP popup = {
					.name		= "dns-error",
					.type		= LIB3270_NOTIFY_ERROR,
					.title		= _("DNS error"),
					.summary	= _("Error resolving host name"),
					.body		= gai_strerror(rc),
					.label		= _("OK")
				};

				failed(hSession,context);
				lib3270_popup(hSession, &popup, 0);

			} else {

				trace_dsn(hSession,"DNS request completed\n");

				int error = -1;

				for(struct addrinfo *ai = req->ar_result; ai; ai = ai->ai_next) {

					char host[NI_MAXHOST];
					if (getnameinfo(ai->ai_addr, ai->ai_addrlen, host, sizeof(host), NULL, 0, NI_NUMERICHOST) == 0) {
						trace_dsn(
							hSession,
							"Connecting address %s for '%s:%s'\n",
							host,
							context->list[0]->ar_name,
							context->list[0]->ar_service
						);
					} else {
						trace_dsn(
							hSession,
							"Unable to get address for '%s:%s'\n",
							context->list[0]->ar_name,
							context->list[0]->ar_service
						);
					}

					int sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
					if(sock < 0) {
						// Can't get socket.
						error = errno;
						trace_dsn(
							hSession,
							"Cant get socket for connection to %s: %s\n",
							host,
							strerror(error)
						);
						continue;
					} else if(lib3270_connect_socket(hSession, sock, ai->ai_addr, ai->ai_addrlen) == 0) {
						// Connect socket suceeded, the context was replaced, release it.
						freeaddrinfo(req->ar_result);	
						finalize(hSession,context);
						lib3270_free(context);
						return;
					}

				}

				// Release resources
				debug("%s: Releasing ar_result",__FUNCTION__);
				freeaddrinfo(req->ar_result);	

			}

		}

	}

 }

 static int net_timeout(H3270 *hSession, Context *context) {

	debug("%s: TIMEOUT",__FUNCTION__);

	context->timer = NULL;

	int rc = gai_cancel(context->list[0]);
	failed(hSession,context);

	debug("gai_cancel() = %d",rc);

	if(rc) {

		LIB3270_POPUP popup = {
			.name		= "dns-error",
			.type		= LIB3270_NOTIFY_ERROR,
			.title		= _("DNS error"),
			.summary	= _("Unexpected response to cancel request"),
			.body		= gai_strerror(rc),
			.label		= _("OK")
		};

		lib3270_popup(hSession, &popup, 0);

	} else {

		LIB3270_POPUP popup = {
			.name		= "dns-timeout",
			.type		= LIB3270_NOTIFY_ERROR,
			.title		= _("DNS error"),
			.summary	= _("Unable to resolve host name"),
			.body		= strerror(ETIMEDOUT),
			.label		= _("OK")
		};

		lib3270_popup(hSession, &popup, 0);

	}
	return 0;
 }

 ///
 /// @brief Asynchronously resolves the hostname to an IP address.
 ///
 /// This function takes a hostname as input and performs an asynchronous DNS
 /// resolution to obtain the corresponding IP address. It uses non-blocking
 /// operations to ensure that the main thread is not blocked while waiting for
 /// the DNS resolution to complete.
 ///
 /// @param hostname The hostname to be resolved.
 /// @param service The service/port name.
 /// @param timeout The connection timeout in seconds.
 /// @return The resolver network context, NULL if failed.
 ///
 LIB3270_INTERNAL LIB3270_NET_CONTEXT * resolv_hostname(H3270 *hSession, const char *hostname, const char *service, time_t timeout) {

	// Reference: https://github.com/kazuho/examples/blob/master/getaddrinfo_a%2Bsignalfd.c

	debug("Connection %s:%s",hostname,service);

	// Allocate and setup context
	Context *context = lib3270_malloc(sizeof(Context) + strlen(hostname) + strlen(service) + 3);
	memset(context,0,sizeof(Context));
	
	// Set disconnect handler
	context->parent.disconnect = (void *) finalize;

	// Setup DNS search
	context->list[0] = malloc(sizeof(struct gaicb));
	context->list[0]->ar_name = context->buffer;
	strcpy((char *) context->list[0]->ar_name,hostname);

	context->list[0]->ar_service = context->list[0]->ar_name + strlen(hostname) + 1;
	strcpy((char *) context->list[0]->ar_service,service);

	context->hints.ai_family 		= AF_UNSPEC;	// Allow IPv4 or IPv6
	context->hints.ai_socktype		= SOCK_STREAM;	// Stream socket
	context->hints.ai_flags			= AI_PASSIVE;	// For wildcard IP address
	context->hints.ai_protocol		= 0;			// Any protocol
	context->list[0]->ar_request 	= &context->hints;

	// Setup signal handler.
	sigemptyset(&context->sigs);
	sigaddset(&context->sigs, SIGRTMIN);
	sigprocmask(SIG_BLOCK, &context->sigs, NULL);
	context->sock = signalfd(-1, &context->sigs, SFD_NONBLOCK | SFD_CLOEXEC);

	struct sigevent sev;
	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIGRTMIN;
	sev.sigev_value.sival_ptr = context->list[0];

	// Send request.
	int ret = getaddrinfo_a(GAI_NOWAIT, context->list, 1, &sev);
	if(ret) {

		failed(hSession,context);

		LIB3270_POPUP popup = {
			.name		= "dns-error",
			.type		= LIB3270_NOTIFY_ERROR,
			.title		= _("DNS error"),
			.summary	= _("Unable to resolve host name"),
			.body		= gai_strerror(ret),
			.label		= _("OK")
		};

		lib3270_popup(hSession, &popup, 0);

		return NULL;

	}

	context->timer = lib3270_add_timer(timeout*1000,hSession,(void *) net_timeout,context);
	context->resolved = lib3270_add_poll_fd(hSession,context->sock,LIB3270_IO_FLAG_READ,(void *) net_response,context);

	return (LIB3270_NET_CONTEXT *) context;
 }
