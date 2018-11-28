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
 * Este programa está nomeado como lib3270++.h e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

/**
 * @file src/lib3270++/local/events.cc
 *
 * @brief Implement lib3270 direct access events.
 *
 * @author perry.werneck@gmail.com
 *
 */

 #include "../private.h"
 #include <stdio.h>

 extern "C" {
	 #include <lib3270/actions.h>
	 #include <lib3270/session.h>
 }

 using std::string;

/*---[ Implement ]----------------------------------------------------------------------------------*/

#ifndef HAVE_VASPRINTF
	int vasprintf(char **strp, const char *fmt, va_list ap) {
		char buf[1024];

		int nc = vsnprintf(buf, sizeof(buf), fmt, args);

		if(nc < 0) {

			*strp = strdup(_("Error in vasprintf"));

		} else if (nc < sizeof(buf)) {

			*strp = malloc(nc+1);
			strcpy(*strp, buf);

		} else {

			*strp = malloc(nc + 1);
			if(vsnprintf(*strp, nc, fmt, args) < 0) {
				free(*strp);
				*strp = strdup(NULL,_( "Out of memory in vasprintf" ) );
			}

		}

		return nc;
	}
#endif // !HAVE_VASPRINTF

 namespace TN3270 {

	/// @brief Popup Handler.
	void Local::Session::popupHandler(H3270 *h3270, LIB3270_NOTIFY type, const char *title, const char *msg, const char *fmt, va_list arg) {

		Local::Session * session = (Local::Session *) lib3270_get_user_data(h3270);

		if(!session) {
			throw std::runtime_error(_( "Invalid session handler" ));
		}

        class PopupEvent : public TN3270::Event {
		private:
			LIB3270_NOTIFY type;
			string title;
			string msg;
			string description;

		public:
			PopupEvent(LIB3270_NOTIFY type, const char *title, const char *msg, const char *fmt, va_list arg) : Event(Event::Popup) {

				this->type = type;
				this->title = title;
				this->msg = msg;

				char * buffer = NULL;
				if(vasprintf(&buffer,fmt,arg) != -1) {
					this->description = buffer;
					free(buffer);
				}

#ifdef DEBUG
				std::cerr	<< "Popup:"				<< std::endl
							<<	"\t" << title		<< std::endl
							<<	"\t" << msg			<< std::endl
							<<	"\t" <<	description	<< std::endl;
#endif // DEBUG

			}

			virtual ~PopupEvent() {
			}

			/// @brief Get event description.
			std::string toString() const override {
				return msg;
			}


        };

        session->fire(PopupEvent(type,title,msg,fmt,arg));

	}

	/// @brief Connect Handler.
	void Local::Session::connectHandler(H3270 *h3270, unsigned char connected) {

		Local::Session * session = (Local::Session *) lib3270_get_user_data(h3270);

		if(!session) {
			throw std::runtime_error(_("Invalid session handler"));
		}

        class ConnectionEvent : public TN3270::Event {
		private:
			bool connected;

		public:
			ConnectionEvent(unsigned char connected) : Event(Event::Connection) {
				this->connected = (connected != 0);

#ifdef DEBUG
				std::cerr << "Session is " << this->toString().c_str() << std::endl;
#endif // DEBUG

			}

			virtual ~ConnectionEvent() {
			}

			/// @brief Get event description.
			std::string toString() const override {
				return this->connected ? _("connected") : _("disconnected");
			}

        };

		session->fire(ConnectionEvent(connected));

	}


 }


