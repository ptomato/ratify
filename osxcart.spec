Name: osxcart
Version: 1.1
Release: 1%{?dist}
Summary: Library for interfacing OS X file formats with GTK+
URL: http://sourceforge.net/apps/trac/osxcart/wiki
License: LGPLv3
Group: Development/Libraries
Source: http://downloads.sourceforge.net/%{name}/%{name}-%{version}.tar.gz
Requires: glib2%{?_isa} >= 2.18
Requires: gtk2%{?_isa} >= 2.10
BuildRequires: glib2-devel%{?_isa} >= 2.18
BuildRequires: gtk2-devel%{?_isa} >= 2.10
BuildRequires: gettext
BuildRequires: libtool >= 2.2
BuildRequires: vala-tools
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
%configure --disable-static
make %{?_smp_mflags}

%install
make DESTDIR=$RPM_BUILD_ROOT install
rm -f $RPM_BUILD_ROOT/%{_libdir}/libosxcart.la
%find_lang %{name}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files -f %{name}.lang
%defattr(-,root,root,-)
%doc README NEWS COPYING ChangeLog
%{_libdir}/libosxcart.so.0*
%{_libdir}/girepository-1.0/Osxcart*1.0.typelib

%files devel
%defattr(-,root,root,-)
%{_libdir}/libosxcart.so
%{_libdir}/pkgconfig/osxcart.pc
%{_includedir}/%{name}/osxcart/*.h
%{_datadir}/gtk-doc/html/osxcart/*
%{_datadir}/gir-1.0/Osxcart*1.0.gir
%{_datadir}/vala/vapi/osxcart.vapi

%changelog
* Sun Feb 12 2012 P. F. Chimento <philip.chimento@gmail.com> - 1.1-1
- Fix typo in spec file.
- Read various spec file guidelines and improved the spec file.
