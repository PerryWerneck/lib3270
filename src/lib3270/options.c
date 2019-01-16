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
 * Este programa está nomeado como options.c e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 * licinio@bb.com.br		(Licínio Luis Branco)
 * kraucer@bb.com.br		(Kraucer Fernandes Mazuco)
 *
 */

#include "private.h"

/*---[ Globals ]--------------------------------------------------------------------------------------------------------------*/

/*---[ Statics ]--------------------------------------------------------------------------------------------------------------*/

 static const LIB3270_HOST_TYPE_ENTRY host_type[] =
 {
	{
		LIB3270_HOST_S390,
		"S390",
		N_( "IBM S/390" ),
		NULL
	},
	{
		LIB3270_HOST_AS400,
		"AS400",
		N_( "IBM AS/400" ),
		NULL
	},
	{
		LIB3270_HOST_TSO,
		"TSO",
		N_( "Other (TSO)" ),
		NULL
	},
	{
		0,
		"VM/CMS",
		N_( "Other (VM/CMS)"	),
		NULL
	},

	{
		0,
		NULL,
		NULL,
		NULL
	}
 };


/*---[ Implement ]------------------------------------------------------------------------------------------------------------*/

LIB3270_EXPORT LIB3270_HOST_TYPE lib3270_get_host_type(H3270 *hSession)
{
	CHECK_SESSION_HANDLE(hSession);
	return hSession->host_type;
}

LIB3270_EXPORT int lib3270_set_host_type(H3270 *hSession, LIB3270_HOST_TYPE opt)
{
    FAIL_IF_ONLINE(hSession);
	hSession->host_type = opt;
	return 0;
}

LIB3270_EXPORT int lib3270_get_color_type(H3270 *hSession)
{
	CHECK_SESSION_HANDLE(hSession);
	return (int) (hSession->mono ? 2 : hSession->colors);
}

LIB3270_EXPORT int lib3270_set_color_type(H3270 *hSession, int colortype)
{
	CHECK_SESSION_HANDLE(hSession);

	if(hSession->cstate != LIB3270_NOT_CONNECTED)
		return errno = EBUSY;

	switch(colortype)
	{
	case 0:
	case 16:
		hSession->colors 	= 16;
		hSession->mono		= 0;
		hSession->m3279		= 1;
		break;

	case 8:
		hSession->colors	= 8;
		hSession->mono		= 0;
		hSession->m3279		= 1;
		break;

	case 2:
		hSession->colors 	= 16;
		hSession->mono		= 1;
		hSession->m3279		= 0;
		break;

	default:
		return errno = EINVAL;
	}


	return 0;
}


LIB3270_EXPORT const LIB3270_HOST_TYPE_ENTRY * lib3270_get_option_list(void)
{
	return host_type;
}

LIB3270_EXPORT int lib3270_is_tso(H3270 *hSession)
{
	CHECK_SESSION_HANDLE(hSession);
	return (hSession->host_type & LIB3270_HOST_TSO) != 0;
}

LIB3270_EXPORT int lib3270_set_tso(H3270 *hSession, int on)
{
    FAIL_IF_ONLINE(hSession);

    if(on)
		hSession->host_type = LIB3270_HOST_TSO;
	else
		hSession->host_type &= ~LIB3270_HOST_TSO;

	return 0;
}

LIB3270_EXPORT int lib3270_is_as400(H3270 *hSession)
{
	CHECK_SESSION_HANDLE(hSession);
	return (hSession->host_type & LIB3270_HOST_AS400) != 0;
}

LIB3270_EXPORT int lib3270_set_as400(H3270 *hSession, int on)
{
    FAIL_IF_ONLINE(hSession);

    if(on)
		hSession->host_type |= LIB3270_HOST_AS400;
	else
		hSession->host_type &= ~LIB3270_HOST_AS400;

	return 0;
}

LIB3270_EXPORT LIB3270_HOST_TYPE lib3270_parse_host_type(const char *name)
{

	int f;

	for(f=0;host_type[f].name;f++)
	{
		if(!strcasecmp(host_type[f].name,name))
			return host_type[f].type;
	}

	errno = ENOENT;
	return 0;
}

LIB3270_EXPORT int lib3270_set_host_type_by_name(H3270 *hSession, const char *name)
{
	FAIL_IF_ONLINE(hSession);

	size_t f;
	for(f=0;f<(sizeof(host_type)/sizeof(host_type[0]));f++)
	{
		if(host_type[f].name && !strcasecmp(host_type[f].name,name))
		{
			hSession->host_type = host_type[f].type;
			return 0;
		}
	}

	return errno = EINVAL;
}

LIB3270_EXPORT const char * lib3270_get_host_type_name(H3270 *hSession)
{
	size_t f;

	for(f=0;f<(sizeof(host_type)/sizeof(host_type[0]));f++)
	{
		if(hSession->host_type == host_type[f].type)
		{
			return host_type[f].name;
		}
	}

	errno = EINVAL;
	return "";

}
