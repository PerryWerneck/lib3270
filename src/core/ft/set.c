/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270. Registro no INPI sob o nome G3270.
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
 * Este programa está nomeado como set.c e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

#include <config.h>
#include <lib3270.h>
#include <lib3270/filetransfer.h>
#include <lib3270/log.h>
#include <internals.h>

/*---[ Implement ]-------------------------------------------------------------------------------------------------------*/

 LIB3270_EXPORT int	lib3270_ft_set_lrecl(H3270 *hSession, int lrecl)
 {
 	CHECK_SESSION_HANDLE(hSession);

 	if(!hSession->ft)
		return errno = EINVAL;

	hSession->ft->lrecl	= lrecl;

	return 0;
 }

 LIB3270_EXPORT int	lib3270_ft_set_blksize(H3270 *hSession, int blksize)
 {
 	CHECK_SESSION_HANDLE(hSession);

 	if(!hSession->ft)
		return errno = EINVAL;

	hSession->ft->blksize = blksize;

	return 0;
 }

 LIB3270_EXPORT int	lib3270_ft_set_primspace(H3270 *hSession, int primspace)
 {
 	CHECK_SESSION_HANDLE(hSession);

 	if(!hSession->ft)
		return errno = EINVAL;

	hSession->ft->primspace	= primspace;

	return 0;
 }

 LIB3270_EXPORT int	lib3270_ft_set_secspace(H3270 *hSession, int secspace)
 {
 	CHECK_SESSION_HANDLE(hSession);

 	if(!hSession->ft)
		return errno = EINVAL;

	hSession->ft->secspace = secspace;

	return 0;
 }

 LIB3270_EXPORT int lib3270_ft_set_options(H3270 *hSession, LIB3270_FT_OPTION options)
 {
 	CHECK_SESSION_HANDLE(hSession);

 	if(!hSession->ft)
		return errno = EINVAL;

	hSession->ft->ascii_flag	= (options & LIB3270_FT_OPTION_ASCII)	? 1 : 0;
	hSession->ft->cr_flag   	= (options & LIB3270_FT_OPTION_CRLF)	? 1 : 0;
	hSession->ft->remap_flag	= (options & LIB3270_FT_OPTION_REMAP)	? 1 : 0;
	hSession->ft->unix_text		= (options & LIB3270_FT_OPTION_UNIX)	? 1 : 0;
	hSession->ft->flags			|= options;

	return 0;
 }
