Name: osxcart
Version: 1.2.0
Release: 1%{?dist}
Summary: Library for interfacing OS X file formats with GTK+
URL: https://github.com/ptomato/%{name}
License: LGPLv3
Group: Development/Libraries
Source: http://github.com/ptomato/%{name}/releases/download/%{version}/%{name}-%{version}.tar.xz
Requires: glib2%{?_isa} >= 2.18
Requires: gtk2%{?_isa} >= 2.10
BuildRequires: glib2-devel%{?_isa} >= 2.18
BuildRequires: gtk2-devel%{?_isa} >= 2.10
BuildRequires: gettext
BuildRequires: libtool >= 2.2
BuildRequires: pkgconfig(vapigen) >= 0.20
BuildRequires: gtk-doc

%description
Osxcart, or OS X Converting And Reading Tool, is a library designed to import 
file formats used in Mac OS X, NeXTSTEP, and GnuSTEP into GTK+/GLib-based 
programs easily, using a lightweight interface. Examples: property lists, RTF 
and RTFD documents.

%package devel
Summary: Library for interfacing OS X file formats with GTK+ (development files)
Group: Development/Libraries
Requires: %{name}%{?_isa} = %{version}-%{release}

%description devel
Development files for Osxcart.

%prep
%setup -q

%build
%configure --disable-static --enable-introspection --enable-gtk-doc --with-xvfb-tests
make %{?_smp_mflags}

%install
make DESTDIR=$RPM_BUILD_ROOT install
rm -f $RPM_BUILD_ROOT/%{_libdir}/libosxcart-*.la
%find_lang %{name}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files -f %{name}.lang
%defattr(-,root,root,-)
%doc README.md AUTHORS.md COPYING COPYING.LESSER ChangeLog
%{_libdir}/libosxcart-*.so.0*
%{_libdir}/girepository-1.0/Osxcart*.typelib

%files devel
%defattr(-,root,root,-)
%{_libdir}/libosxcart-*.so
%{_libdir}/pkgconfig/%{name}-*.pc
%{_includedir}/%{name}-*/%{name}/*.h
%{_datadir}/gtk-doc/html/%{name}-*/*
%{_datadir}/gir-1.0/Osxcart*.gir
%{_datadir}/vala/vapi/%{name}-*.vapi

%changelog
* Mon Oct 12 2015 Philip Chimento <philip.chimento@gmail.com> - 1.2.0-1
- Require pkgconfig(vapigen) instead of vala-tools, to support OpenSUSE.
- Configure with gtk-doc, introspection, and run tests under XVFB.
* Sun Oct 11 2015 Philip Chimento <philip.chimento@gmail.com>
- Update URIs for project which moved from SourceForge to GitHub.
- Release new version.
* Sun Feb 12 2012 P. F. Chimento <philip.chimento@gmail.com> - 1.1-1
- Fix typo in spec file.
- Read various spec file guidelines and improved the spec file.
