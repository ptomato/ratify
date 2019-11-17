/* Copyright 2009, 2012, 2019 P. F. Chimento
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

#include "config.h"

#include <stdarg.h>
#include <string.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <pango/pango.h>

#include "rtf.h"
#include "rtf-deserialize.h"
#include "rtf-document.h"
#include "rtf-langcode.h"
#include "rtf-state.h"

/* rtf-document.c - Main document destination. This destination is not entirely
contained within one source file, since some other destinations share code with
it. */

/* Document destination functions */
static DocFunc doc_ansi, doc_footnote, doc_mac, doc_pc, doc_pca;
static DocParamFunc doc_ansicpg, doc_deff, doc_deflang, doc_ilvl, doc_rtf;

extern const DestinationInfo colortbl_destination, field_destination,
                             fonttbl_destination, footnote_destination,
                             nextgraphic_destination, pict_destination,
                             stylesheet_destination;

const ControlWord document_word_table[] = {
    DOCUMENT_TEXT_CONTROL_WORDS,
    { "ansi", NO_PARAMETER, FALSE, doc_ansi },
    { "ansicpg", REQUIRED_PARAMETER, FALSE, doc_ansicpg },
    { "cell", SPECIAL_CHARACTER, FALSE, NULL, 0, "\t" }, /* Fake tables */
    { "colortbl", DESTINATION, FALSE, NULL, 0, NULL, &colortbl_destination },
    { "deff", REQUIRED_PARAMETER, FALSE, doc_deff },
    { "deflang", REQUIRED_PARAMETER, FALSE, doc_deflang },
    { "field", DESTINATION, TRUE, NULL, 0, NULL, &field_destination },
    { "fonttbl", DESTINATION, FALSE, NULL, 0, NULL, &fonttbl_destination },
    { "footnote", DESTINATION, TRUE, doc_footnote, 0, NULL, &footnote_destination },
    { "header", DESTINATION, FALSE, NULL, 0, NULL, &ignore_destination },
    { "ilvl", REQUIRED_PARAMETER, FALSE, doc_ilvl },
    { "info", DESTINATION, FALSE, NULL, 0, NULL, &ignore_destination },
    { "mac", NO_PARAMETER, FALSE, doc_mac },
    { "NeXTGraphic", DESTINATION, FALSE, NULL, 0, NULL, &nextgraphic_destination }, /* Apple extension */
    { "pc", NO_PARAMETER, FALSE, doc_pc },
    { "pca", NO_PARAMETER, FALSE, doc_pca },
    { "pict", DESTINATION, FALSE, NULL, 0, NULL, &pict_destination },
    { "row", SPECIAL_CHARACTER, FALSE, NULL, 0, "\n" }, /* Fake tables */
    { "rtf", REQUIRED_PARAMETER, FALSE, doc_rtf },
    { "stylesheet", DESTINATION, FALSE, NULL, 0, NULL, &stylesheet_destination },
    { NULL }
};

DEFINE_ATTR_STATE_FUNCTIONS(Attributes, document)

const DestinationInfo document_destination = {
    document_word_table,
    document_text,
    document_state_new,
    document_state_copy,
    document_state_free,
    NULL, /* cleanup */
    document_get_codepage
};

/* Space-saving function for apply_attributes() */
static void
apply_attribute(ParserContext *ctx, GtkTextIter *start, GtkTextIter *end, gchar *format, ...)
{
    gchar *tagname;
    va_list args;

    va_start(args, format);
    tagname = g_strdup_vprintf(format, args);
    va_end(args);
    gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, tagname, start, end);
    g_free(tagname);
}

