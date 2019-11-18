%if 0%{?suse_version}
%define dist .%{vendor}
%endif

Name: ratify
Version: 2.0.0
Release: 1%{?dist}
Summary: Library for importing and exporting RTF documents in GTK
URL: https://github.com/ptomato/%{name}
License: LGPLv3
Group: Development/Libraries
Source: http://github.com/ptomato/%{name}/releases/download/%{version}/%{name}-%{version}.tar.xz
Requires: glib2%{?_isa} >= 2.44
Requires: gtk3%{?_isa}
BuildRequires: glib2-devel%{?_isa} >= 2.44
BuildRequires: gtk3-devel%{?_isa}
BuildRequires: gettext
BuildRequires: libtool >= 2.2
BuildRequires: pkgconfig(gobject-introspection-1.0)
BuildRequires: pkgconfig(vapigen) >= 0.20
BuildRequires: gtk-doc
%if 0%{?suse_version}
BuildRequires: vala
%define vala_version %(rpm -q --queryformat='%{VERSION}' vala | sed 's/\.[0-9]*$//g')
%endif

%description
Ratify is a library for importing and exporting RTF documents to and
from GtkTextBuffers.

%package devel
Summary: Library for importing and exporting RTF documents in GTK (development files)
Group: Development/Libraries
Requires: %{name}%{?_isa} = %{version}-%{release}

%description devel
Development files for Ratify.

%prep
%setup -q

%build
%configure --disable-static --enable-introspection --enable-gtk-doc --with-xvfb-tests
make %{?_smp_mflags}

%install
make DESTDIR=$RPM_BUILD_ROOT install
rm -f $RPM_BUILD_ROOT/%{_libdir}/lib%{name}-*.la
# Install vapi file in versioned vala directories on OpenSUSE
%if 0%{?suse_version}
mkdir -pv %{buildroot}%{_datadir}/vala-%{vala_version}
mv %{buildroot}%{_datadir}/vala/* %{buildroot}%{_datadir}/vala-%{vala_version}
rm -rf %{buildroot}%{_datadir}/vala
%endif

%find_lang %{name}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files -f %{name}.lang
%defattr(-,root,root,-)
%doc README.md AUTHORS.md COPYING COPYING.LESSER ChangeLog
%{_libdir}/lib%{name}-*.so.0*
%{_libdir}/girepository-1.0/Ratify*.typelib

%files devel
%defattr(-,root,root,-)
%{_libdir}/lib%{name}-*.so
%{_libdir}/pkgconfig/%{name}-*.pc
%{_includedir}/%{name}-*
%{_datadir}/gtk-doc/html/%{name}-*
%{_datadir}/gir-1.0/Ratify*.gir
%if 0%{?suse_version}
%{_datadir}/vala-%{vala_version}/vapi/%{name}-*.vapi
%else
%{_datadir}/vala/vapi/%{name}-*.vapi
%endif

%changelog
* Sun Nov 17 2019 Philip Chimento <philip.chimento@gmail.com> - 2.0.0-1
- Initial packaging.
