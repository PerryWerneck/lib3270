dnl
dnl Software pw3270, desenvolvido com base nos códigos fontes do  C3270  e  X3270
dnl (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
dnl aplicativos mainframe. Registro no INPI sob o nome G3270.
dnl
dnl Copyright (C) <2008> <Banco do Brasil S.A.>
dnl
dnl Este programa é software livre. Você pode redistribuí-lo e/ou modificá-lo sob
dnl os termos da GPL v.2 - Licença Pública Geral  GNU,  conforme  publicado  pela
dnl Free Software Foundation.
dnl
dnl Este programa é distribuído na expectativa de  ser  útil,  mas  SEM  QUALQUER
dnl GARANTIA; sem mesmo a garantia implícita de COMERCIALIZAÇÃO ou  de  ADEQUAÇÃO
dnl A QUALQUER PROPÓSITO EM PARTICULAR. Consulte a Licença Pública Geral GNU para
dnl obter mais detalhes.
dnl
dnl Você deve ter recebido uma cópia da Licença Pública Geral GNU junto com este
dnl programa;  se  não, escreva para a Free Software Foundation, Inc., 59 Temple
dnl Place, Suite 330, Boston, MA, 02111-1307, USA
dnl
dnl Contatos:
dnl
dnl perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
dnl erico.mendonca@gmail.com	(Erico Mascarenhas de Mendonça)
dnl

dnl Process this file with autoconf to produce a configure script.

dnl The minimum version of autoconf required.
AC_PREREQ(2.61)

dnl Initialise automake with the package name, version and
dnl bug-reporting address.
AC_INIT([lib3270], [5.2], [perry.werneck@gmail.com])

dnl Place auxilliary scripts here.
AC_CONFIG_AUX_DIR([scripts])

dnl Compute the canonical host-system type
AC_CANONICAL_HOST

dnl Put macro definitions here (though they aren't used).
AC_CONFIG_HEADER([src/include/config.h])

dnl Initialise automake stuff.
AM_INIT_AUTOMAKE

dnl Check for iconv
AM_ICONV

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_SED
AC_PROG_LN_S
AC_PATH_TOOL([AR], [ar], [ar])
AC_PATH_TOOL([DLLTOOL], [dlltool], [dlltool])

AC_LANG([C])

dnl ---------------------------------------------------------------------------
dnl Initialize defaults
dnl ---------------------------------------------------------------------------

DBG_CFLAGS="-g -fstack-check -DDEBUG=1"
RLS_CFLAGS="-DNDEBUG=1"
APP_RESOURCES=""
APP_LDFLAGS=""
DLL_LDFLAGS="-shared -Wl,-soname,\$(@F)"
STATIC_LDFLAGS=""
INSTALL_PACKAGES="dev"

dnl ---------------------------------------------------------------------------
dnl Check for OS specifics
dnl ---------------------------------------------------------------------------

DLLPREFIX="lib"

case "$host" in
	*-mingw32|*-pc-msys)
		app_cv_osname="windows"
		CFLAGS="$CFLAGS -pthread -D_WIN32_WINNT=0x0600"
		LIBS="$LIBS -lws2_32 -lwtsapi32 -lcomdlg32 -lwinhttp"
		LDFLAGS="$LDFLAGS -pthread"
		DLL_LDFLAGS="-shared -Wl,--output-def,\$(@D)/\$(LIBNAME).def"
		DLLEXT=".dll"

		INSTALL_PACKAGES="windows-lib ${INSTALL_PACKAGES}"

		app_win32_revision=$(date +%-y.%-m.%-d.%-H)
		AC_SUBST(WIN32_VERSION,$app_win32_revision)

		app_win32_file_version=$(date +%-y,%-m,%-d,%-H)
		AC_SUBST(WIN32_FILE_VERSION,$app_win32_file_version)

		AC_CONFIG_FILES(src/core/windows/resources.rc)

		app_cv_static='yes'

		AC_CONFIG_FILES(sdk/lib3270-delayed.pc)
		INSTALL_PACKAGES="$INSTALL_PACKAGES delayed"

		AC_CONFIG_FILES(win/lib3270.mak)

		;;

 	*)
		CFLAGS="$CFLAGS -pthread -DCONFDIR=\$(confdir) -DDATADIR=\$(datadir)"
		LDFLAGS="$LDFLAGS -pthread"
		app_cv_osname="linux"
		LOGDIR="/var/log"
		DLLEXT=".so"

		INSTALL_PACKAGES="linux-lib ${INSTALL_PACKAGES}"

		app_cv_static='no'

