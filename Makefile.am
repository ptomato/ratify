SUBDIRS = osxcart testcases docs po

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = osxcart-@OSXCART_API_VERSION@.pc

EXTRA_DIST = \
	README.md \
	COPYING \
	COPYING.LESSER \
	AUTHORS.md \
	ChangeLog \
	m4/introspection.m4

DISTCLEANFILES = $(pkgconfig_DATA)

macros_ignore = codeset gettext glibc2 glibc21 gtk-doc iconv intdiv0 intl \
	intldir intlmacosx intmax inttypes-pri inttypes_h lcmessage lib-ld \
	lib-link lib-prefix lock longlong nls po printf-posix progtest size_max \
	stdint_h uintmax_t visibility wchar_t wint_t xsize

MAINTAINERCLEANFILES = ABOUT-NLS config.rpath gtk-doc.make \
	$(GITIGNORE_MAINTAINERCLEANFILES_TOPLEVEL) \
	$(GITIGNORE_MAINTAINERCLEANFILES_MAKEFILE_IN) \
	$(GITIGNORE_MAINTAINERCLEANFILES_M4_LIBTOOL) \
	$(addprefix 'm4/', $(addsuffix '.m4', $(macros_ignore))) \
	$(NULL)

ACLOCAL_AMFLAGS = -I m4

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc --enable-introspection

-include $(top_srcdir)/git.mk
