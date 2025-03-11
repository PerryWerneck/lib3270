#!/bin/bash
#
# References:
#
#	* https://www.msys2.org/docs/ci/
#
#
echo "Running ${0}"

LOGFILE=build.log
rm -f ${LOGFILE}

die ( ) {
	[ -s $LOGFILE ] && tail $LOGFILE
	[ "$1" ] && echo "$*"
	exit -1
}

srcdir="$(dirname $(dirname $(readlink -f "${0}")))"
cd ${srcdir}
if [ "$?" != "0" ]; then
	echo "Cant cd to ${srcdir}"
	exit -1
fi

if [ -z ${PKG_CONFIG} ]; then
	PKG_CONFIG=${MINGW_PREFX}/bin/pkg-config
fi

PACKAGE_NAME=$(grep AC_INIT configure.ac | cut -d[ -f2 | cut -d] -f1)
if [ -z ${PACKAGE_NAME} ]; then
	echo "Cant determine package name"
	exit -1
fi

PACKAGE_VERSION=$(grep AC_INIT configure.ac | cut -d[ -f3 | cut -d] -f1)
if [ -z ${PACKAGE_VERSION} ]; then
	echo "Cant determine package name"
	exit -1
fi

#
# Build LIB3270
#
echo "Building lib3270"
./autogen.sh > $LOGFILE 2>&1 || die "Autogen failure"
./configure > $LOGFILE 2>&1 || die "Configure failure"
make clean > $LOGFILE 2>&1 || die "Make clean failure"
make all  > $LOGFILE 2>&1 || die "Make failure"
make DESTDIR=.bin/package.msvc install > $LOGFILE 2>&1 || die "Install failure"

cd .bin/package.msvc
zip \
	-9 -r \
	 -x'*.a' \
	 -x'*.pc' \
	 ${srcdir}/${MINGW_PACKAGE_PREFIX}-${PACKAGE_NAME}-${PACKAGE_VERSION}.devel.zip * \
	 	> $LOGFILE 2>&1 || die "Zip failure"

	