esac

AC_SUBST(OSNAME,$app_cv_osname)
AC_SUBST(LIBS)
AC_SUBST(LOGDIR)
AC_SUBST(DLLEXT)
AC_SUBST(DLLPREFIX)
AC_SUBST(DLL_LDFLAGS)

dnl ---------------------------------------------------------------------------
dnl Check host compiler
dnl ---------------------------------------------------------------------------

if test $host != $build; then
	AC_CHECK_PROGS(HOST_CC, gcc cc)
else
	HOST_CC=$CC
fi

AC_SUBST(HOST_CC)

dnl ---------------------------------------------------------------------------
dnl Check for other programs
dnl ---------------------------------------------------------------------------

AC_PATH_TOOL([AR], [ar], [ar])
AC_PATH_TOOL([WINDRES], [windres], [no])

PKG_CHECK_EXISTS

dnl ---------------------------------------------------------------------------
dnl Version info
dnl ---------------------------------------------------------------------------

AC_ARG_WITH([product-name], [AS_HELP_STRING([--with-product-name], [Set product name])], [ app_cv_product="$withval" ],[ app_cv_product="pw3270" ])

AC_DEFINE_UNQUOTED(PRODUCT_NAME, $app_cv_product)
AC_SUBST(PRODUCT_NAME,$app_cv_product)

AC_SUBST(PACKAGE_DESCRIPTION,"3270 C Access Library")

app_vrs_major=$(echo $VERSION | cut -d. -f1)
app_vrs_minor=$(echo $VERSION | cut -d. -f2)

AC_DEFINE_UNQUOTED(PACKAGE_MAJOR_VERSION, $app_vrs_major)
AC_DEFINE_UNQUOTED(PACKAGE_MINOR_VERSION, $app_vrs_minor)

AC_SUBST(PACKAGE_MAJOR_VERSION,$app_vrs_major)
AC_SUBST(PACKAGE_MINOR_VERSION,$app_vrs_minor)

AC_ARG_WITH([release], [AS_HELP_STRING([--with-release], [Set release])], [ app_cv_release="$withval" ],[ app_cv_release="`date +%-y`.`date +%-m`.`date +%-d`" ])

app_rls_major=$(echo $app_cv_release.0.0 | cut -d. -f1)
app_rls_minor=$(echo $app_cv_release.0.0 | cut -d. -f2)

AC_DEFINE_UNQUOTED(PACKAGE_MAJOR_RELEASE, $app_rls_major)
AC_DEFINE_UNQUOTED(PACKAGE_MINOR_RELEASE, $app_rls_minor)
AC_DEFINE_UNQUOTED(PACKAGE_RELEASE, `date +%y%j`)

AC_SUBST(PACKAGE_RELEASE,$app_cv_release)
AC_SUBST(PACKAGE_MAJOR_RELEASE,$app_rls_major)
AC_SUBST(PACKAGE_MINOR_RELEASE,$app_rls_minor)

date="date"
test -z "$SOURCE_DATE_EPOCH" || date="$date -u -d@$SOURCE_DATE_EPOCH"
rpq_build_date=`LANG=C $date`
rpq_timestamp=`$date +%Y%m%d%H%M%S`
rpq_revision=`$date +"%Y%m%d"`
sccs_date=`$date +%Y/%m/%d`
sccs_user=$USER

AC_SUBST(RPQ_BUILD_DATE,$rpq_build_date)
AC_SUBST(RPQ_TIMESTAMP,$rpq_timestamp)
AC_SUBST(RPQ_REVISION,$rpq_revision)

AC_SUBST(SCCS_USER,$USER)
AC_SUBST(SCCS_DATE,$sccs_date)

dnl ---------------------------------------------------------------------------
dnl Check for libintl
dnl ---------------------------------------------------------------------------

INTL_LIBS=""

