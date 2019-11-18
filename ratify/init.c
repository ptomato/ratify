/* Copyright 2009, 2019 P. F. Chimento
This file is part of Ratify.

Ratify is free software: you can redistribute it and/or modify it under the
terms of the GNU Lesser General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

Ratify is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with Ratify.  If not, see <http://www.gnu.org/licenses/>. */

#include "config.h"

#include <stdbool.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

/* This function is called at every entry point of the library, as suggested in
chapter 4.10 of the gettext manual. It sets up gettext for the library. */
void
rtf_init(void)
{
    static bool initialized = false;
    if (G_UNLIKELY(!initialized)) {
        bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
        bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    }
}
