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
 * Este programa está nomeado como lib3270.h e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 * licinio@bb.com.br		(Licínio Luis Branco)
 * kraucer@bb.com.br		(Kraucer Fernandes Mazuco)
 *
 */

 /**
  * @brief TN3270 API definitions.
  *
  * @author perry.werneck@gmail.com
  *
  */

#ifndef LIB3270_H_INCLUDED

	#define LIB3270_H_INCLUDED 1

	#include <stdarg.h>
	#include <errno.h>

	#ifdef _WIN32
		#include <winsock2.h>
		#include <windows.h>
	#endif // _WIN32

	#ifndef ENOTCONN
		#define ENOTCONN 126
	#endif // !ENOTCONN

	#if defined (__GNUC__) || defined (__clang__)

		#define LIB3270_AUTOPTR_FUNC_NAME(TypeName) lib3270_autoptr_cleanup_##TypeName
		#define lib3270_autoptr(TypeName) TypeName * __attribute__ ((__cleanup__(LIB3270_AUTOPTR_FUNC_NAME(TypeName))))

	#endif // __GNUC__

	#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1) || defined (__clang__)

		#define LIB3270_DEPRECATED(func) func __attribute__ ((deprecated))

	#elif defined(_WIN32) && !defined(_MSC_VER)

		#define LIB3270_DEPRECATED(func) __declspec(deprecated) func

	#else

		#define LIB3270_DEPRECATED(func) func

	#endif /* __GNUC__ */

	#if defined(__GNUC__)

		#define LIB3270_GNUC_FORMAT(s,f) __attribute__ ((__format__ (__printf__, s, f)))
		#define LIB3270_GNUC_NULL_TERMINATED __attribute__((__sentinel__))

	#elif defined(_WIN32) && !defined(_MSC_VER)

		#define LIB3270_GNUC_NULL_TERMINATED __attribute__((__sentinel__))
		#define LIB3270_GNUC_FORMAT(s,f)

	#else

		#define LIB3270_GNUC_NULL_TERMINATED
		#define LIB3270_GNUC_FORMAT(s,f)

	#endif

	#define LIB3270_STRINGIZE(x) #x
	#define LIB3270_STRINGIZE_VALUE_OF(x) LIB3270_STRINGIZE(x)

	/**
	 * @brief BIND definitions.
	 */
	#define LIB3270_BIND_RU					0x31
	#define LIB3270_BIND_OFF_PLU_NAME_LEN	26
	#define LIB3270_BIND_OFF_PLU_NAME		27
	#define LIB3270_BIND_PLU_NAME_MAX		8

	/**
	 * @brief Maximum size for LUNAME
	 *
	 */
	#define LIB3270_LUNAME_LENGTH			16

	/**
	 * @brief Selection mode.
	 *
	 */
	typedef enum _lib3270_content_option
	{
		LIB3270_CONTENT_ALL,			///< @brief Get all the terminal data.
		LIB3270_CONTENT_SELECTED,		///< @brief Get only selected contents.
		LIB3270_CONTENT_COPY			///< @brief Get internal copy.
	} LIB3270_CONTENT_OPTION;

	/**
	 * @brief Character attributes.
	 */
	typedef enum _lib3270_attr
	{
		LIB3270_ATTR_COLOR_BACKGROUND		= 0x0000,

		LIB3270_ATTR_COLOR_BLUE				= 0x0001,
		LIB3270_ATTR_COLOR_RED				= 0x0002,
		LIB3270_ATTR_COLOR_PINK				= 0x0003,
		LIB3270_ATTR_COLOR_GREEN			= 0x0004,
		LIB3270_ATTR_COLOR_TURQUOISE		= 0x0005,
		LIB3270_ATTR_COLOR_YELLOW			= 0x0006,
		LIB3270_ATTR_COLOR_WHITE			= 0x0007,
		LIB3270_ATTR_COLOR_BLACK			= 0x0008,
		LIB3270_ATTR_COLOR_DARK_BLUE		= 0x0009,
		LIB3270_ATTR_COLOR_ORANGE			= 0x000A,
		LIB3270_ATTR_COLOR_PURPLE			= 0x000B,
		LIB3270_ATTR_COLOR_DARK_GREEN		= 0x000C,
		LIB3270_ATTR_COLOR_DARK_TURQUOISE	= 0x000D,
		LIB3270_ATTR_COLOR_MUSTARD			= 0x000E,
		LIB3270_ATTR_COLOR_GRAY				= 0x000F,

		LIB3270_ATTR_COLOR					= 0x00FF,

		LIB3270_ATTR_FIELD					= 0x0100,
		LIB3270_ATTR_BLINK					= 0x0200,
		LIB3270_ATTR_UNDERLINE				= 0x0400,
		LIB3270_ATTR_INTENSIFY				= 0x0800,

		LIB3270_ATTR_CG						= 0x1000,
		LIB3270_ATTR_MARKER					= 0x2000,
		LIB3270_ATTR_BACKGROUND_INTENSITY	= 0x4000,
		LIB3270_ATTR_SELECTED				= 0x8000

	} LIB3270_ATTR;

	typedef enum _lib3270_direction
	{
		LIB3270_DIR_UP,
		LIB3270_DIR_DOWN,
		LIB3270_DIR_LEFT,
		LIB3270_DIR_RIGHT,

		LIB3270_DIR_END,

		LIB3270_DIR_COUNT						/**< @brief Nº máximo de direções. */

	} LIB3270_DIRECTION;

	/**
	 * @brief OIA Status indicators.
	 *
	 */
	typedef enum _lib3270_flag
	{
		LIB3270_FLAG_BOXSOLID,	/**< @brief System available */
		LIB3270_FLAG_UNDERA,	/**< @brief Control Unit STATUS */
		LIB3270_FLAG_TYPEAHEAD,
		LIB3270_FLAG_PRINTER,	/**< @brief Printer session status */
		LIB3270_FLAG_REVERSE,
		LIB3270_FLAG_SCRIPT,	/**< @brief Script status */

		LIB3270_FLAG_COUNT

	} LIB3270_FLAG;


	/**
	 * @brief 3270 program messages.
	 *
	 */
	typedef enum _LIB3270_MESSAGE
	{
		LIB3270_MESSAGE_NONE,				///< @brief No message
		LIB3270_MESSAGE_SYSWAIT,			///< @brief --
		LIB3270_MESSAGE_TWAIT,				///< @brief --
		LIB3270_MESSAGE_CONNECTED,			///< @brief Connected
		LIB3270_MESSAGE_DISCONNECTED,		///< @brief Disconnected from host
		LIB3270_MESSAGE_AWAITING_FIRST,		///< @brief --
		LIB3270_MESSAGE_MINUS,				///< @brief --
		LIB3270_MESSAGE_PROTECTED,			///< @brief --
		LIB3270_MESSAGE_NUMERIC,			///< @brief --
		LIB3270_MESSAGE_OVERFLOW,			///< @brief --
		LIB3270_MESSAGE_INHIBIT,			///< @brief --
		LIB3270_MESSAGE_KYBDLOCK,			///< @brief Keyboard is locked

		LIB3270_MESSAGE_X,					///< @brief --
		LIB3270_MESSAGE_RESOLVING,			///< @brief Resolving hostname (running DNS query)
		LIB3270_MESSAGE_CONNECTING,			///< @brief Connecting to host

		LIB3270_MESSAGE_USER

	} LIB3270_MESSAGE;


	/**
	 * @brief Pointer modes.
	 *
	 * Pointer modes set by library; an application can use it
	 * as a hint to change the mouse pointer based on connection status.
	 *
	 */
	typedef enum _LIB3270_POINTER
	{
		LIB3270_POINTER_UNLOCKED,				///< @brief Ready for user actions
		LIB3270_POINTER_WAITING,				///< @brief Waiting for host
		LIB3270_POINTER_LOCKED,					///< @brief Locked, can't receive user actions

		LIB3270_POINTER_PROTECTED,
		LIB3270_POINTER_MOVE_SELECTION,
		LIB3270_POINTER_SELECTION_TOP_LEFT,
		LIB3270_POINTER_SELECTION_TOP_RIGHT,
		LIB3270_POINTER_SELECTION_TOP,
		LIB3270_POINTER_SELECTION_BOTTOM_LEFT,
		LIB3270_POINTER_SELECTION_BOTTOM_RIGHT,
		LIB3270_POINTER_SELECTION_BOTTOM,
		LIB3270_POINTER_SELECTION_LEFT,
		LIB3270_POINTER_SELECTION_RIGHT,
		LIB3270_POINTER_QUESTION,

		LIB3270_POINTER_COUNT

	} LIB3270_POINTER;


	/**
	 * @brief Connection state
	 */
	typedef enum lib3270_cstate
	{
		LIB3270_NOT_CONNECTED,			///< @brief no socket, disconnected
		LIB3270_RESOLVING,				///< @brief resolving hostname
		LIB3270_PENDING,				///< @brief connection pending
		LIB3270_CONNECTED_INITIAL,		///< @brief connected, no mode yet
		LIB3270_CONNECTED_ANSI,			///< @brief connected in NVT ANSI mode
		LIB3270_CONNECTED_3270,			///< @brief connected in old-style 3270 mode
		LIB3270_CONNECTED_INITIAL_E,	///< @brief connected in TN3270E mode, unnegotiated
		LIB3270_CONNECTED_NVT,			///< @brief connected in TN3270E mode, NVT mode
		LIB3270_CONNECTED_SSCP,			///< @brief connected in TN3270E mode, SSCP-LU mode
		LIB3270_CONNECTED_TN3270E		///< @brief connected in TN3270E mode, 3270 mode
	} LIB3270_CSTATE;

	/**
	 * @brief Field attributes.
	 */
	 typedef enum lib3270_field_attribute
	 {
		LIB3270_FIELD_ATTRIBUTE_INVALID			= 0x00,	///< @brief Attribute is invalid.
		LIB3270_FIELD_ATTRIBUTE_PRINTABLE		= 0xc0,	///< @brief these make the character "printable"
		LIB3270_FIELD_ATTRIBUTE_PROTECT			= 0x20,	///< @brief unprotected (0) / protected (1)
		LIB3270_FIELD_ATTRIBUTE_NUMERIC			= 0x10,	///< @brief alphanumeric (0) / numeric (1)
		LIB3270_FIELD_ATTRIBUTE_INTENSITY		= 0x0c,	///< @brief display/selector pen detectable:
		LIB3270_FIELD_ATTRIBUTE_INT_NORM_NSEL	= 0x00,	///< @brief 00 normal, non-detect
		LIB3270_FIELD_ATTRIBUTE_INT_NORM_SEL	= 0x04,	///< @brief 01 normal, detectable
		LIB3270_FIELD_ATTRIBUTE_INT_HIGH_SEL	= 0x08,	///< @brief 10 intensified, detectable
		LIB3270_FIELD_ATTRIBUTE_INT_ZERO_NSEL	= 0x0c,	///< @brief 11 nondisplay, non-detect
		LIB3270_FIELD_ATTRIBUTE_RESERVED		= 0x02,	///< @brief must be 0
		LIB3270_FIELD_ATTRIBUTE_MODIFIED		= 0x01	///< @brief modified (1)
	} LIB3270_FIELD_ATTRIBUTE;

	/**
	 * @brief Host types.
	 *
	 */
	typedef enum lib3270_host_type
	{
		LIB3270_HOST_AS400		= 0x0001,	///< @brief AS400 host - Prefix every PF with PA1
		LIB3270_HOST_TSO		= 0x0002,	///< @brief Host is TSO
		LIB3270_HOST_S390		= 0x0006,	///< @brief Host is S390 (TSO included)

	} LIB3270_HOST_TYPE;

	#define LIB3270_HOSTTYPE_DEFAULT LIB3270_HOST_S390

	typedef struct _LIB3270_HOST_TYPE_entry
	{
		LIB3270_HOST_TYPE	  type;
		const char			* name;
		const char			* description;
		const char			* tooltip;
	} LIB3270_HOST_TYPE_ENTRY;

	/**
	 * @brief SSL state
	 *
	 */
	typedef enum lib3270_ssl_state
	{
		LIB3270_SSL_UNSECURE,			/**< @brief No secure connection */
		LIB3270_SSL_SECURE,				/**< @brief Connection secure with CA check */
		LIB3270_SSL_NEGOTIATED,			/**< @brief Connection secure, no CA, self-signed or expired CRL */
		LIB3270_SSL_NEGOTIATING,		/**< @brief Negotiating SSL */
		LIB3270_SSL_UNDEFINED			/**< @brief Undefined */
	} LIB3270_SSL_STATE;

	/**
	 * @brief Field information.
	 *
	 */
	typedef struct _lib3270_field  {

		unsigned short	baddr;				/**< @brief Address of the field. */
		unsigned short	length;				/**< @brief Field length */
		unsigned char	attribute;			/**< @brief Field attribute */

		struct {
			unsigned char foreground;		/**< @brief foreground color (0x00 or 0xf) */
			unsigned char bacground;		/**< @brief background color (0x00 or 0xf) */
		} color;

	} LIB3270_FIELD;

	#define LIB3270_SSL_FAILED LIB3270_SSL_UNSECURE

