#!/bin/bash

export LIBCURL_LIBS="-L/usr/local/opt/curl/lib"
export LIBCURL_CFLAGS="-I/usr/local/opt/curl/include"

echo "------------------ CURL"
brew --prefix curl

echo "------------------ OpenSSL "
brew --prefix openssl

./autogen.sh
if [ "$?" != "0" ]; then
	exit -1
fi

make all
./autogen.sh
if [ "$?" != "0" ]; then
	exit -1
fi

echo "Build complete"

