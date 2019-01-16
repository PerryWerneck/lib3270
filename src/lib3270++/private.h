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
 * @file src/lib3270++/private.h
 *
 * @brief
 *
 * @author perry.werneck@gmail.com
 *
 */

#ifndef PRIVATE_H_INCLUDED

	#define PRIVATE_H_INCLUDED

	#include <config.h>

	#ifdef WIN32
		#include <winsock2.h>
		#include <windows.h>
		#include <ws2tcpip.h>
	#else
		#include <dbus/dbus.h>
	#endif // WIN32

	#include <iostream>
	#include <mutex>
	#include <lib3270++.h>
	#include <lib3270/popup.h>
	#include <system_error>
	#include <stdexcept>

#ifdef HAVE_LIBINTL
        #include <libintl.h>
        #define _( x )                  gettext(x)
        #define N_( x )                 x
#else
        #define _( x )                  x
        #define N_( x )                 x
#endif // HAVE_LIBINTL

#ifdef HAVE_ICONV
	#include <iconv.h>
#endif // HAVE_ICONV

#ifdef WIN32
	#define SYSTEM_CHARSET "CP1252"
#else
	#define SYSTEM_CHARSET "UTF-8"
#endif // WIN32

#ifdef DEBUG

	inline void console(std::ostream &out) {
		out << std::endl;
	}

	template<typename T, typename... Targs>
	void console(std::ostream &out, T value, Targs... Fargs) {
		out << value;
		console(out, Fargs...);
	}

	template<typename T, typename... Targs>
	void log(T value, Targs... Fargs) {
		console(std::clog,value,Fargs...);
	}

	#define debug(...) log(__FILE__, "(", __LINE__, ") ", __VA_ARGS__);

#else

	#define debug(...) /* __VA_ARGS__ */