AC_PATH_TOOL([XGETTEXT], [xgettext], [no])
AC_PATH_TOOL([MSGCAT], [msgcat], [no])
AC_PATH_TOOL([MSGINIT], [msginit], [no])
AC_PATH_TOOL([MSGMERGE], [msgmerge], [no])
AC_PATH_TOOL([MSGFMT], [msgfmt], [no])
AC_PATH_TOOL([VALGRIND], [valgrind], [no])
AC_PATH_TOOL([SVG2PNG],[rsvg-convert],[no])
AC_PATH_TOOL([CONVERT], [convert], [no])
AC_PATH_TOOL([OPTIPNG],[optipng],[no])

AC_CHECK_HEADER(libintl.h, [
	AC_DEFINE(HAVE_LIBINTL, 1)

	case "$host" in
	*-mingw32|*-pc-msys)
		INTL_LIBS="-lintl"
		;;

	*)
		INTL_LIBS=""

	esac

])

AC_SUBST(INTL_LIBS)

dnl ---------------------------------------------------------------------------
dnl Check for doxygen
dnl ---------------------------------------------------------------------------

AC_PATH_TOOL([DOXYGEN], [doxygen])

dnl Help line for graphviz (optionally used by doxygen)
AC_ARG_ENABLE(graphviz,
	AS_HELP_STRING([--disable-graphviz], [prevent doxygen from using dot]),
	[wantgraphviz=$enableval], [wantgraphviz=yes])

AC_CHECK_PROGS([GRAPHVIZDOT], [dot])
AC_MSG_CHECKING([whether doxygen should use graphviz])
if test "x$wantgraphviz" = "xyes" ; then
	if test $GRAPHVIZDOT ; then
		AC_MSG_RESULT([yes])
		usegraphviz=yes
	else
		AC_MSG_RESULT([no, graphviz missing])
		usegraphviz=no
	fi
else
	AC_MSG_RESULT([no, disabled by configure])
	usegraphviz=no
fi

AC_SUBST(HAVE_DOT,`echo $usegraphviz | tr yesno YESNO`)

if test "x$DOXYGEN" != "x"; then
	AC_CONFIG_FILES(doxygen/doxyfile)
fi

dnl ---------------------------------------------------------------------------
dnl Check for ELF symbol visibility support
dnl ---------------------------------------------------------------------------
m4_define([GLIB_CHECK_COMPILE_WARNINGS],
[m4_ifvaln([$1], [AC_LANG_CONFTEST([$1])])dnl
rm -f conftest.$ac_objext
glib_ac_compile_save="$ac_compile"
ac_compile='$CC -c $CFLAGS $CPPFLAGS conftest.$ac_ext'
AS_IF([_AC_EVAL_STDERR($ac_compile) &&
         AC_TRY_COMMAND([(if test -s conftest.err; then false ; else true; fi)])],
      [$2],
      [echo "$as_me: failed program was:" >&AS_MESSAGE_LOG_FD
cat conftest.$ac_ext >&AS_MESSAGE_LOG_FD
m4_ifvaln([$3],[$3])dnl])
ac_compile="$glib_ac_compile_save"
rm -f conftest.$ac_objext conftest.err m4_ifval([$1], [conftest.$ac_ext])[]dnl
])# GLIB_CHECK_COMPILE_WARNINGS

AC_MSG_CHECKING(for GNUC visibility attribute)
GLIB_CHECK_COMPILE_WARNINGS([
void
__attribute__ ((visibility ("hidden")))
     f_hidden (void)
{
}
void
__attribute__ ((visibility ("internal")))
     f_internal (void)
{
}
void
__attribute__ ((visibility ("protected")))
     f_protected (void)
{
}
void
__attribute__ ((visibility ("default")))
     f_default (void)
{
}
int main (int argc, char **argv)
{
        f_hidden();
        f_internal();
        f_protected();
        f_default();
        return 0;
}
],g_have_gnuc_visibility=yes,g_have_gnuc_visibility=no)
AC_MSG_RESULT($g_have_gnuc_visibility)
if test x$g_have_gnuc_visibility = xyes; then
  AC_DEFINE(HAVE_GNUC_VISIBILITY, 1, [supports GCC visibility attributes])
fi

