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
