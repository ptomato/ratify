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
	NEWS

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc
