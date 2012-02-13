AM_CFLAGS = -Wall

lib_LTLIBRARIES = libosxcart.la

libosxcart_la_SOURCES = \
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
	
libosxcart_la_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"@PACKAGE_NAME@\" \
	-DLOCALEDIR=\""$(datadir)/locale"\" \
	-I$(top_srcdir)
libosxcart_la_CFLAGS = @OSXCART_CFLAGS@ $(AM_CFLAGS)
libosxcart_la_LIBADD = @OSXCART_LIBS@
libosxcart_la_LDFLAGS = -version-info $(LT_VERSION_INFO) \
						-no-undefined \
						-export-symbols-regex "^(plist|rtf)_"
libosxcart_includedir = $(includedir)/osxcart/osxcart
libosxcart_include_HEADERS = plist.h rtf.h

CLEANFILES=
MAINTAINERCLEANFILES=

# Introspection boilerplate
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir)
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)

if HAVE_INTROSPECTION

# There can't be two namespaces in one .gir file?

OsxcartPlist-1.0.gir: libosxcart.la
OsxcartPlist_1_0_gir_FILES = plist.h plist.c plist-parser.c plist-write.c
OsxcartPlist_1_0_gir_LIBS = libosxcart.la
OsxcartPlist_1_0_gir_INCLUDES = GLib-2.0 Gio-2.0
OsxcartPlist_1_0_gir_SCANNERFLAGS = \
	--identifier-prefix=Plist \
	--c-include='osxcart/plist.h' \
	--warn-all
INTROSPECTION_GIRS += OsxcartPlist-1.0.gir

OsxcartRtf-1.0.gir: libosxcart.la
OsxcartRtf_1_0_gir_FILES = rtf.h rtf.c
OsxcartRtf_1_0_gir_LIBS = libosxcart.la
OsxcartRtf_1_0_gir_INCLUDES = Gtk-2.0
OsxcartRtf_1_0_gir_CFLAGS = @OSXCART_CFLAGS@  # Otherwise can't find gtk/gtk.h?
OsxcartRtf_1_0_gir_SCANNERFLAGS = \
	--identifier-prefix=Rtf \
	--c-include='osxcart/rtf.h' \
	--warn-all
INTROSPECTION_GIRS += OsxcartRtf-1.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)

if BUILDING_VAPI
vapidir = $(datadir)/vala/vapi
dist_vapi_DATA = osxcart.vapi

osxcart.vapi: $(INTROSPECTION_GIRS)
	$(AM_V_GEN)$(VAPIGEN) --library=osxcart --pkg gtk+-2.0 $(INTROSPECTION_GIRS) && \
	touch $@  # Doesn't update the timestamp if unchanged?

MAINTAINERCLEANFILES += osxcart.vapi

endif # BUILDING_VAPI
endif # HAVE_INTROSPECTION

GITIGNOREFILES = \
	OsxcartPlist-1.0.gir OsxcartPlist-1.0.typelib \
	OsxcartRtf-1.0.gir OsxcartRtf-1.0.typelib \
	osxcart.vapi

-include $(top_srcdir)/git.mk
