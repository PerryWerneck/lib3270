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
 * @file src/lib3270++/linux/request.cc
 *
 * @brief Implements D-Bus message.
 *
 * @author perry.werneck@gmail.com
 *
 */

 #include "../private.h"

 using std::string;

/*---[ Implement ]----------------------------------------------------------------------------------*/

 namespace TN3270 {

	IPC::Request::Request(const Session &session) {
		this->conn = session.conn;
		this->msg.in = nullptr;
		this->msg.out = nullptr;
	}

	IPC::Request::Request(const Session &session, const char *method) : Request(session) {

		this->msg.out = dbus_message_new_method_call(
							session.name.c_str(),					// Destination
							session.path.c_str(),					// Path
							session.interface.c_str(),				// Interface
							method									// Method
						);

		if(!msg.out) {
			throw std::runtime_error("Can't create D-Bus Method Call");
		}

	}

	IPC::Request::Request(const Session &session, bool isSet, const char *property) : Request(session) {

/*
		dbus-send \
			--session \
			--dest=br.com.bb.pw3270.a\
			--print-reply \
			"/br/com/bb/tn3270/session" \
			"org.freedesktop.DBus.Properties.Get" \
			string:br.com.bb.tn3270.session \
			string:${1}
*/
		this->msg.out = dbus_message_new_method_call(
							session.name.c_str(),					// Destination
							session.path.c_str(),					// Path
							"org.freedesktop.DBus.Properties",		// Interface
							(isSet ? "Set" : "Get")
						);

		if(!msg.out) {
			throw std::runtime_error("Can't create D-Bus Property Call");
		}

		//
		// https://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-properties
		// org.freedesktop.DBus.Properties.Get (in STRING interface_name,
		// 								   in STRING property_name,
		// 								   out VARIANT value);
		// org.freedesktop.DBus.Properties.Set (in STRING interface_name,
		// 								   in STRING property_name,
		//
		const char *interface_name = session.interface.c_str();

		dbus_message_append_args(
				this->msg.out,
					DBUS_TYPE_STRING,&interface_name,
					DBUS_TYPE_STRING,&property,
					DBUS_TYPE_INVALID
				);

	}

	IPC::Request::~Request() {
		if(msg.out) {
			dbus_message_unref(msg.out);
		}
		if(msg.in) {
			dbus_message_unref(msg.in);
		}
	}

	IPC::Request & IPC::Request::call() {

		if(msg.in) {
			dbus_message_unref(msg.in);
			msg.in = nullptr;
		}

		DBusError error;
		dbus_error_init(&error);
		this->msg.in = dbus_connection_send_with_reply_and_block(this->conn,this->msg.out,10000,&error);

		if(!this->msg.in) {
			string message = error.message;
			dbus_error_free(&error);
			throw std::runtime_error(message.c_str());
		}

		dbus_message_iter_init(msg.in, &msg.iter);

		debug(__FUNCTION__," got a valid response");

		return *this;

	}

	IPC::Request & IPC::Request::push(const char *arg) {
		dbus_message_append_args(this->msg.out,DBUS_TYPE_STRING,&arg,DBUS_TYPE_INVALID);
		return *this;
	}

	IPC::Request & IPC::Request::pop(std::string &value) {

		const char * str = "";

		if(dbus_message_iter_get_arg_type(&msg.iter) == DBUS_TYPE_STRING) {

			dbus_message_iter_get_basic(&msg.iter, &str);

		} else if(dbus_message_iter_get_arg_type(&msg.iter) == DBUS_TYPE_VARIANT) {

			DBusMessageIter sub;
			int current_type;

			dbus_message_iter_recurse(&msg.iter, &sub);

            while ((current_type = dbus_message_iter_get_arg_type(&sub)) != DBUS_TYPE_INVALID) {

                if (current_type == DBUS_TYPE_STRING) {
                    dbus_message_iter_get_basic(&sub, &str);
                    break;
                }
                dbus_message_iter_next(&sub);
            }

		} else {

			debug("Argument type is ", ((char) dbus_message_iter_get_arg_type(&msg.iter)) );
			throw std::runtime_error("Expected an string data type");

		}

		value.assign(str);

		debug(__FUNCTION__,"= \"",str,"\"");

		return *this;
	}


 }