/* Apply GtkTextTags to the range from start to end, depending on the current
attributes 'attr'. */
void
apply_attributes(ParserContext *ctx, Attributes *attr, GtkTextIter *start, GtkTextIter *end)
{
    /* Tags with parameters */
    if(attr->style != -1)
        apply_attribute(ctx, start, end, "osxcart-rtf-style-%i", attr->style);
    if(attr->foreground != -1)
        apply_attribute(ctx, start, end, "osxcart-rtf-foreground-%i", attr->foreground);
    if(attr->background != -1)
        apply_attribute(ctx, start, end, "osxcart-rtf-background-%i", attr->background);
    if(attr->highlight != -1)
        apply_attribute(ctx, start, end, "osxcart-rtf-highlight-%i", attr->highlight);
    if(attr->size != 0.0)
        apply_attribute(ctx, start, end, "osxcart-rtf-fontsize-%.3f", attr->size);
    if(attr->space_before != 0 && !attr->ignore_space_before)
        apply_attribute(ctx, start, end, "osxcart-rtf-space-before-%i", attr->space_before);
    if(attr->space_after != 0 && !attr->ignore_space_after)
        apply_attribute(ctx, start, end, "osxcart-rtf-space-after-%i", attr->space_after);
    if(attr->left_margin != 0)
        apply_attribute(ctx, start, end, "osxcart-rtf-left-margin-%i", attr->left_margin);
    if(attr->right_margin != 0)
        apply_attribute(ctx, start, end, "osxcart-rtf-right-margin-%i", attr->right_margin);
    if(attr->indent != 0)
        apply_attribute(ctx, start, end, "osxcart-rtf-indent-%i", attr->indent);
    if(attr->invisible)
        gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "osxcart-rtf-invisible", start, end);
    if(attr->language != 1024)
        apply_attribute(ctx, start, end, "osxcart-rtf-language-%i", attr->language);
    if(attr->rise != 0)
        apply_attribute(ctx, start, end, "osxcart-rtf-%s-%i",
            (attr->rise > 0)? "up" : "down",
            ((attr->rise > 0)? 1 : -1) * attr->rise);
    if(attr->leading != 0)
        apply_attribute(ctx, start, end, "osxcart-rtf-leading-%i", attr->leading);
    if(attr->scale != 100)
        apply_attribute(ctx, start, end, "osxcart-rtf-scale-%i", attr->scale);
    /* Boolean tags */
    if(attr->italic)
        gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "osxcart-rtf-italic", start, end);
    if(attr->bold)
        gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "osxcart-rtf-bold", start, end);
    if(attr->smallcaps)
        gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "osxcart-rtf-smallcaps", start, end);
    if(attr->strikethrough)
        gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "osxcart-rtf-strikethrough", start, end);
    if(attr->underline == PANGO_UNDERLINE_SINGLE)
        gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "osxcart-rtf-underline-single", start, end);
    if(attr->underline == PANGO_UNDERLINE_DOUBLE)
        gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "osxcart-rtf-underline-double", start, end);
    if(attr->underline == PANGO_UNDERLINE_ERROR)
        gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "osxcart-rtf-underline-wave", start, end);
    if(attr->justification == GTK_JUSTIFY_LEFT)
        gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "osxcart-rtf-left", start, end);
    if(attr->justification == GTK_JUSTIFY_RIGHT)
        gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "osxcart-rtf-right", start, end);
    if(attr->justification == GTK_JUSTIFY_CENTER)
        gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "osxcart-rtf-center", start, end);
    if(attr->justification == GTK_JUSTIFY_FILL)
        gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "osxcart-rtf-justified", start, end);
    if(attr->pardirection == GTK_TEXT_DIR_RTL)
        gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "osxcart-rtf-right-to-left", start, end);
    if(attr->pardirection == GTK_TEXT_DIR_LTR)
        gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "osxcart-rtf-left-to-right", start, end);
    /* Character-formatting direction overrides paragraph formatting */
    if(attr->chardirection == GTK_TEXT_DIR_RTL)
        gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "osxcart-rtf-right-to-left", start, end);
    if(attr->chardirection == GTK_TEXT_DIR_LTR)
        gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "osxcart-rtf-left-to-right", start, end);
    if(attr->subscript)
        gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "osxcart-rtf-subscript", start, end);
    if(attr->superscript)
        gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "osxcart-rtf-superscript", start, end);
    /* Special */
    if(attr->font != -1)
        apply_attribute(ctx, start, end, "osxcart-rtf-font-%i", attr->font);
    else if(ctx->default_font != -1 && g_slist_length(ctx->font_table) > (unsigned)ctx->default_font)
        apply_attribute(ctx, start, end, "osxcart-rtf-font-%i", ctx->default_font);
    if(attr->tabs != NULL)
    {
        /* Create a separate tag for each PangoTabArray */
        gchar *tagname = g_strdup_printf("osxcart-rtf-tabs-%p", attr->tabs);
        GtkTextTag *tag;
        if((tag = gtk_text_tag_table_lookup(ctx->tags, tagname)) == NULL)
        {
            tag = gtk_text_tag_new(tagname);
            g_object_set(tag,
                         "tabs", attr->tabs,
                         "tabs-set", TRUE,
                         NULL);
            gtk_text_tag_table_add(ctx->tags, tag);
        }
        g_free(tagname);
        gtk_text_buffer_apply_tag(ctx->textbuffer, tag, start, end);
    }
}

