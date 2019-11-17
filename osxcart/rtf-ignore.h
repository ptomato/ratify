#pragma once

/* Copyright 2009, 2019 P. F. Chimento
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

#include <glib.h>

#include "rtf-deserialize.h"

G_GNUC_INTERNAL void ignore_pending_text(ParserContext *ctx);
G_GNUC_INTERNAL gpointer ignore_state_new(void);
G_GNUC_INTERNAL gpointer ignore_state_copy(gconstpointer state);
G_GNUC_INTERNAL void ignore_state_free(gpointer state);

extern const DestinationInfo ignore_destination;
