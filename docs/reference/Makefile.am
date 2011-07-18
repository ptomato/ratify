## Process this file with automake to produce Makefile.in

# We require automake 1.6 at least.
AUTOMAKE_OPTIONS = 1.6

# This is a blank Makefile.am for using gtk-doc.
# Copy this to your project's API docs directory and modify the variables to
# suit your project. See the GTK+ Makefiles in gtk+/docs/reference for examples
# of using the various options.
	
# The name of the module, e.g. 'glib'.
DOC_MODULE=osxcart

# Uncomment for versioned docs and specify the version of the module, e.g. '2'.
#DOC_MODULE_VERSION=2

# The top-level SGML file. You can change this if you want to.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting the functions and macros.
DOC_SOURCE_DIR=../../osxcart

# Extra options to pass to gtkdoc-scangobj. Not normally needed.
SCANGOBJ_OPTIONS=

# Extra options to supply to gtkdoc-scan.
SCAN_OPTIONS=--rebuild-types --rebuild-sections

# Extra options to supply to gtkdoc-mkdb.
MKDB_OPTIONS=--sgml-mode --output-format=xml
	
# Extra options to supply to gtkdoc-mktmpl
MKTMPL_OPTIONS=
	
# Extra options to supply to gtkdoc-mkhtml
MKHTML_OPTIONS=
	
# Extra options to supply to gtkdoc-fixref. Not normally needed.
FIXXREF_OPTIONS=

# Used for dependencies. The docs will be rebuilt if any of these change.
HFILE_GLOB=$(top_srcdir)/osxcart/*.h
CFILE_GLOB=$(top_srcdir)/osxcart/*.c

# Extra header to include when scanning, which are not under DOC_SOURCE_DIR
EXTRA_HFILES=

# Header files to ignore when scanning. Use base file name, no paths
IGNORE_HFILES=plist-common.h rtf-deserialize.h rtf-document.h rtf-ignore.h rtf-state.h

# Images to copy into HTML directory.
HTML_IMAGES=

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE).
content_files=version.xml

# SGML files where gtk-doc abbrevations (#GtkWidget) are expanded
# These files must be listed here *and* in content_files
expand_content_files=

# CFLAGS and LDFLAGS for compiling gtkdoc-scangobj with your library.
# Only needed if you are using gtkdoc-scangobj to dynamically query widget
# signals and properties.
GTKDOC_CFLAGS=
GTKDOC_LIBS=

# This includes the standard gtk-doc make rules, copied by gtkdocize.
include $(top_srcdir)/gtk-doc.make

# Other files to distribute
EXTRA_DIST += version.xml.in

# Files not to distribute
DISTCLEANFILES = \
	$(DOC_MODULE).types \
	$(DOC_MODULE)-sections.txt \
	$(DOC_MODULE)-overrides.txt

# Comment this out if you want your docs-status tested during 'make check'
if ENABLE_GTK_DOC
TESTS_ENVIRONMENT = cd $(srcdir) &&
TESTS = $(GTKDOC_CHECK)
endif 

-include $(top_srcdir)/git.mk
