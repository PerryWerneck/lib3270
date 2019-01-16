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
 * @file src/lib3270++/windows/request.cc
 *
 * @brief Implements WIN32 Pipe Based IPC.
 *
 * @author perry.werneck@gmail.com
 *
 */

 #include "../private.h"

 using std::string;

/*---[ Implement ]----------------------------------------------------------------------------------*/

 namespace TN3270 {

	IPC::Request::Request(const Session &session) {
	}

	IPC::Request::Request(const Session &session, const char *method) : Request(session) {
	}

	IPC::Request::Request(const Session &session, bool isSet, const char *property) : Request(session) {
	}

	IPC::Request::~Request() {

		for(auto block : input) {
			delete[] ((uint8_t *) block);
		}

		for(auto block : output) {
			delete[] ((uint8_t *) block);
		}

	}

	/// @brief Create DataBlock
	IPC::Request::DataBlock * IPC::Request::createDataBlock(const void *ptr, size_t length) {

		IPC::Request::DataBlock * rc = (IPC::Request::DataBlock *) (new uint8_t[sizeof(IPC::Request::DataBlock)+length]);
		memset((void *) rc, 0, sizeof(IPC::Request::DataBlock));
		memcpy(((uint8_t *) (rc+1)), ((uint8_t *) ptr), length);

		return rc;
	}

	IPC::Request & IPC::Request::call() {
		return *this;
	}

	IPC::Request & IPC::Request::push(const char *arg) {
		IPC::Request::DataBlock * block = createDataBlock(arg, strlen(arg)+1);
		output.push_back(block);
		return *this;
	}

	IPC::Request & IPC::Request::pop(std::string &value) {

		return *this;
	}

	IPC::Request & IPC::Request::Request::pop(int &value) {
		return *this;
	}

 }


