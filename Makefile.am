SUBDIRS = osxcart testcases docs po

osxcartdocdir = ${prefix}/doc/osxcart
dist_osxcartdoc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = osxcart.pc

EXTRA_DIST = intltool-extract.in intltool-merge.in intltool-update.in \
	osxcart.pc.in

DISTCLEANFILES = intltool-extract intltool-merge intltool-update

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc
