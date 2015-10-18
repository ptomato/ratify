# Human-readable configure.ac
### REQUIREMENTS ###############################################################

AC_PREREQ([2.62]) # Earliest version that works with gobject introspection

### INITIALIZATION #############################################################

AC_INIT([Osxcart], [1.2.1])
AC_CONFIG_SRCDIR([testcases/test.c]) # Sanity check
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([1.11 -Wall -Wno-portability foreign parallel-tests
	subdir-objects dist-xz no-dist-gzip])
# Shut up and tell libtool to shut up too
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

### LIBRARY VERSIONING #########################################################

dnl Only update immediately before a public release.
dnl current:revision:age
dnl 1. If source code has changed at all, then increment revision.
dnl 2. If any interfaces have been added, changed, or removed, increment current
dnl    and set revision to 0.
dnl 3. If any interfaces have been added, increment age.
dnl 4. If any interfaces have been removed or changed, set age to 0.
AC_SUBST([LT_VERSION_INFO], [1:1:1])
OSXCART_API_VERSION=0
AC_SUBST([OSXCART_API_VERSION])

### PROGRAMS ###################################################################

AC_PROG_CC                 # C compiler
AM_PROG_CC_C_O             # Required for per-target CFLAGS
AC_PROG_INSTALL            # Install
AC_PATH_PROG([TEST], [test])
AC_PATH_PROG([ECHO], [echo])
LT_INIT                    # Libtool
PKG_PROG_PKG_CONFIG        # Pkg-config
AM_GNU_GETTEXT([external]) # Gettext, link to system libintl
GTK_DOC_CHECK([1.14], [--copy --flavour no-tmpl])
dnl Adding arguments in configure.ac requires gtk-doc 1.14

### LIBRARIES ##################################################################

OSXCART_REQUIRES="glib-2.0 >= 2.18 gio-2.0 >= 2.18 gdk-2.0 gtk+-2.0 >= 2.10"
OSXCART_REQUIRES_PRIVATE="gdk-pixbuf-2.0 >= 2.6"
PKG_CHECK_MODULES([OSXCART], [$OSXCART_REQUIRES $OSXCART_REQUIRES_PRIVATE])
PKG_CHECK_MODULES([TEST], [glib-2.0 gtk+-2.0])
AC_SUBST([OSXCART_REQUIRES])
AC_SUBST([OSXCART_REQUIRES_PRIVATE])

### HEADER FILES ###############################################################

AC_CHECK_HEADERS([stdlib.h string.h])
AC_CHECK_FUNCS([strchr strrchr strstr strtol])

# COMPILER CHARACTERISTICS #####################################################

AC_C_INLINE

### INTERNATIONALIZATION ######################################################

AM_GNU_GETTEXT_VERSION([0.17])
GETTEXT_PACKAGE=AC_PACKAGE_TARNAME-"$OSXCART_API_VERSION"
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])

### INTROSPECTION ##############################################################

GOBJECT_INTROSPECTION_CHECK([0.6.7])

### WHETHER TO GENERATE A .VAPI FILE ##########################################
# Requires vapigen
AC_PATH_PROG([VAPIGEN], [vapigen], [notfound])
AM_CONDITIONAL(BUILDING_VAPI, $TEST "x$VAPIGEN" != xnotfound)

### WHETHER TO RUN TESTS IN VIRTUAL X SERVER ###################################
# Requires xvfb
AC_ARG_WITH([xvfb-tests],
	AS_HELP_STRING([--with-xvfb-tests],
		[Use xvfb to run tests, e.g. on build servers @<:@default=no@:>@]))
AS_IF([test "x$with_xvfb_tests" != "xno"],
	[AC_PATH_PROG([XVFB], [xvfb-run], [notfound])])
AS_IF([test "x$XVFB" != "xnotfound"],
	[AC_SUBST([XVFB_RUN], ["$XVFB "])],  dnl can add other xvfb-run args here
	[AS_IF([test "x$with_vfb_tests" = "yes"],
		[AC_MSG_ERROR([XVFB tests requested, but xvfb-run not found])])])

### DEFINE OTHER VARIABLES #####################################################

# Compiler flags
# Justification for -Wno-missing-field-initializers: We make extensive use of
# partially initializing structs, and the code for all the control word tables
# would be quite verbose if we didn't.
# Justification for -Wno-unused-parameter: GLib's callback-style code has many
# callback types with certain signatures, and more often than not we don't need
# to use all the parameters passed to a callback.
AX_APPEND_COMPILE_FLAGS([-Wall -Wextra -Wno-missing-field-initializers \
	-Wno-unused-parameter], [WARNINGFLAGS])
AC_SUBST([WARNINGFLAGS])

# DECLARE OUTPUT FILES #########################################################

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
	Makefile
	docs/reference/Makefile
	docs/reference/version.xml
	po/Makefile.in
])
AC_CONFIG_FILES([osxcart-$OSXCART_API_VERSION.pc:osxcart.pc.in], [],
	[OSXCART_API_VERSION='$OSXCART_API_VERSION'])

# Do it
AC_OUTPUT