/* Inserts the pending text with the current attributes. This function is called
whenever a group is opened or closed, or a control word specifies to flush the
pending text. */
void
document_text(ParserContext *ctx)
{
    GtkTextIter start, end;
    Attributes *attr;
    int length;
    gchar *text;

    g_assert(ctx != NULL);

    attr = get_state(ctx);
    text = ctx->text->str;

    if(text[0] == '\0')
        return;

    length = strlen(text) - 1;
    if(!ctx->group_nesting_level && text[length] == '\n')
        text[length] = '\0';

    if(!attr->unicode_ignore)
    {
        gtk_text_buffer_get_iter_at_mark(ctx->textbuffer, &end, ctx->endmark); /* shouldn't invalidate end, but it does? */
        gtk_text_buffer_insert(ctx->textbuffer, &end, text, -1);
        gtk_text_buffer_get_iter_at_mark(ctx->textbuffer, &start, ctx->startmark);
        gtk_text_buffer_get_iter_at_mark(ctx->textbuffer, &end, ctx->endmark);

        apply_attributes(ctx, attr, &start, &end);

        /* Move the two marks back together again */
        gtk_text_buffer_move_mark(ctx->textbuffer, ctx->startmark, &end);
    }
    g_string_truncate(ctx->text, 0);
}

/* Return the codepage associated with the current font */
gint
document_get_codepage(ParserContext *ctx)
{
    Attributes *attr = get_state(ctx);
    if(attr->font != -1)
    {
        FontProperties *fontprop = get_font_properties(ctx, attr->font);
        g_assert(fontprop);
        return fontprop->codepage;
    }
    return -1;
}

/* Control word functions */

static gboolean
doc_ansi(ParserContext *ctx, Attributes *attr, GError **error)
{
    ctx->default_codepage = 1252;
    return TRUE;
}

static gboolean
doc_ansicpg(ParserContext *ctx, Attributes *attr, gint32 codepage, GError **error)
{
    ctx->codepage = codepage;
    return TRUE;
}

