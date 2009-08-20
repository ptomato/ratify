#!/bin/sh
### autogen.sh with sensible comments ###############################

### CREATE MACRO DIRECTORY ##########################################
# Needed by Gtk-Doc
mkdir -p m4

### GTK-DOC #########################################################
# Run before autotools
echo "Setting up Gtk-Doc"
gtkdocize --copy --flavour no-tmpl || exit 1

### AUTOTOOLS #######################################################
# Runs autoconf, autoheader, aclocal, automake, autopoint, libtoolize
echo "Regenerating autotools files"
autoreconf --force --install || exit 1