dnl ---------------------------------------------------------------------------
dnl Check for compiler visibility flag
dnl ---------------------------------------------------------------------------
AC_MSG_CHECKING(whether $CC accepts -fvisibility=hidden)
AC_CACHE_VAL(app_cv_fvisibility_ok,
[ac_save_cc="$CC"
CC="$CC -fvisibility=hidden"
AC_TRY_RUN([int main() { return 0; }],
  app_cv_fvisibility_ok=yes,
  app_cv_fvisibility_ok=no,
  app_cv_fvisibility_ok=no)
CC="$ac_save_cc"])
AC_MSG_RESULT($app_cv_fvisibility_ok)
if test $app_cv_fvisibility_ok = yes; then
  CFLAGS="$CFLAGS -fvisibility=hidden"
  CXXFLAGS="$CXXFLAGS -fvisibility=hidden"
fi

dnl ---------------------------------------------------------------------------
dnl Check for C++ 2011 support
dnl ---------------------------------------------------------------------------
AC_DEFUN([AX_CHECK_COMPILE_FLAG],
[AC_PREREQ(2.59)dnl for _AC_LANG_PREFIX
AS_VAR_PUSHDEF([CACHEVAR],[ax_cv_check_[]_AC_LANG_ABBREV[]flags_$4_$1])dnl
AC_CACHE_CHECK([whether _AC_LANG compiler accepts $1], CACHEVAR, [
  ax_check_save_flags=$[]_AC_LANG_PREFIX[]FLAGS
  _AC_LANG_PREFIX[]FLAGS="$[]_AC_LANG_PREFIX[]FLAGS $4 $1"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM()],
    [AS_VAR_SET(CACHEVAR,[yes])],
    [AS_VAR_SET(CACHEVAR,[no])])
  _AC_LANG_PREFIX[]FLAGS=$ax_check_save_flags])
AS_IF([test x"AS_VAR_GET(CACHEVAR)" = xyes],
  [m4_default([$2], :)],
  [m4_default([$3], :)])
AS_VAR_POPDEF([CACHEVAR])dnl
])dnl AX_CHECK_COMPILE_FLAGS

AX_CHECK_COMPILE_FLAG([-std=c++11], [CPPFLAGS="$CPPFLAGS -std=c++11"])

dnl ---------------------------------------------------------------------------
dnl SSL Security options
dnl ---------------------------------------------------------------------------

AC_ARG_ENABLE([self-signed-cert-check],
	[AS_HELP_STRING([--enable-self-signed-cert-check], [Reject SSL connection when host presents a self signed certificate])],
[
	app_cv_self_signed_certs="$enableval"
],[
	app_cv_self_signed_certs="no"
])

if test "$app_cv_self_signed_certs" == "yes"; then
	AC_DEFINE(SSL_ENABLE_SELF_SIGNED_CERT_CHECK)
fi

AC_ARG_ENABLE([ssl-crl-check],
	[AS_HELP_STRING([--disable-ssl-crl-check], [Disable use of SSL Certificate Revocation List])],
[
	app_cv_enable_crl_check="$enableval"
],[
	app_cv_enable_crl_check="yes"
])

if test "$app_cv_enable_crl_check" == "yes"; then
	AC_DEFINE(SSL_ENABLE_CRL_CHECK)
fi

AC_ARG_ENABLE([crl-expiration-check],
	[AS_HELP_STRING([--disable-crl-expiration-check], [Reject SSL connection when host presents an expired certificate revocation list])],
[
	app_cv_expired_crl="$enableval"
],[
	app_cv_expired_crl="yes"
])

if test "$app_cv_expired_crl" == "yes"; then
	AC_DEFINE(SSL_ENABLE_CRL_EXPIRATION_CHECK)
fi


AC_ARG_WITH([crl-url],
	[AS_HELP_STRING([--with-crl-url], [Set a hardcoded URL for CRL download])],
[
	AC_DEFINE_UNQUOTED(SSL_CRL_URL,"$withval")
],[
	AC_MSG_NOTICE(No default crl url)
])

AC_ARG_ENABLE([ssl-error-notification],
	[AS_HELP_STRING([--disable-ssl-error-notification], [Disable notifications when the security negotiation fails])],
[
	app_cv_enable_ssl_notification="$enableval"
],[
	app_cv_enable_ssl_notification="yes"
])

if test "$app_cv_enable_ssl_notification" == "yes"; then
	AC_DEFINE(SSL_ENABLE_NOTIFICATION_WHEN_FAILED)
