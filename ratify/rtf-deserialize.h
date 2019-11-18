#pragma once

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

#include <stdbool.h>

#include <glib.h>
#include <gtk/gtk.h>

#include "rtf-state.h"

typedef struct _ParserContext ParserContext;
typedef struct _ControlWord ControlWord;
typedef struct _Destination Destination;
typedef struct _DestinationInfo DestinationInfo;

#define POINTS_TO_PANGO(pts) ((int)(pts * PANGO_SCALE))
#define HALF_POINTS_TO_PANGO(halfpts) (halfpts * PANGO_SCALE / 2)
#define TWIPS_TO_PANGO(twips) (twips * PANGO_SCALE / 20)

struct _ParserContext {
    /* Header information */
    int codepage;
    int default_codepage;
    int default_font;
    int default_language;

    /* Destination stack management */
    int group_nesting_level;
    GQueue *destination_stack;

    /* Tables */
    GSList *color_table;
    GSList *font_table;

    /* Other document attributes */
    int footnote_number;

    /* Text information */
    const char *rtftext;
    const char *pos;
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
    int (*get_codepage)(ParserContext *);
};

typedef enum {
    NO_PARAMETER,
    OPTIONAL_PARAMETER,
    REQUIRED_PARAMETER,
    SPECIAL_CHARACTER,
    DESTINATION
} ControlWordType;

struct _ControlWord {
    const char *word;
    ControlWordType type;
    bool flush_buffer;
    bool (*action)();
    int32_t defaultparam;
    const char *replacetext;
    const DestinationInfo *destinfo;
};

struct _Destination {
    int nesting_level;
    GQueue *state_stack;
    const DestinationInfo *info;
};

typedef struct {
    int index;
    int codepage;
    char *font_name;
} FontProperties;

G_GNUC_INTERNAL void push_new_destination(ParserContext *ctx, const DestinationInfo *destinfo, void *state_to_copy);
G_GNUC_INTERNAL void *get_state(ParserContext *ctx);
G_GNUC_INTERNAL FontProperties *get_font_properties(ParserContext *ctx, int index);
G_GNUC_INTERNAL void flush_text(ParserContext *ctx);
G_GNUC_INTERNAL bool skip_character_or_control_word(ParserContext *ctx, GError **error);
G_GNUC_INTERNAL bool rtf_deserialize(GtkTextBuffer *register_buffer, GtkTextBuffer *content_buffer, GtkTextIter *iter, const char *data, size_t length, bool create_tags, void *user_data, GError **error);
