AM_CFLAGS = -Wall

lib_LTLIBRARIES = libosxcart.la

if GLIB_218
PLIST_PARSER_SOURCE = plist-parser.c
else
PLIST_PARSER_SOURCE = plist-parser-oldglib.c
endif

libosxcart_la_SOURCES = \
	init.c init.h \
	plist.c plist.h \
	plist-common.c plist-common.h \
	plist-write.c \
	$(PLIST_PARSER_SOURCE) \
	rtf.c rtf.h \
	rtf-serialize.c \
	rtf-deserialize.c rtf-deserialize.h \
	rtf-document.c rtf-document.h \
	rtf-fonttbl.c \
	rtf-colortbl.c \
	rtf-ignore.c \
	rtf-footnote.c \
	rtf-picture.c \
	rtf-field.c \
	rtf-stylesheet.c \
	rtf-langcode.c rtf-langcode.h 
	
libosxcart_la_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Osxcart\" \
	-DLOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"
libosxcart_la_CFLAGS = @OSXCART_CFLAGS@ $(AM_CFLAGS)
libosxcart_la_LIBADD = @OSXCART_LIBS@
libosxcart_la_LDFLAGS = -no-undefined -export-symbols-regex "^(plist|rtf)_"
libosxcart_includedir = $(includedir)/osxcart/osxcart
libosxcart_include_HEADERS = plist.h rtf.h

