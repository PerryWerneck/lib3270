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
 * @file src/lib3270++/local/session.cc
 *
 * @brief Implement lib3270 direct access layout (NO IPC).
 *
 * @author perry.werneck@gmail.com
 *
 */

 #include "../private.h"

 extern "C" {
	 #include <lib3270/actions.h>
	 #include <lib3270/session.h>
 }

 using std::string;

/*---[ Implement ]----------------------------------------------------------------------------------*/

 namespace TN3270 {

	Local::Session::Session() : Abstract::Session() {

		std::lock_guard<std::mutex> lock(sync);

		this->hSession = lib3270_session_new("");
		lib3270_set_user_data(this->hSession,(void *) this);
		setCharSet(lib3270_get_display_charset(this->hSession));

		lib3270_set_popup_handler(this->hSession, popupHandler);

		// Setup callbacks
		struct lib3270_session_callbacks *cbk;

		cbk = lib3270_get_session_callbacks(this->hSession,sizeof(struct lib3270_session_callbacks));
		if(!cbk) {
			throw std::runtime_error( "Invalid callback table, possible version mismatch in lib3270" );
		}

		cbk->update_connect	= connectHandler;


	}

	Local::Session::~Session() {

		std::lock_guard<std::mutex> lock(sync);

		lib3270_session_free(this->hSession);
		this->hSession = nullptr;
	}

	void Local::Session::wait(time_t timeout) {

		int rc = lib3270_wait_for_ready(this->hSession, timeout);

		if(rc) {
			throw std::system_error(rc, std::system_category());
		}

	}

	void Local::Session::connect(const char *url) {
		std::lock_guard<std::mutex> lock(sync);
		int rc = lib3270_connect_url(hSession,url,0);

		if(rc) {
            throw std::system_error(rc, std::system_category());
		}

		wait();

	}

	void Local::Session::disconnect() {
		std::lock_guard<std::mutex> lock(sync);
		lib3270_disconnect(hSession);
	}

	// Wait for session state.
	void Local::Session::waitForReady(time_t timeout) throw() {

		std::lock_guard<std::mutex> lock(sync);
		wait(timeout);

	}

	// Gets
	std::string Local::Session::toString() const {
		std::lock_guard<std::mutex> lock(const_cast<Local::Session *>(this)->sync);
	}

	std::string	Local::Session::toString(int baddr, size_t len, bool lf) {
		std::lock_guard<std::mutex> lock(sync);
	}

	std::string	Local::Session::toString(int row, int col, size_t sz, bool lf) {
		std::lock_guard<std::mutex> lock(sync);
	}

	ProgramMessage Local::Session::getProgramMessage() const {
		std::lock_guard<std::mutex> lock(const_cast<Local::Session *>(this)->sync);
		return (ProgramMessage) lib3270_get_program_message(this->hSession);
	}

	ConnectionState Local::Session::getConnectionState() const {
		std::lock_guard<std::mutex> lock(const_cast<Local::Session *>(this)->sync);
		return (ConnectionState) lib3270_get_connection_state(this->hSession);
	}

	/// @brief Set field at current position, jumps to next writable field.
	TN3270::Session & Local::Session::push(const char *text) {
		std::lock_guard<std::mutex> lock(sync);
		return *this;
	}

	TN3270::Session & Local::Session::push(int baddr, const std::string &text) {
		std::lock_guard<std::mutex> lock(sync);
		return *this;
	}

	TN3270::Session & Local::Session::push(int row, int col, const std::string &text) {
		std::lock_guard<std::mutex> lock(sync);
		return *this;
	}

	TN3270::Session & Local::Session::push(const PFKey key) {
		std::lock_guard<std::mutex> lock(sync);
		lib3270_pfkey(hSession,(int) key);
		return *this;
	}

	TN3270::Session & Local::Session::push(const PAKey key) {
		std::lock_guard<std::mutex> lock(sync);
		lib3270_pakey(hSession,(int) key);
		return *this;
	}

	TN3270::Session & Local::Session::push(const Action action) {
		std::lock_guard<std::mutex> lock(sync);
		return *this;
	}

	TN3270::Session & Local::Session::pop(int baddr, std::string &text) {
		std::lock_guard<std::mutex> lock(sync);
		return *this;
	}

	TN3270::Session & Local::Session::pop(int row, int col, std::string &text) {
		std::lock_guard<std::mutex> lock(sync);
		return *this;
	}

	TN3270::Session & Local::Session::pop(std::string &text) {
		std::lock_guard<std::mutex> lock(sync);
		return *this;
	}

 }


