TN3270 Protocol Library for Linux/Windows
=========================================

Created originally as part of PW3270 application.

See more details at https://softwarepublico.gov.br/social/pw3270/

Installation repositories
=========================

 You can find instalation repositories in SuSE Build Service:

 * Linux (Many distributions): https://build.opensuse.org/project/show/home:PerryWerneck:pw3270
 * Windows 32 bits: https://build.opensuse.org/project/show/home:PerryWerneck:mingw32
 * Windows 64 bits: https://build.opensuse.org/project/show/home:PerryWerneck:mingw64

Building for Linux
==================


Cross-compiling for Windows
===========================

Cross-compiling on SuSE Linux (Native or WSL)
---------------------------------------------

1. First add the MinGW Repositories for your SuSE version from:

	* 32 bits: https://build.opensuse.org/project/show/windows:mingw:win32
	* 64 bits: https://build.opensuse.org/project/show/windows:mingw:win64

2. Get lib3270 sources from git

	* git clone http://softwarepublico.gov.br/gitlab/pw3270/lib3270.git ./lib3270

3. Install cross compilers

	* ./lib3270/win/install-cross.sh --32 (for 32 bits)
	* ./lib3270/win/install-cross.sh --64 (for 64 bits)
	* ./lib3270/win/install-cross.sh --all (for 32 and 64 bits)

3. Configure build

	* ./lib3270/win/win-configure.sh --32 (for 32 bits)
	* ./lib3270/win/win-configure.sh --64 (for 64 bits)


Compiling for Windows (With MSYS2)
-------------------------------

1. Install MSYS2 

	* Get installer from https://www.msys2.org/
	* Run "pacman -Syu" repeatedly

2. Install devel packages

	* pacman -S --needed mingw-w64-x86_64-toolchain automake autoconf make

