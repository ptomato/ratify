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

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include "rtf-langcode.h"

/* rtf-serialize.c - RTF writer */

/* FIXME: these definitions conflate points and pixels */
#define PIXELS_TO_TWIPS(pixels) (pixels * 20)
#define PANGO_TO_HALF_POINTS(pango) (2 * pango / PANGO_SCALE)
#define PANGO_TO_TWIPS(pango) (20 * pango / PANGO_SCALE)

typedef struct {
    GtkTextBuffer *textbuffer;
    const GtkTextIter *start, *end;
    GString *output;
    GtkTextBuffer *linebuffer;
    GHashTable *tag_codes; /* Translation table of GtkTextTags to RTF code */
    GList *font_table;
    GList *color_table;
} WriterContext;

/* Initialize the writer context */
static WriterContext *
writer_context_new(void)
{
    WriterContext *ctx = g_slice_new0(WriterContext);
    ctx->output = g_string_new("");
    ctx->tag_codes = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_free);
    ctx->font_table = NULL;
    ctx->color_table = g_list_prepend(NULL, g_strdup("")); /* Color 0 always black */
    return ctx;
}

/* Free the writer context */
static void
writer_context_free(WriterContext *ctx)
{
    g_hash_table_unref(ctx->tag_codes);
    g_list_foreach(ctx->color_table, (GFunc)g_free, NULL);
    g_list_free(ctx->color_table);
    g_slice_free(WriterContext, ctx);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC(WriterContext, writer_context_free);

/* Return the number of color in the color table. If color is not in the color
table, then add it. */
static int
get_color_from_gdk_color(GdkColor *color, WriterContext *ctx)
{
    if (color->red == 0 && color->green == 0 && color->blue == 0)
        return 0; /* Color 0 always black in this implementation */

    g_autofree char *colorcode = g_strdup_printf("\\red%d\\green%d\\blue%d", color->red >> 8, color->green >> 8, color->blue >> 8);
    int colornum;
    GList *link = g_list_find_custom(ctx->color_table, colorcode, (GCompareFunc)strcmp);
    if (link == NULL) {
        colornum = g_list_length(ctx->color_table);
        ctx->color_table = g_list_append(ctx->color_table, g_strdup(colorcode));
    } else {
        colornum = g_list_position(ctx->color_table, link);
    }

    g_assert(colornum > 0 && colornum < 256);
    return colornum;
}

/* Generate RTF code for tag, and add it to the context's hashtable of tags to
RTF code. */
static void
convert_tag_to_code(GtkTextTag *tag, WriterContext *ctx)
{
    gboolean val;
    int pixels, pango, colornum;
    double factor, points;
    GdkColor *color;
    char *name;

    /* First check if this is a ratify-named tag that doesn't have a direct
     Pango attributes equivalent, such as superscript or subscript. Treat these
     separately. */
    g_object_get(tag, "name", &name, NULL);
    if (name) {
        if (strcmp(name, "rtf-superscript") == 0) {
            g_hash_table_insert(ctx->tag_codes, tag, g_strdup("\\super"));
            g_free(name);
            return;
        } else if (strcmp(name, "rtf-subscript") == 0) {
            g_hash_table_insert(ctx->tag_codes, tag, g_strdup("\\sub"));
            g_free(name);
            return;
        }
        g_free(name);
    }

    /* Otherwise, read the attributes one by one and add RTF code for them */
    GString *code = g_string_new("");

    g_object_get(tag, "background-set", &val, NULL);
    if (val) {
        g_object_get(tag, "background-gdk", &color, NULL);
        colornum = get_color_from_gdk_color(color, ctx);
        g_string_append_printf(code, "\\chshdng0\\chcbpat%d\\cb%d", colornum, colornum);
    }

    g_object_get(tag, "family-set", &val, NULL);
    if (val) {
        g_autofree char *family = NULL;
        g_object_get(tag, "family", &family, NULL);

        int fontnum;
        GList *link = g_list_find_custom(ctx->font_table, family, (GCompareFunc)strcmp);
        if (link == NULL) {
            fontnum = g_list_length(ctx->font_table);
            ctx->font_table = g_list_append(ctx->font_table, g_strdup(family));
        } else {
            fontnum = g_list_position(ctx->font_table, link);
        }
        g_string_append_printf(code, "\\f%d", fontnum);
    }

    g_object_get(tag, "foreground-set", &val, NULL);
    if (val) {
        g_object_get(tag, "foreground-gdk", &color, NULL);
        colornum = get_color_from_gdk_color(color, ctx);
        g_string_append_printf(code, "\\cf%d", colornum);
    }

    g_object_get(tag, "indent-set", &val, NULL);
    if (val) {
        g_object_get(tag, "indent", &pixels, NULL);
        g_string_append_printf(code, "\\fi%d", PIXELS_TO_TWIPS(pixels));
    }

    g_object_get(tag, "invisible-set", &val, NULL);
    if (val) {
        g_object_get(tag, "invisible", &val, NULL);
        if (val)
            g_string_append(code, "\\v");
        else
            g_string_append(code, "\\v0");
    }

    g_object_get(tag, "justification-set", &val, NULL);
    if (val) {
        GtkJustification just;
        g_object_get(tag, "justification", &just, NULL);
        switch (just) {
        case GTK_JUSTIFY_LEFT:
            g_string_append(code, "\\ql");
            break;
        case GTK_JUSTIFY_RIGHT:
            g_string_append(code, "\\qr");
            break;
        case GTK_JUSTIFY_CENTER:
            g_string_append(code, "\\qc");
            break;
        case GTK_JUSTIFY_FILL:
            g_string_append(code, "\\qj");
            break;
        }
    }

    g_object_get(tag, "language-set", &val, NULL);
    if (val) {
        g_autofree char *isocode = NULL;
        g_object_get(tag, "language", &isocode, NULL);
        g_string_append_printf(code, "\\lang%d", language_to_wincode(isocode));
    }

    g_object_get(tag, "left-margin-set", &val, NULL);
    if (val) {
        g_object_get(tag, "left-margin", &pixels, NULL);
        g_string_append_printf(code, "\\li%d", PIXELS_TO_TWIPS(pixels));
    }

    g_object_get(tag, "paragraph-background-set", &val, NULL);
    if (val) {
        g_object_get(tag, "paragraph-background-gdk", &color, NULL);
        colornum = get_color_from_gdk_color(color, ctx);
        g_string_append_printf(code, "\\highlight%d", colornum);
    }

    g_object_get(tag, "pixels-above-lines-set", &val, NULL);
    if (val) {
        g_object_get(tag, "pixels-above-lines", &pixels, NULL);
        g_string_append_printf(code, "\\sb%d", PIXELS_TO_TWIPS(pixels));
    }

    g_object_get(tag, "pixels-below-lines-set", &val, NULL);
    if (val) {
        g_object_get(tag, "pixels-below-lines", &pixels, NULL);
        g_string_append_printf(code, "\\sa%d", PIXELS_TO_TWIPS(pixels));
    }

    g_object_get(tag, "pixels-inside-wrap-set", &val, NULL);
    if (val) {
        g_object_get(tag, "pixels-inside-wrap", &pixels, NULL);
        g_string_append_printf(code, "\\slleading%d", PIXELS_TO_TWIPS(pixels));
    }

    g_object_get(tag, "right-margin-set", &val, NULL);
    if (val) {
        g_object_get(tag, "right-margin", &pixels, NULL);
        g_string_append_printf(code, "\\ri%d", PIXELS_TO_TWIPS(pixels));
    }

    g_object_get(tag, "rise-set", &val, NULL);
    if (val) {
        g_object_get(tag, "rise", &pango, NULL);
        if (pango > 0)
            g_string_append_printf(code, "\\up%d", PANGO_TO_HALF_POINTS(pango));
        else if (pango < 0)
            g_string_append_printf(code, "\\dn%d", PANGO_TO_HALF_POINTS(-pango));
        else
            g_string_append(code, "\\up0\\dn0");
    }

    g_object_get(tag, "scale-set", &val, NULL);
    if (val) {
        g_object_get(tag, "scale", &factor, NULL);
        g_string_append_printf(code, "\\charscalex%d", (int)(factor * 100));
    }

    g_object_get(tag, "size-set", &val, NULL);
    if (val) {
        g_object_get(tag, "size-points", &points, NULL);
        g_string_append_printf(code, "\\fs%d", (int)(points * 2));
        /* Override with an \fsmilli command if the font size is not a multiple
        of 1/2 point */
        int milli = (int)(points * 1000);
        if (milli % 500 != 0)
            g_string_append_printf(code, "\\fsmilli%d", milli);
    }

    g_object_get(tag, "strikethrough-set", &val, NULL);
    if (val) {
        g_object_get(tag, "strikethrough", &val, NULL);
        if (val)
            g_string_append(code, "\\strike");
        else
            g_string_append(code, "\\strike0");
    }

    g_object_get(tag, "style-set", &val, NULL);
    if (val) {
        PangoStyle style;
        g_object_get(tag, "style", &style, NULL);
        switch (style) {
        case PANGO_STYLE_NORMAL:
            g_string_append(code, "\\i0");
            break;
        case PANGO_STYLE_OBLIQUE:
        case PANGO_STYLE_ITALIC:
            g_string_append(code, "\\i");
            break;
        }
    }

    g_object_get(tag, "tabs-set", &val, NULL);
    if (val) {
        PangoTabArray *tabs;
        g_object_get(tag, "tabs", &tabs, NULL);

        bool in_pixels = pango_tab_array_get_positions_in_pixels(tabs);
        int size = pango_tab_array_get_size(tabs);
        for (int count = 0; count < size; count++) {
            int location;
            /* alignment can only be LEFT in the current version of Pango */
            pango_tab_array_get_tab(tabs, count, NULL, &location);
            g_string_append_printf(code, "\\tx%d", in_pixels? PIXELS_TO_TWIPS(location) : PANGO_TO_TWIPS(location));
        }
    }

    g_object_get(tag, "underline-set", &val, NULL);
    if (val) {
        PangoUnderline underline;
        g_object_get(tag, "underline", &underline, NULL);
        switch (underline) {
        case PANGO_UNDERLINE_NONE:
            g_string_append(code, "\\ul0\\ulnone");
            break;
        case PANGO_UNDERLINE_SINGLE:
        case PANGO_UNDERLINE_LOW:
#if PANGO_VERSION_CHECK(1, 46, 0)
        case PANGO_UNDERLINE_SINGLE_LINE:
#endif
            g_string_append(code, "\\ul");
            break;
        case PANGO_UNDERLINE_DOUBLE:
#if PANGO_VERSION_CHECK(1, 46, 0)
        case PANGO_UNDERLINE_DOUBLE_LINE:
#endif
            g_string_append(code, "\\uldb");
            break;
        case PANGO_UNDERLINE_ERROR:
#if PANGO_VERSION_CHECK(1, 46, 0)
        case PANGO_UNDERLINE_ERROR_LINE:
#endif
            g_string_append(code, "\\ulwave");
            break;
        }
    }

    g_object_get(tag, "variant-set", &val, NULL);
    if (val) {
        PangoVariant variant;
        g_object_get(tag, "variant", &variant, NULL);
        switch (variant) {
        case PANGO_VARIANT_NORMAL:
            g_string_append(code, "\\scaps0\\caps0");
            break;
        case PANGO_VARIANT_SMALL_CAPS:
#if PANGO_VERSION_CHECK(1, 50, 0)
        case PANGO_VARIANT_ALL_SMALL_CAPS:
        case PANGO_VARIANT_PETITE_CAPS:
        case PANGO_VARIANT_ALL_PETITE_CAPS:
        case PANGO_VARIANT_UNICASE:
#endif
            g_string_append(code, "\\scaps");
            break;
#if PANGO_VERSION_CHECK(1, 50, 0)
        case PANGO_VARIANT_TITLE_CAPS:
            g_string_append(code, "\\caps");
            break;
#endif
        }
    }

    g_object_get(tag, "weight-set", &val, NULL);
    if (val) {
        int weight;
        g_object_get(tag, "weight", &weight, NULL);
        if (weight >= PANGO_WEIGHT_BOLD)
            g_string_append(code, "\\b");
        else
            g_string_append(code, "\\b0");
    }

    g_hash_table_insert(ctx->tag_codes, tag, g_string_free(code, false));
}

/* This function is run before processing the actual contents of the buffer. It
generates RTF code for all of the tags in the buffer's tag table, and tells the
context which portion of the text buffer to serialize. */
static void
analyze_buffer(WriterContext *ctx, GtkTextBuffer *textbuffer, const GtkTextIter *start, const GtkTextIter *end)
{
    GtkTextTagTable *tagtable = gtk_text_buffer_get_tag_table(textbuffer);
    gtk_text_tag_table_foreach(tagtable, (GtkTextTagTableForeach)convert_tag_to_code, ctx);
    ctx->textbuffer = textbuffer;
    ctx->start = start;
    ctx->end = end;
}

/* Write a space to the output buffer if the number of characters output on the
current line is less than 60; otherwise, a newline. If the next space occurs
more than 20 characters further on, the line will still be wider than 80
characters, but this is probably the easiest way to break lines without
looking ahead or backtracking to insert spaces. */
static void
write_space_or_newline(WriterContext *ctx)
{
    size_t linelength = strlen(strrchr(ctx->output->str, '\n'));
    g_string_append_c(ctx->output, (linelength > 60)? '\n' : ' ');
}

/* This function translates a piece of text, without formatting codes, to RTF.
It replaces special characters by their RTF control word equivalents. */
static void
write_rtf_text(WriterContext *ctx, const char *text)
{
    for (const char *ptr = text; *ptr; ptr = g_utf8_next_char(ptr)) {
        gunichar ch = g_utf8_get_char(ptr);

        if (ch == 0x09) {
            g_string_append(ctx->output, "\\tab");
        } else if (ch == '\n') {
            /* whatever value that is */
            g_string_append(ctx->output, "\\par");
        } else if (ch == ' ') {
            if (strlen(strrchr(ctx->output->str, '\n')) > 60)
                g_string_append_c(ctx->output, '\n');
            g_string_append_c(ctx->output, ' ');
            continue;
        } else if (ch == 0x5C) {
            g_string_append(ctx->output, "\\\\");
            continue;
        } else if (ch == 0x7B) {
            g_string_append(ctx->output, "\\{");
            continue;
        } else if (ch == 0x7D) {
            g_string_append(ctx->output, "\\}");
            continue;
        } else if (ch > 0 && ch < 0x80) {
            g_string_append_c(ctx->output, (char)ch);
            continue;
        } else if (ch == 0xA0) {
            g_string_append(ctx->output, "\\~");
            continue;
        } else if (ch == 0xAD) {
            g_string_append(ctx->output, "\\-");
            continue;
        } else if (ch >= 0xA1 && ch <= 0xFF) {
            g_string_append_printf(ctx->output, "\\'%2X", ch);
            continue;
        } else if (ch == 0x2002) {
            g_string_append(ctx->output, "\\enspace");
        } else if (ch == 0x2003) {
            g_string_append(ctx->output, "\\emspace");
        } else if (ch == 0x2005) {
            g_string_append(ctx->output, "\\qmspace");
        } else if (ch == 0x200B) {
            g_string_append(ctx->output, "\\zwbo");
        } else if (ch == 0x200C) {
            g_string_append(ctx->output, "\\zwnj");
        } else if (ch == 0x200D) {
            g_string_append(ctx->output, "\\zwj");
        } else if (ch == 0x200E) {
            g_string_append(ctx->output, "\\ltrmark");
        } else if (ch == 0x200F) {
            g_string_append(ctx->output, "\\rtlmark");
        } else if (ch == 0x2011) {
            g_string_append(ctx->output, "\\_");
            continue;
        } else if (ch == 0x2013) {
            g_string_append(ctx->output, "\\endash");
        } else if (ch == 0x2014) {
            g_string_append(ctx->output, "\\emdash");
        } else if (ch == 0x2018) {
            g_string_append(ctx->output, "\\lquote");
        } else if (ch == 0x2019) {
            g_string_append(ctx->output, "\\rquote");
        } else if (ch == 0x201C) {
            g_string_append(ctx->output, "\\ldblquote");
        } else if (ch == 0x201D) {
            g_string_append(ctx->output, "\\rdblquote");
        } else if (ch == 0x2022) {
            g_string_append(ctx->output, "\\bullet");
        } else if (ch == 0x2028) {
            g_string_append(ctx->output, "\\line");
        } else {
            g_string_append_printf(ctx->output, "\\u%d", ch);
        }
        write_space_or_newline(ctx);
    }
}

/* Analyze a segment of text in which there are no tag flips, but possibly embedded pictures */
static void
write_rtf_text_and_pictures(WriterContext *ctx, const GtkTextIter *start, const GtkTextIter *end)
{
    GtkTextIter iter;
    GdkPixbuf *pixbuf = NULL;

    for (iter = *start; !gtk_text_iter_equal(&iter, end); gtk_text_iter_forward_char(&iter)) {
        if ((pixbuf = gtk_text_iter_get_pixbuf(&iter)))
            break;
    }

    if (!pixbuf) {
        g_autofree char *text = gtk_text_buffer_get_text(ctx->linebuffer, start, end, true);
        write_rtf_text(ctx, text);
        return;
    }

    /* Write the text before the pixbuf, insert a \pict destination into the document, and recurse on the text after */
    g_autofree char *text = gtk_text_buffer_get_text(ctx->linebuffer, start, &iter, true);
    write_rtf_text(ctx, text);

    char *pngbuffer;
    size_t bufsize;
    GError *error = NULL;
    if (gdk_pixbuf_save_to_buffer(pixbuf, &pngbuffer, &bufsize, "png", &error, "compression", "9", NULL)) {
        g_string_append_printf(ctx->output, "{\\pict\\pngblip\\picw%d\\pich%d", gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf));
        for (size_t count = 0; count < bufsize; count++) {
            if (count % 40 == 0)
                g_string_append_c(ctx->output, '\n');
            g_string_append_printf(ctx->output, "%02X", (unsigned char)pngbuffer[count]);
        }
        g_string_append(ctx->output, "\n}");
        g_free(pngbuffer);
    } else {
        g_warning(_("Could not serialize picture, skipping: %s"), error->message);
    }

    gtk_text_iter_forward_char(&iter);
    write_rtf_text_and_pictures(ctx, &iter, end);
}

