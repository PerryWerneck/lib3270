/*
 * "Software PW3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes paul.mattes@case.edu), de emulação de terminal 3270 para acesso a
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
 * Este programa está nomeado como popupsc.h e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas de Mendonça)
 * licinio@bb.com.br		(Licínio Luis Branco)
 * kraucer@bb.com.br		(Kraucer Fernandes Mazuco)
 *
 */

/**
 * @brief Popup calls
 *
 */

#include <lib3270.h>

#define popup_an_errno(hSession, errn, fmt, ...) lib3270_popup_an_errno(hSession, errn, fmt, __VA_ARGS__)

LIB3270_INTERNAL void popup_an_error(H3270 *session, const char *fmt, ...) LIB3270_GNUC_FORMAT(2,3);
LIB3270_INTERNAL void popup_system_error(H3270 *session, const char *title, const char *message, const char *fmt, ...) LIB3270_GNUC_FORMAT(4,5);
LIB3270_INTERNAL void popup_a_sockerr(H3270 *session, char *fmt, ...) LIB3270_GNUC_FORMAT(2,3);

LIB3270_INTERNAL void Error(H3270 *session, const char *fmt, ...);
LIB3270_INTERNAL void Warning(H3270 *session, const char *fmt, ...);