gboolean
doc_b(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "osxcart-rtf-bold"))
    {
        GtkTextTag *tag = gtk_text_tag_new("osxcart-rtf-bold");
        g_object_set(tag, "weight", PANGO_WEIGHT_BOLD, NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->bold = (param != 0);
    return TRUE;
}

gboolean
doc_cb(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
    gchar *color;

    if((color = g_slist_nth_data(ctx->color_table, param)) == NULL)
    {
        g_set_error(error, RTF_ERROR, RTF_ERROR_UNDEFINED_COLOR, _("Color '%i' undefined"), param);
        return FALSE;
    }

    gchar *tagname = g_strdup_printf("osxcart-rtf-background-%i", param);
    if(!gtk_text_tag_table_lookup(ctx->tags, tagname))
    {
        GtkTextTag *tag = gtk_text_tag_new(tagname);
        g_object_set(tag,
                     "background", color,
                     "background-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    g_free(tagname);

    attr->background = param;
    return TRUE;
}

gboolean
doc_cf(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
    gchar *color;

    if((color = g_slist_nth_data(ctx->color_table, param)) == NULL)
    {
        g_set_error(error, RTF_ERROR, RTF_ERROR_UNDEFINED_COLOR, _("Color '%i' undefined"), param);
        return FALSE;
    }

    gchar *tagname = g_strdup_printf("osxcart-rtf-foreground-%i", param);
    if(!gtk_text_tag_table_lookup(ctx->tags, tagname))
    {
        GtkTextTag *tag = gtk_text_tag_new(tagname);
        g_object_set(tag,
                     "foreground", color,
                     "foreground-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    g_free(tagname);

    attr->foreground = param;
    return TRUE;
}

gboolean
doc_charscalex(ParserContext *ctx, Attributes *attr, gint32 scale, GError **error)
{
    if(scale <= 0)
    {
        g_set_error(error, RTF_ERROR, RTF_ERROR_BAD_FONT_SIZE, _("\\charscalex%d is invalid, negative or zero scales not allowed"), scale);
        return FALSE;
    }

    char *tagname = g_strdup_printf("osxcart-rtf-scale-%i", scale);
    if(!gtk_text_tag_table_lookup(ctx->tags, tagname))
    {
        GtkTextTag *tag = gtk_text_tag_new(tagname);
        g_object_set(tag,
                     "scale", (double)scale / 100.0,
                     "scale-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    g_free(tagname);

    attr->scale = scale;
    return TRUE;
}

gboolean
doc_chftn(ParserContext *ctx, Attributes *attr, GError **error)
{
    gchar *footnotestring = g_strdup_printf("%d", ctx->footnote_number);
    g_string_append(ctx->text, footnotestring);
    g_free(footnotestring);
    return TRUE;
}

static gboolean
doc_deff(ParserContext *ctx, Attributes *attr, gint32 font, GError **error)
{
    ctx->default_font = font;
    return TRUE;
}

static gboolean
doc_deflang(ParserContext *ctx, Attributes *attr, gint32 language, GError **error)
{
    ctx->default_language = language;
    return doc_lang(ctx, attr, language, error);
}

gboolean
doc_dn(ParserContext *ctx, Attributes *attr, gint32 halfpoints, GError **error)
{
    if(halfpoints != 0)
    {
        gchar *tagname = g_strdup_printf("osxcart-rtf-down-%i", halfpoints);
        if(!gtk_text_tag_table_lookup(ctx->tags, tagname))
        {
            GtkTextTag *tag = gtk_text_tag_new(tagname);
            g_object_set(tag,
                         "rise", HALF_POINTS_TO_PANGO(-halfpoints),
                         "rise-set", TRUE,
                         NULL);
            gtk_text_tag_table_add(ctx->tags, tag);
        }
        g_free(tagname);
    }

    attr->rise = -halfpoints;
    return TRUE;
}

gboolean
doc_f(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
    if(get_font_properties(ctx, param) == NULL)
    {
        g_warning(_("Font '%i' undefined"), param);
        return TRUE;
    }

    attr->font = param;
    return TRUE;
}

gboolean
doc_fi(ParserContext *ctx, Attributes *attr, gint32 twips, GError **error)
{
    gchar *tagname = g_strdup_printf("osxcart-rtf-indent-%i", twips);
    if(!gtk_text_tag_table_lookup(ctx->tags, tagname))
    {
        GtkTextTag *tag = gtk_text_tag_new(tagname);
        g_object_set(tag,
                     "indent", PANGO_PIXELS(TWIPS_TO_PANGO(twips)),
                     "indent-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    g_free(tagname);

    attr->indent = twips;
    return TRUE;
}

static gboolean
doc_footnote(ParserContext *ctx, Attributes *attr, GError **error)
{
    GtkTextIter iter;

    /* Insert a newline at the end of the document, to separate the coming
    footnote */
    gtk_text_buffer_get_end_iter(ctx->textbuffer, &iter);
    gtk_text_buffer_insert(ctx->textbuffer, &iter, "\n", -1);
    /* Move the start and end marks back together */
    gtk_text_buffer_get_iter_at_mark(ctx->textbuffer, &iter, ctx->startmark);
    gtk_text_buffer_move_mark(ctx->textbuffer, ctx->endmark, &iter);
    return TRUE;
}

gboolean
doc_fs(ParserContext *ctx, Attributes *attr, gint32 halfpoints, GError **error)
{
    if(halfpoints <= 0)
    {
        g_set_error(error, RTF_ERROR, RTF_ERROR_BAD_FONT_SIZE, _("\\fs%d is invalid, negative or zero font sizes not allowed"), halfpoints);
        return FALSE;
    }

    gdouble points = halfpoints / 2.0;
    gchar *tagname = g_strdup_printf("osxcart-rtf-fontsize-%.3f", points);
    if(!gtk_text_tag_table_lookup(ctx->tags, tagname))
    {
        GtkTextTag *tag = gtk_text_tag_new(tagname);
        g_object_set(tag,
                     "size", POINTS_TO_PANGO(points),
                     "size-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    g_free(tagname);

    attr->size = points;
    return TRUE;
}

gboolean
doc_fsmilli(ParserContext *ctx, Attributes *attr, gint32 milli, GError **error)
{
    if(milli <= 0)
    {
        g_set_error(error, RTF_ERROR, RTF_ERROR_BAD_FONT_SIZE, _("\\fsmilli%d is invalid, negative or zero font sizes not allowed"), milli);
        return FALSE;
    }

    gdouble points = milli / 1000.0;
    gchar *tagname = g_strdup_printf("osxcart-rtf-fontsize-%.3f", points);
    if(!gtk_text_tag_table_lookup(ctx->tags, tagname))
    {
        GtkTextTag *tag = gtk_text_tag_new(tagname);
        g_object_set(tag,
                     "size", POINTS_TO_PANGO(points),
                     "size-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    g_free(tagname);

    attr->size = points;
    return TRUE;
}

gboolean
doc_highlight(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
    gchar *color;

    if((color = g_slist_nth_data(ctx->color_table, param)) == NULL)
    {
        g_set_error(error, RTF_ERROR, RTF_ERROR_UNDEFINED_COLOR, _("Color '%i' undefined"), param);
        return FALSE;
    }

    gchar *tagname = g_strdup_printf("osxcart-rtf-highlight-%i", param);
    if(!gtk_text_tag_table_lookup(ctx->tags, tagname))
    {
        GtkTextTag *tag = gtk_text_tag_new(tagname);
        g_object_set(tag,
                     "paragraph-background", color,
                     "paragraph-background-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    g_free(tagname);

    attr->background = param;
    return TRUE;
}

gboolean
doc_i(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "osxcart-rtf-italic"))
    {
        GtkTextTag *tag = gtk_text_tag_new("osxcart-rtf-italic");
        g_object_set(tag,
                     "style", PANGO_STYLE_ITALIC,
                     "style-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->italic = (param != 0);
    return TRUE;
}

static gboolean
doc_ilvl(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
    /* Insert n tabs at beginning of line */
    GtkTextIter iter;
    gchar *tabstring = g_strnfill(param, '\t');

    gtk_text_buffer_get_end_iter(ctx->textbuffer, &iter);
    gtk_text_iter_set_line_offset(&iter, 0);
    gtk_text_buffer_insert(ctx->textbuffer, &iter, tabstring, -1);
    /* Move the start and end marks back together */
    gtk_text_buffer_get_iter_at_mark(ctx->textbuffer, &iter, ctx->startmark);
    gtk_text_buffer_move_mark(ctx->textbuffer, ctx->endmark, &iter);

    g_free(tabstring);
    return TRUE;
}

gboolean
doc_lang(ParserContext *ctx, Attributes *attr, gint32 language, GError **error)
{

    gchar *tagname = g_strdup_printf("osxcart-rtf-language-%i", language);
    if(!gtk_text_tag_table_lookup(ctx->tags, tagname))
    {
        GtkTextTag *tag = gtk_text_tag_new(tagname);
        g_object_set(tag,
                     "language", language_to_iso(language),
                     "language-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    g_free(tagname);

    attr->language = language;
    return TRUE;
}

gboolean
doc_li(ParserContext *ctx, Attributes *attr, gint32 twips, GError **error)
{
    if(twips < 0)
        return TRUE; /* Silently ignore, not supported in GtkTextBuffer */

    gchar *tagname = g_strdup_printf("osxcart-rtf-left-margin-%i", twips);
    if(!gtk_text_tag_table_lookup(ctx->tags, tagname))
    {
        GtkTextTag *tag = gtk_text_tag_new(tagname);
        g_object_set(tag,
                     "left-margin", PANGO_PIXELS(TWIPS_TO_PANGO(twips)),
                     "left-margin-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    g_free(tagname);

    attr->left_margin = twips;
    return TRUE;
}

gboolean
doc_ltrch(ParserContext *ctx, Attributes *attr, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "osxcart-rtf-left-to-right"))
    {
        GtkTextTag *tag = gtk_text_tag_new("osxcart-rtf-left-to-right");
        g_object_set(tag, "direction", GTK_TEXT_DIR_LTR, NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->chardirection = GTK_TEXT_DIR_LTR;
    return TRUE;
}

gboolean
doc_ltrpar(ParserContext *ctx, Attributes *attr, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "osxcart-rtf-left-to-right"))
    {
        GtkTextTag *tag = gtk_text_tag_new("osxcart-rtf-left-to-right");
        g_object_set(tag, "direction", GTK_TEXT_DIR_LTR, NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->pardirection = GTK_TEXT_DIR_LTR;
    return TRUE;
}

static gboolean
doc_mac(ParserContext *ctx, Attributes *attr, GError **error)
{
    ctx->default_codepage = 10000;
    return TRUE;
}

gboolean
doc_nosupersub(ParserContext *ctx, Attributes *attr, GError **error)
{
    attr->subscript = FALSE;
    attr->superscript = FALSE;
    return TRUE;
}

gboolean
doc_pard(ParserContext *ctx, Attributes *attr, GError **error)
{
    attr->style = -1;
    attr->justification = -1;
    attr->pardirection = -1;
    attr->space_before = 0;
    attr->space_after = 0;
    attr->ignore_space_before = FALSE;
    attr->ignore_space_after = FALSE;
    attr->left_margin = 0;
    attr->right_margin = 0;
    attr->indent = 0;
    if(attr->tabs)
        pango_tab_array_free(attr->tabs);
    attr->tabs = NULL;
    return TRUE;
}

static gboolean
doc_pc(ParserContext *ctx, Attributes *attr, GError **error)
{
    ctx->default_codepage = 437;
    return TRUE;
}

static gboolean
doc_pca(ParserContext *ctx, Attributes *attr, GError **error)
{
    ctx->default_codepage = 850;
    return TRUE;
}

gboolean
doc_plain(ParserContext *ctx, Attributes *attr, GError **error)
{
    set_default_character_attributes(attr);
    attr->language = ctx->default_language; /* Override "1024" */
    return TRUE;
}

gboolean
doc_qc(ParserContext *ctx, Attributes *attr, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "osxcart-rtf-center"))
    {
        GtkTextTag *tag = gtk_text_tag_new("osxcart-rtf-center");
        g_object_set(tag,
                     "justification", GTK_JUSTIFY_CENTER,
                     "justification-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->justification = GTK_JUSTIFY_CENTER;
    return TRUE;
}

gboolean
doc_qj(ParserContext *ctx, Attributes *attr, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "osxcart-rtf-justified"))
    {
        GtkTextTag *tag = gtk_text_tag_new("osxcart-rtf-justified");
        g_object_set(tag,
                     "justification", GTK_JUSTIFY_FILL,
                     "justification-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->justification = GTK_JUSTIFY_FILL;
    return TRUE;
}

gboolean
doc_ql(ParserContext *ctx, Attributes *attr, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "osxcart-rtf-left"))
    {
        GtkTextTag *tag = gtk_text_tag_new("osxcart-rtf-left");
        g_object_set(tag,
                     "justification", GTK_JUSTIFY_LEFT,
                     "justification-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->justification = GTK_JUSTIFY_LEFT;
    return TRUE;
}

gboolean
doc_qr(ParserContext *ctx, Attributes *attr, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "osxcart-rtf-right"))
    {
        GtkTextTag *tag = gtk_text_tag_new("osxcart-rtf-right");
        g_object_set(tag,
                     "justification", GTK_JUSTIFY_RIGHT,
                     "justification-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->justification = GTK_JUSTIFY_RIGHT;
    return TRUE;
}

gboolean
doc_ri(ParserContext *ctx, Attributes *attr, gint32 twips, GError **error)
{
    if(twips < 0)
        return TRUE; /* Silently ignore, not supported in GtkTextBuffer */

    gchar *tagname = g_strdup_printf("osxcart-rtf-right-margin-%i", twips);
    if(!gtk_text_tag_table_lookup(ctx->tags, tagname))
    {
        GtkTextTag *tag = gtk_text_tag_new(tagname);
        g_object_set(tag,
                     "right-margin", PANGO_PIXELS(TWIPS_TO_PANGO(twips)),
                     "right-margin-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    g_free(tagname);

    attr->right_margin = twips;
    return TRUE;
}

static gboolean
doc_rtf(ParserContext *ctx, Attributes *attr, gint32 version, GError **error)
{
    if(version != 1)
    {
        g_set_error(error, RTF_ERROR, RTF_ERROR_BAD_VERSION, _("Unsupported RTF version '%i'"), (gint)version);
        return FALSE;
    }
    return TRUE;
}

gboolean
doc_rtlch(ParserContext *ctx, Attributes *attr, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "osxcart-rtf-right-to-left"))
    {
        GtkTextTag *tag = gtk_text_tag_new("osxcart-rtf-right-to-left");
        g_object_set(tag, "direction", GTK_TEXT_DIR_RTL, NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->chardirection = GTK_TEXT_DIR_RTL;
    return TRUE;
}

gboolean
doc_rtlpar(ParserContext *ctx, Attributes *attr, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "osxcart-rtf-right-to-left"))
    {
        GtkTextTag *tag = gtk_text_tag_new("osxcart-rtf-right-to-left");
        g_object_set(tag, "direction", GTK_TEXT_DIR_RTL, NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->pardirection = GTK_TEXT_DIR_RTL;
    return TRUE;
}

gboolean
doc_s(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
    gchar *tagname = g_strdup_printf("osxcart-rtf-style-%i", param);
    if(!gtk_text_tag_table_lookup(ctx->tags, tagname))
    {
        g_warning(_("Style '%i' undefined"), param);
        g_free(tagname);
        return TRUE;
    }
    g_free(tagname);
    attr->style = param;
    return TRUE;
}

gboolean
doc_sa(ParserContext *ctx, Attributes *attr, gint32 twips, GError **error)
{
    if(twips < 0)
        return TRUE; /* Silently ignore, not supported in GtkTextBuffer */

    gchar *tagname = g_strdup_printf("osxcart-rtf-space-after-%i", twips);
    if(!gtk_text_tag_table_lookup(ctx->tags, tagname))
    {
        GtkTextTag *tag = gtk_text_tag_new(tagname);
        g_object_set(tag,
                     "pixels-below-lines", PANGO_PIXELS(TWIPS_TO_PANGO(twips)),
                     "pixels-below-lines-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    g_free(tagname);

    attr->space_after = twips;
    return TRUE;
}

gboolean
doc_saauto(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
    attr->ignore_space_after = (param != 0);
    return TRUE;
}

gboolean
doc_sb(ParserContext *ctx, Attributes *attr, gint32 twips, GError **error)
{
    if(twips < 0)
        return TRUE; /* Silently ignore, not supported in GtkTextBuffer */

    gchar *tagname = g_strdup_printf("osxcart-rtf-space-before-%i", twips);
    if(!gtk_text_tag_table_lookup(ctx->tags, tagname))
    {
        GtkTextTag *tag = gtk_text_tag_new(tagname);
        g_object_set(tag,
                     "pixels-above-lines", PANGO_PIXELS(TWIPS_TO_PANGO(twips)),
                     "pixels-above-lines-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    g_free(tagname);

    attr->space_before = twips;
    return TRUE;
}

gboolean
doc_sbauto(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
    attr->ignore_space_before = (param != 0);
    return TRUE;
}

gboolean
doc_scaps(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "osxcart-rtf-smallcaps"))
    {
        GtkTextTag *tag = gtk_text_tag_new("osxcart-rtf-smallcaps");
        g_object_set(tag,
                     "variant", PANGO_VARIANT_SMALL_CAPS,
                     "variant-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->smallcaps = (param != 0);
    return TRUE;
}

gboolean
doc_slleading(ParserContext *ctx, Attributes *attr, gint32 twips, GError **error)
{
    if(twips < 0)
        return TRUE; /* Silently ignore, not supported in GtkTextBuffer */

    gchar *tagname = g_strdup_printf("osxcart-rtf-leading-%i", twips);
    if(!gtk_text_tag_table_lookup(ctx->tags, tagname))
    {
        GtkTextTag *tag = gtk_text_tag_new(tagname);
        g_object_set(tag,
                     "pixels-inside-wrap", PANGO_PIXELS(TWIPS_TO_PANGO(twips)),
                     "pixels-inside-wrap-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    g_free(tagname);

    attr->leading = twips;
    return TRUE;
}

gboolean
doc_strike(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "osxcart-rtf-strikethrough"))
    {
        GtkTextTag *tag = gtk_text_tag_new("osxcart-rtf-strikethrough");
        g_object_set(tag,
                     "strikethrough", TRUE,
                     "strikethrough-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->strikethrough = (param != 0);
    return TRUE;
}

gboolean
doc_sub(ParserContext *ctx, Attributes *attr, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "osxcart-rtf-subscript"))
    {
        GtkTextTag *tag = gtk_text_tag_new("osxcart-rtf-subscript");
        g_object_set(tag,
                     "rise", POINTS_TO_PANGO(-6),
                     "rise-set", TRUE,
                     "scale", PANGO_SCALE_X_SMALL,
                     "scale-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->subscript = TRUE;
    return TRUE;
}

gboolean
doc_super(ParserContext *ctx, Attributes *attr, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "osxcart-rtf-superscript"))
    {
        GtkTextTag *tag = gtk_text_tag_new("osxcart-rtf-superscript");
        g_object_set(tag,
                     "rise", POINTS_TO_PANGO(6),
                     "rise-set", TRUE,
                     "scale", PANGO_SCALE_X_SMALL,
                     "scale-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->superscript = TRUE;
    return TRUE;
}

gboolean
doc_tx(ParserContext *ctx, Attributes *attr, gint32 twips, GError **error)
{
    gint tab_index;

    if(attr->tabs == NULL)
    {
        attr->tabs = pango_tab_array_new(1, FALSE);
        tab_index = 0;
    }
    else
    {
        tab_index = pango_tab_array_get_size(attr->tabs);
        pango_tab_array_resize(attr->tabs, tab_index + 1);
    }

    pango_tab_array_set_tab(attr->tabs, tab_index, PANGO_TAB_LEFT, TWIPS_TO_PANGO(twips));

    return TRUE;
}

gboolean
doc_u(ParserContext *ctx, Attributes *attr, gint32 ch, GError **error)
{
    gchar utf8[7];
    gint length, foo;
    gunichar code;

    if(ch < 0)
        code = ch + 65536;
    else
        code = ch;

    length = g_unichar_to_utf8(code, utf8);
    utf8[length] = '\0';
    g_string_append(ctx->text, utf8);

    for(foo = 0; foo < attr->unicode_skip; foo++)
        if(!skip_character_or_control_word(ctx, error))
            return FALSE;
    return TRUE;
}

gboolean
doc_uc(ParserContext *ctx, Attributes *attr, gint32 skip, GError **error)
{
    attr->unicode_skip = skip;
    return TRUE;
}

gboolean
doc_ud(ParserContext *ctx, Attributes *attr, GError **error)
{
    attr->unicode_ignore = FALSE;
    return TRUE;
}

gboolean
doc_ul(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "osxcart-rtf-underline-single"))
    {
        GtkTextTag *tag = gtk_text_tag_new("osxcart-rtf-underline-single");
        g_object_set(tag,
                     "underline", PANGO_UNDERLINE_SINGLE,
                     "underline-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->underline = param? PANGO_UNDERLINE_SINGLE : PANGO_UNDERLINE_NONE;
    return TRUE;
}

gboolean
doc_uldb(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "osxcart-rtf-underline-double"))
    {
        GtkTextTag *tag = gtk_text_tag_new("osxcart-rtf-underline-double");
        g_object_set(tag,
                     "underline", PANGO_UNDERLINE_DOUBLE,
                     "underline-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->underline = param? PANGO_UNDERLINE_DOUBLE : PANGO_UNDERLINE_NONE;
    return TRUE;
}

gboolean
doc_ulnone(ParserContext *ctx, Attributes *attr, GError **error)
{
    attr->underline = PANGO_UNDERLINE_NONE;
    return TRUE;
}

gboolean
doc_ulstyle(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
    switch(param & 0xF)
    {
        case 1:
            return doc_ul(ctx, attr, 1, error);
        case 9:
            return doc_uldb(ctx, attr, 1, error);
    }
    return doc_ulnone(ctx, attr, error);
}

gboolean
doc_ulwave(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "osxcart-rtf-underline-wave"))
    {
        GtkTextTag *tag = gtk_text_tag_new("osxcart-rtf-underline-wave");
        g_object_set(tag,
                     "underline", PANGO_UNDERLINE_ERROR,
                     "underline-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->underline = param? PANGO_UNDERLINE_ERROR : PANGO_UNDERLINE_NONE;
    return TRUE;
}

gboolean
doc_up(ParserContext *ctx, Attributes *attr, gint32 halfpoints, GError **error)
{
    if(halfpoints != 0)
    {
        gchar *tagname = g_strdup_printf("osxcart-rtf-up-%i", halfpoints);
        if(!gtk_text_tag_table_lookup(ctx->tags, tagname))
        {
            GtkTextTag *tag = gtk_text_tag_new(tagname);
            g_object_set(tag,
                         "rise", HALF_POINTS_TO_PANGO(halfpoints),
                         "rise-set", TRUE,
                         NULL);
            gtk_text_tag_table_add(ctx->tags, tag);
        }
        g_free(tagname);
    }

    attr->rise = halfpoints;
    return TRUE;
}

gboolean
doc_upr(ParserContext *ctx, Attributes *attr, GError **error)
{
    attr->unicode_ignore = TRUE;
    return TRUE;
}

gboolean
doc_v(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "osxcart-rtf-invisible"))
    {
        GtkTextTag *tag = gtk_text_tag_new("osxcart-rtf-invisible");
        g_object_set(tag,
                     "invisible", TRUE,
                     "invisible-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->invisible = (param != 0);
    return TRUE;
}
