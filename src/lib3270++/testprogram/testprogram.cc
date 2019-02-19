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
 * @file src/lib3270++/testprogram/testprogram.cc
 *
 * @brief
 *
 * @author perry.werneck@gmail.com
 *
 */

 #include <cstdlib>
 #include <lib3270++.h>

 using namespace std;

/*---[ Implement ]----------------------------------------------------------------------------------*/

 int main(int argc, const char *argv[]) {

	TN3270::Host host{"pw3270:a"};

	try {

		cout
			<< "Version: " << host.getVersion()
			<< "\tRevision: " << host.getRevision()
			<< std::endl;

		cout
			<< "Connection state is " << host.getConnectionState()
			<< "\tProgram message is " << host.getProgramMessage()
			<< std::endl;

		// host.connect(getenv("LIB3270_DEFAULT_HOST"));

		if(host) {
			cout << host << endl;
		}

	} catch(const std::exception &e) {

		cerr << std::endl << e.what() << std::endl << std::endl;

	}


	return 0;
 }


