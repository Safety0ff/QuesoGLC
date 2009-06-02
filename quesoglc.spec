%define version 0.7.9
%define major_ver 0
%define minor_ver 7
%define release 1

Summary: QuesoGLC - a free implementation of the OpenGL Character Renderer (GLC).
Name: QuesoGLC
Version: %{version}
Release: %{release}
Source0: quesoglc-%{version}.tar.gz
License: LGPL
Group: System Environment/Libraries
BuildRoot: /var/tmp/%{name}-buildroot
Prefix: %{_prefix}

%description
QuesoGLC is a free (as in free speech) implementation of the OpenGL Character Renderer (GLC).

%package devel
Summary: Libraries, includes and more to develop %{name} applications.
Group: Development/Libraries
Requires: %{name} = %{version}

%description devel
Development package for %{name}.

%ifos linux
%ifarch x86_64
%package 32bit
Summary: QuesoGLC - a free implementation of the OpenGL Character Renderer (GLC).
Group: System Environment/Libraries
Requires: %{name} = %{version}

%description 32bit
This package contains 32-bit binaries for %{name}.

%package devel-32bit
Summary: Libraries and includes to develop %{name} programs
Group: Development/Libraries
Requires: %{name}-devel = %{version}

%description devel-32bit
This package contains 32-bit development binaries for %{name}.
%endif
%endif

%prep
rm -rf $RPM_BUILD_ROOT
%setup -q -n quesoglc-%{version}

%build
%ifos linux
%ifarch x86_64
%define arch_flags -m32 -march=i686 -mtune=i686
CFLAGS="$RPM_OPT_FLAGS %{arch_flags}" LDFLAGS="%{arch_flags}" GLEW_CFLAGS="-DGLEW_MX" ./configure --prefix=%{prefix} --libdir='%{prefix}'/lib --x-libraries=/usr/lib
make
mkdir -p build32/lib
cp -a build/.libs/libGLC.so* build32/lib
cp -a build/.libs/libGLC.a build32/lib
cp -a build/.libs/libGLC.lai build32/lib/libGLC.la
make clean
%endif
%endif
CFLAGS="$RPM_OPT_FLAGS" GLEW_CFLAGS="-DGLEW_MX" ./configure --prefix=%{prefix} --libdir=%{_libdir}
make

%install
%makeinstall
%ifos linux
%ifarch x86_64
cp -a build32/lib $RPM_BUILD_ROOT/%{prefix}
%endif
%endif

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc README THANKS INSTALL COPYING AUTHORS ChangeLog 
%{_libdir}/libGLC.so.*

%files devel
%defattr(-,root,root)
%{_libdir}/libGLC.a
%{_libdir}/libGLC.la
%{_libdir}/libGLC.so
%{_includedir}/GL/glc.h
%{_libdir}/pkgconfig/quesoglc.pc

%ifos linux
%ifarch x86_64
%files 32bit
%defattr(-,root,root)
%{prefix}/lib/libGLC.so.*

%files devel-32bit
%defattr(-,root,root)
%{prefix}/lib/libGLC.a
%{prefix}/lib/libGLC.la
%{prefix}/lib/libGLC.so
%endif
%endif

%changelog
* Sun May 31 2009 Edward Cullen
- Initial spec file for QuesoGLC version 0.7.2.
