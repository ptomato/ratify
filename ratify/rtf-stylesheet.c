/* Copyright 2009, 2012, 2019 P. F. Chimento
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
#include <string.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

#include "rtf.h"
#include "rtf-deserialize.h"
#include "rtf-document.h"
#include "rtf-state.h"

/* rtf-stylesheet.c - Implementation of style sheets. */

typedef enum {
    STYLE_PARAGRAPH,
    STYLE_CHARACTER,
    STYLE_SECTION,
    STYLE_TABLE
} StyleType;

typedef struct {
    Attributes attr; /* must be first */
    int index;
    StyleType type;
} StylesheetState;

/* Forward declarations */
static void stylesheet_text(ParserContext *ctx);

#define DEFINE_STYLE_FUNCTION(fn, styletype) \
    static bool \
    fn(ParserContext *ctx, StylesheetState *state, int32_t param, GError **error) \
    { \
        state->index = param; \
        state->type = styletype; \
        return true; \
    }
DEFINE_STYLE_FUNCTION(sty_cs, STYLE_CHARACTER)
DEFINE_STYLE_FUNCTION(sty_ds, STYLE_SECTION)
DEFINE_STYLE_FUNCTION(sty_s, STYLE_PARAGRAPH)
DEFINE_STYLE_FUNCTION(sty_ts, STYLE_TABLE)

const ControlWord stylesheet_word_table[] = {
    FORMATTED_TEXT_CONTROL_WORDS,
    { "*cs", REQUIRED_PARAMETER, true, sty_cs },
    { "*ds", REQUIRED_PARAMETER, true, sty_ds },
    { "s", OPTIONAL_PARAMETER, true, sty_s, 0 },
    { "*ts", REQUIRED_PARAMETER, true, sty_ts },
    { NULL }
};

DEFINE_ATTR_STATE_FUNCTIONS(StylesheetState, stylesheet)

const DestinationInfo stylesheet_destination = {
    stylesheet_word_table,
    stylesheet_text,
    stylesheet_state_new,
    stylesheet_state_copy,
    stylesheet_state_free
};

