#!/bin/bash
myDIR=$(dirname $(readlink -f ${0}))

echo $myDIR

install_packages() {

	TEMPFILE=$(mktemp)
	
	for spec in $(find ${myDIR} -name "${1}*.spec")
	do
		echo "Parsing ${spec}"
		grep -i "^Requires:" "${spec}" | grep -v "%" | cut -d: -f2- | tr -d '[:blank:]' | cut -d'>' -f1 >> ${TEMPFILE}
		grep -i "^BuildRequires:" "${spec}" | grep -v "%" | cut -d: -f2- | tr -d '[:blank:]'  | cut -d'>' -f1 >> ${TEMPFILE}
	done
	
	cat ${TEMPFILE} \
		| sort --unique \
		| xargs sudo zypper --non-interactive --verbose in


}

## Instala apicativos e temas necessários
#sudo zypper --non-interactive in \
#	gettext-tools \
#	automake
#
#while read FILE
#do
#	sudo zypper --non-interactive in ${1}-${FILE}
#done < ${TEMPFILE}
#
#rm -f ${TEMPFILE}
#
#}
#

if [ -z ${1} ]; then
	echo "Use ${0} --32 for 32 bits cross-compiler"
	echo "Use ${0} --64 for 64 bits cross-compiler"
	exit -1
fi


until [ -z "${1}" ]
do
	if [ ${1:0:2} = '--' ]; then
		tmp=${1:2}
		parameter=${tmp%%=*}
		parameter=$(echo $parameter | tr "[:lower:]" "[:upper:]")

		case $parameter in

		ar)
			sudo zypper ar obs://windows:mingw:win32 mingw32
			sudo zypper ar obs://windows:mingw:win64 mingw64
			;;

		32)
			install_packages mingw32
			;;

		64)
			install_packages mingw64
			;;

		ALL)
			install_packages mingw32
			install_packages mingw64
			;;


		*)
			value=${tmp##*=}
			eval $parameter=$value
		esac

	fi

	shift
done


