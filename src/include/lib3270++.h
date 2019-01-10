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

#ifndef LIB3270_HPP_INCLUDED

	#define LIB3270_HPP_INCLUDED 1

	#include <iostream>
	#include <cstdarg>
	#include <vector>
	#include <functional>
	#include <lib3270.h>

	#if defined(_WIN32)

		#define TN3270_PUBLIC	__declspec (dllexport)
		#define TN3270_PRIVATE

	#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x550)

		#define TN3270_PUBLIC
		#define TN3270_PRIVATE

	#elif defined(__GNUC__)

		#define TN3270_PUBLIC	__attribute__((visibility("default")))
		#define TN3270_PRIVATE	__attribute__((visibility("hidden")))

	#else

		#error Unable to set visibility attribute

	#endif

#ifdef __cplusplus

	#include <string>

	namespace TN3270 {

		class Host;
		class Controller;

		#define DEFAULT_TIMEOUT 5

		class TN3270_PUBLIC Event {
		public:
			enum Type : uint8_t {
				All,			///< @brief All events (undefined).
				Popup,			///< @brief Popup message.
				Trace,			///< @brief Trace message.
				Message,		///< @brief Generic message.
				Connection		///< @brief Connect/Disconnect event.
			};

		private:
			Type type;

		protected:
			Event(enum Type type);

		public:
			virtual ~Event();

			/// @brief Check event type
			inline bool is(Event::Type type) const noexcept {
				return this->type == type;
			}

			/// @brief Check event type
			inline bool operator==(Event::Type type) const noexcept {
				return this->type == type;
			}

			inline operator Event::Type() const noexcept {
				return this->type;
			}

			/// @brief Get event description.
			virtual std::string toString() const = 0;

		};

		enum ProgramMessage : uint8_t {
			MESSAGE_NONE			= LIB3270_MESSAGE_NONE,				///< @brief No message
			MESSAGE_SYSWAIT			= LIB3270_MESSAGE_SYSWAIT,			///< @brief --
			MESSAGE_TWAIT			= LIB3270_MESSAGE_TWAIT,			///< @brief --
			MESSAGE_CONNECTED		= LIB3270_MESSAGE_CONNECTED,		///< @brief Connected
			MESSAGE_DISCONNECTED	= LIB3270_MESSAGE_DISCONNECTED,		///< @brief Disconnected from host
			MESSAGE_AWAITING_FIRST	= LIB3270_MESSAGE_AWAITING_FIRST,	///< @brief --
			MESSAGE_MINUS			= LIB3270_MESSAGE_MINUS,			///< @brief --
			MESSAGE_PROTECTED		= LIB3270_MESSAGE_PROTECTED,		///< @brief --
			MESSAGE_NUMERIC			= LIB3270_MESSAGE_NUMERIC,			///< @brief --
			MESSAGE_OVERFLOW		= LIB3270_MESSAGE_OVERFLOW,			///< @brief --
			MESSAGE_INHIBIT			= LIB3270_MESSAGE_INHIBIT,			///< @brief --
			MESSAGE_KYBDLOCK		= LIB3270_MESSAGE_KYBDLOCK,			///< @brief Keyboard is locked

			MESSAGE_X				= LIB3270_MESSAGE_X,				///< @brief --
			MESSAGE_RESOLVING		= LIB3270_MESSAGE_RESOLVING,		///< @brief Resolving hostname (running DNS query)
			MESSAGE_CONNECTING		= LIB3270_MESSAGE_CONNECTING		///< @brief Connecting to host

		};

		/// @brief connection state.
		enum ConnectionState : uint8_t {
			DISCONNECTED		= LIB3270_NOT_CONNECTED,				///< @brief disconnected
			RESOLVING			= LIB3270_RESOLVING,					///< @brief resolving hostname
			PENDING				= LIB3270_PENDING,						///< @brief connection pending
			CONNECTED_INITIAL	= LIB3270_CONNECTED_INITIAL,			///< @brief connected, no mode yet
			CONNECTED_ANSI		= LIB3270_CONNECTED_ANSI,				///< @brief connected in NVT ANSI mode
			CONNECTED_3270		= LIB3270_CONNECTED_3270,				///< @brief connected in old-style 3270 mode
			CONNECTED_INITIAL_E	= LIB3270_CONNECTED_INITIAL_E,			///< @brief connected in TN3270E mode, unnegotiated
			CONNECTED_NVT		= LIB3270_CONNECTED_NVT,				///< @brief connected in TN3270E mode, NVT mode
			CONNECTED_SSCP		= LIB3270_CONNECTED_SSCP,				///< @brief connected in TN3270E mode, SSCP-LU mode
			CONNECTED_TN3270E	= LIB3270_CONNECTED_TN3270E,			///< @brief connected in TN3270E mode, 3270 mode
		};

		/// @brief PF Keys
		enum PFKey : uint8_t {
			PF_1,
			PF_2,
			PF_3,
			PF_4,
			PF_5,
			PF_6,
			PF_7,
			PF_8,
			PF_9,
			PF_10,
			PF_11,
			PF_12
		};

		/// @brief PF Keys
		enum PAKey : uint8_t {
			PA_1,
			PA_2,
			PA_3
		};

		/// @brief Actions keys
		enum Action : uint8_t {
			ENTER,		///< Enter key
			ERASE,
			ERASE_EOF,
			ERASE_EOL,
			ERASE_INPUT
		};

		/// @brief TN3270 Session.
		class TN3270_PUBLIC Session {
		protected:
			Session();

			/// @brief Write information to log file.
			void info(const char *fmt, ...) const;

			/// @brief Write warning to log file.
			void warning(const char *fmt, ...) const;

			/// @brief Write error to log file.
			void error(const char *fmt, ...) const;

			/// @brief Fire event.
			void fire(const Event &event);

		public:

			/// @brief Create a tn3270 session.
			static Session * create(const char *id = nullptr);

			virtual ~Session();

			// Connect/disconnect
			virtual void connect(const char *url) = 0;
			virtual void disconnect() = 0;

			// Wait for session state.
			virtual void waitForReady(time_t timeout = DEFAULT_TIMEOUT) throw() = 0;

			// Gets
			virtual std::string	toString(int baddr = 0, size_t len = -1, char lf = '\n') const = 0;
			virtual std::string	toString(int row, int col, size_t sz, char lf = '\n') const = 0;

			inline operator std::string() const {
				return toString();
			}

			// Get properties.
			virtual void getProperty(const char *name, int &value) const = 0;
			virtual void getProperty(const char *name, std::string &value) const = 0;

			virtual std::string getVersion() const = 0;
			virtual std::string getRevision() const = 0;

			virtual ProgramMessage getProgramMessage() const = 0;
			inline operator ProgramMessage() const {
				return getProgramMessage();
			}

			virtual ConnectionState getConnectionState() const = 0;
			inline operator ConnectionState() const {
				return getConnectionState();
			}

			inline bool operator==(ConnectionState state) const noexcept {
				return this->getConnectionState() == state;
			}

			// Set contents.

			/// @brief Set field at current posicion, jumps to next writable field.
			virtual Session & push(const char *text) = 0;
			inline Session & push(const std::string &text) {
				return push(text.c_str());
			}

			/// @brief Set cursor address.
			virtual void setCursorPosition(unsigned short addr) = 0;

			/// @brief Set cursor position.
			virtual void setCursorPosition(unsigned short row, unsigned short col) = 0;

			virtual Session & push(int baddr, const std::string &text) = 0;
			virtual Session & push(int row, int col, const std::string &text) = 0;
			virtual Session & push(const PFKey key) = 0;
			virtual Session & push(const PAKey key) = 0;
			virtual Session & push(const Action action) = 0;

			// Get contents.
			virtual Session & pop(int baddr, std::string &text) = 0;
			virtual Session & pop(int row, int col, std::string &text) = 0;
			virtual Session & pop(std::string &text) = 0;

			/// @brief Insert event listener.
			void insert(Event::Type type, std::function <void(const Event &event)> listener);

		};

		/// @brief TN3270 Host
		class TN3270_PUBLIC Host : public std::basic_streambuf<char, std::char_traits<char> > {
		private:

			/// @brief Connection with the host
			Session *session;

			/// @brief How much seconds we wait for the terminal to be ready?
			time_t timeout;

		protected:

			/// @brief Writes characters to the associated file from the put area
			int sync() override;

			/// @brief Writes characters to the associated output sequence from the put area.
			int overflow(int c) override;

			/// @brief Write information to log file.
			void info(const char *fmt, ...) const;

			/// @brief Write warning to log file.
			void warning(const char *fmt, ...) const;

			/// @brief Write error to log file.
			void error(const char *fmt, ...) const;

		public:
			Host(const char *id = nullptr, const char *url = nullptr, time_t timeout = DEFAULT_TIMEOUT);
			~Host();

			inline bool operator==(ConnectionState state) const noexcept {
				return session->getConnectionState() == state;
			}

			void connect(const char *url, bool sync = true);

			inline ProgramMessage getProgramMessage() const {
				return session->getProgramMessage();
			}

			inline operator bool() const {
				return isConnected() && isReady();
			}

			inline operator ProgramMessage() const {
				return getProgramMessage();
			}

			inline ConnectionState getConnectionState() const {
				return session->getConnectionState();
			}

			bool isReady() const;
			bool isConnected() const;

			inline operator ConnectionState() const {
				return getConnectionState();
			}

			/// @brief Set cursor address.
			inline void setCursorPosition(unsigned short addr) {
				session->setCursorPosition(addr);
			}

			/// @brief Set cursor position.
			inline void setCursorPosition(unsigned short row, unsigned short col) {
				session->setCursorPosition(row,col);
			}

			// Get properties

			/// @brief Get lib3270 version.
			inline std::string getVersion() const {
				return session->getVersion();
			}

			/// @brief Get lib3270 revision.
			std::string getRevision() const {
				return session->getRevision();
			}

			// Set contents.

			/// @brief Set field at current posicion, jumps to next writable field.
			inline Host & push(const char *text) {
				session->push(text);
				return *this;
			};

			inline Host & push(const std::string &text) {
				session->push(text);
				return *this;

			}

			inline Host & push(int baddr, const std::string &text) {
				session->push(baddr,text);
				return *this;
			}

			inline Host & push(int row, int col, const std::string &text) {
				session->push(row,col,text);
				return *this;
			}

			inline Host & push(const PFKey key) {
				session->push(key);
				return *this;
			}

			inline Host & push(const PAKey key) {
				session->push(key);
				return *this;
			}

			Host & push(const Action action);

			// Get contents.

			Host & pop(int baddr, std::string &text);
			Host & pop(int row, int col, std::string &text);
			Host & pop(std::string &text);

			std::string toString() const;
			std::string toString(int baddr, size_t len = -1, char lf = '\n') const;
			std::string toString(int row, int col, size_t sz, char lf = '\n') const;

			// Event listeners
			inline Host & insert(Event::Type type, std::function <void(const Event &event)> listener) noexcept {
				session->insert(type, listener);
				return *this;
			}


		};

	}

	TN3270_PUBLIC const char * toCharString(const TN3270::ProgramMessage programMessage) noexcept;
	TN3270_PUBLIC const char * toCharString(const TN3270::ConnectionState connectionState) noexcept;

	template <typename T>
	inline TN3270_PUBLIC TN3270::Session & operator<<(TN3270::Session& session, const T value) {
		return session.push(value);
	}

	template <typename T>
	inline TN3270_PUBLIC TN3270::Session & operator>>(TN3270::Session& session, const T value) {
		return session.pop(value);
	}

	template <typename T>
	inline TN3270_PUBLIC TN3270::Host & operator<<(TN3270::Host& host, const T value) {
		return host.push(value);
	}

	inline std::ostream & operator<<(std::ostream &stream, const TN3270::Host& host) {
        stream << host.toString();
        return stream;
	}


#endif

#endif // LIB3270_H_INCLUDED
