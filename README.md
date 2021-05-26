TN3270 Protocol Library for Linux/Windows
=========================================

Created originally as part of PW3270 application.

See more details at https://softwarepublico.gov.br/social/pw3270/

[![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
![CodeQL](https://github.com/PerryWerneck/lib3270/workflows/CodeQL/badge.svg)
![Analytics](https://ga-beacon.appspot.com/UA-35100728-2/github//lib3270)
![Downloads](https://img.shields.io/github/downloads/PerryWerneck/lib3270/total.svg)

Installation repositories
=========================

 You can find instalation repositories in SuSE Build Service:

 * Linux: https://software.opensuse.org/download.html?project=home%3APerryWerneck%3Apw3270&package=lib3270
 * Windows cross (32 bits): https://software.opensuse.org/download.html?project=home%3APerryWerneck%3Apw3270&package=mingw32-lib3270
 * Windows cross (64 bits): https://software.opensuse.org/download.html?project=home%3APerryWerneck%3Apw3270&package=mingw64-lib3270

Building for Linux
==================

1. Get lib3270 sources from git

	```shell
	git clone https://github.com/PerryWerneck/lib3270.git ./lib3270
	```

2. Install the required libraries

	* autoconf
	* automake
	* binutils
	* coreutils
	* gcc-c++
	* gettext-devel
	* m4
	* pkgconfig
	* openssl-devel
	* dbus-1-devel
	* xz

(This command can make it easy on SuSE: grep -i buildrequires rpm/lib3270.spec | cut -d: -f2 | sudo xargs zypper in )

3. Configure and build

	```shell
	./autogen.sh
	make clean
	make all
	```

Building for Windows
====================

Cross-compiling on SuSE Linux (Native or WSL)
---------------------------------------------

1. First add the MinGW Repositories for your SuSE version from:

	```shell
	sudo zypper ar obs://windows:mingw:win32 mingw32
	sudo zypper ar obs://windows:mingw:win64 mingw64
	sudo zypper ref
	```

2. Get lib3270 sources from git

	```shell
	git clone https://github.com/PerryWerneck/lib3270.git ./lib3270
	```

3. Install cross compilers

	```shell
	./lib3270/win/install-cross.sh --all (for 32 and 64 bits)
	```

3. Configure build

	```shell
	./lib3270/win/win-configure.sh --64 (for 64 bits)
	```

4. Build

	```shell
	cd lib3270
	make clean
	make all
	```

Windows native with MSYS2
-------------------------

1. Install and update MSYS2 

	* Download and install msys2 from https://www.msys2.org/ (Don't forget to update the package database and core system packages using pacman -Syu)

2. Update system path

	* Add c:\msys64\usr\bin and c:\msys64\mingw64\bin to system path

3. Install devel packages using pacman on mingw shell

	```shell
	pacman -S --needed mingw-w64-x86_64-gcc automake autoconf make git pkg-config mingw-w64-x86_64-gettext mingw-w64-x86_64-openssl libtool
	```
	Afther this close and reopen mingw shell.

4. Get lib3270 sources from git using the mingw shell

	```shell
	git clone https://github.com/PerryWerneck/lib3270.git ./lib3270
	```

5. Build library using the mingw shell

	```shell
	cd lib3270
	./autogen.sh
	make all
	```

6. Install

	```shell
	make install
	```

Building for macOS (using homebrew)
===================================

1. Install [homebrew](https://brew.sh/)

2. Install dependencies

	```shell
	brew install automake binutils coreutils curl gettext libtool openldap openssl pkgconfig
	```

3. Use [open-keg](https://gist.github.com/andrebreves/5f36e78575e20162ed0a62bd27c4bcea) to make keg-only dependencies available during build process

	```shell
	open-keg curl openldap openssl
	```

4. Configure, build and install (inside the [open-keg](https://gist.github.com/andrebreves/5f36e78575e20162ed0a62bd27c4bcea) shell opened above)

	```shell
	./autogen.sh --prefix="$(brew --cellar)/lib3270/5.3"
	make all && make install
	$ brew link lib3270
	```

Uninstalling
------------

1. To uninstall

	```shell
	brew unlink lib3270
	rm -fr "$(brew --cellar)/lib3270"
	```
