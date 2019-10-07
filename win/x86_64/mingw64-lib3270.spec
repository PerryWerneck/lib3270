#
# spec file for package mingw64-lib3279
#
# Copyright (c) 2014 SUSE LINUX Products GmbH, Nuernberg, Germany.
# Copyright (C) <2008> <Banco do Brasil S.A.>
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via http://bugs.opensuse.org/
#

%define __strip %{_mingw64_strip}
%define __objdump %{_mingw64_objdump}
%define _use_internal_dependency_generator 0
%define __find_requires %{_mingw64_findrequires}
%define __find_provides %{_mingw64_findprovides}
%define __os_install_post %{_mingw64_debug_install_post} \
                          %{_mingw64_install_post}

#---[ Main package ]--------------------------------------------------------------------------------------------------

Summary:		TN3270 Access library
Name:			mingw64-lib3270
Version:		5.2

%define MAJOR_VERSION %(echo %{version} | cut -d. -f1)
%define MINOR_VERSION %(echo %{version} | cut -d. -f2)
%define _libvrs %{MAJOR_VERSION}_%{MINOR_VERSION}


Release:		0
License:		GPL-2.0

Source:			lib3270-%{version}.tar.xz

Url:			https://portal.softwarepublico.gov.br/social/pw3270/

Group:			Development/Libraries/C and C++
BuildRoot:		/var/tmp/%{name}-%{version}

Provides:		mingw64(lib:3270) = %{version}
Provides:		mingw64(lib:3270-%{_libvrs}) = %{version}

BuildRequires:	autoconf
BuildRequires:	automake
BuildRequires:	gettext-tools

BuildRequires:	mingw64-cross-binutils
BuildRequires:	mingw64-cross-gcc
BuildRequires:	mingw64-cross-gcc-c++
BuildRequires:	mingw64-cross-pkg-config
BuildRequires:	mingw64-filesystem
BuildRequires:	mingw64-libopenssl-devel
BuildRequires:	mingw64-zlib-devel
BuildRequires:	mingw64(lib:iconv)
BuildRequires:	mingw64(lib:intl)

%description

TN3270 access library originally designed as part of the pw3270 application.

See more details at https://softwarepublico.gov.br/social/pw3270/

#---[ Development ]---------------------------------------------------------------------------------------------------

%package devel

Summary:	TN3270 Access library development files
Group:		Development/Libraries/C and C++
Requires:	%{name} = %{version}

Provides:	mingw64-lib3270-devel = %{version}
Conflicts:	otherproviders(mingw64-lib3270-devel)

%description devel

TN3270 access library for C development files.

Originally designed as part of the pw3270 application.

See more details at https://softwarepublico.gov.br/social/pw3270/

#---[ Build & Install ]-----------------------------------------------------------------------------------------------

%prep
%setup -n lib3270-%{version}

NOCONFIGURE=1 ./autogen.sh

%{_mingw64_configure} \
	--with-sdk-version=%{version}

%build
make clean
make all

%{_mingw64_strip} \
	--strip-all \
	.bin/Release/*.dll

%install
%{_mingw64_makeinstall}

%clean
rm -rf %{buildroot}

#---[ Files ]---------------------------------------------------------------------------------------------------------

%files
%defattr(-,root,root)
%doc AUTHORS LICENSE README.md

%{_mingw64_libdir}/lib3270.dll
%{_mingw64_libdir}/lib3270.dll.%{MAJOR_VERSION}
%{_mingw64_libdir}/lib3270.dll.%{MAJOR_VERSION}.%{MINOR_VERSION}

%dir %{_mingw64_datadir}/pw3270

%files devel
%defattr(-,root,root)

%{_mingw64_includedir}/lib3270

%{_mingw64_includedir}/*.h
%{_mingw64_libdir}/pkgconfig/*.pc
%{_mingw64_libdir}/*.a
%{_mingw64_libdir}/*.lib

%dir %{_mingw64_datadir}/pw3270/def
%{_mingw64_datadir}/pw3270/def/*.def

%dir %{_mingw64_datadir}/pw3270/pot
%{_mingw64_datadir}/pw3270/pot/*.pot

%changelog
