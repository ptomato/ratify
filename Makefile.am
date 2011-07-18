SUBDIRS = osxcart testcases docs po

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = osxcart.pc

EXTRA_DIST = \
    osxcart.pc.in \
    osxcart.spec.in \
    README \
	COPYING \
	COPYING.LESSER \
	AUTHORS \
	ChangeLog \
	INSTALL \
	NEWS \
	m4/introspection.m4

DISTCLEANFILES = osxcart.pc osxcart.spec

MAINTAINERCLEANFILES = ABOUT-NLS INSTALL aclocal.m4 compile config.guess \
        config.h.in config.rpath config.sub depcomp install-sh gtk-doc.make \
        intltool-extract.in intltool-merge.in intltool-update.in ltmain.sh \
        missing autom4te.cache m4

ACLOCAL_AMFLAGS = -I m4

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc --enable-introspection

if BUILDING_RPM
EXTRA_DIST += osxcart.spec
rpm: osxcart.spec dist
	$(ECHO) "Building RPM package..."; \
	$(RPMBUILD) -ta --clean $(PACKAGE)-$(VERSION).tar.gz
else
rpm:
	@$(ECHO) \
	"To build an RPM package, you must run configure with --enable-rpm."; \
	exit 1
endif

-include $(top_srcdir)/git.mk
