#pragma once

/* Copyright 2009, 2015, 2019 P. F. Chimento
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

#include <stdbool.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <pango/pango.h>

typedef void *StateNewFunc(void);
typedef void *StateCopyFunc(const void *);
typedef void StateFreeFunc(void *);

typedef struct {
    int style; /* Index into style sheet */

    /* Paragraph formatting */

    int justification;  /* GtkJustification value or -1 if unset */
    int pardirection;  /* GtkTextDirection value or -1 if unset */
    int space_before;
    int space_after;
    bool ignore_space_before;
    bool ignore_space_after;
    PangoTabArray *tabs;
    int left_margin;
    int right_margin;
    int indent;
    int leading;

    /* Character formatting */

    int foreground; /* Index into the color table */
    int background;
    int highlight;
    int font; /* Index into the font table */
    double size;
    bool italic;
    bool bold;
    bool smallcaps;
    bool strikethrough;
    bool subscript;
    bool superscript;
    bool invisible;
    int underline;  /* PangoUnderline value or -1 if unset */
    int chardirection;  /* GtkTextDirection value or -1 if unset */
    int language;
    int rise;
    int scale;

    /* Number of characters to skip after \u */
    int unicode_skip;
    /* Skip characters within \upr but not \*ud */
    bool unicode_ignore;
} Attributes;

G_GNUC_INTERNAL void set_default_character_attributes(Attributes *attr);
G_GNUC_INTERNAL void set_default_paragraph_attributes(Attributes *attr);

#ifndef G_PASTE_ARGS /* available since 2.20 */
#define G_PASTE_ARGS(identifier1,identifier2) identifier1 ## identifier2
#endif /* G_PASTE_ARGS */

#define ATTR_NEW \
    set_default_paragraph_attributes((Attributes *)state); \
    set_default_character_attributes((Attributes *)state); \
    ((Attributes *)state)->unicode_skip = 1; \
    ((Attributes *)state)->unicode_ignore = false;
#define ATTR_COPY \
    if (((Attributes *)state)->tabs) \
        ((Attributes *)copy)->tabs = pango_tab_array_copy(((Attributes *)state)->tabs);
#define ATTR_FREE \
    if (((Attributes *)state)->tabs) \
        pango_tab_array_free(((Attributes *)state)->tabs);

#define DEFINE_STATE_FUNCTIONS_FULL(tn, fn, init_code, copy_code, free_code) \
    static void * \
    G_PASTE_ARGS(fn, _state_new)(void) \
    { \
        tn *state = g_slice_new0(tn); \
        init_code \
        return state; \
    } \
    static void * \
    G_PASTE_ARGS(fn, _state_copy)(const void *st) \
    { \
        const tn *state = (const tn *)st; \
        tn *copy = g_slice_dup(tn, state); \
        copy_code \
        return copy; \
    } \
    static void \
    G_PASTE_ARGS(fn, _state_free)(void *st) \
    { \
        tn *state = (tn *)st; \
        free_code \
        g_slice_free(tn, state); \
    }

#define DEFINE_SIMPLE_STATE_FUNCTIONS(tn, fn) \
    DEFINE_STATE_FUNCTIONS_FULL(tn, fn, ;, ;, ;)
#define DEFINE_ATTR_STATE_FUNCTIONS(tn, fn) \
    DEFINE_STATE_FUNCTIONS_FULL(tn, fn, ATTR_NEW, ATTR_COPY, ATTR_FREE)
#define DEFINE_STATE_FUNCTIONS_WITH_INIT(tn, fn, init_code) \
    DEFINE_STATE_FUNCTIONS_FULL(tn, fn, init_code, ;, ;)
