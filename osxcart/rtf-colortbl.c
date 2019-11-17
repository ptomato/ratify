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

#include <string.h>
#include <glib.h>
#include "rtf-deserialize.h"

/* rtf-colortbl.c - \colortbl destination */

typedef struct {
    gint red;
    gint green;
    gint blue;
} ColorTableState;

/* Forward declarations */
static void color_table_text(ParserContext *ctx);

#define DEFINE_COLOR_TABLE_FUNCTION(arg) \
    static gboolean \
    G_PASTE_ARGS(ct_, arg)(ParserContext *ctx, ColorTableState *state, gint32 param, GError **error) \
    { \
        state->arg = param; \
        return TRUE; \
    }
DEFINE_COLOR_TABLE_FUNCTION(red)
DEFINE_COLOR_TABLE_FUNCTION(green)
DEFINE_COLOR_TABLE_FUNCTION(blue)
DEFINE_SIMPLE_STATE_FUNCTIONS(ColorTableState, colortbl)

const ControlWord colortbl_word_table[] = {
    { "red", REQUIRED_PARAMETER, TRUE, ct_red },
    { "green", REQUIRED_PARAMETER, TRUE, ct_green },
    { "blue", REQUIRED_PARAMETER, TRUE, ct_blue },
    { NULL }
};

const DestinationInfo colortbl_destination = {
    colortbl_word_table,
    color_table_text,
    colortbl_state_new,
    colortbl_state_copy,
    colortbl_state_free
};

/* If the text contains a semicolon, add the RGB code to the color table and
reset the color table state */
static void
color_table_text(ParserContext *ctx)
{
    ColorTableState *state = get_state(ctx);
    if(strchr(ctx->text->str, ';'))
    {
        gchar *color = g_strdup_printf("#%02x%02x%02x", state->red, state->green, state->blue);
        ctx->color_table = g_slist_append(ctx->color_table, color);
        state->red = state->green = state->blue = 0;
    }
    g_string_truncate(ctx->text, 0);
}
