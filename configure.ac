# Human-readable configure.ac
### REQUIREMENTS ###############################################################

AC_PREREQ([2.61]) # Version in Debian stable

### INITIALIZATION #############################################################

AC_INIT([osxcart], [0.1])
AC_CONFIG_SRCDIR([testcases/test.c]) # Sanity check
AM_INIT_AUTOMAKE([-Wall])
AM_SILENT_RULES # Shut up and tell libtool to shut up too

### PROGRAMS ###################################################################

AC_PROG_CC                 # C compiler
AM_PROG_CC_C_O             # Required for per-target CFLAGS
AC_PROG_INSTALL            # Install
m4_defun([_LT_AC_LANG_CXX_CONFIG], [:]) # Disable unnecessary Libtool checks
m4_defun([_LT_AC_LANG_F77_CONFIG], [:]) # to save time (1.5 only)
AC_PROG_LIBTOOL            # Libtool (1.5 series)
PKG_PROG_PKG_CONFIG        # Pkg-config
AM_GLIB_GNU_GETTEXT        # Use GLib gettext instead of standard
IT_PROG_INTLTOOL([0.35.0]) # Intltool
GTK_DOC_CHECK(1.9)         # Gtk-Doc

### LIBRARIES ##################################################################

PKG_CHECK_EXISTS([glib-2.0 >= 2.18], [glib218=yes], [glib218=no])
AM_CONDITIONAL([GLIB_218], [test x$glib218 = xyes])
GLIB_REQUIRED_VERSION=2.16
GTK_REQUIRED_VERSION=2.10
GDK_PIXBUF_REQUIRED_VERSION=2.6
PKG_CHECK_MODULES([OSXCART], [
	glib-2.0 >= $GLIB_REQUIRED_VERSION
	gdk-2.0
	gdk-pixbuf-2.0 >= $GDK_PIXBUF_REQUIRED_VERSION
	gtk+-2.0 >= $GTK_REQUIRED_VERSION
])
PKG_CHECK_MODULES([TEST], [
	glib-2.0 >= $GLIB_REQUIRED_VERSION
	gtk+-2.0 >= $GTK_REQUIRED_VERSION
])
AC_SUBST(GLIB_REQUIRED_VERSION)
AC_SUBST(GTK_REQUIRED_VERSION)
AC_SUBST(GDK_PIXBUF_REQUIRED_VERSION)

### HEADER FILES ###############################################################

AC_CHECK_HEADERS([stdlib.h string.h])
AC_CHECK_FUNCS([strchr strtol])

# COMPILER CHARACTERISTICS #####################################################

AC_C_INLINE

### INTERNATIONALIZATION ######################################################

AM_GLIB_DEFINE_LOCALEDIR(LOCALEDIR)
GETTEXT_PACKAGE=osxcart
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])

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

