# Human-readable configure.ac
### REQUIREMENTS ###############################################################

AC_PREREQ([2.62]) # Earliest version that works with gobject introspection

### INITIALIZATION #############################################################

AC_INIT([Osxcart], [1.1])
AC_CONFIG_SRCDIR([testcases/test.c]) # Sanity check
AC_CONFIG_MACRO_DIR(m4)
AM_INIT_AUTOMAKE([1.11 -Wall -Wno-portability foreign parallel-tests])
# Shut up and tell libtool to shut up too
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

### LIBRARY VERSIONING #########################################################
# Only update immediately before a public release

OSXCART_CURRENT=1  # Increment if interface changes
OSXCART_REVISION=0 # Increment if source code changes; set 0 if interface
OSXCART_AGE=1 # Increment if interfaces added; set 0 if removed
LT_VERSION_INFO="$OSXCART_CURRENT:$OSXCART_REVISION:$OSXCART_AGE"
AC_SUBST(LT_VERSION_INFO)

### PROGRAMS ###################################################################

AC_PROG_CC                 # C compiler
AM_PROG_CC_C_O             # Required for per-target CFLAGS
AC_PROG_INSTALL            # Install
AC_PATH_PROG([TEST], [test])
AC_PATH_PROG([ECHO], [echo])
LT_INIT                    # Libtool
PKG_PROG_PKG_CONFIG        # Pkg-config
AM_GNU_GETTEXT([external]) # Gettext, link to system libintl
GTK_DOC_CHECK(1.12)        # Gtk-Doc

### LIBRARIES ##################################################################

GLIB_REQUIRED_VERSION=2.18
GIO_REQUIRED_VERSION=2.18
GTK_REQUIRED_VERSION=2.10
GDK_PIXBUF_REQUIRED_VERSION=2.6
PKG_CHECK_MODULES([OSXCART], [
	gobject-2.0
	glib-2.0 >= $GLIB_REQUIRED_VERSION
	gio-2.0 >= $GIO_REQUIRED_VERSION
	gdk-2.0
	gdk-pixbuf-2.0 >= $GDK_PIXBUF_REQUIRED_VERSION
	gtk+-2.0 >= $GTK_REQUIRED_VERSION
])
PKG_CHECK_MODULES([TEST], [
	glib-2.0 >= $GLIB_REQUIRED_VERSION
	gtk+-2.0 >= $GTK_REQUIRED_VERSION
])
AC_SUBST(GLIB_REQUIRED_VERSION)
AC_SUBST(GIO_REQUIRED_VERSION)
AC_SUBST(GTK_REQUIRED_VERSION)
AC_SUBST(GDK_PIXBUF_REQUIRED_VERSION)

### HEADER FILES ###############################################################

AC_CHECK_HEADERS([stdlib.h string.h])
AC_CHECK_FUNCS([strchr strrchr strstr strtol])

# COMPILER CHARACTERISTICS #####################################################

AC_C_INLINE

### INTERNATIONALIZATION ######################################################

AM_GNU_GETTEXT_VERSION([0.17])
GETTEXT_PACKAGE=osxcart
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])

### INTROSPECTION ##############################################################

GOBJECT_INTROSPECTION_CHECK([0.6.7])

### WHETHER TO GENERATE A .VAPI FILE ##########################################
# Requires vapigen
AC_PATH_PROG([VAPIGEN], [vapigen], [notfound])
AM_CONDITIONAL(BUILDING_VAPI, $TEST "x$VAPIGEN" != xnotfound)

# DECLARE OUTPUT FILES #########################################################

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
	osxcart.pc
	Makefile
	osxcart/Makefile
	testcases/Makefile
	docs/Makefile
	docs/reference/Makefile
	docs/reference/version.xml
	po/Makefile.in
])

# Do it
AC_OUTPUT
