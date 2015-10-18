SUBDIRS = . docs/reference po

### DEFINE VARIABLES ###########################################################

EXTRA_DIST = \
	README.md \
	COPYING \
	COPYING.LESSER \
	AUTHORS.md \
	ChangeLog \
	m4/introspection.m4
CLEANFILES =
DISTCLEANFILES =

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
AM_DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc --enable-introspection
AM_CFLAGS = $(WARNINGFLAGS)

### C LIBRARY AND HEADERS ######################################################

plist_public_headers = osxcart/plist.h
plist_introspection_sources = \
	osxcart/plist.c \
	osxcart/plist-parser.c \
	osxcart/plist-write.c \
	$(NULL)
plist_sources = $(plist_introspection_sources)
rtf_public_headers = osxcart/rtf.h
rtf_introspection_sources = osxcart/rtf.c
rtf_sources = \
	$(rtf_introspection_sources) \
	osxcart/rtf-colortbl.c \
	osxcart/rtf-deserialize.c \
	osxcart/rtf-deserialize.h \
	osxcart/rtf-document.c \
	osxcart/rtf-document.h \
	osxcart/rtf-field.c \
	osxcart/rtf-fonttbl.c \
	osxcart/rtf-footnote.c \
	osxcart/rtf-ignore.c \
	osxcart/rtf-ignore.h \
	osxcart/rtf-langcode.c \
	osxcart/rtf-langcode.h \
	osxcart/rtf-picture.c \
	osxcart/rtf-serialize.c \
	osxcart/rtf-serialize.h \
	osxcart/rtf-state.c \
	osxcart/rtf-state.h \
	osxcart/rtf-stylesheet.c \
	$(NULL)

lib_LTLIBRARIES = libosxcart-@OSXCART_API_VERSION@.la
libosxcart_@OSXCART_API_VERSION@_la_SOURCES = \
	osxcart/init.c \
	osxcart/init.h \
	$(plist_public_headers) \
	$(plist_sources) \
	$(rtf_public_headers) \
	$(rtf_sources) \
	$(NULL)
libosxcart_@OSXCART_API_VERSION@_la_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"@PACKAGE_NAME@\" \
	-DLOCALEDIR=\""$(datadir)/locale"\" \
	-I$(top_srcdir) \
	$(NULL)
libosxcart_@OSXCART_API_VERSION@_la_CFLAGS = @OSXCART_CFLAGS@ $(AM_CFLAGS)
libosxcart_@OSXCART_API_VERSION@_la_LIBADD = @OSXCART_LIBS@
libosxcart_@OSXCART_API_VERSION@_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO) \
	-no-undefined \
	-export-symbols-regex "^(plist|rtf)_" \
	$(NULL)

libosxcart_includedir = $(includedir)/osxcart-@OSXCART_API_VERSION@/osxcart
libosxcart_include_HEADERS = \
	$(plist_public_headers) \
	$(rtf_public_headers) \
	$(NULL)

### PKGCONFIG FILE #############################################################

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = osxcart-@OSXCART_API_VERSION@.pc
DISTCLEANFILES += $(pkgconfig_DATA)

### GOBJECT INTROSPECTION BINDINGS #############################################

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir) --warn-all
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)

if HAVE_INTROSPECTION

# There can't be two namespaces in one .gir file?

OsxcartPlist-@OSXCART_API_VERSION@.gir: libosxcart-@OSXCART_API_VERSION@.la
OsxcartPlist_@OSXCART_API_VERSION@_gir_FILES = \
	$(plist_public_headers) \
	$(plist_introspection_sources) \
	$(NULL)
OsxcartPlist_@OSXCART_API_VERSION@_gir_LIBS = \
	libosxcart-@OSXCART_API_VERSION@.la \
	$(NULL)
OsxcartPlist_@OSXCART_API_VERSION@_gir_INCLUDES = GLib-2.0 Gio-2.0
OsxcartPlist_@OSXCART_API_VERSION@_gir_SCANNERFLAGS = \
	--identifier-prefix=Plist \
	--c-include='osxcart/plist.h' \
	$(NULL)
INTROSPECTION_GIRS += OsxcartPlist-@OSXCART_API_VERSION@.gir