/* Copy the text paragraph-by-paragraph into a separate buffer and output each
one sequentially with formatting codes */
static void
write_rtf_paragraphs(WriterContext *ctx)
{
    GtkTextIter linestart = *(ctx->start), lineend = linestart;

    while (gtk_text_iter_in_range(&lineend, ctx->start, ctx->end)) {
        /* Begin the paragraph by resetting the paragraph properties */
        g_string_append(ctx->output, "{\\pard\\plain");

        /* Get two iterators around the next paragraph of text */
        gtk_text_iter_forward_to_line_end(&lineend);
        /* Skip to the end of any clump of paragraph separators */
        while (gtk_text_iter_ends_line(&lineend) && !gtk_text_iter_is_end(&lineend))
            gtk_text_iter_forward_char(&lineend);
        if (gtk_text_iter_compare(&lineend, ctx->end) > 0)
            lineend = *(ctx->end);

        /* Copy the entire paragraph to a separate buffer */
        if (ctx->linebuffer)
            g_object_unref(ctx->linebuffer);
        ctx->linebuffer = gtk_text_buffer_new(gtk_text_buffer_get_tag_table(ctx->textbuffer));
        GtkTextIter start, end;
        gtk_text_buffer_get_start_iter(ctx->linebuffer, &start);
        gtk_text_buffer_insert_range(ctx->linebuffer, &start, &linestart, &lineend);
        gtk_text_buffer_get_bounds(ctx->linebuffer, &start, &end);

        /* Insert codes for tags that apply to the whole line, then remove those
        tags because we've dealt with them */
        g_autoptr(GSList) taglist = gtk_text_iter_get_tags(&linestart);
        for (GSList *ptr = taglist; ptr; ptr = g_slist_next(ptr)) {
            GtkTextIter tagend = start;
            gtk_text_iter_forward_to_tag_toggle(&tagend, ptr->data);
            if (gtk_text_iter_equal(&tagend, &end)) {
                g_string_append(ctx->output, g_hash_table_lookup(ctx->tag_codes, ptr->data));
                gtk_text_buffer_remove_tag(ctx->linebuffer, ptr->data, &start, &end);
            }
        }
        write_space_or_newline(ctx);
        g_string_append_c(ctx->output, '{');

        end = start;
        while (!gtk_text_iter_is_end(&end)) {
            /* Enclose a section of text without any tag flips between start
            and end. Then, make tagstartlist a list of tags that open at the
            beginning of this section, and tagendlist a list of tags that end
            at the end of this section. */

            gtk_text_iter_forward_to_tag_toggle(&end, NULL);
            g_autoptr(GSList) tagstartlist = gtk_text_iter_get_toggled_tags(&start, true);
            g_autoptr(GSList) tagendlist = gtk_text_iter_get_toggled_tags(&end, false);
            g_autoptr(GSList) tagonlylist = NULL;

            /* Move tags that do not extend before or after this section to
            tagonlylist. */
            for (GSList *ptr = tagstartlist; ptr; ptr = g_slist_next(ptr)) {
                if (g_slist_find(tagendlist, ptr->data))
                    tagonlylist = g_slist_prepend(tagonlylist, ptr->data);
            }
            for (GSList *ptr = tagonlylist; ptr; ptr = g_slist_next(ptr)) {
                tagstartlist = g_slist_remove(tagstartlist, ptr->data);
                tagendlist = g_slist_remove(tagendlist, ptr->data);
            }

            /* Output the tags in tagstartlist */
            size_t length = ctx->output->len;
            for (GSList *ptr = tagstartlist; ptr; ptr = g_slist_next(ptr))
                g_string_append(ctx->output, g_hash_table_lookup(ctx->tag_codes, ptr->data));
            if (length != ctx->output->len)
                write_space_or_newline(ctx);

            /* Output the tags in tagonlylist, within their own group */
            if (tagonlylist) {
                g_string_append_c(ctx->output, '{');
                length = ctx->output->len;
                for (GSList *ptr = tagonlylist; ptr; ptr = g_slist_next(ptr))
                    g_string_append(ctx->output, g_hash_table_lookup(ctx->tag_codes, ptr->data));
                if (length != ctx->output->len)
                    write_space_or_newline(ctx);
            }

            /* Output the actual contents of this section */
            write_rtf_text_and_pictures(ctx, &start, &end);

            /* Close the tagonlylist group */
            if (tagonlylist)
                g_string_append_c(ctx->output, '}');

            /* If any tags end here, close the group and open another one,
            then output the tags that _apply_ to the end iter but do not _start_
            there (those will be output in the next iteration and may need to
            be in a separate group.) */
            if (tagendlist) {
                g_string_append(ctx->output, "}{");
                g_autoptr(GSList) new_taglist = gtk_text_iter_get_tags(&end);
                g_autoptr(GSList) new_tagstartlist = gtk_text_iter_get_toggled_tags(&end, true);
                for (GSList *ptr = new_tagstartlist; ptr; ptr = g_slist_next(ptr))
                    new_taglist = g_slist_remove(new_taglist, ptr->data);

                length = ctx->output->len;
                for (GSList *ptr = new_taglist; ptr; ptr = g_slist_next(ptr))
                    g_string_append(ctx->output, g_hash_table_lookup(ctx->tag_codes, ptr->data));
                if (length != ctx->output->len)
                    write_space_or_newline(ctx);
            }

            start = end;
        }
        g_string_append(ctx->output, "}}\n");
        linestart = lineend;
    }
}