#ifdef __cplusplus
	extern "C" {
#endif

	#if defined( ANDROID )

		#define LIB3270_INTERNAL	extern __attribute__((visibility("hidden")))
		#define LIB3270_EXPORT		extern __attribute__((visibility("hidden")))

	#elif defined(_WIN32) || defined(_MSC_VER)

		#define LIB3270_INTERNAL	extern
		#define LIB3270_EXPORT		extern __declspec (dllexport)

	#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x550)

		#define LIB3270_INTERNAL	__hidden extern
		#define LIB3270_EXPORT		extern

	#elif defined(__GNUC__) || defined(HAVE_GNUC_VISIBILITY)

		#define LIB3270_INTERNAL	__attribute__((visibility("hidden"))) extern
		#define LIB3270_EXPORT		__attribute__((visibility("default"))) extern

	#else

		#define LIB3270_INTERNAL
		#define LIB3270_EXPORT

	#endif

	/// @brief Action/property groups.
	typedef enum _lib3270_action_group
	{
		LIB3270_ACTION_GROUP_NONE,					///< @brief Simple action, no signals os special treatment.
		LIB3270_ACTION_GROUP_ONLINE,				///< @brief Action requires online state.
		LIB3270_ACTION_GROUP_OFFLINE,				///< @brief Action requires offline state.
		LIB3270_ACTION_GROUP_SELECTION,				///< @brief Action depends on selection.
		LIB3270_ACTION_GROUP_LOCK_STATE,			///< @brief Action depends on keyboard lock state.
		LIB3270_ACTION_GROUP_FORMATTED,				///< @brief Action depends on a formatted screen.
		LIB3270_ACTION_GROUP_COPY,					///< @brief Action depends on stored string.

		LIB3270_ACTION_GROUP_CUSTOM					///< @brief Custom group/Number of groups.
	} LIB3270_ACTION_GROUP;


	/**
	 * @brief Head for property descriptors.
	 *
	 */
	#define LIB3270_PROPERTY_HEAD	\
		const char * name; \
		LIB3270_ACTION_GROUP group; \
		const char * label; \
		const char * icon; \
		const char * summary; \
		const char * description;

	typedef struct _lib3270_property {
		LIB3270_PROPERTY_HEAD
	} LIB3270_PROPERTY;

	/**
	 * @brief State change IDs.
	 *
	 */
	typedef enum _lib3270_state
	{
		LIB3270_STATE_RESOLVING,			///< @brief Resolving DNS.
		LIB3270_STATE_CONNECTING,			///< @brief Connecting to host.
		LIB3270_STATE_HALF_CONNECT,
		LIB3270_STATE_CONNECT,
		LIB3270_STATE_3270_MODE,
		LIB3270_STATE_LINE_MODE,
		LIB3270_STATE_REMODEL,
		LIB3270_STATE_PRINTER,
		LIB3270_STATE_EXITING,
		LIB3270_STATE_CHARSET,

		LIB3270_STATE_USER				// Always the last one
	} LIB3270_STATE;

	typedef struct _h3270	H3270;
	typedef struct _h3270ft	H3270FT;

	/**
	 * Get current screen size.
	 *
	 * Get the size of the terminal in rows/cols; this value can differ from
	 * the model if there's an active "altscreen" with diferent size.
	 *
	 * @param h	Handle of the desired session.
	 * @param r Pointer to screen rows.
	 * @param c Pointer to screen columns.
	 *
	 */
	LIB3270_EXPORT void lib3270_get_screen_size(const H3270 *h, unsigned int *r, unsigned int *c);

	/**
	 * Get current screen width in columns.
	 *
	 * @param h	Handle of the desired session.
	 *
	 * @return screen width.
	 *
	 */
	LIB3270_EXPORT unsigned int lib3270_get_width(const H3270 *h);

	LIB3270_EXPORT unsigned int lib3270_get_max_width(const H3270 *h);

	/**
	 * Get current screen width in rows.
	 *
	 * @param h	Handle of the desired session.
	 *
	 * @return screen rows.
	 *
	 */
	LIB3270_EXPORT unsigned int lib3270_get_height(const H3270 *h);

	LIB3270_EXPORT unsigned int lib3270_get_max_height(const H3270 *h);

	LIB3270_EXPORT unsigned int lib3270_get_length(const H3270 *h);

	/**
	 * @brief Creates an empty TN3270 session.
	 *
	 * @param model	Terminal model.
	 *
	 * @return Handle of the new session (release it with lib3270_session_free to avoid memory leaks).
	 *
	 */
	LIB3270_EXPORT H3270 * lib3270_session_new(const char *model);

	/**
	 * @brief Closes a TN3270 session releasing resources.
	 *
	 * @param h handle of the session to close.
	 *
	 */
	LIB3270_EXPORT void lib3270_session_free(H3270 *h);

	/**
	 * @brief Register a function interested in a state change.
	 *
	 * @param hSession	Session handle.
	 * @param tx		State ID
	 * @param func		Callback
	 * @param data		Data
	 *
	 * @return State change identifier.
	 *
	 */
	LIB3270_EXPORT const void * lib3270_register_schange(H3270 *hSession, LIB3270_STATE tx, void (*func)(H3270 *, int, void *),void *data);

	/**
	 * @brief Unregister a function interested in a state change.
	 *
	 * @param hSession	Session handle.
	 * @param id		State change identifier.
	 *
	 * @return 0 if suceeds, non zero if fails (sets errno).
	 *
	 */
	LIB3270_EXPORT int lib3270_unregister_schange(H3270 *hSession, LIB3270_STATE tx, const void * id);

	LIB3270_EXPORT void lib3270_reset_callbacks(H3270 *hSession);

	/**
	 * @brief Set host id for the connect/reconnect operations.
	 *
	 * @param h		Session handle.
	 * @param url	URL of host to set in the format tn3270://hostname:service or tn3270s://hostname:service
	 *
	 * @return 0
	 *
	 */
	 LIB3270_EXPORT int lib3270_set_url(H3270 *h, const char *url);

	/**
	 * @brief Get the URL of the predefined tn3270 host.
	 *
	 * @param hSession	Session handle.
	 *
	 * @return URL of the predefined host in the format tn3270://hostname:service or tn3270s://hostname:service
	 */
	 LIB3270_EXPORT const char * lib3270_get_default_host(const H3270 *hSession);

	/**
	 * @brief Set URL for the certificate revocation list.
	 *
	 * @param hSession	Session handle.
	 * @param crl		URL for the certificate revocation list.
	 *
	 * @return 0 on sucess, non zero on error (sets errno).
	 *
	 */
	 LIB3270_EXPORT int lib3270_crl_set_url(H3270 *hSession, const char *crl);
	 LIB3270_EXPORT const char * lib3270_crl_get_url(const H3270 *hSession);

	 LIB3270_EXPORT int lib3270_crl_set_preferred_protocol(H3270 *hSession, const char *protocol);
	 LIB3270_EXPORT const char * lib3270_crl_get_preferred_protocol(const H3270 *hSession);

	 /**
	  * @brief Get the available protocols for CRL download.
	  *
	  */
	 LIB3270_EXPORT const char ** lib3270_get_available_crl_protocols(void);

	/**
	 * @brief Get hostname for the connect/reconnect operations.
	 *
	 * @param h		Session handle.
	 *
	 * @return Pointer to host id set (internal data, do not change it)
	 *
	 */
	 LIB3270_EXPORT const char * LIB3270_DEPRECATED(lib3270_get_hostname(const H3270 *h));

	 LIB3270_EXPORT void LIB3270_DEPRECATED(lib3270_set_hostname(H3270 *h, const char *hostname));

	/**
	 * @brief Get SSL host option.
	 *
	 * @return Non zero if the host URL has SSL scheme.
	 *
	 */
	LIB3270_EXPORT int lib3270_get_secure_host(const H3270 *hSession);

	/**
	 * @brief Get security state.
	 *
	 */
	LIB3270_EXPORT LIB3270_SSL_STATE lib3270_get_ssl_state(const H3270 *session);

	LIB3270_EXPORT long 				lib3270_get_SSL_verify_result(const H3270 *session);

	/**
	 * @brief Get security state as text.
	 *
	 */
	LIB3270_EXPORT const char * lib3270_get_ssl_state_message(const H3270 *hSession);

	LIB3270_EXPORT const char * lib3270_get_ssl_state_icon_name(const H3270 *hSession);

	/**
	 * @brief Get security state message.
	 *
	 */
	LIB3270_EXPORT const char * lib3270_get_ssl_state_description(const H3270 *hSession);

	LIB3270_EXPORT char * lib3270_get_ssl_crl_text(const H3270 *hSession);
	LIB3270_EXPORT char * lib3270_get_ssl_peer_certificate_text(const H3270 *hSession);


	/**
	 * @brief Get service or port for the connect/reconnect operations.
	 *
	 * @param h		Session handle.
	 *
	 * @return Pointer to service name (internal data, do not change it)
	 *
	 */
	 LIB3270_EXPORT const char * LIB3270_DEPRECATED(lib3270_get_srvcname(const H3270 *h));

	 LIB3270_EXPORT void LIB3270_DEPRECATED(lib3270_set_srvcname(H3270 *h, const char *srvc));

	/**
	 * @brief Get HOST URL.
	 *
	 * @return TN3270 Connection URL.
	 *
	 */
	 LIB3270_EXPORT const char * lib3270_get_url(const H3270 *hSession);

	/**
	 * @brief Get session options.
	 *
	 * @param h		Session handle.
	 *
	 */
	LIB3270_EXPORT LIB3270_HOST_TYPE lib3270_get_host_type(const H3270 *hSession);

	LIB3270_EXPORT const char * lib3270_get_host_type_name(const H3270 *hSession);

	/**
	 * @brief Get URL of the hostname for the connect/reconnect operations.
	 *
	 * @param h		Session handle.
	 *
	 * @return Pointer to host URL set (internal data, do not change it)
	 *
	 */
    LIB3270_EXPORT const char * LIB3270_DEPRECATED(lib3270_get_host(const H3270 *h));

	/**
	 * @brief Check if the session can reconnect.
	 *
	 * @param hSession			Session handle.
	 *
	 * @return zero if reconnect is unavailable (sets errno), non zero if available.
	 *
	 */
	LIB3270_EXPORT int lib3270_allow_reconnect(const H3270 *hSession);

	/**
	 * @brief Reconnect to host.
	 *
	 * @param hSession	Session handle.
	 * @param seconds	Seconds to wait for connection.
	 *
	 * @return 0 for success, non zero if fails (sets errno).
	 *
	 */
	LIB3270_EXPORT int lib3270_reconnect(H3270 *hSession,int seconds);

	/**
	 * @brief Connect by URL
	 *
	 * @param hSession	Session handle.
	 * @param url		Host URL
	 * @param seconds	Seconds to wait for connection.
	 *
	 * @see lib3270_reconnect
	 *
	 * @return 0 for success, non zero if fails (sets errno).
	 */
	LIB3270_EXPORT int lib3270_connect_url(H3270 *hSession, const char *url, int seconds);

	/**
	 * @brief Disconnect from host.
	 *
	 * @param h		Session handle.
	 *
	 * @return -1 if failed (sets errno).
	 *
	 */
	LIB3270_EXPORT int lib3270_disconnect(H3270 *h);

	/**
	 * @brief Get connection state.
	 *
	 * @param h		Session handle.
	 *
	 * @return Connection state.
	 *
	 */
	LIB3270_EXPORT LIB3270_CSTATE lib3270_get_connection_state(const H3270 *h);

	LIB3270_EXPORT const char * lib3270_connection_state_get_name(const LIB3270_CSTATE cstate);

	/**
	 * @brief Pretend that a sequence of keys was entered at the keyboard.
	 *
	 * "Pasting" means that the sequence came from the clipboard.  Returns are
	 * ignored; newlines mean "move to beginning of next line"; tabs and formfeeds
	 * become spaces.  Backslashes are not special, but ASCII ESC characters are
	 * used to signify 3270 Graphic Escapes.
	 *
	 * "Not pasting" means that the sequence is a login string specified in the
	 * hosts file, or a parameter to the String action.  Returns are "move to
	 * beginning of next line"; newlines mean "Enter AID" and the termination of
	 * processing the string.  Backslashes are processed as in C.
	 *
	 * @param s			String to input.
	 * @param len		Size of the string (or -1 to null terminated strings)
	 * @param pasting	Non zero for pasting (See comments).
	 *
	 * @return The number of unprocessed characters or -1 if failed
	 */
	LIB3270_EXPORT int lib3270_emulate_input(H3270 *session, const char *s, int len, int pasting);

	/**
	 * @brief Converts row/col in a buffer address.
	 *
	 * @param hSession	TN3270 Session.
	 * @param row		Row inside the screen.
	 * @param col		Col inside the screen.
	 *
	 * @return Current address or negative if invalid (sets errno).
	 *
	 * @retval -EOVERFLOW	The coordinates are out of the screen.
	 *
	 */
	LIB3270_EXPORT int lib3270_translate_to_address(const H3270 *hSession, unsigned int row, unsigned int col);

	/**
	 * @brief Set field contents, jump to the next one.
	 *
	 * Set the string inside the corrent field, jump to the next one.
	 *
	 * @param hSession	Session handle.
	 * @param text		String to input.
	 * @param length	Length of the string (-1 for auto-detect).
	 *
	 * @return address of the cursor, negative if failed.
	 *
	 * @retval 0			No next field.
	 * @retval -EPERM		The keyboard is locked.
	 * @retval -ENOTCONN	Disconnected from host.
 	 * @retval -ENODATA		No field at the current cursor position.
 	 * @retval -ENOTSUP		The screen is not formatted.
	 *
	 */
	LIB3270_EXPORT int lib3270_set_field(H3270 *hSession, const char *text, int length);

	/**
	 * @brief Set string at current cursor position.
	 *
	 * Returns are ignored; newlines mean "move to beginning of next line";
	 * tabs and formfeeds become spaces.  Backslashes are not special.
	 *
	 * @param hSession	Session handle.
	 * @param text		String to input.
	 * @param length	Length of the string (-1 for auto-detect).
	 *
	 * @return Negative if error or number (sets errno) of processed characters.
	 *
	 * @retval -EPERM		The keyboard is locked.
	 * @retval -ENOTCONN	Disconnected from host.
	 *
	 */
	LIB3270_EXPORT int lib3270_set_string(H3270 *h, const unsigned char *text, int length);

	/**
	 * @brief Set string at defined row/column.
	 *
	 * Set the string in the defined row/column; returns number of processed caracter if succeeds or negative value if not.
	 *
	 * @param hSession	Session handle.
	 * @param row		Row for the first character.
	 * @param col		Col for the first character.
	 * @param str		String to set.
	 * @param length	Length of the string (-1 for auto-detect).
	 *
	 * @return Negative if error or number (sets errno) of processed characters.
	 *
	 * @retval -EPERM		The keyboard is locked.
	 * @retval -EOVERFLOW	The row or col is bigger than the screen size.
	 * @retval -ENOTCONN	Disconnected from host.
	 *
	 */
	LIB3270_EXPORT int lib3270_set_string_at(H3270 *hSession, unsigned int row, unsigned int col, const unsigned char *str, int length);

	/**
	 * @brief Set string at defined adress.
	 *
	 * @param hSession	Session handle.
	 * @param baddr		Adress for the first character (-1 for cursor position).
	 * @param str		String to set.
	 * @param length	Length of the string (-1 for auto-detect).
	 *
	 * @return Negative if error or number of processed characters.
	 *
	 * @retval -EPERM		The keyboard is locked.
	 * @retval -EOVERFLOW	The address is beyond the screen length.
	 * @retval -ENOTCONN	Disconnected from host.
	 *
	 */
	LIB3270_EXPORT int lib3270_set_string_at_address(H3270 *hSession, int baddr, const unsigned char *str, int length);

	/**
	 * @brief Insert string at current cursor position.
	 *
	 * @param hSession	Session handle.
	 * @param str		Text to insert.
	 * @param length	Length of the string (-1 for auto-detect).
	 *
	 * @return 0 if success, non zero if failed (sets errno).
	 *
	 * @retval EPERM		The keyboard is locked.
	 * @retval ENOTCONN		Disconnected from host.
	 *
	 */
	LIB3270_EXPORT int lib3270_input_string(H3270 *hSession, const unsigned char *str, int length);

	/**
	 * @brief Move cursor to a new position.
	 *
	 * @see lib3270_set_cursor_position
	 *
	 * @param hSession	TN3270 session.
	 * @param baddr		New cursor position.
	 *
	 * @return Old cursor address or negative in case of error (sets errno).
	 *
	 * @retval -EOVERFLOW	The address is beyond the screen length.
	 * @retval -ENOTCONN	Disconnected from host.
	 *
	 */
	LIB3270_EXPORT int lib3270_set_cursor_address(H3270 *hSession, unsigned int baddr);

	/**
	 * @brief Set cursor position.
	 *
	 * @param h		Session handle.
	 * @param row	New cursor row.
	 * @param col	New cursor col.
	 *
	 * @return Old cursor address or negative in case of error (sets errno).
	 *
	 * @retval -EOVERFLOW	The address is beyond the screen length.
	 * @retval -ENOTCONN	Disconnected from host.
	 */
	LIB3270_EXPORT int lib3270_set_cursor_position(H3270 *h, unsigned int row, unsigned int col);

	/**
	 * @brief Get cursor address.
	 *
	 * @param hSession Session handle.
	 *
	 * @return Cursor address or 0 if invalid (sets errno).
	 *
	 */
	LIB3270_EXPORT unsigned int lib3270_get_cursor_address(const H3270 *hSession);

	/**
	 * @brief Move cursor
	 *
	 * @param h		Session handle.
	 * @param dir	Direction to move
	 * @param sel	Non zero to move and selected to the current cursor position
	 *
	 * @return 0 if the movement can be done, non zero if failed.
	 */
	 LIB3270_EXPORT int lib3270_move_cursor(H3270 *h, LIB3270_DIRECTION dir, unsigned char sel);

	/**
	 * @brief Default print operation.
	 *
	 * If the terminal has selected area print them, if not, print all contents.
	 *
	 * @param hSession	Session Handle.
	 *
	 * @return 0 if ok, error code if not.
	 *
	 */
	LIB3270_EXPORT int lib3270_print(H3270 *hSession);

	/**
	 * @brief Print terminal screen.
	 *
	 * @param hSession	Session Handle.
	 *
	 * @return 0 if ok, error code if not.
	 *
	 */
	LIB3270_EXPORT int lib3270_print_all(H3270 *hSession);

	/**
	 * @brief Print only selected area (if available).
	 *
	 * @param hSession	Session Handle.
	 *
	 * @return 0 if ok, error code if not.
	 *
	 */
	LIB3270_EXPORT int lib3270_print_selected(H3270 *hSession);

	/**
	 * @brief Ask the front end module to print stored copy.
	 *
	 * @param hSession	Session Handle.
	 *
	 * @return 0 if ok, error code if not.
	 *
	 */
	LIB3270_EXPORT int lib3270_print_copy(H3270 *hSession);

	/**
	 * @brief Save contents to file.
	 *
	 * @param hSession	Session Handle.
	 * @param mode		Content option.
	 * @param filename	File name.
	 *
	 * @return 0 if ok, error code if not.
	 */
	LIB3270_EXPORT int lib3270_save(H3270 *hSession, LIB3270_CONTENT_OPTION mode, const char *filename);

	LIB3270_EXPORT int lib3270_save_all(H3270 *hSession, const char *filename);
	LIB3270_EXPORT int lib3270_save_selected(H3270 *hSession, const char *filename);
	LIB3270_EXPORT int lib3270_save_copy(H3270 *hSession, const char *filename);

	/**
	 * @brief Paste from file.
	 *
	 * @param hSession	Session Handle.
	 * @param filename	File name.
	 *
	 * @return 0 if ok, error code if not.
	 */
	LIB3270_EXPORT int lib3270_load(H3270 *hSession, const char *filename);

	/**
	 * @brief Get buffer contents.
	 *
	 * @param h		Session handle.
	 * @param first	First element to get.
	 * @param last	Last element to get.
	 * @param chr	Pointer to buffer which will receive the read chars.
	 * @param attr	Pointer to buffer which will receive the chars attributes.
	 *
	 */
	LIB3270_EXPORT int lib3270_get_contents(H3270 *h, int first, int last, unsigned char *chr, unsigned short *attr);

	/**
	 * @brief IO flags.
	 *
	 */
	typedef enum _lib3270_io_event {
		LIB3270_IO_FLAG_READ		= 0x01,
		LIB3270_IO_FLAG_EXCEPTION	= 0x02,
		LIB3270_IO_FLAG_WRITE		= 0x04,

		LIB3270_IO_FLAG_MASK		= 0x07
	} LIB3270_IO_FLAG;

	LIB3270_EXPORT void		* lib3270_add_poll_fd(H3270 *session, int fd, LIB3270_IO_FLAG flag, void(*call)(H3270 *, int, LIB3270_IO_FLAG, void *), void *userdata );
	LIB3270_EXPORT void		  lib3270_remove_poll(H3270 *session, void *id);
	LIB3270_EXPORT void		  lib3270_set_poll_state(H3270 *session, void *id, int enabled);

	LIB3270_EXPORT void		  lib3270_remove_poll_fd(H3270 *session, int fd);
	LIB3270_EXPORT void		  lib3270_update_poll_fd(H3270 *session, int fd, LIB3270_IO_FLAG flag);

	/**
	 * @brief I/O Controller.
	 *
	 * GUI unblocking I/O calls, used to replace the lib3270´s internal ones.
	 *
	 */
	typedef struct lib3270_io_controller
	{
		unsigned short sz;

		void	* (*AddTimer)(H3270 *session, unsigned long interval_ms, int (*proc)(H3270 *session));
		void	  (*RemoveTimer)(H3270 *session, void *timer);

		void	* (*add_poll)(H3270 *session, int fd, LIB3270_IO_FLAG flag, void(*proc)(H3270 *, int, LIB3270_IO_FLAG, void *), void *userdata);
		void	  (*remove_poll)(H3270 *session, void *id);
		void	  (*set_poll_state)(H3270 *session, void *id, int enabled);

		int		  (*Wait)(H3270 *hSession, int seconds);
		int		  (*event_dispatcher)(H3270 *session, int wait);
		void	  (*ring_bell)(H3270 *session);
		int		  (*run_task)(H3270 *session, int(*callback)(H3270 *, void *), void *parm);

	} LIB3270_IO_CONTROLLER;

	/**
	 * Register application Handlers.
	 *
	 * @param cbk	Structure with the application I/O handles to set.
	 *
	 * @return 0 if ok, error code if not.
	 *
	 */
	LIB3270_EXPORT int lib3270_register_io_controller(const LIB3270_IO_CONTROLLER *cbk);

	/**
	 * Register time handlers.
	 *
	 * @param add	Callback for adding a timeout
	 * @param rm	Callback for removing a timeout
	 *
	 */
	LIB3270_EXPORT void lib3270_register_timer_handlers(void * (*add)(H3270 *session, unsigned long interval_ms, int (*proc)(H3270 *session)), void (*rm)(H3270 *session, void *timer));

	LIB3270_EXPORT void lib3270_register_fd_handlers(void * (*add)(H3270 *session, int fd, LIB3270_IO_FLAG flag, void(*proc)(H3270 *, int, LIB3270_IO_FLAG, void *), void *userdata), void (*rm)(H3270 *, void *id));

	/**
	 * @brief Get program message.
	 *
	 * @see LIB3270_MESSAGE
	 *
	 * @param h	Session handle.
	 *
	 * @return Latest program message.
	 *
	 */
	LIB3270_EXPORT LIB3270_MESSAGE	  lib3270_get_program_message(const H3270 *h);

	/**
	 * @brief Get the LU name associated with the session, if there is one.
	 *
	 * Get the name LU associated with the session; the value is
	 * internal to lib3270 and should not be changed ou freed.
	 *
	 * @param hSession	Session handle.
	 *
	 * @return The associated LU name or NULL if not available.
	 *
	 */
	LIB3270_EXPORT const char * lib3270_get_associated_luname(const H3270 *hSession);

	/**
	 * @brief Set the LU names.
	 *
	 * @param hSession	Session handle.
	 * @param lunames	Comma separated list of the LU names to set.
	 *
	 * @return 0 if the list was set, non zero if not (sets errno)
	 *
	 * @retval EISCONN	The session is online.
	 * @retval EINVAL	Invalid session handle.
	 *
	 */
	LIB3270_EXPORT int lib3270_set_lunames(H3270 *hSession, const char *luname);

	LIB3270_EXPORT const char ** lib3270_get_lunames(H3270 *hSession);

	LIB3270_EXPORT int lib3270_is_connected(const H3270 *h);


	LIB3270_EXPORT int lib3270_is_disconnected(const H3270 *h);

	LIB3270_EXPORT int lib3270_is_unlocked(const H3270 *h);

	LIB3270_EXPORT int lib3270_has_active_script(const H3270 *h);
	LIB3270_EXPORT int lib3270_get_typeahead(const H3270 *h);
	LIB3270_EXPORT int lib3270_get_undera(const H3270 *h);
	LIB3270_EXPORT int lib3270_get_oia_box_solid(const H3270 *h);
	LIB3270_EXPORT int lib3270_pconnected(const H3270 *h);
	LIB3270_EXPORT int lib3270_half_connected(const H3270 *h);
	LIB3270_EXPORT int lib3270_in_neither(const H3270 *h);
	LIB3270_EXPORT int lib3270_in_ansi(const H3270 *h);
	LIB3270_EXPORT int lib3270_in_3270(const H3270 *h);
	LIB3270_EXPORT int lib3270_in_sscp(const H3270 *h);
	LIB3270_EXPORT int lib3270_in_tn3270e(const H3270 *h);
	LIB3270_EXPORT int lib3270_in_e(const H3270 *h);

	LIB3270_EXPORT int lib3270_is_ready(const H3270 *h);
	LIB3270_EXPORT int lib3270_is_secure(const H3270 *h);

	LIB3270_EXPORT LIB3270_MESSAGE		lib3270_get_lock_status(const H3270 *h);

	/**
	 * Run main iteration.
	 *
	 * Run lib3270 internal iterations, check for network inputs, process signals.
	 *
	 * @param h		Related session.
	 * @param wait	Wait for signal if not available.
	 *
	 */
	LIB3270_EXPORT void lib3270_main_iterate(H3270 *h, int wait);

	/**
	 * @brief Associate user data with 3270 session.
	 *
	 */
	LIB3270_EXPORT void lib3270_set_user_data(H3270 *h, void *ptr);

	/**
	 * @brief Get associated user data.
	 *
	 */
	LIB3270_EXPORT void * lib3270_get_user_data(H3270 *h);

	/**
	 * @brief Wait for "N" seconds keeping main loop active.
	 *
	 * @param seconds	Number of seconds to wait.
	 *
	 */
	LIB3270_EXPORT int lib3270_wait(H3270 *hSession, int seconds);

	/**
	 * @brief Wait for "N" seconds or screen change; keeps main loop active.
	 *
	 * @param seconds	Number of seconds to wait.
	 *
	 */
	LIB3270_EXPORT int lib3270_wait_for_update(H3270 *hSession, int seconds);

	/**
	 * Wait "N" seconds for "ready" state.
	 *
	 * @param seconds	Number of seconds to wait.
	 *
	 * @return 0 if ok, errno code if not.
	 *
	 */
	LIB3270_EXPORT int lib3270_wait_for_ready(H3270 *hSession, int seconds);

	/**
	 * "beep" to notify user.
	 *
	 * If available play a sound signal do alert user.
	 *
	 * @param h		Session handle.
	 *
	 */
	 LIB3270_EXPORT void lib3270_ring_bell(H3270 *session);

	/**
	 * Get lib3270's charset.
	 *
	 * @param h Session handle.
	 *
	 * @return String with current encoding.
	 *
	 */
	 LIB3270_EXPORT const char * lib3270_get_display_charset(const H3270 *session);

	 #define lib3270_get_charset(s) lib3270_get_display_charset(s)

	 LIB3270_EXPORT const char * lib3270_get_default_charset(void);

	/**
	 * @brief Get selected area.
	 *
	 * @param h	Session Handle.
	 *
	 * @return selected text if available, or NULL. Release it with free()
	 *
	 */
	LIB3270_EXPORT char * lib3270_get_selected(H3270 *hSession);

	LIB3270_EXPORT char * lib3270_cut_selected(H3270 *hSession);

	/**
	 * @brief Check if the terminal has selected area (allways sets errno).
	 *
	 * @param hSession	Session handle.
	 *
	 * @return Greater than zero if the terminal has selected area, 0 if not.
	 *
	 */
	LIB3270_EXPORT int	  lib3270_has_selection(const H3270 *hSession);

	/**
	 * @brief Get all text inside the terminal.
	 *
	 * @param h			Session Handle.
	 * @param offset	Start position (-1 to current cursor position).
	 * @param len		Text length or -1 to all text.
	 * @param lf		Line break char (0 to disable line breaks).
	 *
	 * @return Contents at position if available, or NULL if error (sets errno). Release it with lib3270_free()
	 *
	 * @exception ENOTCONN	Not connected to host.
	 * @exception EOVERFLOW	Invalid offset.
	 *
	 */
	LIB3270_EXPORT char * lib3270_get_string_at_address(H3270 *h, int offset, int len, char lf);

	/**
	 * @brief Get text at requested position
	 *
	 * @param h			Session Handle.
	 * @param row		Desired row.
	 * @param col		Desired col.
	 * @param len		Text length or -1 to all text.
	 * @param lf		Line break char (0 to disable line breaks).
	 *
	 * @return Contents at position if available, or NULL if error (sets errno). Release it with lib3270_free()
	 *
	 * @exception ENOTCONN	Not connected to host.
	 * @exception EOVERFLOW	Invalid position.
	 *
	 */
	LIB3270_EXPORT char * lib3270_get_string_at(H3270 *h, unsigned int row, unsigned int col, int len, char lf);

	/**
	 * @brief Check for text at requested position
	 *
	 * @param h			Session Handle.
	 * @param row		Desired row.
	 * @param col		Desired col.
	 * @param text		Text to check.
	 * @param lf		Line break char (0 to disable line breaks).
	 *
	 * @return Test result from strcmp
	 *
	 */
	 LIB3270_EXPORT int lib3270_cmp_string_at(H3270 *h, unsigned int row, unsigned int col, const char *text, char lf);

	 LIB3270_EXPORT int lib3270_cmp_string_at_address(H3270 *h, int baddr, const char *text, char lf);

	/**
	 * @brief Get contents of the field at position.
	 *
	 * @param h			Session Handle.
	 * @param baddr		Reference position.
	 *
	 * @return NULL if failed (sets errno), contents of the entire field if suceeds (release it with lib3270_free()).
	 *
	 * @exception ENOTCONN	Not connected to host.
	 * @exception EOVERFLOW	Invalid position.
	 *
	 */
	LIB3270_EXPORT char * lib3270_get_field_string_at(H3270 *h, int baddr);

	/**
	 * @brief Find the next unprotected field.
	 *
	 * @param hSession	Session handle.
	 * @param baddr0	Search start addr (-1 to use current cursor position).
	 *
	 * @return address following the unprotected attribute byte, or 0 if no nonzero-width unprotected field can be found.
	 *
	 */
	LIB3270_EXPORT int lib3270_get_next_unprotected(H3270 *hSession, int baddr0);

	/**
	 * @brief Check if the screen position is protected.
	 *
	 * @param hSession	Session handle.
	 * @param baddr0	Search start addr (-1 to use current cursor position).
     *
     * @return -1 when failed 1 if the addr is protected and 0 if not.
     *
	 */
	LIB3270_EXPORT int lib3270_get_is_protected(const H3270 *hSession, int baddr0);

	LIB3270_EXPORT int LIB3270_DEPRECATED(lib3270_is_protected(H3270 *h, unsigned int baddr));

	/**
	 * @brief Check if the screen is formatted.
	 *
	 * @param hSession	Session handle.
     *
     * @return -1 when failed 1 if the session is formatted and 0 if not.
     *
     * @retval -1	Failed (check errno for error code).
     * @retval  0	Screen is not formatted.
     * @retval  1	Screen is formatted.
	 */
	LIB3270_EXPORT int lib3270_is_formatted(const H3270 *hSession);

	/**
	 * @brief Get Check if the screen position is protected.
	 *
	 * @param h			Session Handle.
	 * @param row		Desired row.
	 * @param col		Desired col.
	 *
	 */
	LIB3270_EXPORT int lib3270_get_is_protected_at(const H3270 *h, unsigned int row, unsigned int col);

	/**
	 * @brief Get address of the first blank.
	 *
	 * Get address of the first blank after the last nonblank in the
	 * field, or if the field is full, to the last character in the field.
	 *
	 * @param hSession	Session handle.
	 * @param baddr		Field address.
	 *
	 * @return address of the first blank or negative if invalid.
	 *
	 * @retval -ENOTSUP		Screen is not formatted.
	 * @retval -EPERM		Current cursor position is protected.
	 */
	LIB3270_EXPORT int lib3270_get_field_end(H3270 *hSession, int baddr);

	/**
	 * @brief Find the buffer address of the field attribute for a given buffer address.
	 *
	 * @param hSession	Session handle.
	 * @param addr		Buffer address of the field.
	 *
	 * @return field address or negative if the screen isn't formatted (sets errno).
	 *
	 * @retval -ENOTCONN	Not connected to host.
	 * @retval -EOVERFLOW	Invalid position.
	 * @retval -ENOTSUP		Screen is not formatted.
	 * @retval -ENODATA		No field at the address.
	 *
	 */
	LIB3270_EXPORT int lib3270_field_addr(const H3270 *hSession, int baddr);

	/**
	 * @brief Get field attribute for a given buffer address.
	 *
	 * @param hSession	Session handle.
	 * @param addr		Buffer address of the field (-1 to use the cursor address).
	 *
	 * @return field attribute or LIB3270_FIELD_ATTRIBUTE_INVALID when failed (sets errno).
	 */
	LIB3270_EXPORT LIB3270_FIELD_ATTRIBUTE lib3270_get_field_attribute(H3270 *hSession, int baddr);

	/**
	 * @brief Get the length of the field at given buffer address.
	 *
	 * @param hSession	Session handle.
	 * @param addr		Buffer address of the field.
	 *
	 * @return field length or -1 if invalid or not connected (sets errno).
	 *
	 */
	LIB3270_EXPORT int lib3270_field_length(H3270 *h, int baddr);

	/**
	 * @brief Get a terminal character and attribute.
	 *
	 * @param h		Session Handle.
	 * @param baddr	Element address ((element_row*cols)+element_col)
	 * @param c		Pointer to character.
	 * @param attr	Pointer to attribute.
	 *
	 * @return 0 if ok, -1 if fails (sets errno).
	 *
	 */
	LIB3270_EXPORT int lib3270_get_element(H3270 *h, unsigned int baddr, unsigned char *c, unsigned short *attr);

	/**
	 * @brief Check if the informed addr is marked as selected.
	 *
	 * @param h		Session Handle.
	 * @param baddr	Element address ((element_row*cols)+element_col)
	 *
	 * @return >0 zero if element is selected, 0 if not, -1 if fails (sets errno).
	 *
	 */
	LIB3270_EXPORT int lib3270_is_selected(H3270 *hSession, unsigned int baddr);

	/**
	 * @brief Get attribute at the requested ADDR.
	 *
	 * @param h		Session Handle.
	 * @param baddr	Element address ((element_row*cols)+element_col)
	 *
	 * @return Attribute of the required address or -1 if failed.
	 *
	 */
	LIB3270_EXPORT LIB3270_ATTR lib3270_get_attribute_at_address(H3270 *hSession, unsigned int baddr);

	/**
	 * Get field region
	 *
	 * @param h		Session handle.
	 * @param baddr	Reference position to get the field start/stop offsets.
	 * @param start	return location for start of selection, as a character offset.
	 * @param end	return location for end of selection, as a character offset.
	 *
	 * @return Non 0 if invalid
	 *
	 */
	LIB3270_EXPORT int lib3270_get_field_bounds(H3270 *hSession, int baddr, int *start, int *end);

	LIB3270_EXPORT int lib3270_get_field_start(H3270 *hSession, int baddr);
	LIB3270_EXPORT int lib3270_get_field_len(H3270 *hSession, int baddr);

	LIB3270_EXPORT int lib3270_get_word_bounds(H3270 *hSession, int baddr, int *start, int *end);

	LIB3270_EXPORT int 			  LIB3270_DEPRECATED(lib3270_set_model(H3270 *hSession, const char *model_name));
	LIB3270_EXPORT const char	* LIB3270_DEPRECATED(lib3270_get_model(const H3270 *session));

	LIB3270_EXPORT const char	* lib3270_get_model_name(const H3270 *session);
	LIB3270_EXPORT int			  lib3270_set_model_name(H3270 *hSession, const char *model_name);

	LIB3270_EXPORT unsigned int	  lib3270_get_model_number(const H3270 *hSession);

	/**
	 * @brief Set TN3270 model number.
	 *
	 * @param hSession	Session handle.
	 * @param model_number	The new model number (2-5).
	 *
	 * @return
	 */
	LIB3270_EXPORT int			  lib3270_set_model_number(H3270 *hSession, unsigned int model_number);

	/**
	 *
	 * @brief Set the unlock delay in milliseconds.
	 *
	 * When lib3270 sends the host an AID (the Enter, Clear, PF or PA actions),
	 * it locks the keyboard until the host sends a reply to unlock it. Some
	 * hosts unlock the keyboard before they are actually finished processing
	 * the command, which can cause scripts to malfunction subtly.
	 *
	 * To avoid this, lib3270 implements a hack to briefly delay actually
	 * unlocking the keyboard. When the unlock delay is not 0, the keyboard
	 * unlock will be delayed for the number of milliseconds set by this call.
	 *
	 * Setting the delay to 0 disables the hack.
	 *
	 * @param session	lib3270 session.
	 * @param delay		Delay in milliseconds.
	 *
	 */
	LIB3270_EXPORT int lib3270_set_unlock_delay(H3270 *session, unsigned int delay);
	LIB3270_EXPORT unsigned int lib3270_get_unlock_delay(const H3270 *session);

	/**
	 * @brief Alloc/Realloc memory buffer.
	 *
	 * Allocate/reallocate an array.
	 *
	 * @param elsize	Element size.
	 * @param nelem		Number of elements in the array.
	 * @param ptr		Pointer to the actual array.
	 *
	 * @return Clean buffer with size for the number of elements.
	 *
	 */
	LIB3270_EXPORT void * lib3270_calloc(int elsize, int nelem, void *ptr);

	LIB3270_EXPORT void * lib3270_malloc(int len);
	LIB3270_EXPORT void * lib3270_realloc(void *p, int len);
	LIB3270_EXPORT void * lib3270_strdup(const char *str);

	/**
	 * @brief Release allocated memory.
	 *
	 * @param p	Memory block to release (can be NULL)
	 *
	 * @return NULL
	 */
	LIB3270_EXPORT void  * lib3270_free(void *p);

	LIB3270_EXPORT void   lib3270_autoptr_cleanup_char(char **ptr);

	/**
	 * Get default session handle.
	 *
	 * @return Internal's lib3270 session handle.
	 *
	 */
	LIB3270_EXPORT H3270 * lib3270_get_default_session_handle(void);

	/**
	 * Get library version.
	 *
	 * @return Version of active library as string.
	 *
	 */
	LIB3270_EXPORT const char * lib3270_get_version(void);

	/**
	 * Get source code revision.
	 *
	 * @return The revision of the current source code.
	 *
	 */
	LIB3270_EXPORT const char * lib3270_get_revision(void);

	LIB3270_EXPORT char * lib3270_vsprintf(const char *fmt, va_list args);
	LIB3270_EXPORT char * lib3270_strdup_printf(const char *fmt, ...);

	LIB3270_EXPORT int lib3270_clear_operator_error(H3270 *hSession);

	/**
	 * @brief Set the terminal color type.
	 *
	 * @param hSession	Session handle.
	 * @param colortype	The color type for the emulator (2, 8, 16 or 0 to lib3270's default).
	 *
	 * @return 0 if ok, error code if failed (sets errno).
	 *
	 * @retval EINVAL	Invalid color type value.
	 * @retval EISCONN	The session is active.
	 *
	 */
	LIB3270_EXPORT int lib3270_set_color_type(H3270 *hSession, unsigned int colortype);

	LIB3270_EXPORT unsigned int lib3270_get_color_type(const H3270 *hSession);

	LIB3270_EXPORT int lib3270_set_host_type_by_name(H3270 *hSession, const char *name);
	LIB3270_EXPORT int lib3270_set_host_type(H3270 *hSession, LIB3270_HOST_TYPE opt);

	LIB3270_EXPORT LIB3270_HOST_TYPE lib3270_parse_host_type(const char *name);

	LIB3270_EXPORT const LIB3270_HOST_TYPE_ENTRY * lib3270_get_option_list(void);

	LIB3270_EXPORT LIB3270_POINTER lib3270_get_pointer(H3270 *hSession, int baddr);

	/**
	 * @brief Run background task.
	 *
	 * Call task in a separate thread, keep gui main loop running until
	 * the function returns.
	 *
	 * @param hSession	TN3270 session.
	 * @param callback	Function to call.
	 * @param parm		Parameter to callback function.
	 *
	 */
	LIB3270_EXPORT int lib3270_run_task(H3270 *hSession, int(*callback)(H3270 *h, void *), void *parm);

	/**
	 * @brief The host is TSO?
	 *
	 * @param hSession	Session Handle
	 *
	 * @return Non zero if the host is TSO.
	 *
	 */
	LIB3270_EXPORT int lib3270_is_tso(const H3270 *hSession);

	LIB3270_EXPORT int lib3270_set_tso(H3270 *hSession, int on);

	/**
	 * @brief Host is AS400 (Prefix every PF with PA1).
	 *
	 * @param hSession	Session Handle
	 *
	 * @return Non zero if the host is AS400.
	 *
	 */
	LIB3270_EXPORT int lib3270_is_as400(const H3270 *hSession);

	LIB3270_EXPORT int lib3270_set_as400(H3270 *hSession, int on);

#ifdef _WIN32
	LIB3270_EXPORT const char	* lib3270_win32_strerror(int e);
	LIB3270_EXPORT const char	* lib3270_win32_local_charset(void);
	LIB3270_EXPORT LSTATUS		  lib3270_win32_create_regkey(LPCSTR lpSubKey, REGSAM samDesired, PHKEY phkResult);

	/**
	 * @brief Translate windows error code.
	 *
	 * @param lasterror	Windows error code (from GetLastError()).
	 *
	 * @return String with translated message (release it with lib3270_free).
	 *
	 */
	LIB3270_EXPORT char 		* lib3270_win32_translate_error_code(int lasterror);

	/**
	 * @brief Get lib3270's installation path.
	 *
	 * @return Full path for the lib3270 installation path (release it with lib3270_free)
	 *
	 */
	 LIB3270_EXPORT char		* lib3270_get_installation_path();

#endif // WIn32

	/**
	 * @brief Build filename on application data dir.
	 *
	 * @return Full path for the file (release it with lib3270_free).
	 *
	 */
	LIB3270_EXPORT char * lib3270_build_data_filename(const char *str, ...) LIB3270_GNUC_NULL_TERMINATED;

	/**
	 * @brief Build filename on application configuration dir.
	 *
	 * @return Full path for the file (release it with lib3270_free).
	 *
	 */
	LIB3270_EXPORT char * lib3270_build_config_filename(const char *str, ...) LIB3270_GNUC_NULL_TERMINATED;

	/**
	 * @brief Build and search for filename.
	 *
	 * Build filename and search for it on current, configuration and data dirs.
	 *
	 * @return Full path for the file (release it with lib3270_free).
	 *
	 */
	LIB3270_EXPORT char * lib3270_build_filename(const char *str, ...) LIB3270_GNUC_NULL_TERMINATED;

	LIB3270_EXPORT void lib3270_set_session_id(H3270 *hSession, char id);
	LIB3270_EXPORT char lib3270_get_session_id(H3270 *hSession);

	/**
	 * @brief Wait for string at screen.
	 *
	 * @param hSession	TN3270 Session.
	 * @param key		The string to wait for.
	 * @param seconds	Maximum wait time.
	 *
	 * @return 0 if the string was found, error code if not (sets errno).
	 *
	 * @retval ENOTCONN		Not connected to host.
	 * @retval ETIMEDOUT	Timeout.
	 * @retval EPERM		The keyboard is locked.
	 *
	 */
	LIB3270_EXPORT int lib3270_wait_for_string(H3270 *hSession, const char *key, int seconds);

	/**
	 * @brief Wait for string at position.
	 *
	 * @param hSession	TN3270 Session.
	 * @param row		Row inside the screen.
	 * @param col		Col inside the screen.
	 * @param key		The string to wait for.
	 * @param seconds	Maximum wait time.
	 *
	 * @return 0 if the string was found, error code if not (sets errno).
	 *
	 * @retval ENOTCONN		Not connected to host.
	 * @retval EOVERFLOW	Invalid position.
	 * @retval ETIMEDOUT	Timeout.
	 * @retval EPERM		The keyboard is locked.
	 *
	 */
	LIB3270_EXPORT int lib3270_wait_for_string_at(H3270 *hSession, unsigned int row, unsigned int col, const char *key, int seconds);

	/**
	 * @brief Wait for string at addrress.
	 *
	 * @param hSession	TN3270 Session.
	 * @param baddr		Start position (-1 to current cursor position).
	 * @param key		The string to wait for.
	 * @param seconds	Maximum wait time.
	 *
	 * @return 0 if the string was found, error code if not (sets errno).
	 *
	 * @retval ENOTCONN	Not connected to host.
	 * @retval EOVERFLOW	Invalid position.
	 * @retval ETIMEDOUT	Timeout.
	 * @retval EPERM		The keyboard is locked.
	 *
	 */
	LIB3270_EXPORT int lib3270_wait_for_string_at_address(H3270 *hSession, int baddr, const char *key, int seconds);

	/**
	 * @brief Notify action group.
	 *
	 */
	LIB3270_EXPORT void lib3270_action_group_notify(H3270 *hSession, LIB3270_ACTION_GROUP group);

#ifdef __cplusplus
	}
#endif

#endif // LIB3270_H_INCLUDED
