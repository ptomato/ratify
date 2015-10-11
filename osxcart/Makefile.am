AM_CFLAGS = -Wall

lib_LTLIBRARIES = libosxcart-@OSXCART_API_VERSION@.la

libosxcart_@OSXCART_API_VERSION@_la_SOURCES = \
	init.c init.h \
	plist.c plist.h \
	plist-write.c \
	plist-parser.c \
	rtf.c rtf.h \
	rtf-serialize.c rtf-serialize.h \
	rtf-deserialize.c rtf-deserialize.h \
	rtf-document.c rtf-document.h \
	rtf-fonttbl.c \
	rtf-colortbl.c \
	rtf-ignore.c rtf-ignore.h \
	rtf-footnote.c \
	rtf-picture.c \
	rtf-field.c \
	rtf-stylesheet.c \
	rtf-langcode.c rtf-langcode.h \
	rtf-state.c rtf-state.h

libosxcart_@OSXCART_API_VERSION@_la_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"@PACKAGE_NAME@\" \
	-DLOCALEDIR=\""$(datadir)/locale"\" \
	-I$(top_srcdir)
libosxcart_@OSXCART_API_VERSION@_la_CFLAGS = @OSXCART_CFLAGS@ $(AM_CFLAGS)
libosxcart_@OSXCART_API_VERSION@_la_LIBADD = @OSXCART_LIBS@
libosxcart_@OSXCART_API_VERSION@_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO) \
	-no-undefined \
	-export-symbols-regex "^(plist|rtf)_" \
	$(NULL)
libosxcart_includedir = $(includedir)/osxcart-@OSXCART_API_VERSION@/osxcart
libosxcart_include_HEADERS = plist.h rtf.h

CLEANFILES=
DISTCLEANFILES=
MAINTAINERCLEANFILES=

# Introspection boilerplate
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir)
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)

if HAVE_INTROSPECTION

# There can't be two namespaces in one .gir file?

OsxcartPlist-@OSXCART_API_VERSION@.gir: libosxcart-@OSXCART_API_VERSION@.la
OsxcartPlist_@OSXCART_API_VERSION@_gir_FILES = \
	plist.h plist.c \
	plist-parser.c \
	plist-write.c \
	$(NULL)
OsxcartPlist_@OSXCART_API_VERSION@_gir_LIBS = \
	libosxcart-@OSXCART_API_VERSION@.la \
	$(NULL)
OsxcartPlist_@OSXCART_API_VERSION@_gir_INCLUDES = GLib-2.0 Gio-2.0
OsxcartPlist_@OSXCART_API_VERSION@_gir_SCANNERFLAGS = \
	--identifier-prefix=Plist \
	--c-include='osxcart/plist.h' \
	--warn-all
INTROSPECTION_GIRS += OsxcartPlist-@OSXCART_API_VERSION@.gir

OsxcartRtf-@OSXCART_API_VERSION@.gir: libosxcart-@OSXCART_API_VERSION@.la
OsxcartRtf_@OSXCART_API_VERSION@_gir_FILES = rtf.h rtf.c
OsxcartRtf_@OSXCART_API_VERSION@_gir_LIBS = libosxcart-@OSXCART_API_VERSION@.la
OsxcartRtf_@OSXCART_API_VERSION@_gir_INCLUDES = Gtk-2.0
OsxcartRtf_@OSXCART_API_VERSION@_gir_CFLAGS = \
	@OSXCART_CFLAGS@ \
	$(NULL)
# Otherwise can't find gtk/gtk.h?
OsxcartRtf_@OSXCART_API_VERSION@_gir_SCANNERFLAGS = \
	--identifier-prefix=Rtf \
	--c-include='osxcart/rtf.h' \
	--warn-all
INTROSPECTION_GIRS += OsxcartRtf-@OSXCART_API_VERSION@.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)

if BUILDING_VAPI
vapidir = $(datadir)/vala/vapi
dist_vapi_DATA = osxcart-@OSXCART_API_VERSION@.vapi

osxcart-@OSXCART_API_VERSION@.vapi: $(INTROSPECTION_GIRS)
	$(AM_V_GEN)$(VAPIGEN) --library=osxcart --pkg gtk+-2.0 $(INTROSPECTION_GIRS) && \
	mv osxcart.vapi $@ && \
	touch $@  # Doesn't update the timestamp if unchanged?

DISTCLEANFILES += osxcart-@OSXCART_API_VERSION@.vapi

endif # BUILDING_VAPI
endif # HAVE_INTROSPECTION

-include $(top_srcdir)/git.mk
