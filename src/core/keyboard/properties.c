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
 * Este programa está nomeado como - e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

#include <lib3270-internals.h>
#include <lib3270/keyboard.h>
#include <lib3270/properties.h>

LIB3270_EXPORT LIB3270_KEYBOARD_LOCK_STATE lib3270_get_keyboard_lock_state(H3270 *hSession)
{
	if(check_online_session(hSession))
		return LIB3270_KL_NOT_CONNECTED;

	return (LIB3270_KEYBOARD_LOCK_STATE) hSession->kybdlock;
}

LIB3270_EXPORT int lib3270_set_lock_on_operator_error(H3270 *hSession, int enable)
{
	hSession->oerr_lock = (enable ? 1 : 0);
	return 0;
}

int lib3270_get_lock_on_operator_error(H3270 *hSession)
{
 	return (int) hSession->oerr_lock;
}

LIB3270_EXPORT int lib3270_set_unlock_delay(H3270 *hSession, unsigned int delay)
{
	hSession->unlock_delay		= (delay == 0 ? 0 : 1);
	hSession->unlock_delay_ms 	= (unsigned short) delay;
	return 0;
}

LIB3270_EXPORT unsigned int lib3270_get_unlock_delay(H3270 *hSession)
{
	return (unsigned int) hSession->unlock_delay_ms;
}

