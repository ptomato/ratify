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

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include "rtf.h"
#include "rtf-document.h"
#include "rtf-deserialize.h"
#include "rtf-ignore.h"

/* rtf-deserialize.c - Modular RTF reader. Works by maintaining a stack of
destinations (for more information on what a destination is, read the excellent
book 'RTF Pocket Guide' by Sean M. Burke, published by O'Reilly) which each
maintain a table of control words and a stack of states.

Whenever an opening brace { is encountered, the destination copies its state and
pushes it onto its state stack. A closing brace } pops the topmost state from
the stack. Whenever a control word is encountered, the reader calls the function
associated with it, and that function modifies the current state. Each
destination also has a text function, which specifies what to do with plain text
encountered inside the destination, and a cleanup function, which specifies what
to do when the destination's closing brace is reached.

Control words can also change the current destination, in which case the new
destination is pushed onto the destination stack, and a new table of control
words applies.

 * Originally based on gnomeicu-rtf-reader.c from GnomeICU
 * Copyright (C) 1998-2002 Jeremy Wise
 * First created by Frédéric Riss (2002)
 * License: GPLv2
 */

/* Allocate a new parser context and initialize it with the main document
destination */
static ParserContext *
parser_context_new(const char *rtftext, GtkTextBuffer *textbuffer, GtkTextIter *insert)
{
    g_assert(rtftext != NULL && textbuffer != NULL);

    ParserContext *ctx = g_slice_new0(ParserContext);
    ctx->codepage = -1;
    ctx->default_codepage = 1252;
    ctx->default_font = -1;
    ctx->default_language = 1024;
    ctx->group_nesting_level = 0;
    ctx->color_table = NULL;
    ctx->font_table = NULL;
    ctx->footnote_number = 1;
    ctx->rtftext = rtftext;
    ctx->pos = rtftext;
    ctx->convertbuffer = g_string_new("");
    ctx->text = g_string_new("");

    ctx->textbuffer = textbuffer;
    ctx->tags = gtk_text_buffer_get_tag_table(textbuffer);
    ctx->startmark = gtk_text_buffer_create_mark(textbuffer, NULL, insert, true);
    ctx->endmark = gtk_text_buffer_create_mark(textbuffer, NULL, insert, false);

    Destination *dest = g_slice_new0(Destination);
    dest->info = &document_destination;
    dest->nesting_level = 0;
    dest->state_stack = g_queue_new();
    g_queue_push_head(dest->state_stack, dest->info->state_new());

    ctx->destination_stack = g_queue_new();
    g_queue_push_head(ctx->destination_stack, dest);

    return ctx;
}

/* Free font properties */
static void
font_properties_free(FontProperties *fontprop)
{
    g_free(fontprop->font_name);
    g_slice_free(FontProperties, fontprop);
}

/* Push new destination onto the destination stack. If state_to_copy is not
NULL, then initializes the state stack with a copy of that state, otherwise a
blank state. */
void
push_new_destination(ParserContext *ctx, const DestinationInfo *destinfo, void *state_to_copy)
{
    Destination *dest = g_slice_new0(Destination);
    dest->info = destinfo;
    dest->nesting_level = ctx->group_nesting_level;
    dest->state_stack = g_queue_new();
    if (state_to_copy)
        g_queue_push_head(dest->state_stack, dest->info->state_copy(state_to_copy));
    else
        g_queue_push_head(dest->state_stack, dest->info->state_new());
    g_queue_push_head(ctx->destination_stack, dest);
}

/* Free destination */
static void
destination_free(Destination *dest)
{
    g_queue_foreach(dest->state_stack, (GFunc)dest->info->state_free, NULL);
    g_queue_free(dest->state_stack);
    g_slice_free(Destination, dest);
}

