#!/bin/bash

echo "------------------ CURL"
brew --prefix curl

echo "------------------ OpenSSL "
brew --prefix openssl

PROJECT_NAME=$(grep AC_INIT configure.ac | cut -d[ -f2 | cut -d] -f1)
VERSION=$(grep AC_INIT configure.ac | cut -d[ -f3 | cut -d] -f1)

./autogen.sh --prefix="/${PROJECT_NAME}/${VERSION}"
if [ "$?" != "0" ]; then
	exit -1
fi

make all
./autogen.sh
if [ "$?" != "0" ]; then
	exit -1
fi

echo "Build complete"

