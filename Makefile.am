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

MAINTAINERCLEANFILES = ABOUT-NLS INSTALL config.rpath gtk-doc.make m4 \
        intltool-extract.in intltool-merge.in intltool-update.in \
        $(GITIGNORE_MAINTAINERCLEANFILES_TOPLEVEL) \
        $(GITIGNORE_MAINTAINERCLEANFILES_M4_LIBTOOL) \
        $(NULL)

ACLOCAL_AMFLAGS = -I m4

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc --enable-introspection

-include $(top_srcdir)/git.mk
