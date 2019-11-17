/* Copyright 2009 P. F. Chimento
This file is part of Osxcart.

Osxcart is free software: you can redistribute it and/or modify it under the
terms of the GNU Lesser General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

Osxcart is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with Osxcart.  If not, see <http://www.gnu.org/licenses/>. */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

static gboolean osxcart_initialized = FALSE;

/* This function is called at every entry point of the library, as suggested in
chapter 4.10 of the gettext manual. It sets up gettext for the library. */
void
osxcart_init(void)
{
    if(G_UNLIKELY(!osxcart_initialized))
    {
        bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
        bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    }
}
