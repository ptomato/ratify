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
#include <gtk/gtk.h>
#include "rtf-state.h"

typedef struct _ParserContext ParserContext;
typedef struct _ControlWord ControlWord;
typedef struct _Destination Destination;
typedef struct _DestinationInfo DestinationInfo;

#define POINTS_TO_PANGO(pts) ((gint)(pts * PANGO_SCALE))
#define HALF_POINTS_TO_PANGO(halfpts) (halfpts * PANGO_SCALE / 2)
#define TWIPS_TO_PANGO(twips) (twips * PANGO_SCALE / 20)

struct _ParserContext {
    /* Header information */
    gint codepage;
    gint default_codepage;
    gint default_font;
    gint default_language;

    /* Destination stack management */
    gint group_nesting_level;
    GQueue *destination_stack;

    /* Tables */
    GSList *color_table;
    GSList *font_table;

    /* Other document attributes */
    gint footnote_number;

    /* Text information */
    const gchar *rtftext;
    const gchar *pos;
    GString *convertbuffer;
    /* Text waiting for insertion */
    GString *text;

    /* Output references */
    GtkTextBuffer *textbuffer;
    GtkTextTagTable *tags;
    GtkTextMark *startmark;
    GtkTextMark *endmark;
};

struct _DestinationInfo {
    const ControlWord *word_table;
    void (*flush)(ParserContext *);
    StateNewFunc *state_new;
    StateCopyFunc *state_copy;
    StateFreeFunc *state_free;
    void (*cleanup)(ParserContext *);
    gint (*get_codepage)(ParserContext *);
};

typedef enum {
    NO_PARAMETER,
    OPTIONAL_PARAMETER,
    REQUIRED_PARAMETER,
    SPECIAL_CHARACTER,
    DESTINATION
} ControlWordType;

struct _ControlWord {
    const gchar *word;
    ControlWordType type;
    gboolean flush_buffer;
    gboolean (*action)();
    gint32 defaultparam;
    const gchar *replacetext;
    const DestinationInfo *destinfo;
};

struct _Destination {
    gint nesting_level;
    GQueue *state_stack;
    const DestinationInfo *info;
};

typedef struct {
    gint index;
    gint codepage;
    gchar *font_name;
} FontProperties;

G_GNUC_INTERNAL void push_new_destination(ParserContext *ctx, const DestinationInfo *destinfo, gpointer state_to_copy);
G_GNUC_INTERNAL gpointer get_state(ParserContext *ctx);
G_GNUC_INTERNAL FontProperties *get_font_properties(ParserContext *ctx, int index);
G_GNUC_INTERNAL void flush_text(ParserContext *ctx);
G_GNUC_INTERNAL gboolean skip_character_or_control_word(ParserContext *ctx, GError **error);
G_GNUC_INTERNAL gboolean rtf_deserialize(GtkTextBuffer *register_buffer, GtkTextBuffer *content_buffer, GtkTextIter *iter, const gchar *data, gsize length, gboolean create_tags, gpointer user_data, GError **error);
