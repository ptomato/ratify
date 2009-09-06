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
	NEWS

DISTCLEANFILES = osxcart.pc osxcart.spec

ACLOCAL_AMFLAGS = -I m4

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

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