else
	AC_MSG_NOTICE(No notifications when SSL negotiation fails)
fi

dnl ---------------------------------------------------------------------------
dnl Check for pic
dnl ---------------------------------------------------------------------------
AC_ARG_ENABLE([pic],
	[AS_HELP_STRING([--disable-pic], [disable support for PIC libraries (required to compile on 64bits)])],
[
	app_cv_pic="$enableval"
],[

	case "$host_cpu" in
	x86_64)
		app_cv_pic="yes"
		;;

	aarch64)
		app_cv_pic="yes"
		;;

	*)
		app_cv_pic="no"
	esac

])

if test "$app_cv_pic" == "yes"; then
	CFLAGS="$CFLAGS -fPIC"
fi

dnl ---------------------------------------------------------------------------
dnl Lib3270 flags
dnl ---------------------------------------------------------------------------

AC_ARG_WITH([libname], [AS_HELP_STRING([--with-libname], [Setup library name])], [ app_cv_libname="$withval" ],[ app_cv_libname="3270" ])

AC_DEFINE_UNQUOTED(LIB3270_NAME,$app_cv_libname)
AC_SUBST(LIB3270_NAME,$app_cv_libname)

dnl ---------------------------------------------------------------------------
dnl OS Defs
dnl ---------------------------------------------------------------------------

case "$host" in
	*-mingw32|*-pc-msys)

		AC_SUBST(SONAME,lib$app_cv_libname.dll)

		if test "$app_cv_enable_crl_check" == "yes"; then
			LIBS="$LIBS -lwinhttp"
		fi

		;;

 	s390x-*)

		AC_SUBST(SONAME,lib$app_cv_libname.so)

		;;

 	*)

		AC_SUBST(SONAME,lib$app_cv_libname.so.$app_vrs_major.$app_vrs_minor)

esac


AC_ARG_WITH([sdk-version], [AS_HELP_STRING([--with-sdk-version], [Setup library version for SDK])], [ app_cv_sdkversion="$withval" ],[ app_cv_sdkversion=$VERSION ])

AC_DEFINE(LIB3270_SDK_VERSION,$app_cv_sdkversion)
AC_SUBST(LIB3270_SDK_VERSION,$app_cv_sdkversion)

AC_ARG_WITH([default-host], [AS_HELP_STRING([--with-default-host], [Set lib3270 default host url])], [ AC_DEFINE_UNQUOTED(LIB3270_DEFAULT_HOST,"$withval") ],[ AC_MSG_NOTICE(No default host)])

AC_ARG_ENABLE([static],
 	[AS_HELP_STRING([--enable-static], [Enable build and install the static library])],
[
 	app_cv_static="$enableval"
],[
	AC_MSG_NOTICE([Keep default selection for static library.])
])

if test "$app_cv_expired_crl" == "yes"; then
	AC_DEFINE(SSL_ENABLE_CRL_EXPIRATION_CHECK)
fi

if test "$app_cv_static" == "yes"; then
	INSTALL_PACKAGES="$INSTALL_PACKAGES static"
fi

dnl ---------------------------------------------------------------------------
dnl Check for headers
dnl ---------------------------------------------------------------------------

AC_CHECK_HEADER(malloc.h, AC_DEFINE(HAVE_MALLOC_H,,[do we have malloc.h?]))
AC_CHECK_HEADER(syslog.h, AC_DEFINE(HAVE_SYSLOG,,[do we have malloc.h?]))

AC_CHECK_FUNCS(getaddrinfo, AC_DEFINE(HAVE_GETADDRINFO) )
AC_CHECK_FUNC(vasprintf, AC_DEFINE(HAVE_VASPRINTF) )
AC_CHECK_FUNC(strtok_r, AC_DEFINE(HAVE_STRTOK_R) )

AC_ARG_WITH([inet-ntop], [AS_HELP_STRING([--with-inet-ntop], [Assume that inet_nto() is available])], [ app_cv_inet_ntop="$withval" ],[ app_cv_inet_ntop="auto" ])

if test "$app_cv_inet_ntop" == "auto"; then

	case "$host" in
	*-mingw32|*-pc-msys)
		app_cv_inet_ntop="yes"
		;;

	*)
		AC_CHECK_FUNC(inet_ntop, app_cv_inet_ntop="yes" )

	esac

