#
# spec file for package lib3270
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

#---[ Package header ]------------------------------------------------------------------------------------------------

Summary:		TN3270 Access library
Name:			lib3270
Version:		5.2
Release:		0
License:		LGPL-3.0

Source:			%{name}-%{version}.tar.xz

URL:			https://github.com/PerryWerneck/lib3270

Group:			System/Libraries
BuildRoot:		/var/tmp/%{name}-%{version}

BuildRequires:	autoconf >= 2.61
BuildRequires:	automake
BuildRequires:	binutils
BuildRequires:	coreutils
BuildRequires:	gcc-c++
BuildRequires:	gettext-devel
BuildRequires:	m4
BuildRequires:	pkgconfig
BuildRequires:	fdupes

%if 0%{?fedora} ||  0%{?suse_version} > 1200

BuildRequires:	pkgconfig(openssl)
BuildRequires:	pkgconfig(libssl)
BuildRequires:	pkgconfig(libcrypto)
BuildRequires:	pkgconfig(libcurl)

%else

BuildRequires:	openssl-devel
BuildRequires:	xz
BuildRequires:	libcurl-devel

%endif

%if 0%{?centos_version}
# CENTOS Requires gdb for debuginfo
BuildRequires:	gdb
%endif

%description
TN3270 access library, originally designed as part of the pw3270 application.

For more details, see https://softwarepublico.gov.br/social/pw3270/ .

#---[ Library ]-------------------------------------------------------------------------------------------------------

%define MAJOR_VERSION %(echo %{version} | cut -d. -f1)
%define MINOR_VERSION %(echo %{version} | cut -d. -f2)
%define _libvrs %{MAJOR_VERSION}_%{MINOR_VERSION}

%package -n %{name}-%{_libvrs}
Summary:		TN3270 Access library
Group:			Development/Libraries/C and C++

%description -n %{name}-%{_libvrs}
TN3270 access library, originally designed as part of the pw3270 application.

For more details, see https://softwarepublico.gov.br/social/pw3270/ .

%package devel

Summary:		TN3270 Access library development files
Group:			Development/Libraries/C and C++
Requires:		%{name}-%{_libvrs} = %{version}

%description devel
Header files for the TN3270 access library.

#---[ Build & Install ]-----------------------------------------------------------------------------------------------

%prep
%setup

NOCONFIGURE=1 \
	./autogen.sh

%configure

%build
make all %{?_smp_mflags}

%install

%make_install
%fdupes %{buildroot}/%{_prefix}

%files -n %{name}-%{_libvrs}
%defattr(-,root,root)

# https://en.opensuse.org/openSUSE:Packaging_for_Leap#RPM_Distro_Version_Macros
%if 0%{?sle_version} > 120200
%doc AUTHORS README.md
%license LICENSE
%else
%doc AUTHORS README.md LICENSE
%endif

%dir %{_datadir}/pw3270

%{_libdir}/%{name}.so.%{MAJOR_VERSION}.%{MINOR_VERSION}

%files devel
%defattr(-,root,root)

%{_libdir}/%{name}.so

%{_includedir}/*.h
%{_includedir}/%{name}

%{_libdir}/pkgconfig/*.pc

%dir %{_datadir}/pw3270/pot
%{_datadir}/pw3270/pot/*.pot

%post -n %{name}-%{_libvrs} -p /sbin/ldconfig

%postun -n %{name}-%{_libvrs} -p /sbin/ldconfig

%changelog