#endif

	namespace TN3270 {

		namespace Abstract {

			class TN3270_PRIVATE Session : public TN3270::Session {
			private:

#ifdef HAVE_ICONV
				struct {

					/// @brief Convert strings from host codepage to local codepage.
					iconv_t local;

					/// @brief Convert string from local codepage to host codepage.
					iconv_t	host;

				} converter;
#endif

				/// @brief Converte charset.
				static std::string convertCharset(iconv_t &converter, const char *str);

			protected:

				/// @brief Current in/out position.
				int baddr;

				Session();
				virtual ~Session();

				/// @brief Setup charsets
				void setCharSet(const char *remote, const char *local = SYSTEM_CHARSET);

				/// @brief Converte string recebida do host para o charset atual.
				std::string convertFromHost(const char *str) const;

				/// @brief Converte string do charset atual para o charset do host.
				std::string convertToHost(const char *str) const;

			};

		}

		/// @brief lib3270 direct access objects (no IPC);
		namespace Local {

			class TN3270_PRIVATE Session : public TN3270::Abstract::Session {
			private:

				/// @brief Handle of the related instance of lib3270
				H3270 * hSession;

				/// @brief Mutex to serialize access to lib3270
				std::mutex sync;

				/// @brief Popup Handler.
				static void popupHandler(H3270 *session, LIB3270_NOTIFY type, const char *title, const char *msg, const char *fmt, va_list arg);

				/// @brief Connect Handler.
				static void connectHandler(H3270 *session, unsigned char connected);

				/// @brief Wait for network events
				void wait(time_t timeout = 5);

			public:
				Session();
				virtual ~Session();

				// Connect/disconnect
				void connect(const char *url) override;
				void disconnect() override;

				// Wait for session state.
				void waitForReady(time_t timeout = 5)  throw() override;

				// Get properties.
				void getProperty(const char *name, int &value) const override;
				void getProperty(const char *name, std::string &value) const override;
				void getProperty(const char *name, bool &value) const override;

				std::string getVersion() const override;
				std::string getRevision() const override;

				// Gets
				std::string	toString(int baddr, size_t len, char lf) const override;
				std::string	toString(int row, int col, size_t sz, char lf) const override;

				ProgramMessage getProgramMessage() const override;

				ConnectionState getConnectionState() const override;

				void setCursorPosition(unsigned short addr);
				void setCursorPosition(unsigned short row, unsigned short col);

				/// @brief Set field at current posicion, jumps to next writable field.
				TN3270::Session & push(const char *text) override;

				TN3270::Session & push(int baddr, const std::string &text) override;
				TN3270::Session & push(int row, int col, const std::string &text) override;
				TN3270::Session & push(const PFKey key) override;
				TN3270::Session & push(const PAKey key) override;
				TN3270::Session & push(const Action action) override;

				// Get contents.
				TN3270::Session & pop(int baddr, std::string &text) override;
				TN3270::Session & pop(int row, int col, std::string &text) override;
				TN3270::Session & pop(std::string &text) override;

			};

		}

		/// @brief IPC Based acess (Access and active instance of pw3270 or pw3270d)
		namespace IPC {

			class Session;

			/// @brief PW3270 IPC Request/Response.
			class Request {
			private:

#ifdef _WIN32
				#pragma pack(1)
				struct DataBlock {
					uint8_t	  type;
				};
				#pragma pack()

				std::vector<DataBlock *> input;

				std::vector<DataBlock *> output;

				/// @brief Create DataBlock
				static DataBlock * createDataBlock(const void *ptr, size_t len);

				/// @brief Descompacta argumentos recebidos.
				static void unpack(std::vector<DataBlock *> &args, const uint8_t * buffer, size_t szBuffer);

				/// @brief Compacta array de argumentos em um bloco de dados.
				static DWORD pack(std::vector<DataBlock *> &args, uint8_t * outBuffer, size_t szBuffer);
#else
				struct {
					DBusMessage		* in;
					DBusMessage		* out;
					DBusMessageIter	  iter;

				} msg;
				DBusConnection	* conn;

#endif // _WIN32

				Request(const Session &session);

			public:

				/// @brief Create a method call.
				Request(const Session &session, const char *method);

				/// @brief Create a get/set property call.
				///
				/// @param session	Session object.
				/// @param isSet	true if this is a setProperty call.
				/// @param property	Property name.
				//
				Request(const Session &session, bool isSet, const char *property);

				~Request();

				Request & call();

				// Push values

				Request & push(const char *arg);

				// Pop values
				Request & pop(std::string &value);
				Request & pop(int &value);

			};

			class TN3270_PRIVATE Session : public TN3270::Abstract::Session {
			private:

				friend class Request;

#ifdef _WIN32
				/// @brief Pipe Handle.
				HANDLE hPipe;
#else

				DBusConnection	* conn;
				std::string		  name;			///< @brief D-Bus Object name.
				std::string		  path;			///< @brief D-Bus Object path.
				std::string		  interface;	///< @brief D-Bus interface.

#endif // _WIN32

				void call(Request &request);

			public:

				Session(const char *id);
				virtual ~Session();

				// Connect/disconnect
				void connect(const char *url) override;
				void disconnect() override;

				// Wait for session state.
				void waitForReady(time_t timeout = 5)  throw() override;

				// Get properties.
				void getProperty(const char *name, int &value) const override;
				void getProperty(const char *name, std::string &value) const override;
				void getProperty(const char *name, bool &value) const override;

				std::string getVersion() const override;
				std::string getRevision() const override;

				// Gets
				std::string	toString(int baddr, size_t len, char lf) const override;
				std::string	toString(int row, int col, size_t sz, char lf) const override;

				ProgramMessage getProgramMessage() const override;

				ConnectionState getConnectionState() const override;

				void setCursorPosition(unsigned short addr);
				void setCursorPosition(unsigned short row, unsigned short col);

				/// @brief Set field at current posicion, jumps to next writable field.
				TN3270::Session & push(const char *text) override;

				TN3270::Session & push(int baddr, const std::string &text) override;
				TN3270::Session & push(int row, int col, const std::string &text) override;
				TN3270::Session & push(const PFKey key) override;
				TN3270::Session & push(const PAKey key) override;
				TN3270::Session & push(const Action action) override;

				// Get contents.
				TN3270::Session & pop(int baddr, std::string &text) override;
				TN3270::Session & pop(int row, int col, std::string &text) override;
				TN3270::Session & pop(std::string &text) override;

			};

		}

	}

#endif // PRIVATE_H_INCLUDED