fi

if test "$app_cv_inet_ntop" == "yes"; then
	AC_DEFINE(HAVE_INET_NTOP)
fi

dnl ---------------------------------------------------------------------------
dnl Check for SSL
dnl ---------------------------------------------------------------------------

PKG_CHECK_MODULES( [LIBSSL], [libssl libcrypto], AC_DEFINE(HAVE_LIBSSL), AC_MSG_NOTICE([libssl not present.]) )

AC_SUBST(LIBSSL_LIBS)
AC_SUBST(LIBSSL_CFLAGS)

dnl ---------------------------------------------------------------------------
dnl Check for D-Bus
dnl ---------------------------------------------------------------------------

PKG_CHECK_MODULES( [DBUS], [dbus-1], AC_DEFINE(HAVE_DBUS), AC_MSG_NOTICE([ No DBUS support.]) )
AC_SUBST(DBUS_LIBS)
AC_SUBST(DBUS_CFLAGS)

dnl ---------------------------------------------------------------------------
dnl Check for LDAP
dnl ---------------------------------------------------------------------------

AC_ARG_ENABLE([ldap],
        AS_HELP_STRING([--enable-ldap],[Enable optional LDAP support]),
                [case "${enableval}" in
                        yes) have_ldap=yes ;;
                        no) have_ldap=no ;;
                        *) AC_MSG_ERROR(bad value ${enableval} for --disable-ldap);;
                esac],
                [have_ldap=no])


if test "x${have_ldap}" != xno ; then

	case "$host" in
		*-mingw32|*-pc-msys)
			LDAP_LIBS="-lwldap32"
		     	AC_DEFINE(HAVE_LDAP,1,[LDAP is available])
			;;

	 	*)

			AC_CHECK_HEADERS(
				[ldap.h],
				[have_ldap=yes],
				[if test "x$have_ldap" = xyes ; then
				        AC_MSG_ERROR([LDAP headers not found.])
				fi])

			if test "x$have_ldap" = xyes ; then

				LDAP_LIBS="-lldap -llber"
			     	AC_DEFINE(HAVE_LDAP,1,[LDAP is available])
				M4_DEFINES="$M4_DEFINES -DHAVE_LDAP"

			else

				have_ldap=no

			fi


	esac

else
        LDAP_LIBS=
fi

AC_SUBST(LDAP_CFLAGS)
AC_SUBST(LDAP_LIBS)

dnl ---------------------------------------------------------------------------
dnl Check for CURL
dnl ---------------------------------------------------------------------------

AC_ARG_ENABLE([curl],
        AS_HELP_STRING([--disable-curl],[Disable use of libcurl]),
                [case "${enableval}" in
                        yes) have_curl=yes ;;
                        no) have_curl=no ;;
                        *) AC_MSG_ERROR(bad value ${enableval} for --disable-curl);;
                esac],
                [case "$host" in
			*-mingw32|*-pc-msys) have_curl=no ;;
			*) have_curl=yes;;
		esac])


if test "x${have_curl}" != xno ; then
	PKG_CHECK_MODULES( [LIBCURL], [libcurl], AC_DEFINE(HAVE_LIBCURL), AC_MSG_ERROR([libcurl not present.]) )
fi

AC_SUBST(LIBCURL_LIBS)
AC_SUBST(LIBCURL_CFLAGS)

dnl ---------------------------------------------------------------------------
dnl Directory config
dnl ---------------------------------------------------------------------------

AC_ARG_WITH([build-dir], [AS_HELP_STRING([--with-build-dir], [Setup build path])], [ app_cv_buildir="$withval" ],[ app_cv_buildir="$ac_pwd" ])
AC_SUBST(BASEDIR,$app_cv_buildir)

dnl ---------------------------------------------------------------------------
dnl Configure which files to generate.
dnl ---------------------------------------------------------------------------

AC_SUBST(INSTALL_PACKAGES)

AC_CONFIG_FILES(Makefile)

AC_CONFIG_FILES(src/core/version.c)

AC_CONFIG_FILES(sdk/lib3270.pc)
AC_CONFIG_FILES(sdk/lib3270-static.pc)

dnl ---------------------------------------------------------------------------
dnl Output the generated config.status script.
dnl ---------------------------------------------------------------------------
AC_OUTPUT