/* Add a style tag to the GtkTextBuffer's tag table with all the attributes of
the current style */
static void
stylesheet_text(ParserContext *ctx)
{
    StylesheetState *state = get_state(ctx);
    Attributes *attr = (Attributes *)state;

    const char *semicolon = strchr(ctx->text->str, ';');
    if (!semicolon) {
        g_string_truncate(ctx->text, 0);
        return;
    }
    g_string_assign(ctx->text, semicolon + 1); /* Leave the text after the semicolon in the buffer */

    g_autofree char *tagname = g_strdup_printf("rtf-style-%i", state->index);
    GtkTextTag *tag = gtk_text_tag_table_lookup(ctx->tags, tagname);
    if (tag != NULL)
        gtk_text_tag_table_remove(ctx->tags, tag);
    tag = gtk_text_tag_new(tagname);

    /* Add each paragraph attribute to the tag */
    if (attr->justification != -1) {
        g_object_set(tag,
                     "justification", attr->justification,
                     "justification-set", true,
                     NULL);
    }
    if (attr->pardirection != -1)
        g_object_set(tag, "direction", attr->pardirection, NULL);
    if (attr->space_before != 0 && !attr->ignore_space_before) {
        g_object_set(tag,
                     "pixels-above-lines", PANGO_PIXELS(TWIPS_TO_PANGO(attr->space_before)),
                     "pixels-above-lines-set", true,
                     NULL);
    }
    if (attr->space_after != 0 && !attr->ignore_space_after) {
        g_object_set(tag,
                     "pixels-below-lines", PANGO_PIXELS(TWIPS_TO_PANGO(attr->space_after)),
                     "pixels-below-lines-set", true,
                     NULL);
    }
    if (attr->tabs) {
        g_object_set(tag,
                     "tabs", attr->tabs,
                     "tabs-set", true,
                     NULL);
    }
    if (attr->left_margin) {
        g_object_set(tag,
                     "left-margin", PANGO_PIXELS(TWIPS_TO_PANGO(attr->left_margin)),
                     "left-margin-set", true,
                     NULL);
    }
    if (attr->right_margin) {
        g_object_set(tag,
                     "right-margin", PANGO_PIXELS(TWIPS_TO_PANGO(attr->right_margin)),
                     "right-margin-set", true,
                     NULL);
    }
    if (attr->indent) {
        g_object_set(tag,
                     "indent", PANGO_PIXELS(TWIPS_TO_PANGO(attr->indent)),
                     "indent-set", true,
                     NULL);
    }
    if (attr->scale != 100) {
        g_object_set(tag,
                     "scale", (double)(attr->scale) / 100.0,
                     "scale-set", true,
                     NULL);
    }

    /* Add each character attribute to the tag */
    if (attr->foreground != -1) {
        const char *color = g_slist_nth_data(ctx->color_table, attr->foreground);
        /* color must exist, because that was already checked when executing
         the \cf command */
        g_object_set(tag,
                     "foreground", color,
                     "foreground-set", true,
                     NULL);
    }
    if (attr->background != -1) {
        const char *color = g_slist_nth_data(ctx->color_table, attr->background);
        /* color must exist, because that was already checked when executing
         the \cf command */
        g_object_set(tag,
                     "background", color,
                     "background-set", true,
                     NULL);
    }
    if (attr->highlight != -1) {
        const char *color = g_slist_nth_data(ctx->color_table, attr->highlight);
        /* color must exist, because that was already checked when executing
         the \cf command */
        g_object_set(tag,
                     "paragraph-background", color,
                     "paragraph-background-set", true,
                     NULL);
    }
    if (attr->font != -1) {
        g_autofree char *tagname = g_strdup_printf("rtf-font-%d", attr->font);
        GtkTextTag *fonttag = gtk_text_tag_table_lookup(ctx->tags, tagname);
        PangoFontDescription *fontdesc;

        g_object_get(fonttag, "font-desc", &fontdesc, NULL); /* do not free */
        if (fontdesc && pango_font_description_get_family(fontdesc)) {
            g_object_set(tag,
                         "family", g_strdup(pango_font_description_get_family(fontdesc)),
                         "family-set", true,
                         NULL);
        }
    }
    if (attr->size != 0.0) {
        g_object_set(tag,
                     "size", POINTS_TO_PANGO(attr->size),
                     "size-set", true,
                     NULL);
    }
    if (attr->italic) {
        g_object_set(tag,
                     "style", PANGO_STYLE_ITALIC,
                     "style-set", true,
                     NULL);
    }
    if (attr->bold) {
        g_object_set(tag,
                     "weight", PANGO_WEIGHT_BOLD,
                     "weight-set", true,
                     NULL);
    }
    if (attr->smallcaps) {
        g_object_set(tag,
                     "variant", PANGO_VARIANT_SMALL_CAPS,
                     "variant-set", true,
                     NULL);
    }
    if (attr->strikethrough) {
        g_object_set(tag,
                     "strikethrough", true,
                     "strikethrough-set", true,
                     NULL);
    }
    if (attr->subscript) {
        g_object_set(tag,
                     "rise", POINTS_TO_PANGO(-6),
                     "rise-set", true,
                     "scale", PANGO_SCALE_X_SMALL,
                     "scale-set", true,
                     NULL);
    }
    if (attr->superscript) {
        g_object_set(tag,
                     "rise", POINTS_TO_PANGO(6),
                     "rise-set", true,
                     "scale", PANGO_SCALE_X_SMALL,
                     "scale-set", true,
                     NULL);
    }
    if (attr->invisible) {
        g_object_set(tag,
                     "invisible", true,
                     "invisible-set", true,
                     NULL);
    }
    if (attr->underline != -1) {
        g_object_set(tag,
                     "underline", attr->underline,
                     "underline-set", true,
                     NULL);
    }
    if (attr->chardirection != -1)
        g_object_set(tag, "direction", attr->chardirection, NULL);
    if (attr->rise != 0) {
        g_object_set(tag,
                     "rise", HALF_POINTS_TO_PANGO(attr->rise),
                     "rise-set", true,
                     NULL);
    }

    gtk_text_tag_table_add(ctx->tags, tag);

    state->index = 0;
    state->type = STYLE_PARAGRAPH;
    set_default_paragraph_attributes(attr);
    set_default_character_attributes(attr);
}
