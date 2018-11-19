/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
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
 * Este programa está nomeado como lib3270++.h e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

/**
 * @file src/lib3270++/abstract.cc
 *
 * @brief
 *
 * @author perry.werneck@gmail.com
 *
 */

 #include "private.h"
 #include <cstring>


/*---[ Implement ]----------------------------------------------------------------------------------*/

 namespace TN3270 {

	Abstract::Session::Session() {

#ifdef HAVE_ICONV
		this->converter.local	= (iconv_t) (-1);
		this->converter.host	= (iconv_t) (-1);
#endif

		this->baddr = 0;

	}

	Abstract::Session::~Session() {

#ifdef HAVE_ICONV

		if(this->converter.local != (iconv_t) (-1))
			iconv_close(this->converter.local);

		if(this->converter.host != (iconv_t) (-1))
			iconv_close(this->converter.host);

#endif

	}

	/// @brief Setup charsets
	void Abstract::Session::setCharSet(const char *remote, const char *local) {

#ifdef HAVE_ICONV

		if(this->converter.local != (iconv_t) (-1))
			iconv_close(converter.local);

		if(this->converter.host != (iconv_t) (-1))
			iconv_close(converter.host);

		if(strcmp(local,remote)) {

			// Local and remote charsets aren't the same, setup conversion
			converter.local	= iconv_open(local, remote);
			converter.host	= iconv_open(remote,local);

		} else {
			// Same charset, doesn't convert
			converter.local = converter.host = (iconv_t)(-1);
		}

#else

		#error No ICONV Support

#endif


	}

	/// @brief Converte charset.
	std::string Abstract::Session::convertCharset(iconv_t &converter, const char *str) {

		std::string rc;

#ifdef HAVE_ICONV
		size_t in = strlen(str);

		if(in && converter != (iconv_t)(-1)) {

			size_t	  out 		= (in << 1);
			char	* ptr;
			char	* outBuffer = (char *) malloc(out);
			char	* inBuffer	= (char	*) str;

			memset(ptr=outBuffer,0,out);

			iconv(converter,NULL,NULL,NULL,NULL);	// Reset state

			if(iconv(converter,&inBuffer,&in,&ptr,&out) != ((size_t) -1))
				rc.assign(outBuffer);

			free(outBuffer);

		}

#else

		rc = str;

#endif // HAVE_ICONV

		return rc;
	}

	/// @brief Converte string recebida do host para o charset atual.
	std::string Abstract::Session::convertFromHost(const char *str) const {
		return convertCharset(const_cast<Abstract::Session *>(this)->converter.local,str);
	}

	/// @brief Converte string do charset atual para o charset do host.
	std::string Abstract::Session::convertToHost(const char *str) const {
		return convertCharset(const_cast<Abstract::Session *>(this)->converter.host,str);
	}


 }




