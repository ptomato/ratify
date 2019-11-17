#ifndef __OSXCART_RTF_STATE_H__
#define __OSXCART_RTF_STATE_H__

/* Copyright 2009, 2015 P. F. Chimento
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
#include <pango/pango.h>

typedef gpointer StateNewFunc(void);
typedef gpointer StateCopyFunc(gconstpointer);
typedef void StateFreeFunc(gpointer);

typedef struct {
    gint style; /* Index into style sheet */

    /* Paragraph formatting */

    int justification;  /* GtkJustification value or -1 if unset */
    int pardirection;  /* GtkTextDirection value or -1 if unset */
    gint space_before;
    gint space_after;
    gboolean ignore_space_before;
    gboolean ignore_space_after;
    PangoTabArray *tabs;
    gint left_margin;
    gint right_margin;
    gint indent;
    gint leading;

    /* Character formatting */

    gint foreground; /* Index into the color table */
    gint background;
    gint highlight;
    gint font; /* Index into the font table */
    gdouble size;
    gboolean italic;
    gboolean bold;
    gboolean smallcaps;
    gboolean strikethrough;
    gboolean subscript;
    gboolean superscript;
    gboolean invisible;
    int underline;  /* PangoUnderline value or -1 if unset */
    int chardirection;  /* GtkTextDirection value or -1 if unset */
    gint language;
    gint rise;
    int scale;

    /* Number of characters to skip after \u */
    gint unicode_skip;
    /* Skip characters within \upr but not \*ud */
    gboolean unicode_ignore;
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
    ((Attributes *)state)->unicode_ignore = FALSE;
#define ATTR_COPY \
    if(((Attributes *)state)->tabs) \
        ((Attributes *)copy)->tabs = pango_tab_array_copy(((Attributes *)state)->tabs);
#define ATTR_FREE \
    if(((Attributes *)state)->tabs) \
        pango_tab_array_free(((Attributes *)state)->tabs);

#define DEFINE_STATE_FUNCTIONS_FULL(tn, fn, init_code, copy_code, free_code) \
    static gpointer \
    G_PASTE_ARGS(fn, _state_new)(void) \
    { \
        tn *state = g_slice_new0(tn); \
        init_code \
        return state; \
    } \
    static gpointer \
    G_PASTE_ARGS(fn, _state_copy)(gconstpointer st) \
    { \
        const tn *state = (const tn *)st; \
        tn *copy = g_slice_dup(tn, state); \
        copy_code \
        return copy; \
    } \
    static void \
    G_PASTE_ARGS(fn, _state_free)(gpointer st) \
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

#endif /* __OSXCART_RTF_STATE_H__ */
