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
 * @file src/lib3270++/ipc/linux/session.cc
 *
 * @brief Implements Linux session create/destroy session.
 *
 * @author perry.werneck@gmail.com
 *
 */

 #include "../private.h"
 #include <cstring>
 #include <lib3270/trace.h>

 using std::string;

/*---[ Implement ]----------------------------------------------------------------------------------*/

 static void throws_if_error(DBusError &err) {

 	if(dbus_error_is_set(&err)) {
		string message = err.message;
		dbus_error_free(&err);
		throw std::runtime_error(message.c_str());
 	}

 	return;

 }

 namespace TN3270 {

	IPC::Session::Session(const char *id) : Abstract::Session() {

		// Create D-Bus session.
		DBusError err;

		dbus_error_init(&err);
		this->conn = dbus_bus_get(DBUS_BUS_SESSION, &err);

		debug("dbus_bus_get conn=",conn);

		throws_if_error(err);

		if(!conn)
			throw std::runtime_error("DBUS Connection failed");

		auto sep = strchr(id,':');
		if(!sep) {
			throw std::system_error(EINVAL, std::system_category());
		}

		this->name = "br.com.bb.";
		this->name += string(id,(sep - id));
		this->name += ".";
		this->name += (sep+1);
		this->path = "/br/com/bb/tn3270/session";
		this->interface = "br.com.bb.tn3270.session";

		debug("D-Bus Object name=\"",this->name,"\" D-Bus Object path=\"",this->path,"\"");

	}

	IPC::Session::~Session() {

	}

 }


