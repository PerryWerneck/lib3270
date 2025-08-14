## TN3270 Protocol Library for Linux/Windows

Created originally as part of PW3270 application.

[![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
![CodeQL](https://github.com/PerryWerneck/lib3270/workflows/CodeQL/badge.svg)
[![build result](https://build.opensuse.org/projects/home:PerryWerneck:pw3270/packages/lib3270/badge.svg?type=percent)](https://build.opensuse.org/package/show/home:PerryWerneck:pw3270/lib3270)
[![Publish](https://github.com/PerryWerneck/lib3270/actions/workflows/publish.yml/badge.svg)](https://github.com/PerryWerneck/lib3270/actions/workflows/publish.yml)
![Downloads](https://img.shields.io/github/downloads/PerryWerneck/lib3270/total.svg)

## Installation

### Pre build packages

You can download installation package for supported linux distributions in [Open Build Service](https://software.opensuse.org/download.html?project=home%3APerryWerneck%3Apw3270&package=lib3270)

[<img src="https://raw.githubusercontent.com/PerryWerneck/pw3270/develop/branding/obs-badge-en.svg" alt="Download from open build service" height="80px">](https://software.opensuse.org/download.html?project=home%3APerryWerneck%3Apw3270&package=pw3270)
[<img src="https://raw.githubusercontent.com/PerryWerneck/PerryWerneck/3aa96b8275d4310896c3a0b5b3965ed650fb7c2b/badges/github-msys-macos.svg" alt="Download from githut" height="80px">](https://github.com/PerryWerneck/lib3270/releases)

## Building for Linux

1. Get lib3270 sources from git

	```shell
	git clone https://github.com/PerryWerneck/lib3270.git ./lib3270
	```

2. Install the required libraries

  	* pkgconfig
  	* gettext-devel
  	* curl
  	* meson
	* gcc-c++
	* openssl-devel
	* dbus-1-devel
	* xz

	(This command can make it easy on SuSE: grep -i buildrequires rpm/lib3270.spec | cut -d: -f2 | sudo xargs zypper in )

3. Setup, build and install

	```shell
	meson setup .build
	meson compile -C .build
	meson install -C .build
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
	zypper in \
		pkgconfig \
		gettext-devel \
		mingw64-libcurl-devel \
		mingw64-cross-meson \
		mingw64-libopenssl-devel \
		mingw64-cross-gcc-c++
	```

3. Configure and build

	```shell
	meson setup --cross-file /usr/lib/rpm/macros.d/meson-mingw64-cross-file.txt .build
	meson compile -C .build
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
	pacman -S \
		dos2unix \
		mingw-w64-x86_64-gcc \
		mingw-w64-x86_64-meson \
		mingw-w64-x86_64-iconv \
		pkgconf \
		mingw-w64-x86_64-gettext \
		gettext-devel \
		mingw-w64-x86_64-openssl
	```

	Afther this close and reopen mingw shell.

4. Get lib3270 sources from git using the mingw shell

	```shell
	git clone https://github.com/PerryWerneck/lib3270.git ./lib3270
	```

5. Build with packman

	```shell
	makepkg BUILDDIR=/tmp/pkg -p PKGBUILD.mingw
	```

## Building for macOS

### Using homebrew

Install

1. Install [homebrew](https://brew.sh/)

2. Install dependencies

	```shell
	brew update
	brew install xz meson ninja curl gettext openssl pkgconfig
	brew upgrade
	```

3. Get lib3270 sources from git

	```shell
	git clone https://github.com/PerryWerneck/lib3270.git ./lib3270
	cd lib3270
	```

4. Configure, build and install

	```shell
	export PKG_CONFIG_PATH="$(brew --prefix curl)/lib/pkgconfig:$(brew --prefix openssl)/lib/pkgconfig"
	meson setup --prefix=$(brew --prefix)/Cellar/lib3270/$(grep 'version:' meson.build | cut -d: -f2 | cut -d\' -f2) --reconfigure --wipe .build
	meson compile -C .build
	meson install -C .build
	brew link lib3270
	```

Uninstall

	```shell
	brew unlink lib3270
	rm -fr "$(brew --cellar)/lib3270"
	```
	
