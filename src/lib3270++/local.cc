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
 * @file src/lib3270++/local.cc
 *
 * @brief
 *
 * @author perry.werneck@gmail.com
 *
 */

 #include "private.h"


/*---[ Implement ]----------------------------------------------------------------------------------*/

 namespace TN3270 {

	LocalSession::LocalSession() : Abstract::Session() {
		this->hSession = lib3270_session_new("");
		lib3270_set_user_data(this->hSession,(void *) this);
	}

	LocalSession::~LocalSession() {
		lib3270_session_free(this->hSession);
		this->hSession = nullptr;
	}

	void LocalSession::connect(const char *url) {
		lib3270_connect_url(hSession,url,0);
	}

	void LocalSession::disconnect() {
		lib3270_disconnect(hSession);
	}

	// Wait for session state.
	void LocalSession::waitForReady(time_t timeout) throw() {

		int rc = lib3270_wait_for_ready(this->hSession, timeout);

		if(rc) {
            throw std::system_error(rc, std::system_category());
		}

	}

	// Gets
	std::string LocalSession::toString() const {
	}

	std::string	LocalSession::toString(int baddr, size_t len, bool lf) {
	}

	std::string	LocalSession::toString(int row, int col, size_t sz, bool lf) {
	}

	ProgramMessage LocalSession::getProgramMessage() const {
		return (ProgramMessage) lib3270_get_program_message(this->hSession);
	}

	ConnectionState LocalSession::getConnectionState() const {
		return (ConnectionState) lib3270_get_connection_state(this->hSession);
	}

	/// @brief Set field at current posicion, jumps to next writable field.
	TN3270::Session & LocalSession::push(const char *text) {
		return *this;
	}

	TN3270::Session & LocalSession::push(int baddr, const std::string &text) {
		return *this;
	}

	TN3270::Session & LocalSession::push(int row, int col, const std::string &text) {
		return *this;
	}

	TN3270::Session & LocalSession::push(const PFKey key) {
		return *this;
	}

	TN3270::Session & LocalSession::push(const PAKey key) {
		return *this;
	}

	TN3270::Session & LocalSession::push(const Action action) {
		return *this;
	}

	TN3270::Session & LocalSession::pop(int baddr, std::string &text) {
		return *this;
	}

	TN3270::Session & LocalSession::pop(int row, int col, std::string &text) {
		return *this;
	}

	TN3270::Session & LocalSession::pop(std::string &text) {
		return *this;
	}

 }


