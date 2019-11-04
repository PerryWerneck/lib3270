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

 #include <internals.h>
 #include "screen.h"
 #include "ctlrc.h"
 #include "popupsc.h"
 #include <lib3270/trace.h>
 #include <lib3270/log.h>
 #include <lib3270/properties.h>

 const char * lib3270_get_oversize(const H3270 *hSession)
 {
 	return hSession->oversize.str;
 }

 int lib3270_set_oversize(H3270 *hSession, const char *value)
 {
	if(hSession->connection.state != LIB3270_NOT_CONNECTED)
		return errno = EISCONN;

	if(!hSession->extended)
		return errno = ENOTSUP;

	if(hSession->oversize.str)
	{
		// Do nothing if it's the same value!
		if(value && !strcasecmp(hSession->oversize.str,value))
			return 0;

		lib3270_free(hSession->oversize.str);
		hSession->oversize.str = NULL;
	}

	int ovc = 0, ovr = 0;

	if(value)
	{
		char junk;

		if(sscanf(value, "%dx%d%c", &ovc, &ovr, &junk) != 2)
			return errno = EINVAL;

		hSession->oversize.str = lib3270_strdup(value);

	}

	ctlr_set_rows_cols(hSession, hSession->model_num, ovc, ovr);
	ctlr_model_changed(hSession);
	screen_update(hSession,0,hSession->view.rows*hSession->view.cols);

	return 0;

 }

/**
 * @brief Get current 3270 model.
 *
 * @param hSession selected 3270 session.
 * @return Current model number.
 */
unsigned int lib3270_get_model_number(const H3270 *hSession)
{
	return hSession->model_num;
}

const char * lib3270_get_model(const H3270 *hSession)
{
	return hSession->model_name;
}

const char * lib3270_get_model_name(const H3270 *hSession)
{
	return hSession->model_name;
}

 /**
  * @brief Parse the model number.
  *
  * @param session	Session Handle.
  * @param m		Model number (NULL for "2").
  *
  * @return -1 (error), 0 (default), or the specified number.
  */
static int parse_model_number(H3270 *session, const char *m)
{
	int sl;
	int n;

	if(!m)
		m = "2";

	sl = strlen(m);

	// An empty model number is no good.
	if (!sl)
		return 0;

	if (sl > 1) {

		// If it's longer than one character, it needs to start with
		// '327[89]', and it sets the m3279 resource.

		if (!strncmp(m, "3278", 4))
		{
			session->m3279 = 0;
		}
		else if (!strncmp(m, "3279", 4))
		{
			session->m3279 = 1;
		}
		else
		{
			return -1;
		}
		m += 4;
		sl -= 4;

		// Check more syntax.  -E is allowed, but ignored.
		switch (m[0]) {
		case '\0':
			// Use default model number.
			return 0;
		case '-':
			// Model number specified.
			m++;
			sl--;
			break;
		default:
			return -1;
		}
		switch (sl) {
		case 1: // n
			break;
		case 3:	// n-E
			if (strcasecmp(m + 1, "-E")) {
				return -1;
			}
			break;
		default:
			return -1;
		}
	}

	// Check the numeric model number.
	n = atoi(m);
	if (n >= 2 && n <= 5) {
		return n;
	} else {
		return -1;
	}

}

int lib3270_set_model_name(H3270 *hSession, const char *model_name)
{
	return lib3270_set_model_number(hSession,parse_model_number(hSession, model_name));
}

int lib3270_set_model(H3270 *hSession, const char *model_name)
{
	return lib3270_set_model_number(hSession,parse_model_number(hSession, model_name));
}

int lib3270_set_model_number(H3270 *hSession, unsigned int model_number)
{
	if(hSession->connection.state != LIB3270_NOT_CONNECTED)
		return errno = EISCONN;

	strncpy(hSession->full_model_name,"IBM-",LIB3270_FULL_MODEL_NAME_LENGTH);
	hSession->model_name = &hSession->full_model_name[4];

	if (!model_number)
	{
#if defined(RESTRICT_3279)
		model_number = 3;
#else
		model_number = 4;
#endif
	}

	if(hSession->mono)
		hSession->m3279 = 0;
	else
		hSession->m3279 = 1;

	if(!hSession->extended)
	{
		if(hSession->oversize.str)
			lib3270_free(hSession->oversize.str);
		hSession->oversize.str = CN;
	}

#if defined(RESTRICT_3279)
	if (hSession->m3279 && model_number == 4)
		model_number = 3;
#endif

	trace("Model_number: %d",model_number);

	// Check for oversize
	char junk;
	int ovc, ovr;

	if (!hSession->extended || hSession->oversize.str == CN || sscanf(hSession->oversize.str, "%dx%d%c", &ovc, &ovr, &junk) != 2)
	{
		ovc = 0;
		ovr = 0;
	}

	ctlr_set_rows_cols(hSession, model_number, ovc, ovr);
	ctlr_model_changed(hSession);
	screen_update(hSession,0,hSession->view.rows*hSession->view.cols);

	return 0;
}