/* Free parser context */
static void
parser_context_free(ParserContext *ctx)
{
    g_assert(ctx != NULL);
    g_string_free(ctx->convertbuffer, false);

    g_slist_foreach(ctx->color_table, (GFunc)g_free, NULL);
    g_slist_free(ctx->color_table);

    g_slist_foreach(ctx->font_table, (GFunc)font_properties_free, NULL);
    g_slist_free(ctx->font_table);

    g_queue_foreach(ctx->destination_stack, (GFunc)destination_free, NULL);
    g_queue_free(ctx->destination_stack);

    gtk_text_buffer_delete_mark(ctx->textbuffer, ctx->startmark);
    gtk_text_buffer_delete_mark(ctx->textbuffer, ctx->endmark);

    g_string_free(ctx->text, true);

    g_slice_free(ParserContext, ctx);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC(ParserContext, parser_context_free);

/* Convenience function to get the current state of the current destination */
void *
get_state(ParserContext *ctx)
{
    Destination *dest = g_queue_peek_head(ctx->destination_stack);
    return g_queue_peek_head(dest->state_stack);
}

/* Returns properties for font numbered index in the font table, or NULL if such
font does not exist */
FontProperties *
get_font_properties(ParserContext *ctx, int index)
{
    for (unsigned i = 0; i < g_slist_length(ctx->font_table); i++) {
        FontProperties *properties = g_slist_nth_data(ctx->font_table, i);
        if (properties != NULL && properties->index == index)
            return properties;
    }
    return NULL;
}

/* Return the name of a GIconv converter for the specified codepage, if it
exists; otherwise NULL */
static char *
get_charset_for_codepage(int codepage)
{
    struct codepage_to_locale {
        int codepage;
        const char* locale;
    };

    static struct codepage_to_locale ansicpgs[] = {
        { 943, "SJIS" },
        { 950, "BIG5" },
        { 709, "ASMO_449" },
        { 10000, "MAC" },
        { 10001, "SJIS" }, /* approximation? */
        { 65001, "UTF-8" },
        { 0, NULL }
    };

    if (codepage == -1)
        return NULL;

    /* First try the "CP<cpge>" charset */
    g_autofree char *charset = g_strdup_printf("CP%i", codepage);
    GIConv converter = g_iconv_open("UTF-8", charset);

    if (converter != (GIConv)-1) {
        g_iconv_close(converter);
        return g_steal_pointer(&charset);
    }

    /* If there is no such converter, try the hard-coded table */
    for (int i = 0; ansicpgs[i].codepage != 0; i++) {
        if (ansicpgs[i].codepage == codepage) {
            converter = g_iconv_open("UTF-8", ansicpgs[i].locale);
            if (converter != (GIConv)-1) {
                g_iconv_close(converter);
                return g_strdup(ansicpgs[i].locale);
            }
        }
    }
    return NULL;
}

/* Convert the character ch to UTF-8 and add to the context's buffer */
bool
convert_hex_to_utf8(ParserContext *ctx, char ch, GError **error)
{
    /* Determine the character encoding that ch is in. First see if the current
    destination diverts us to another codepage (e.g., \fcharset in the \fonttbl
    destination) and if not, use either the current codepage or the default codepage. */
    Destination *dest = (Destination *)g_queue_peek_head(ctx->destination_stack);
    int codepage = -1;
    if (dest->info->get_codepage)
        codepage = dest->info->get_codepage(ctx);
    if (codepage == -1)
        codepage = ctx->codepage;
    g_autofree char *charset = get_charset_for_codepage(codepage);
    if (charset == NULL)
        charset = get_charset_for_codepage(ctx->default_codepage);
    if (charset == NULL) {
        g_set_error(error, RTF_ERROR, RTF_ERROR_UNSUPPORTED_CHARSET, _("Character set %d is not supported"), (ctx->default_codepage == -1)? codepage : ctx->default_codepage);
        return false;
    }

    /* Now see if there was any incompletely converted text left over from
    previous characters */
    g_autofree char *text_to_convert = NULL;
    if (ctx->convertbuffer->len) {
        g_string_append_c(ctx->convertbuffer, ch);
        text_to_convert = g_strdup(ctx->convertbuffer->str);
        g_string_truncate(ctx->convertbuffer, 0);
    } else {
        text_to_convert = g_strndup(&ch, 1);
    }

    GError *converterror = NULL;
    g_autofree char *converted_text = g_convert_with_fallback(text_to_convert, -1, "UTF-8", charset, "?", NULL, NULL, &converterror);
    if (converterror) {
        /* If there is a "partial input" error, then save the text
         in the convert buffer and retrieve it if there is another
         consecutive \'xx code */
        if (converterror->code == G_CONVERT_ERROR_PARTIAL_INPUT)
            g_string_append(ctx->convertbuffer, text_to_convert);
        else
            g_warning(_("Conversion error: %s"), converterror->message);
        g_clear_error(&converterror);
    } else {
        g_string_append(ctx->text, converted_text);
    }
    return true;
}

/* Parses a control word from the input buffer. 'word' is the return location
for the control word, without a backslash, but with '*' prefixed if the control
word is preceded by \*, which means that the control word represents a
destination that should be skipped if it is not recognized.
 */
static bool
parse_control_word(ParserContext *ctx, char **word, GError **error)
{
    g_assert(ctx != NULL && *(ctx->pos) == '\\');

    ctx->pos++;
    if (*ctx->pos == '*') {
        /* Ignorable destination */
        ctx->pos++;
        while (isspace(*ctx->pos))
            ctx->pos++;
        g_autofree char *destword = NULL;
        if (!parse_control_word(ctx, &destword, error))
            return false;
        *word = g_strconcat("*", destword, NULL);
    } else if (g_ascii_ispunct(*ctx->pos) || *ctx->pos == '\n' || *ctx->pos == '\r') {
        /* Control symbol */
        *word = g_strndup(ctx->pos, 1);
        ctx->pos++;
    } else {
        /* Control word */
        size_t length = 0;

        while (g_ascii_isalpha(ctx->pos[length]))
            length++;
        if (length == 0) {
            g_set_error(error, RTF_ERROR, RTF_ERROR_INVALID_RTF, _("Backslash encountered without control word"));
            return false;
        }
        *word = g_strndup(ctx->pos, length);
        ctx->pos += length;
    }

    return true;
}

/* Reads an integer at the current position. If there's no integer at that
position the function returns false, otherwise true. The value is stored in the
location pointed by the 'value' parameter, unless 'value' is NULL. Eat a space
after parsing the number. */
static bool
parse_int_parameter(ParserContext *ctx, int32_t *value)
{
    size_t length = 0;

    g_assert(ctx != NULL);

    /* Don't use strtol() directly to convert the value, because it will
    validate a '+' sign in front of the number, whereas that's not valid
    according to the RTF spec */

    /* Find the length of the integer */
    if (ctx->pos[0] == '-' && g_ascii_isdigit(ctx->pos[1]))
        length += 2;
    while (g_ascii_isdigit(ctx->pos[length]))
        length++;

    if (length == 0)
    return false;

    /* Convert it */
    g_autofree char *intstr = g_strndup(ctx->pos, length);
    if (value)
        *value = strtol(intstr, NULL, 10);
    ctx->pos += length;

    /* If the value is delimited by a space, discard the space */
    if (*(ctx->pos) == ' ')
        ctx->pos++;

    return true;
}

/* Skip one character or control word according to the RTF spec's convoluted
skipping rules */
bool
skip_character_or_control_word(ParserContext *ctx, GError **error)
{
    do {
        if (*ctx->pos == '{' || *ctx->pos == '}') {
            return true; /* Skippable data ends before scope delimiter */
        } else if (*ctx->pos == '\\') {
            /* Special case: \' doesn't follow the regular syntax */
            if (ctx->pos[1] == '\'') {
                if (!(isxdigit(ctx->pos[2]) && isxdigit(ctx->pos[3]))) {
                    g_set_error(error, RTF_ERROR, RTF_ERROR_BAD_HEX_CODE, _("Expected a two-character hexadecimal code after \\'"));
                    return false;
                }
                ctx->pos += 4;
                return true;
            } else {
                g_autofree char *word = NULL;
                bool success = parse_control_word(ctx, &word, error);
                if (!parse_int_parameter(ctx, NULL) && *(ctx->pos) == ' ')
                    ctx->pos++;
                return success;
            }
        } else if (*ctx->pos == '\n' || *ctx->pos == '\r') {
            ctx->pos++;
        } else {
            ctx->pos++;
            return true;
        }
    } while (true);
}

/* Carry out the action associated with the control word 'text', as specified
in the current destination's control word table */
static bool
do_word_action(ParserContext *ctx, const char *text, GError **error)
{
    const ControlWord *word;

    Destination *dest = (Destination *)g_queue_peek_head(ctx->destination_stack);

    for (word = dest->info->word_table; word->word != NULL; word++) {
        if (strcmp(text, word->word) == 0)
            break;
    }

    if (word->word != NULL) {
        int32_t param;
        switch (word->type) {
        case NO_PARAMETER:
            if (*ctx->pos == ' ') /* Eat a space */
                ctx->pos++;
            g_assert(word->action);
            if (word->flush_buffer)
                dest->info->flush(ctx);
            return word->action(ctx, get_state(ctx), error);

        case OPTIONAL_PARAMETER:
            /* If the parameter is optional, carry out the action with the
            parameter if there is one, and otherwise with the default
            parameter */
            g_assert(word->action);
            if (parse_int_parameter(ctx, &param)) {
                if (word->flush_buffer)
                    dest->info->flush(ctx);
                return word->action(ctx, get_state(ctx), param, error);
            }
            /* If no parameter, then eat a space */
            if (*ctx->pos == ' ')
                ctx->pos++;
            if (word->flush_buffer)
                dest->info->flush(ctx);
            return word->action(ctx, get_state(ctx), word->defaultparam, error);

        case REQUIRED_PARAMETER:
            g_assert(word->action);
            if (!parse_int_parameter(ctx, &param))
            {
                g_set_error(error, RTF_ERROR, RTF_ERROR_MISSING_PARAMETER, _("Expected a number after control word '\\%s'"), text);
                return false;
            }
            if (word->flush_buffer)
                dest->info->flush(ctx);
            return word->action(ctx, get_state(ctx), param, error);

        case SPECIAL_CHARACTER:
            /* If the control word represents a special character, then just
            insert that character into the buffer */
            if (*ctx->pos == ' ') /* Eat a space */
                ctx->pos++;
            g_assert(word->replacetext);
            g_string_append(ctx->text, word->replacetext);
            return true;

        case DESTINATION:
            if (*ctx->pos == ' ') /* Eat a space */
                ctx->pos++;
            if (word->action && !word->action(ctx, get_state(ctx), error))
                return false;
            push_new_destination(ctx, word->destinfo, NULL);
            return true;

        default:
            g_assert_not_reached();
        }
    }
    /* If the control word was not recognized, then ignore it, and any integer
    parameter that follows */
    if (!parse_int_parameter(ctx, NULL) && *ctx->pos == ' ')
        ctx->pos++;
    /* If the control word was an ignorable destination, and was not recognized,
    push a new "ignore" destination onto the stack */
    if (text[0] == '*')
        push_new_destination(ctx, &ignore_destination, NULL);

    return true;
}

/* When exiting a group in the RTF code ('}'), this function is called to pop
one element from the state stack, hence restoring the state before entering the
current group. */
static void
pop_state(ParserContext *ctx)
{
    g_assert(ctx != NULL);

    ctx->group_nesting_level--;

    Destination *dest = (Destination *)g_queue_peek_head(ctx->destination_stack);
    dest->info->flush(ctx);

    if (ctx->group_nesting_level < dest->nesting_level) {
        if (dest->info->cleanup)
            dest->info->cleanup(ctx);
        dest->info->state_free(g_queue_pop_head(dest->state_stack));
        destination_free(g_queue_pop_head(ctx->destination_stack));

        /* Also pop the state of the destination that called this one, since
         the opening brace was before the destination control word */
        dest = (Destination *)g_queue_peek_head(ctx->destination_stack);
        dest->info->flush(ctx);
        dest->info->state_free(g_queue_pop_head(dest->state_stack));
    } else {
        dest->info->state_free(g_queue_pop_head(dest->state_stack));
    }
}

/* When entering a group in the RTF code ('{'), this function copies the current
state and pushes it onto the state stack, so modifications of the state within
the group do not affect the state outside of the group. */
static void
push_state(ParserContext *ctx)
{
    g_assert(ctx != NULL);

    Destination *dest = (Destination *)g_queue_peek_head(ctx->destination_stack);
    dest->info->flush(ctx);
    ctx->group_nesting_level++;
    g_queue_push_head(dest->state_stack, dest->info->state_copy(g_queue_peek_head(dest->state_stack)));
}

/* The main parser loop */
static bool
parse_rtf(ParserContext *ctx, GError **error)
{
    do {
        if (*ctx->pos == '\0') {
            g_set_error(error, RTF_ERROR, RTF_ERROR_MISSING_BRACE, _("File ended unexpectedly"));
            return false;
        }
        if (*ctx->pos == '{') {
            ctx->pos++;
            push_state(ctx);
        } else if (*ctx->pos == '}') {
            ctx->pos++;
            pop_state(ctx);
        } else if (*ctx->pos == '\\') {
            /* Special case: \' doesn't follow the regular syntax */
            if (ctx->pos[1] == '\'') {
                if (!(isxdigit(ctx->pos[2]) && isxdigit(ctx->pos[3]))) {
                    g_set_error(error, RTF_ERROR, RTF_ERROR_BAD_HEX_CODE, _("Expected a two-character hexadecimal code after \\'"));
                    return false;
                }
                g_autofree char *hexcode = g_strndup(ctx->pos + 2, 2);
                char ch = strtol(hexcode, NULL, 16);
                ctx->pos += 4;

                if (!convert_hex_to_utf8(ctx, ch, error))
                    return false;
            } else {
                g_autofree char *word = NULL;
                if (!parse_control_word(ctx, &word, error) || !do_word_action(ctx, word, error))
                    return false;
            }
        } else if (*ctx->pos == '\n' || *ctx->pos == '\r') {
            /* Ignore newlines */
            ctx->pos++;
        } else if (*ctx->pos < 0) {
            /* Ignore high characters (they should be encoded with \'xx) */
            ctx->pos++;
        } else {
            /* If there is any partial wide character in the convert buffer, then
             try to combine it with this one as a double-byte character */
            if (ctx->convertbuffer->len) {
                if (!convert_hex_to_utf8(ctx, *ctx->pos, error))
                    return false;
            } else {
                /* Add character to current string */
                g_string_append_c(ctx->text, *ctx->pos);
            }
            ctx->pos++;
        }

    } while (ctx->group_nesting_level > 0);

    /* Check that there isn't anything but whitespace after the last brace */
    while (isspace(*ctx->pos))
        ctx->pos++;
    if (*ctx->pos != '\0') {
        g_set_error(error, RTF_ERROR, RTF_ERROR_EXTRA_CHARACTERS, _("Characters found after final closing brace"));
        return false;
    }

    return true;
}

/* This function is called by gtk_text_buffer_deserialize() */
bool
rtf_deserialize(GtkTextBuffer *register_buffer, GtkTextBuffer *content_buffer, GtkTextIter *iter, const char *data, size_t length, bool create_tags, void *user_data, GError **error)
{
    if (!g_str_has_prefix(data, "{\\rtf")) {
        g_set_error(error, RTF_ERROR, RTF_ERROR_INVALID_RTF, _("RTF format must begin with '{\\rtf'"));
        return false;
    }

    g_autoptr(ParserContext) ctx = parser_context_new(data, content_buffer, iter);
    return parse_rtf(ctx, error);
}
