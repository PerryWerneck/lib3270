/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270. Registro no INPI sob
 * o nome G3270.
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

#include <config.h>
#include <internals.h>
#include <string.h>
#include <lib3270.h>
#include <lib3270/filetransfer.h>

/*--[ Globals ]--------------------------------------------------------------------------------------*/

// http://www3.rocketsoftware.com/bluezone/help/v42/en/bz/DISPLAY/IND$FILE/IND$FILE_Technical_Reference.htm
static const LIB3270_FT_MESSAGE ftmsg[] = {

	{
		"TRANS00",
		1,
		N_("Error in file transfer: file transfer canceled"),
		N_("An error occurred in the file transfer, which may be an error in the data being transferred, or an unidentified system error.")
	},

	{
		"TRANS03",
		0,
		N_( "File transfer complete" ),
		N_("The file transfer operation has been successfully completed.")
	},

	{
		"TRANS04",
		0,
		N_("File transfer complete with records segmented"),
		N_("The file transfer operation has been completed, and any record greater than the logical record length (LRECL) of the file being appended has been divided and becomes multiple records.")
	},

	{
		"TRANS05",
		1,
		N_("Personal computer filespec incorrect: file transfer canceled"),
		N_("An error exists in the PC's file name.")
	},

	{
		"TRANS06",
		1,
		N_("Command incomplete: file transfer canceled"),
		N_("You did not enter the required parameters after a SEND or RECEIVE command.")
	},

	{
		"TRANS13",
		1,
		N_("Error writing file to host: file transfer canceled"),
		N_("The host program has detected an error in the file data during a RECEIVE operation.")
	},

	{
		"TRANS14",
		1,
		N_("Error reading file from host: file transfer canceled"),
		N_("The host program has detected an error in the file data during a RECEIVE operation.")
	},

	{
		"TRANS15",
		1,
		N_("Required host storage unavailable: file transfer canceled"),
		N_("You need 30 KB of main storage (not disk space) on the host for the file transfer, in addition to the host requirement.")
	},

	{
		"TRANS16",
		1,
		N_("Incorrect request code: file transfer canceled"),
		N_("An invalid SEND or RECEIVE parameter was sent to the host.")
	},

	{
		"TRANS17",
		1,
		NULL,
		NULL
	},

	/*
	{
		"TRANS17",
		1,
		N_("Missing or incorrect TSO data set name: file transfer canceled"),
		N_("You did not specify the TSO data set name, or the specified TSO data set name is not a sequential or partitioned data set.")
	},

	{
		"TRANS17",
		1,
		N_("Invalid file name: file transfer canceled"),
		N_("The command that started the file transfer specified a file name that is not a valid file name for the host. Correct CICS file names must be 1 to 8 characters, composed of letters and numbers.")
	},

	{
		"TRANS17",
		1,
		N_("Missing or incorrect CMS data set name: file transfer canceled"),
		N_("You did not specify the CMS data set name, or the specified CMS data set name is incorrect.")
	},
	*/

	{
		"TRANS18",
		1,
		N_("Incorrect option specified: file transfer canceled"),
		N_("You specified an option that is invalid.")
	},

	{
		"TRANS19",
		1,
		N_("Error while reading or writing to host disk: file transfer canceled"),
		N_("There is not enough space available for data on the host.")
	},

	{
		"TRANS28",
		1,
		N_("Invalid option xxxxxxxx: file transfer canceled"),
		N_("You selected an option that is either not recognized, is specified as a positional keyword, or has an associated value that is incorrect.")
	},

	{
		"TRANS29",
		1,
		N_("Invalid option xxxxxxxx with RECEIVE: file transfer canceled"),
		N_("You selected an option that is not valid with RECEIVE, but can be used with SEND.")
	},

	{
		"TRANS30",
		1,
		N_("Invalid option xxxxxxxx with APPEND: file transfer canceled"),
		N_("You selected an option that is not valid with APPEND, but otherwise may be used.")
	},

	{
		"TRANS31",
		1,
		N_("Invalid option xxxxxxxx without SPACE: file transfer canceled"),
		N_("You selected an option that can only be used if SPACE is also specified.")
	},

	{
		"TRANS32",
		1,
		N_("Invalid option xxxxxxxx with PDS: file transfer canceled"),
		N_("You selected an option that is invalid with a host-partitioned data set.")
	},

	{
		"TRANS33",
		1,
		N_("Only one of TRACKS, CYLINDERS, AVBLOCK allowed: file transfer canceled"),
		N_("SPACE can be specified in units of TRACKS, CYLINDERS, or AVBLOCK, and only one option can be used.")
	},

	{
		"TRANS34",
		1,
		N_("CMS file not found: file transfer canceled"),
		N_("You did not specify an existing CMS file for RECEIVE.")
	},

	{
		"TRANS35",
		1,
		N_("CMS disk is read-only: file transfer canceled"),
		N_("You specified a CMS file mode for the SEND key that does not allow write access.")
	},

	{
		"TRANS36",
		1,
		N_("CMS disk is not accessed: file transfer canceled"),
		N_("You specified a CMS file mode that is not in the CMS search order.")
	},

	{
		"TRANS37",
		1,
		N_("CMS disk is full: file transfer canceled"),
		N_("The CMS disk is full, or the maximum number of files on the minidisk (3400) has been reached, or the maximum number of data blocks per file (16060) has been reached.")
	},

	{
		"TRANS99",
		1,
		N_("Host program error code xxxxxxxxxx: file transfer canceled"),
		N_("This is a host program error.")
	},

};

/*--[ Implement ]------------------------------------------------------------------------------------*/

LIB3270_EXPORT const LIB3270_FT_MESSAGE * lib3270_translate_ft_message(const char *msg) {
	size_t ix;

	for(ix = 0; ix < (sizeof(ftmsg)/sizeof(ftmsg[0])); ix++) {
		if(strncasecmp(msg,ftmsg[ix].id,7) == 0) {
			return &ftmsg[ix];
		}
	}

	return NULL;

}
