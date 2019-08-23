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
 * Este programa está nomeado como -.h e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas de Mendonça)
 *
 */

 /**
  * @brief TN3270 Session properties.
  *
  * @author perry.werneck@gmail.com
  *
  */

#ifndef LIB3270_PROPERTIES_H_INCLUDED

	#define LIB3270_PROPERTIES_H_INCLUDED

#ifdef __cplusplus
	extern "C" {
#endif

	typedef struct _lib3270_int_property
	{
		const char		* name;											///< @brief Property name.
		const char		* description;									///< @brief Property description.
		int (*get)(H3270 *hSession);									///< @brief Get value.
		int (*set)(H3270 *hSession, int value);							///< @brief Set value.

	} LIB3270_INT_PROPERTY;

	typedef struct _lib3270_uint_property
	{
		const char		* name;											///< @brief Property name.
		const char		* description;									///< @brief Property description.
		unsigned int (*get)(H3270 *hSession);							///< @brief Get value.
		int (*set)(H3270 *hSession, unsigned int value);				///< @brief Set value.

	} LIB3270_UINT_PROPERTY;

	typedef struct _lib3270_string_property
	{
		const char		* name;								///< @brief Property name.
		const char		* description;						///< @brief Property description.
		const char * (*get)(const H3270 *hSession);			///< @brief Get value.
		int (*set)(H3270 *hSession, const char * value);	///< @brief Set value.

	} LIB3270_STRING_PROPERTY;

	/**
	 * @brief Get lib3270 integer properties table.
	 *
	 * @return The properties table.
	 *
	 */
	LIB3270_EXPORT const LIB3270_INT_PROPERTY * lib3270_get_boolean_properties_list(void);

	/**
	 * @brief Get lib3270 signed int properties table.
	 *
	 * @return The properties table.
	 *
	 */
	LIB3270_EXPORT const LIB3270_INT_PROPERTY * lib3270_get_int_properties_list(void);

	/**
	 * @brief Get lib3270 unsigned signed int properties table.
	 *
	 * @return The properties table.
	 *
	 */
	LIB3270_EXPORT const LIB3270_UINT_PROPERTY * lib3270_get_unsigned_properties_list(void);

	/**
	 * @brief Get lib3270 string properties table.
	 *
	 * @return The properties table.
	 *
	 */
	LIB3270_EXPORT const LIB3270_STRING_PROPERTY * lib3270_get_string_properties_list(void);


	/**
	 * @brief Get lib3270 integer property by name.
	 *
	 * @param name		Nome of the property.
	 * @param seconds	Time (in seconds) whe should wait for "ready" state (0 = none).
	 *
	 * @return Property value or -1 in case of error (sets errno).
	 *
	 */
	LIB3270_EXPORT int lib3270_get_int_property(H3270 * hSession, const char *name, int seconds);

	/**
	 * @brief Set lib3270 integer property by name.
	 *
	 * @param name		Nome of the property.
	 * @param value		New property value.
	 * @param seconds	Time (in seconds) whe should wait for "ready" state (0 = none).
	 *
	 * @return 0 if ok, -1 in case of error (sets errno).
	 *
	 */
	LIB3270_EXPORT int lib3270_set_int_property(H3270 * hSession, const char *name, int value, int seconds);

	/**
	 * @brief Set lib3270 integer property by name.
	 *
	 * @param hSession	Session handle.
	 * @param name		Nome of the property.
	 * @param value		New property value.
	 * @param seconds	Time (in seconds) whe should wait for "ready" state (0 = none).
	 *
	 * @return 0 if ok, -1 in case of error (sets errno).
	 *
	 */
	LIB3270_EXPORT int lib3270_set_string_property(H3270 * hSession, const char *name, const char * value, int seconds);


	/**
	 * @brief Get Oversize.
	 *
	 * @param hSession	Session handle.
	 *
	 * @return Oversize definition (NULL if not set).
	 *
	 */
	LIB3270_EXPORT const char * lib3270_get_oversize(const H3270 *hSession);

	 /**
	  * @brief Set oversize.
	  *
	  * @param hSession	Session handle.
	  * @param value	Oversize value.
	  *
	  * @return 0 if success, error code if not (sets errno)
	  *
	  * @retval EISCONN	Already connected to host.
	  * @retval ENOTSUP	Oversize is not supported.
	  *
	  */
	LIB3270_EXPORT int lib3270_set_oversize(H3270 *hSession, const char *value);

#ifdef __cplusplus
	}
#endif

#endif // LIB3270_PROPERTIES_H_INCLUDED

