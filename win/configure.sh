#!/bin/bash

aclocal
if test $? != 0 ; then
	echo "aclocal failed."
	exit -1
fi

autoconf
if test $? != 0 ; then
	echo "autoconf failed."
	exit -1
fi

mkdir -p scripts
automake --add-missing 2> /dev/null | true

export HOST_CC=/usr/bin/gcc

until [ -z "${1}" ]
do
	if [ ${1:0:2} = '--' ]; then
		tmp=${1:2}
		parameter=${tmp%%=*}
		parameter=$(echo $parameter | tr "[:lower:]" "[:upper:]")

		case $parameter in

		32)
			rm -f win32.cache
			./configure \
				--cache-file=win32.cache \
				--host=i686-w64-mingw32 \
				--prefix=/usr/i686-w64-mingw32/sys-root/mingw \
				--libdir=/usr/i686-w64-mingw32/sys-root/mingw/lib

			exit $?
			;;

		64)
			rm -f win64.cache
			./configure \
				--cache-file=win64.cache \
			        --host=x86_64-w64-mingw32 \
			        --prefix=/usr/x86_64-w64-mingw32/sys-root/mingw \
			        --libdir=/usr/x86_64-w64-mingw32/sys-root/mingw/lib
			exit $?
			;;

		ALL)
			;;


		*)
			value=${tmp##*=}
			eval $parameter=$value
		esac

	fi

	shift
done

echo "Execute:"
echo "	${0} --32 for 32 bits windows."
echo "	${0} --64 for 64 bits windows."

exit -1