static void
write_color_table_entry(const char *colorcode, WriterContext *ctx)
{
    g_string_append_printf(ctx->output, "%s;\n", colorcode);
}

/* Write the RTF header and assorted front matter */
static char *
write_rtf(WriterContext *ctx)
{
    GList *iter;
    int count;

    /* Header */
    g_string_append(ctx->output, "{\\rtf1\\ansi\\deff0\\uc0\n");

    /* Font table */
    g_string_append(ctx->output, "{\\fonttbl\n");
    for (count = 0, iter = ctx->font_table; iter; iter = g_list_next(iter), count++) {
        char **fontnames = g_strsplit(iter->data, ",", 2);
        g_string_append_printf(ctx->output, "{\\f%d\\fnil %s;}\n", count, fontnames[0]);
        g_strfreev(fontnames);
    }
    if (!ctx->font_table) /* Write at least one font if there are none */
        g_string_append(ctx->output, "{\\f0\\fswiss Sans;}\n");
    g_string_append(ctx->output, "}\n");

    /* Color table */
    g_string_append(ctx->output, "{\\colortbl\n");
    g_list_foreach(ctx->color_table, (GFunc)write_color_table_entry, ctx);
    g_string_append(ctx->output, "}\n");

    /* Metadata (provide dummy values because Word will overwrite if missing) */
    g_string_append_printf(ctx->output, "{\\*\\generator %s %s}\n", PACKAGE_NAME, PACKAGE_VERSION);
    g_string_append(ctx->output, "{\\info {\\author .}{\\company .}{\\title .}\n");
    char buffer[29];
    time_t timer = time(NULL);
    if (strftime(buffer, 29, "\\yr%Y\\mo%m\\dy%d\\hr%H\\min%M", localtime(&timer)))
        g_string_append_printf(ctx->output, "{\\creatim%s}}\n", buffer);


    /* Preliminary formatting */
    g_string_append_printf(ctx->output, "\\deflang%d", language_to_wincode(pango_language_to_string(pango_language_get_default())));
    g_string_append(ctx->output, "\\plain\\widowctrl\\hyphauto\n");

    /* Document body */
    write_rtf_paragraphs(ctx);

    g_string_append_c(ctx->output, '}');
    return g_string_free(ctx->output, false);
}

/* This function is called by gtk_text_buffer_serialize(). */
uint8_t *
rtf_serialize(GtkTextBuffer *register_buffer, GtkTextBuffer *content_buffer, const GtkTextIter *start, const GtkTextIter *end, size_t *length)
{
    g_autoptr(WriterContext) ctx = writer_context_new();

    analyze_buffer(ctx, content_buffer, start, end);
    char *contents = write_rtf(ctx);
    *length = strlen(contents);
    return (uint8_t *)contents;
}
