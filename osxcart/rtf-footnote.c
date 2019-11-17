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

#include "config.h"

#include <string.h>

#include <glib.h>

#include "rtf-deserialize.h"
#include "rtf-document.h"
#include "rtf-state.h"

/* rtf-footnote.c - Very similar to the main document destination, but adds its
text in at the very end of the document and increments the footnote number. */

/* Forward declarations */
static void footnote_text(ParserContext *ctx);
static void footnote_end(ParserContext *ctx);

const ControlWord footnote_word_table[] = {
    DOCUMENT_TEXT_CONTROL_WORDS,
    { NULL }
};

DEFINE_ATTR_STATE_FUNCTIONS(Attributes, footnote)

const DestinationInfo footnote_destination = {
    footnote_word_table,
    footnote_text,
    footnote_state_new,
    footnote_state_copy,
    footnote_state_free,
    footnote_end,
    document_get_codepage
};

/* This function is mostly the same as document_text(), but adds the text to the
end of the textbuffer */
static void
footnote_text(ParserContext *ctx)
{
    GtkTextIter start, end;
    GtkTextMark *placeholder;
    Attributes *attr;
    int length;
    char *text;

    g_assert(ctx != NULL);

    attr = get_state(ctx);
    text = ctx->text->str;

    if(text[0] == '\0')
        return;

    length = strlen(text) - 1;
    if(!ctx->group_nesting_level && text[length] == '\n')
        text[length] = '\0';

    gtk_text_buffer_get_end_iter(ctx->textbuffer, &end);
    placeholder = gtk_text_buffer_create_mark(ctx->textbuffer, NULL, &end, true);
    gtk_text_buffer_insert(ctx->textbuffer, &end, text, -1);
    gtk_text_buffer_get_iter_at_mark(ctx->textbuffer, &start, placeholder);
    gtk_text_buffer_get_end_iter(ctx->textbuffer, &end);

    apply_attributes(ctx, attr, &start, &end);

    gtk_text_buffer_delete_mark(ctx->textbuffer, placeholder);

    g_string_truncate(ctx->text, 0);

    /* Move the regular document endmark back to the startmark so that
    subsequent document text is inserted before the footnotes */
    gtk_text_buffer_get_iter_at_mark(ctx->textbuffer, &start, ctx->startmark);
    gtk_text_buffer_move_mark(ctx->textbuffer, ctx->endmark, &start);
}

static void
footnote_end(ParserContext *ctx)
{
    ctx->footnote_number++;
}
