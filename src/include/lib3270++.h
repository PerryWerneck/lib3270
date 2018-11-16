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
 * licinio@bb.com.br		(Licínio Luis Branco)
 * kraucer@bb.com.br		(Kraucer Fernandes Mazuco)
 *
 */

#ifndef LIB3270_HPP_INCLUDED

	#define LIB3270_HPP_INCLUDED 1

	#include <iostream>
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
			MESSAGE_CONNECTING,		= LIB3270_MESSAGE_CONNECTING		///< @brief Connecting to host

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
			ENTER		///< Enter key
		};

		/// @brief TN3270 Session.
		class TN3270_PUBLIC Session {
		protected:
			Session();

		public:

			/// @brief Create a tn3270 session.
			static Session * create(const char *id = nullptr);

			virtual ~Session();

			// Connect/disconnect
			virtual void connect(const char *url) = 0;
			virtual void disconnect();

			// Gets
			virtual std::string	toString(int baddr = 0, size_t len = -1, bool lf = false) = 0;
			virtual std::string	toString(int row, int col, size_t sz, bool lf = false) = 0;

			/// @brief Get field at current position, update to next one.
			virtual Session & pop(std::string &value) = 0;

			inline operator std::string() const {
				return toString();
			}

			virtual ProgramMessage getProgramMessage() const = 0;
			inline operator ProgramMessage() const {
				return getProgramMessage();
			}

			virtual ConnectionState getConnectionState() const = 0;
			inline operator ConnectionState() const {
				return getConnectionState();
			}

			// Sets

			/// @brief Set field at current posicion, jumps to next writable field.
			virtual Session & push(const char *text) = 0;
			inline Session & push(const std::string &text) {
				return push(text.c_str());
			}

			virtual Session & push(int baddr, const std::string &text) = 0;
			virtual Session & push(int row, int col, const std::string &text) = 0;
			virtual Session & push(const PFKey key) = 0;
			virtual Session & push(const PAKey key) = 0;
			virtual Session & push(const Action action) = 0;

		};

		/// @brief TN3270 Host
		class TN3270_PUBLIC Host : public std::basic_streambuf<char, std::char_traits<char> > {
		private:

			/// @brief Connection with the host
			Session *session;

		protected:

			/// @brief Writes characters to the associated file from the put area
			int sync() override;

			/// @brief Writes characters to the associated output sequence from the put area.
			int overflow(int c) override;

		public:
			Host(const char *id = nullptr, const char *url = nullptr);
			~Host();

			inline ProgramMessage getProgramMessage() const {
				return session->getProgramMessage();
			}

			inline operator ProgramMessage() const {
				return getProgramMessage();
			}

			inline ConnectionState getConnectionState() const {
				return session->getConnectionState();
			}

			inline operator ConnectionState() const {
				return getConnectionState();
			}


			// Sets

			/// @brief Set field at current posicion, jumps to next writable field.
			inline Host & push(const char *text) {
				session->push(text);
				return *this;
			};

			inline Host & push(const std::string &text) {
				session->push(text));
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

			inline Host & push(const Action action) {
				session->push(action);
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

	template <typename T>
	inline TN3270_PUBLIC TN3270::Host & operator>>(TN3270::Host& host, const T value) {
		return Host.pop(value);
	}

#endif

#endif // LIB3270_H_INCLUDED