OsxcartRtf-@OSXCART_API_VERSION@.gir: libosxcart-@OSXCART_API_VERSION@.la
OsxcartRtf_@OSXCART_API_VERSION@_gir_FILES = \
	$(rtf_public_headers) \
	$(rtf_introspection_sources) \
	$(NULL)
OsxcartRtf_@OSXCART_API_VERSION@_gir_LIBS = libosxcart-@OSXCART_API_VERSION@.la
OsxcartRtf_@OSXCART_API_VERSION@_gir_INCLUDES = Gtk-2.0
OsxcartRtf_@OSXCART_API_VERSION@_gir_CFLAGS = @OSXCART_CFLAGS@
# Otherwise can't find gtk/gtk.h?
OsxcartRtf_@OSXCART_API_VERSION@_gir_SCANNERFLAGS = \
	--identifier-prefix=Rtf \
	--c-include='osxcart/rtf.h' \
	$(NULL)
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

### TESTS ######################################################################

check_PROGRAMS = test-osxcart
test_osxcart_CPPFLAGS = \
	-I$(top_srcdir) \
	-DTESTFILEDIR=\""$(abs_srcdir)/testcases"\" \
	$(NULL)
test_osxcart_SOURCES = \
	testcases/test-plist.c \
	testcases/test-rtf.c \
	testcases/test.c \
	$(NULL)
test_osxcart_CFLAGS = @TEST_CFLAGS@ $(AM_CFLAGS)
test_osxcart_LDADD = @TEST_LIBS@ libosxcart-@OSXCART_API_VERSION@.la

TESTS = test-osxcart
AM_TESTS_ENVIRONMENT = export GNOME_DISABLE_CRASH_DIALOG=1;
LOG_COMPILER = $(XVFB_RUN) gtester
AM_LOG_FLAGS = -k

# Extra targets that build a GTester report
.PHONY: test-report perf-report full-report
test-report: test-osxcart
	GNOME_DISABLE_CRASH_DIALOG=1 gtester -k test-osxcart -o=testlog.xml; \
	gtester-report testlog.xml > testlog.html
perf-report: test-osxcart
	GNOME_DISABLE_CRASH_DIALOG=1 gtester -k -m=perf test-osxcart -o=testlog.xml; \
	gtester-report testlog.xml > testlog.html
full-report: test-osxcart
	GNOME_DISABLE_CRASH_DIALOG=1 gtester -k -m=perf -m=thorough test-osxcart -o=testlog.xml; \
	gtester-report testlog.xml > testlog.html
CLEANFILES += testlog.xml testlog.html

plist_testcases = \
	testcases/Settings.plist \
	testcases/badlyformed.fail.plist \
	testcases/extraneous_key.fail.plist \
	testcases/extraneous_object.fail.plist \
	testcases/incomplete.fail.plist \
	testcases/manifest.plist \
	testcases/missing_key.fail.plist \
	testcases/missing_version.fail.plist \
	testcases/no_data.fail.plist \
	testcases/no_plist.fail.plist \
	testcases/oneofeach.plist \
	testcases/wrong_attribute.fail.plist \
	testcases/wrong_version.fail.plist \
	$(NULL)
