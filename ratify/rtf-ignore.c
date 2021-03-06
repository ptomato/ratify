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

#include <glib.h>

#include "rtf-deserialize.h"
#include "rtf-ignore.h"

/* rtf-ignore.c - Used to ignore destinations that are not implemented */

const ControlWord ignore_word_table[] = {{ NULL }};

const DestinationInfo ignore_destination = {
    ignore_word_table,
    ignore_pending_text,
    ignore_state_new,
    ignore_state_copy,
    ignore_state_free
};

void
ignore_pending_text(ParserContext *ctx)
{
    g_string_truncate(ctx->text, 0);
}

void *
ignore_state_new(void)
{
    return NULL;
}

void *
ignore_state_copy(const void *state)
{
    return NULL;
}

void
ignore_state_free(void *state)
{
}
