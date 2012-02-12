SUBDIRS = osxcart testcases docs po

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = osxcart.pc

EXTRA_DIST = \
    osxcart.pc.in \
    README \
	COPYING \
	COPYING.LESSER \
	AUTHORS \
	ChangeLog \
	INSTALL \
	NEWS \
	m4/introspection.m4

DISTCLEANFILES = osxcart.pc

MAINTAINERCLEANFILES = ABOUT-NLS INSTALL aclocal.m4 compile config.guess \
        config.h.in config.rpath config.sub depcomp install-sh gtk-doc.make \
        intltool-extract.in intltool-merge.in intltool-update.in ltmain.sh \
        missing autom4te.cache m4

ACLOCAL_AMFLAGS = -I m4

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc --enable-introspection

-include $(top_srcdir)/git.mk
