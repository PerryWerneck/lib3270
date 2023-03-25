#!/bin/bash

#export LIBCURL_LIBS="-L$(brew --prefix curl)/lib"
#export LIBCURL_CFLAGS="-I$(brew --prefix curl)/include"

export PKG_CONFIG_PATH="$(brew --prefix curl)/lib/pkgconfig:$(brew --prefix openssl)/lib/pkgconfig"
PROJECT_NAME=$(grep AC_INIT configure.ac | cut -d[ -f2 | cut -d] -f1)
VERSION=$(grep AC_INIT configure.ac | cut -d[ -f3 | cut -d] -f1)

echo "${PKG_CONFIG_PATH}"
pkg-config --list-all

./autogen.sh --prefix="/${PROJECT_NAME}/${VERSION}"
if [ "$?" != "0" ]; then
	exit -1
fi

make all
if [ "$?" != "0" ]; then
	exit -1
fi

echo "Build complete"

