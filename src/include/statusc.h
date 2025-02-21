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
 * Este programa está nomeado como host.c e possui 1078 linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

LIB3270_INTERNAL void 		status_compose(int on, unsigned char c, enum keytype keytype);
LIB3270_INTERNAL void 		status_ctlr_done(H3270 *session);

LIB3270_INTERNAL void 		status_oerr(H3270 *session, int error_type);
LIB3270_INTERNAL void 		status_reset(H3270 *session);
LIB3270_INTERNAL void 		status_twait(H3270 *session);

LIB3270_INTERNAL void 		message_changed(H3270 *session, LIB3270_MESSAGE id);
LIB3270_INTERNAL void 		set_status(H3270 *session, LIB3270_FLAG id, Boolean on);


#define status_typeahead(h,on)	set_status(h,LIB3270_FLAG_TYPEAHEAD,on)

