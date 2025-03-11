#
# spec file for package mingw64-%{_libname}
#
# Copyright (c) 2015 SUSE LINUX GmbH, Nuernberg, Germany.
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

%define _libname lib3270
%define _product pw3270

# Only build in tumbleweed
%if 0%{?suse_version} < 1699
ExclusiveArch:  do_not_build
%endif

#---[ Package header ]------------------------------------------------------------------------------------------------

Summary:		TN3270 access library for 64 bits Windows
Name:			mingw64-%{_libname}
Version: 5.6.0
Release:		0
License:		LGPL-3.0

Source:			lib3270-%{version}.tar.xz

URL:			https://github.com/PerryWerneck/lib3270

Group:			System/Libraries
BuildRoot:		/var/tmp/%{name}-%{version}

BuildRequires:	autoconf >= 2.61
BuildRequires:	automake
BuildRequires:	libtool
BuildRequires:	gettext-devel
BuildRequires:	xz
BuildRequires:	fdupes

BuildRequires:	mingw64-cross-binutils
BuildRequires:	mingw64-cross-gcc
BuildRequires:	mingw64-cross-gcc-c++
BuildRequires:	mingw64-cross-pkg-config
BuildRequires:	mingw64-filesystem
BuildRequires:	mingw64-zlib-devel
BuildRequires:	mingw64-win_iconv-devel-static
BuildRequires:	mingw64-libintl-devel
BuildRequires:	mingw64-gettext-tools
BuildRequires:  mingw64-libressl-devel

%description
TN3270 access library, originally designed as part of the %{_product} application.

For more details, see https://softwarepublico.gov.br/social/pw3270/ .

#---[ Library ]-------------------------------------------------------------------------------------------------------

%define MAJOR_VERSION %(echo %{version} | cut -d. -f1)
%define MINOR_VERSION %(echo %{version} | cut -d. -f2 | cut -d+ -f1)
%define _libvrs %{MAJOR_VERSION}_%{MINOR_VERSION}

%package -n %{name}-%{_libvrs}
Summary:		TN3270 Access library
Group:			Development/Libraries/C and C++

%description -n %{name}-%{_libvrs}
TN3270 access library, originally designed as part of the %{_product} application.

For more details, see https://softwarepublico.gov.br/social/pw3270/ .

%package devel

Summary:		TN3270 Access library development files
Group:			Development/Libraries/C and C++
Requires:		%{name}-%{_libvrs} = %{version}

%description devel
Header files for the TN3270 access library.

#---[ Build & Install ]-----------------------------------------------------------------------------------------------

%prep
%setup -n lib3270-%{version}

NOCONFIGURE=1 \
	./autogen.sh

%{_mingw64_configure}

%build
make all %{?_smp_mflags}

%{_mingw64_strip} \
	--strip-all \
	.bin/Release/*.dll


%install

# The macro changes the prefix!
#%{_mingw64_makeinstall}
make DESTDIR=%{buildroot} install

%_mingw64_find_lang %{_libname}-%{MAJOR_VERSION}.%{MINOR_VERSION} langfiles
%fdupes %{buildroot}

%files -n %{name}-%{_libvrs} -f langfiles
%defattr(-,root,root)

%doc AUTHORS README.md
%license LICENSE

%dir %{_mingw64_datadir}/%{_product}

%{_mingw64_bindir}/*.dll
%exclude %{_mingw64_libdir}/*.dll

%files devel
%defattr(-,root,root)

%{_mingw64_libdir}/*.a

%{_mingw64_includedir}/*.h
%{_mingw64_includedir}/lib3270

%{_mingw64_libdir}/pkgconfig/*.pc

%dir %{_mingw64_datadir}/%{_product}/def
%{_mingw64_datadir}/%{_product}/def/*.def
%{_mingw64_datadir}/%{_product}/def/*.mak

%dir %{_mingw64_datadir}/%{_product}/pot
%{_mingw64_datadir}/%{_product}/pot/*.pot

%changelog