rtf_testcases = \
	testcases/p004_hello_world.rtf \
	testcases/p005a_hello_world.rtf \
	testcases/p005b_hello_world.rtf \
	testcases/p005c_hello_world.rtf \
	testcases/p006a_hello_world.rtf \
	testcases/p006b_hello_world.rtf \
	testcases/p007_salvete_omnes.rtf \
	testcases/p014_annalum.rtf \
	testcases/p015_annalum.rtf \
	testcases/p018_annalum.rtf \
	testcases/p023_martial.rtf \
	testcases/p024_annalum.rtf \
	testcases/p025_annalum.rtf \
	testcases/p027_grep.rtf \
	testcases/p032a_scream.rtf \
	testcases/p032b_scream.rtf \
	testcases/p034_daodejing.rtf \
	testcases/p044_vote.rtf \
	testcases/p045_vote.rtf \
	testcases/p046_vote_with_prelim.rtf \
	testcases/p048_zimgir.rtf \
	testcases/p049a_nondualist.rtf \
	testcases/p049b_supermegacomment.rtf \
	testcases/p050_seti_loewe.rtf \
	testcases/p051a_mysterious.rtf \
	testcases/p051b_chaucer.rtf \
	testcases/p052a_chaucer.rtf \
	testcases/p053_columns.rtf \
	testcases/p055_columns.rtf \
	testcases/p056_footnotes.rtf \
	testcases/p057_yow.rtf \
	testcases/p058a_yow.rtf \
	testcases/p058b_link.rtf \
	testcases/p058c_link.rtf \
	testcases/p059_margins.rtf \
	testcases/p060_horizrule.rtf \
	testcases/p060_landscape_a4.rtf \
	testcases/p060_landscape_usletter.rtf \
	testcases/p060_twoup_a4.rtf \
	testcases/p060_twoup_us.rtf \
	testcases/p061_horizrule.rtf \
	testcases/p062_horizrule.rtf \
	testcases/p063a_image.rtf \
	testcases/p063b_image.rtf \
	testcases/p063c_image.rtf \
	testcases/p065_vhcenter.rtf \
	testcases/p069_styles.rtf \
	testcases/p076a_table_single_cell_no_border.rtf \
	testcases/p076b_table_2x1_no_border.rtf \
	testcases/p076c_table_2x2_no_border.rtf \
	testcases/p077_table_2x2_groups_no_border.rtf \
	testcases/p078_table_cell_stretches_down_still_no_border.rtf \
	testcases/p082_table_all_borders.rtf \
	testcases/p083_table_some_borders.rtf \
	testcases/p085a_table_v_alignment.rtf \
	testcases/p085b_table_h_alignment.rtf \
	testcases/p086a_table_all_alignments.rtf \
	testcases/p086b_table_all_alignments_with_borders.rtf \
	testcases/crazy_fonttable.rtf \
	testcases/outtake_crazy_fonttable.rtf \
	testcases/outtake_latin.rtf \
	testcases/outtake_simplicity.rtf \
	testcases/DefaultText.rtf \
	testcases/minimal.rtf \
	testcases/RtfInterpreterTest_0.rtf \
	testcases/RtfInterpreterTest_1.rtf \
	testcases/RtfInterpreterTest_2.rtf \
	testcases/RtfInterpreterTest_3.rtf \
	testcases/RtfInterpreterTest_4.rtf \
	testcases/RtfInterpreterTest_5.rtf \
	testcases/RtfInterpreterTest_6.rtf \
	testcases/RtfInterpreterTest_7.rtf \
	testcases/RtfInterpreterTest_8.rtf \
	testcases/RtfInterpreterTest_9.rtf \
	testcases/RtfInterpreterTest_10.rtf \
	testcases/RtfInterpreterTest_11.rtf \
	testcases/RtfInterpreterTest_12.rtf \
	testcases/RtfInterpreterTest_13.rtf \
	testcases/RtfInterpreterTest_14.rtf \
	testcases/RtfInterpreterTest_15.rtf \
	testcases/RtfInterpreterTest_16.rtf \
	testcases/RtfInterpreterTest_17.rtf \
	testcases/RtfInterpreterTest_18.rtf \
	testcases/RtfInterpreterTest_19.rtf \
	testcases/RtfInterpreterTest_20.rtf \
	testcases/RtfInterpreterTest_21.rtf \
	testcases/RtfInterpreterTest_22.rtf \
	testcases/RtfInterpreterTest_fail_0.rtf \
	testcases/RtfInterpreterTest_fail_1.rtf \
	testcases/RtfInterpreterTest_fail_2.rtf \
	testcases/RtfInterpreterTest_fail_3.rtf \
	testcases/RtfInterpreterTest_fail_4.rtf \
	testcases/RtfParserTest_0.rtf \
	testcases/RtfParserTest_1.rtf \
	testcases/RtfParserTest_2.rtf \
	testcases/RtfParserTest_3.rtf \
	testcases/RtfParserTest_fail_0.rtf \
	testcases/RtfParserTest_fail_1.rtf \
	testcases/RtfParserTest_fail_2.rtf \
	testcases/RtfParserTest_fail_3.rtf \
	testcases/RtfParserTest_fail_5.rtf \
	testcases/RtfParserTest_fail_6.rtf \
	testcases/rtfdtest.rtfd/TXT.rtf \
	testcases/rtfdtest.rtfd/rtfpocketguide.jpeg \
	testcases/charscalex.rtf \
	testcases/charscalexfail.rtf \
	$(NULL)
EXTRA_DIST += $(plist_testcases) $(rtf_testcases)

-include $(top_srcdir)/git.mk
