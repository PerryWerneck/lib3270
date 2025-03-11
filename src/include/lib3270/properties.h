/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2008 Banco do Brasil S.A.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 */

 /*
 * @brief TN3270 Session properties.
 *
 * @author perry.werneck@gmail.com
 */

 #pragma once
 
 #include <lib3270.h>

 #ifdef __cplusplus
 extern "C" {
 #endif

 //
 // @brief Head for property descriptors.
 //
 //
 #define LIB3270_PROPERTY_HEAD	\
	const char * name; \
	LIB3270_ACTION_GROUP group; \
	const char * label; \
	const char * icon; \
	const char * summary; \
	const char * description; \
	const char * (*describe)(const H3270 *);

 typedef struct _lib3270_property {
	LIB3270_PROPERTY_HEAD
 } LIB3270_PROPERTY;

 typedef struct _lib3270_int_property {
	LIB3270_PROPERTY_HEAD

	int (*get)(const H3270 *hSession);								///< @brief Get value.
	int (*set)(H3270 *hSession, int value);							///< @brief Set value.

	int default_value;												///< @brief Default value for the property.

 } LIB3270_INT_PROPERTY;

 typedef struct _lib3270_uint_property {
	LIB3270_PROPERTY_HEAD

	unsigned int (*get)(const H3270 *hSession);						///< @brief Get value.
	int (*set)(H3270 *hSession, unsigned int value);				///< @brief Set value.

	unsigned int min;												///< @brief Minimum allowable value.
	unsigned int max;												///< @brief Maximum allowable value.
	unsigned int default_value;										///< @brief Default value for the property.

 } LIB3270_UINT_PROPERTY;

 typedef struct _lib3270_string_property {
	LIB3270_PROPERTY_HEAD

	const char * (*get)(const H3270 *hSession);						///< @brief Get value.
	int (*set)(H3270 *hSession, const char * value);				///< @brief Set value.

	const char * default_value;										///< @brief Default value

 } LIB3270_STRING_PROPERTY;

 //
 // @brief Get lib3270 integer properties table.
 //
 // @return The properties table.
 //
 //
 LIB3270_EXPORT const LIB3270_INT_PROPERTY * lib3270_get_boolean_properties_list(void);

 //
 // @brief Get lib3270 signed int properties table.
 //
 // @return The properties table.
 //
 //
 LIB3270_EXPORT const LIB3270_INT_PROPERTY * lib3270_get_int_properties_list(void);

 //
 // @brief Get lib3270 unsigned signed int properties table.
 //
 // @return The properties table.
 //
 //
 LIB3270_EXPORT const LIB3270_UINT_PROPERTY * lib3270_get_unsigned_properties_list(void);

 //
 // @brief Get lib3270 string properties table.
 //
 // @return The properties table.
 //
 //
LIB3270_EXPORT const LIB3270_STRING_PROPERTY * lib3270_get_string_properties_list(void);

 //
 // @brief Get property descriptor by name.
 //
 // @return Property descriptor or NULL if failed.
 //
 //
 LIB3270_EXPORT const LIB3270_PROPERTY * lib3270_property_get_by_name(const char *name);

 //
 // @brief Get lib3270 integer property by name.
 //
 // @param name		Nome of the property.
 // @param seconds	Time (in seconds) whe should wait for "ready" state (0 = none).
 //
 // @return Property value or -1 in case of error (sets errno).
 //
 //
 LIB3270_EXPORT int lib3270_get_int_property(H3270 * hSession, const char *name, int seconds);

 //
 // @brief Set lib3270 signed int property by name.
 //
 // @param name		Nome of the property.
 // @param value		New property value.
 // @param seconds	Time (in seconds) whe should wait for "ready" state (0 = none).
 //
 // @return 0 if ok error code if not (sets errno).
 //
 // @retval EPERM	Property is ready only.
 // @retval ENOENT	Can't find a property with this name.
 //
 //
 LIB3270_EXPORT int lib3270_set_int_property(H3270 * hSession, const char *name, int value, int seconds);

 //
 // @brief Set lib3270 unsigned int property by name.
 //
 // @param name		Nome of the property.
 // @param value		New property value.
 // @param seconds	Time (in seconds) whe should wait for "ready" state (0 = none).
 //
 // @return 0 if ok error code if not (sets errno).
 //
 // @retval EPERM	Property is ready only.
 // @retval ENOENT	Can't find a property with this name.
 //
 //
 LIB3270_EXPORT int lib3270_set_uint_property(H3270 * hSession, const char *name, unsigned int value, int seconds);

 //
 // @brief Set lib3270 boolean property by name.
 //
 // @param name		Nome of the property.
 // @param value		New property value.
 // @param seconds	Time (in seconds) whe should wait for "ready" state (0 = none).
 //
 // @return 0 if ok error code if not (sets errno).
 //
 // @retval EPERM	Property is ready only.
 // @retval ENOENT	Can't find a property with this name.
 //
 //
 LIB3270_EXPORT int lib3270_set_boolean_property(H3270 * hSession, const char *name, int value, int seconds);

 //
 // @brief Set lib3270 string property by name.
 //
 // @param hSession	Session handle.
 // @param name		Nome of the property.
 // @param value		New property value.
 // @param seconds	Time (in seconds) whe should wait for "ready" state (0 = none).
 //
 // @return 0 if ok error code if not (sets errno).
 //
 // @retval EPERM	Property is ready only.
 // @retval ENOENT	Can't find a property with this name.
 //
 LIB3270_EXPORT int lib3270_set_string_property(H3270 * hSession, const char *name, const char * value, int seconds);


 //
 // @brief Get Oversize.
 //
 // @param hSession	Session handle.
 //
 // @return Oversize definition (NULL if not set).
 //
 LIB3270_EXPORT const char * lib3270_get_oversize(const H3270 *hSession);

 //
 // @brief Set oversize.
 //
 // @param hSession	Session handle.
 // @param value	Oversize value.
 //
 // @return 0 if success, error code if not (sets errno)
 //
 // @retval EISCONN	Already connected to host.
 // @retval ENOTSUP	Oversize is not supported.
 //
 LIB3270_EXPORT int lib3270_set_oversize(H3270 *hSession, const char *value);

 //
 // @brief Get property label.
 //
 LIB3270_EXPORT const char * lib3270_property_get_label(const LIB3270_PROPERTY * property);


 LIB3270_EXPORT const char * lib3270_property_get_name(const LIB3270_PROPERTY * property);
 LIB3270_EXPORT const char * lib3270_property_get_tooltip(const LIB3270_PROPERTY * property);

 //
 // @brief Get property description.
 //
 LIB3270_EXPORT const char * lib3270_property_get_description(const LIB3270_PROPERTY * property);

 //
 // @brief Get property summary.
 //
 LIB3270_EXPORT const char * lib3270_property_get_summary(const LIB3270_PROPERTY * property);

 //
 // @brief Get unsigned int property by name.
 //
 // @param name	Property name.
 //
 // @return Property descriptor, or NULL if failed.
 //
 LIB3270_EXPORT const LIB3270_UINT_PROPERTY * lib3270_unsigned_property_get_by_name(const char *name);

 //
 // @brief Get lib3270 version info.
 //
 // @return String with lib3270 version info (release it with lib3270_free).
 //
 LIB3270_EXPORT char * lib3270_get_version_info(void);

 //
 // @brief Get lib3270 product name.
 //
 // @return Internal string with the product name.
 //
 LIB3270_EXPORT const char * lib3270_get_product_name(void);

 //
 // @brief Get hostname for the connect/reconnect operations.
 //
 // @param h		Session handle.
 //
 // @return Pointer to host id set (internal data, do not change it)
 //
 LIB3270_EXPORT const char * lib3270_host_get_name(const H3270 *h);

 //
 // @brief Get service or port for the connect/reconnect operations.
 //
 // @param h		Session handle.
 //
 // @return Pointer to service name (internal data, do not change it)
 //
 LIB3270_EXPORT const char * lib3270_service_get_name(const H3270 *h);

 //
 // @brief Check if there's an active task.
 //
 // @param h	Session handle.
 //
 // @return Number of background tasks.
 //
 LIB3270_EXPORT unsigned int lib3270_get_task_count(const H3270 *h);

 //
 // @brief Set timer for auto-reconnect when connection fails.
 //
 // @param hSession	Session handle.
 //
 LIB3270_EXPORT int lib3270_set_auto_reconnect(H3270 *hSession, unsigned int timer);

 //
 // @brief Get timer for auto-reconnect when connection fails.
 //
 // @param hSession	Session handle.
 //
 LIB3270_EXPORT unsigned int lib3270_get_auto_reconnect(const H3270 *hSession);

 LIB3270_EXPORT const char * lib3270_get_connection_state_as_string(const H3270 *hSession);
 LIB3270_EXPORT const char * lib3270_get_program_message_as_string(const H3270 *hSession);
 LIB3270_EXPORT const char * lib3270_get_ssl_state_as_string(const H3270 * hSession);
 LIB3270_EXPORT const char * lib3270_get_termtype(const H3270 *hSession);
 LIB3270_EXPORT const char * lib3270_get_termname(const H3270 *hSession);

 #ifdef __cplusplus
 }
 #endif

