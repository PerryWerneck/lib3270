/*
 * "Software PW3270, desenvolvido com base nos códigos fontes do WC3270  e  X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270.
 *
 * Copyright (C) <2008> <Banco do Brasil S.A.>
 *
 * Este programa é software livre. Você pode redistribuí-lo e/ou modificá-lo sob
 * os termos da GPL v.2 - Licença Pública Geral  ',  conforme  publicado  pela
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
 * Este programa está nomeado como networking.h e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas de Mendonça)
 *
 */

 /**
  * @brief Default networking methods.
  *
  */

 #include <config.h>
 #include <internals.h>
 #include <networking.h>
 #include <screen.h>
 #include <unistd.h>
 #include <fcntl.h>

 #include <sys/types.h>
 #include <sys/socket.h>
 #include <netdb.h>

int lib3270_network_connect(H3270 *hSession, LIB3270_NETWORK_STATE *state) {

	//
	// Resolve hostname
	//
	struct addrinfo	  hints;
 	struct addrinfo * result	= NULL;
	memset(&hints,0,sizeof(hints));
	hints.ai_family 	= AF_UNSPEC;	// Allow IPv4 or IPv6
	hints.ai_socktype	= SOCK_STREAM;	// Stream socket
	hints.ai_flags		= AI_PASSIVE;	// For wildcard IP address
	hints.ai_protocol	= 0;			// Any protocol

	status_resolving(hSession);

 	int rc = getaddrinfo(hSession->host.current, hSession->host.srvc, &hints, &result);
 	if(rc)
	{
		state->error_message = gai_strerror(rc);
		return -1;
	}

	//
	// Try connecting to hosts.
	//
	int sock = -1;
	struct addrinfo * rp = NULL;

	status_connecting(hSession);

	for(rp = result; sock < 0 && rp != NULL; rp = rp->ai_next)
	{
		// Got socket from host definition.
		sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(sock < 0)
		{
			// Can't get socket.
			state->syserror = errno;
			continue;
		}

		// Try connect.
		if(connect(sock, rp->ai_addr, rp->ai_addrlen))
		{
			// Can't connect to host
			state->syserror = errno;
			close(sock);
			sock = -1;
			continue;
		}

	}

	freeaddrinfo(result);

	if(sock < 0)
	{
		static const LIB3270_POPUP popup = {
			.name = "CantConnect",
			.type = LIB3270_NOTIFY_ERROR,
			.summary = N_("Can't connect to host"),
			.label = N_("Try again")
		};

		state->popup = &popup;
		return sock;
	}

	// don't share the socket with our children
	(void) fcntl(sock, F_SETFD, 1);

	return sock;
}
