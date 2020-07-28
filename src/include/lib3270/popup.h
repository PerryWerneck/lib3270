/*
 * "Software PW3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
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
 * Este programa está nomeado como api.h e possui 444 linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas de Mendonça)
 * licinio@bb.com.br		(Licínio Luis Branco)
 * kraucer@bb.com.br		(Kraucer Fernandes Mazuco)
 *
 */

#ifndef LIB3270_POPUP_INCLUDED

	#define LIB3270_POPUP_INCLUDED 1

#ifdef __cplusplus
	extern "C" {
#endif

	/**
	 * Notification message types.
	 *
	 */
	typedef enum _LIB3270_NOTIFY
	{
		LIB3270_NOTIFY_INFO,		///< @brief Simple information dialog.
		LIB3270_NOTIFY_WARNING,		///< @brief Warning message.
		LIB3270_NOTIFY_ERROR,		///< @brief Error message.
		LIB3270_NOTIFY_CRITICAL,	///< @brief Critical error, user can abort application.
		LIB3270_NOTIFY_SECURE,		///< @brief Secure host dialog.

		LIB3270_NOTIFY_USER			///< @brief Reserved, always the last one.
	} LIB3270_NOTIFY;


	/**
	 * @brief Head for popup descriptors.
	 *
	 */
	#define LIB3270_POPUP_HEAD	\
		const char * name; \
		LIB3270_NOTIFY type; \
		const char * title; \
		const char * summary; \
		const char * body; \
		const char * label;

	typedef struct _LIB3270_POPUP
	{
		LIB3270_POPUP_HEAD
	} LIB3270_POPUP;

	/**
	 * @brief Replace popup handler.
	 *
	 */
	LIB3270_EXPORT void lib3270_set_popup_handler(H3270 *hSession, int (*handler)(H3270 *, const LIB3270_POPUP *, unsigned char wait));

	/**
	 * @brief Pop up an error dialog, based on an error number.
	 *
	 * @param hSession	Session handle
	 * @param errn		Error number (errno).
	 * @param fmt		Message format
	 * @param ...		Arguments for message
	 */
	LIB3270_EXPORT void lib3270_popup_an_errno(H3270 *hSession, int errn, const char *fmt, ...);

	LIB3270_EXPORT void lib3270_popup_dialog(H3270 *session, LIB3270_NOTIFY id , const char *title, const char *message, const char *fmt, ...);

	LIB3270_EXPORT void lib3270_popup_va(H3270 *session, LIB3270_NOTIFY id , const char *title, const char *message, const char *fmt, va_list);

	LIB3270_EXPORT LIB3270_NOTIFY	lib3270_get_ssl_state_icon(const H3270 *hSession);
	LIB3270_EXPORT const char *		lib3270_get_ssl_state_icon_name(const H3270 *hSession);

	/**
	 * @brief Clone popup object replacing the body contents.
	 *
	 * @param origin	Original popup definition.
	 * @param fmt		Printf formatting string.
	 *
	 * @return New popup object with the body replaced (release it with g_free).
	 *
	 */
	LIB3270_EXPORT LIB3270_POPUP * lib3270_popup_clone_printf(const LIB3270_POPUP *origin, const char *fmt, ...);

	/**
	 * @brief Emit popup message.
	 *
	 * @param hSession	TN3270 Session handle.
	 * @param popup		Popup descriptor.
	 * @param wait		If non zero waits for user response.
	 *
	 * @return User action.
	 *
	 * @retval 0			User has confirmed, continue action.
	 * @retval ECANCELED	Operation was canceled.
	 * @retval ENOTSUP		No popup handler available.
	 */
	LIB3270_EXPORT int lib3270_popup(H3270 *hSession, const LIB3270_POPUP *popup, unsigned char wait);

	/**
	 * @brief Auto cleanup method (for use with lib3270_autoptr).
	 *
	 */
	LIB3270_EXPORT void lib3270_autoptr_cleanup_LIB3270_POPUP(LIB3270_POPUP **ptr);

#ifdef __cplusplus
	}
#endif

#endif // LIB3270_POPUP_INCLUDED


