## TN3270 Protocol Library for Linux/Windows

Created originally as part of PW3270 application.

[![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
![CodeQL](https://github.com/PerryWerneck/lib3270/workflows/CodeQL/badge.svg)
[![build result](https://build.opensuse.org/projects/home:PerryWerneck:pw3270/packages/lib3270/badge.svg?type=percent)](https://build.opensuse.org/package/show/home:PerryWerneck:pw3270/lib3270)
![Downloads](https://img.shields.io/github/downloads/PerryWerneck/lib3270/total.svg)

## Installation

### Linux

You can download installation package for supported distributions in Open Build Service.

[<img src="https://raw.githubusercontent.com/PerryWerneck/pw3270/master/branding/obs-badge-en.svg" alt="Download from open build service" height="80px">](https://software.opensuse.org/download.html?project=home%3APerryWerneck%3Apw3270&package=lib3270)

## Building for Linux

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

## Building for Windows

### Cross-compiling on SuSE Linux (Native or WSL)

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

### Windows native with MSYS2

1. Install and update MSYS2 

	* Download and install [msys2](https://www.msys2.org/)
	* Update msys:
	
	```shell
	pacman -Syu
	```
	Afther this close and reopen mingw shell.

2. Update system path

	* Add c:\msys64\usr\bin and c:\msys64\mingw64\bin to system path

3. Install devel packages using pacman on mingw shell

	```shell
	pacman -S --needed zip dos2unix mingw-w64-x86_64-gcc automake autoconf make git pkgconf mingw-w64-x86_64-gettext gettext-devel mingw-w64-x86_64-openssl libtool
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

## Building for macOS

### Using homebrew

Install

1. Install [homebrew](https://brew.sh/)

2. Install dependencies

	```shell
	brew update
	brew install xz automake binutils coreutils curl gettext libtool openssl pkgconfig
	brew upgrade
	```

4. Configure, build and install

	```shell
	export PKG_CONFIG_PATH="$(brew --prefix curl)/lib/pkgconfig:$(brew --prefix openssl)/lib/pkgconfig"
	./autogen.sh --prefix="$(brew --cellar)/lib3270/5.4"
	make all && make install
	brew link lib3270
	```

Uninstall

	```shell
	brew unlink lib3270
	rm -fr "$(brew --cellar)/lib3270"
	```
	
### Using jhbuild

1. Install jhbuild

	https://wiki.gnome.org/Projects/GTK/OSX/Building
	
2. build

	```shell
	jhbuild --moduleset=https://raw.githubusercontent.com/PerryWerneck/lib3270/master/mac/lib3270.modules build lib3270
	```


