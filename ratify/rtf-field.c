/* Copyright 2009, 2011, 2015, 2019 P. F. Chimento
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

#include "rtf-deserialize.h"
#include "rtf-document.h"
#include "rtf-ignore.h"
#include "rtf-state.h"

/* field.c - \field, \fldinst, and \fldrslt destinations. The markup language
for fields is very complicated and only a small and arbitrary subset of the
field types and formatting codes are implemented here. What is here is taken
from ECMA-376 Office Open XML file formats, 2nd Edition (December 2008), Part 1,
section 17.16: Fields & Hyperlinks
(http://www.ecma-international.org/publications/standards/Ecma-376.htm).

The markup language format is simple enough to be parsed with a GScanner. Right
now, the GScanner does not use a symbol table and parses everything as an
identifier.

Basically, there are many opportunities for improvement in this code, at
questionable benefit. */

/* These are the supported field types. Add new values here as more field types
get implemented. */
typedef enum {
    FIELD_TYPE_HYPERLINK,
    FIELD_TYPE_INCLUDEPICTURE,
    FIELD_TYPE_PAGE
} FieldType;

typedef struct {
    const char *name;
    FieldType type;
    bool has_argument;        /* Whether the field itself has an argument */
    const char *switches;        /* 1-character switches without argument */
    const char *argswitches;     /* 1-character switches with argument */
    const char *wideswitches;    /* 2-character switches without argument */
    const char *wideargswitches; /* 2-character switches with argument */
} FieldInfo;

const FieldInfo fields[] = {
    { "HYPERLINK", FIELD_TYPE_HYPERLINK, true, "mn", "lot", "", "" },
    { "INCLUDEPICTURE", FIELD_TYPE_INCLUDEPICTURE, true, "d", "c", "", "" },
    { "PAGE", FIELD_TYPE_PAGE, false, "", "", "", "" },
    { NULL }
};

/* These are the supported general number formats. Not all of the formats
described in the standard are implemented. */
typedef enum {
    NUMBER_ALPHABETIC,
    NUMBER_alphabetic,
    NUMBER_ARABIC,
    NUMBER_ARABIC_DASH,
    NUMBER_CIRCLENUM,
    NUMBER_DECIMAL_ENCLOSED_PERIOD,
    NUMBER_DECIMAL_ENCLOSED_PARENTHESES,
    NUMBER_HEX,
    NUMBER_ORDINAL,
    NUMBER_ROMAN,
    NUMBER_roman
} GeneralNumberFormat;

typedef struct {
    GString *scanbuffer;
    FieldType type;
    GeneralNumberFormat general_number_format;
    char *argument;
    char *date_format;
    char *numeric_format;
} FieldInstructionState;

typedef struct {
    bool ignore_field_result;
} FieldState;

static void field_instruction_text(ParserContext *ctx);
static void field_instruction_end(ParserContext *ctx);
static char *format_integer(int number, GeneralNumberFormat format);

#define FLDINST_STATE_INIT \
    state->scanbuffer = g_string_new(""); \
    state->general_number_format = NUMBER_ARABIC;
/* The GString is shared between all the states on the stack */
DEFINE_STATE_FUNCTIONS_WITH_INIT(FieldInstructionState, fldinst, FLDINST_STATE_INIT)
DEFINE_SIMPLE_STATE_FUNCTIONS(FieldState, field)
DEFINE_ATTR_STATE_FUNCTIONS(Attributes, fldrslt)

const ControlWord field_instruction_word_table[] = {
    { "\\", SPECIAL_CHARACTER, false, NULL, 0, "\\" },
    { NULL }
};

const DestinationInfo field_instruction_destination = {
    field_instruction_word_table,
    field_instruction_text,
    fldinst_state_new,
    fldinst_state_copy,
    fldinst_state_free,
    field_instruction_end
};

const ControlWord field_result_word_table[] = {
    DOCUMENT_TEXT_CONTROL_WORDS,
    { NULL }
};

const DestinationInfo field_result_destination = {
    field_result_word_table,
    document_text,
    fldrslt_state_new,
    fldrslt_state_copy,
    fldrslt_state_free
};

typedef bool FieldFunc(ParserContext *, FieldState *, GError **);
static FieldFunc field_fldrslt;

const ControlWord field_word_table[] = {
    { "*fldinst", DESTINATION, false, NULL, 0, NULL, &field_instruction_destination },
    { "fldrslt", NO_PARAMETER, false, field_fldrslt },
    { NULL }
};

const DestinationInfo field_destination = {
    field_word_table,
    ignore_pending_text,
    field_state_new,
    field_state_copy,
    field_state_free
};

const GScannerConfig field_parser = {
    /* Character sets */
    " \t\n", /* cset_skip_characters */
    G_CSET_A_2_Z G_CSET_a_2_z "\\", /* cset_identifier_first */
    G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "#@*", /* cset_identifier_nth */
    NULL, /* cpair_comment_single */

    /* Should symbol lookup be case sensitive? */
    true, /* case_sensitive */

    /* Boolean values to be adjusted "on the fly" to configure scanning behaviour. */
    false, /* skip_comment_multi */
    false, /* skip_comment_single */
    false, /* scan_comment_multi */
    true,  /* scan_identifier */
    false, /* scan_identifier_1char */
    false, /* scan_identifier_NULL */
    false, /* scan_symbols */
    false, /* scan_binary */
    false, /* scan_octal */
    false, /* scan_float */
    false, /* scan_hex */
    false, /* scan_hex_dollar */
    false, /* scan_string_sq */
    true,  /* scan_string_dq */
    true,  /* numbers_2_int */
    false, /* int_2_float */
    true,  /* identifier_2_string */
    false, /* char_2_token */
    false, /* symbol_2_token */
    false, /* scope_0_fallback */
    false  /* store_int64 */
};

/* Move all text to the GScanner's scan buffer */
static void
field_instruction_text(ParserContext *ctx)
{
    FieldInstructionState *state = get_state(ctx);
    g_string_append(state->scanbuffer, ctx->text->str);
    g_string_truncate(ctx->text, 0);
}

/* Get the next token from the scanner and make sure it is a string. Do not free
the returned value. */
static char *
get_string_token(GScanner *tokenizer)
{
    if (g_scanner_eof(tokenizer)) {
        g_warning(_("Unexpected end of field instructions"));
        g_scanner_destroy(tokenizer);
        return NULL;
    }
    GTokenType token = g_scanner_get_next_token(tokenizer);
    if (token != G_TOKEN_STRING) {
        g_warning(_("Expected a string in field instructions"));
        g_scanner_destroy(tokenizer);
        return NULL;
    }
    return tokenizer->value.v_string;
}

typedef struct {
    char *switchname;
    char *switcharg;
} SwitchInfo;

static void
free_switch_info(SwitchInfo *info)
{
    g_free(info->switchname);
    g_free(info->switcharg);
    g_slice_free(SwitchInfo, info);
}

/* Consume all the tokens belonging to the switches named here */
static GSList *
get_switches(GScanner *tokenizer, GSList *switcheslist, const char *switches, const char *argswitches, const char *wideswitches, const char *wideargswitches)
{
    bool found = false;

    g_assert(strlen(wideswitches) % 2 == 0);
    g_assert(strlen(wideargswitches) % 2 == 0);

    /* Parse switches until an argument without '\' or unexpected switch */
    while (!g_scanner_eof(tokenizer)) {
        GTokenType token = g_scanner_peek_next_token(tokenizer);
        found = false;

        if (token != G_TOKEN_STRING)
            break;
        if (tokenizer->next_value.v_string[0] == '\\') {
            for (const char *ptr = switches; *ptr && !found; ptr++) {
                if (tokenizer->next_value.v_string[1] == ptr[0] && tokenizer->next_value.v_string[2] == '\0') {
                    SwitchInfo *info = g_slice_new0(SwitchInfo);
                    info->switchname = g_strdup(tokenizer->next_value.v_string + 1);
                    info->switcharg = NULL;
                    g_scanner_get_next_token(tokenizer);
                    switcheslist = g_slist_prepend(switcheslist, info);
                    found = true;
                }
            }
            for (const char *ptr = argswitches; *ptr && !found; ptr++) {
                if (tokenizer->next_value.v_string[1] == ptr[0] && tokenizer->next_value.v_string[2] == '\0') {
                    SwitchInfo *info = g_slice_new0(SwitchInfo);
                    info->switchname = g_strdup(tokenizer->next_value.v_string + 1);
                    g_scanner_get_next_token(tokenizer);
                    info->switcharg = g_strdup(get_string_token(tokenizer));
                    switcheslist = g_slist_prepend(switcheslist, info);
                    found = true;
                }
            }
            for (const char *ptr = wideswitches; *ptr && !found; ptr++) {
                if (tokenizer->next_value.v_string[1] == ptr[0] && tokenizer->next_value.v_string[2] == ptr[1] && tokenizer->next_value.v_string[3] == '\0') {
                    SwitchInfo *info = g_slice_new0(SwitchInfo);
                    info->switchname = g_strdup(tokenizer->next_value.v_string + 1);
                    info->switcharg = NULL;
                    g_scanner_get_next_token(tokenizer);
                    switcheslist = g_slist_prepend(switcheslist, info);
                    found = true;
                }
            }
            for (const char *ptr = wideargswitches; *ptr && !found; ptr += 2) {
                if (tokenizer->next_value.v_string[1] == ptr[0] && tokenizer->next_value.v_string[2] == ptr[1] && tokenizer->next_value.v_string[3] == '\0') {
                    SwitchInfo *info = g_slice_new0(SwitchInfo);
                    info->switchname = g_strdup(tokenizer->next_value.v_string + 1);
                    g_scanner_get_next_token(tokenizer);
                    info->switcharg = g_strdup(get_string_token(tokenizer));
                    switcheslist = g_slist_prepend(switcheslist, info);
                    found = true;
                }
            }
        } else {
            break;
        }

        if (!found)
            break;
        /* Unexpected switch, so it must belong to the next part of the field */
    }

    return switcheslist;
}

static void field_instruction_end(ParserContext *ctx)
{
    FieldInstructionState *state = get_state(ctx);
    GSList *switches = NULL, *formatswitches = NULL;
    g_autoptr(GScanner) tokenizer = g_scanner_new(&field_parser);

    g_scanner_input_text(tokenizer, state->scanbuffer->str, strlen(state->scanbuffer->str));

    /* Get field type */
    const char *field_type = get_string_token(tokenizer);
    if (field_type == NULL)
        return;

    /* Determine if field type is supported and get switches and arguments */
    const FieldInfo *field_info = fields;
    while (field_info->name != NULL) {
        if (g_ascii_strcasecmp(field_type, field_info->name) == 0) {
            state->type = field_info->type;
            switches = get_switches(tokenizer, switches, field_info->switches, field_info->argswitches, field_info->wideswitches, field_info->wideargswitches);
            if (field_info->has_argument) {
                const char *buffer = get_string_token(tokenizer);
                if (buffer == NULL)
                    return;
                state->argument = g_strdup(buffer);
                switches = get_switches(tokenizer, switches, field_info->switches, field_info->argswitches, field_info->wideswitches, field_info->wideargswitches);
            }
            break;
        }
        field_info++;
    }
    if (field_info->name == NULL) {
        /* Field name wasn't found in list */
        g_warning(_("'%s' field not supported"), field_type);
        return;
    }

    formatswitches = get_switches(tokenizer, formatswitches, "", "@#*", "", "");
    for (GSList *iter = formatswitches; iter; iter = g_slist_next(iter)) {
        SwitchInfo *info = (SwitchInfo *)iter->data;
        if (strcmp(info->switchname, "@") == 0) {
            /* Parse a date format consisting of \@ and a string */
            state->date_format = g_strdup(info->switcharg);
        } else if (strcmp(info->switchname, "#") == 0) {
            /* Parse a numeric format consisting of \# and a string */
            state->numeric_format = g_strdup(info->switcharg);
        } else if (strcmp(info->switchname, "*") == 0) {
            /* Parse a general format consisting of \* and a keyword */
            if (strcmp(info->switcharg, "ALPHABETIC") == 0)
                state->general_number_format = NUMBER_ALPHABETIC;
            else if (strcmp(info->switcharg, "alphabetic") == 0)
                state->general_number_format = NUMBER_alphabetic;
            else if (strcmp(info->switcharg, "Arabic") == 0)
                state->general_number_format = NUMBER_ARABIC;
            else if (strcmp(info->switcharg, "ArabicDash") == 0)
                state->general_number_format = NUMBER_ARABIC_DASH;
            else if (strcmp(info->switcharg, "CIRCLENUM") == 0)
                state->general_number_format = NUMBER_CIRCLENUM;
            else if (strcmp(info->switcharg, "GB1") == 0)
                state->general_number_format = NUMBER_DECIMAL_ENCLOSED_PERIOD;
            else if (strcmp(info->switcharg, "GB2") == 0)
                state->general_number_format = NUMBER_DECIMAL_ENCLOSED_PARENTHESES;
            else if (strcmp(info->switcharg, "Hex") == 0)
                state->general_number_format = NUMBER_HEX;
            else if (strcmp(info->switcharg, "MERGEFORMATINET") == 0)
                ; /* ignore */
            else if (strcmp(info->switcharg, "Ordinal") == 0)
                state->general_number_format = NUMBER_ORDINAL;
            else if (strcmp(info->switcharg, "Roman") == 0)
                state->general_number_format = NUMBER_ROMAN;
            else if (strcmp(info->switcharg, "roman") == 0)
                state->general_number_format = NUMBER_roman;
            else
                g_warning(_("Format '%s' not supported"), info->switcharg);
                /* Just continue */
        }
    }

    Destination *fielddest = g_queue_peek_nth(ctx->destination_stack, 1);
    FieldState *fieldstate = g_queue_peek_tail(fielddest->state_stack);

    switch (state->type) {
    case FIELD_TYPE_HYPERLINK:
        /* Actually inserting hyperlinks into the text buffer is a whole
        security can of worms I don't want to open! Just use field result */
        fieldstate->ignore_field_result = false;
        break;

    case FIELD_TYPE_INCLUDEPICTURE: {
        GError *error = NULL;
        g_auto(GStrv) pathcomponents = g_strsplit(state->argument, "\\", 0);
        g_autofree char *realfilename = g_build_filenamev(pathcomponents);

        g_autoptr(GdkPixbuf) picture = gdk_pixbuf_new_from_file(realfilename, &error);
        if (!picture) {
            g_warning(_("Error loading picture from file '%s': %s"), realfilename, error->message);
        } else {
            /* Insert picture into text buffer */
            GtkTextIter iter;
            gtk_text_buffer_get_iter_at_mark(ctx->textbuffer, &iter, ctx->endmark);
            gtk_text_buffer_insert_pixbuf(ctx->textbuffer, &iter, picture);
        }
    }
        /* Don't use calculated field result */
        fieldstate->ignore_field_result = true;
        break;

    case FIELD_TYPE_PAGE: {
        g_autofree char *output = format_integer(1, state->general_number_format);
        GtkTextIter iter;
        gtk_text_buffer_get_iter_at_mark(ctx->textbuffer, &iter, ctx->endmark);
        gtk_text_buffer_insert(ctx->textbuffer, &iter, output, -1);
    }
        /* Don't use calculated field result */
        fieldstate->ignore_field_result = true;
        break;

    default:
        g_assert_not_reached();
    }

    g_free(state->argument);
    g_free(state->date_format);
    g_free(state->numeric_format);
    g_slist_foreach(switches, (GFunc)free_switch_info, NULL);
    g_slist_foreach(formatswitches, (GFunc)free_switch_info, NULL);
}

/* Free the return value when done with it */
static char *
format_integer(int number, GeneralNumberFormat format)
{
    switch (format) {
    case NUMBER_ALPHABETIC:
        if (number < 1)
            break;
        return g_strnfill(number / 26 + 1, number % 26 - 1 + 'A');
    case NUMBER_alphabetic:
        if (number < 1)
            break;
        return g_strnfill(number / 26 + 1, number % 26 - 1 + 'a');
    case NUMBER_ARABIC_DASH:
        return g_strdup_printf("- %d -", number);
    case NUMBER_HEX:
        return g_strdup_printf("%X", number);
    case NUMBER_ORDINAL:
        if (number % 10 == 1 && number % 100 != 11)
            return g_strdup_printf("%dst", number);
        if (number % 10 == 2 && number % 100 != 12)
            return g_strdup_printf("%dnd", number);
        if (number % 10 == 3 && number % 100 != 13)
            return g_strdup_printf("%drd", number);
        return g_strdup_printf("%dth", number);
    case NUMBER_ROMAN:
        if (number < 1)
            break;
    {
        const char *r_hundred[] = { "", "C", "CC", "CCC", "CD", "D", "DC", "DCC", "DCCC", "CM" };
        const char *r_ten[] = { "", "X", "XX", "XXX", "XL", "L", "LX", "LXX", "LXXX", "XC" };
        const char *r_one[] = { "", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX" };
        int thousands = number / 1000;
        int hundreds = (number % 1000) / 100;
        int tens = (number % 100) / 10;
        int ones = number % 10;
        g_autofree char *thousandstring = g_strnfill(thousands, 'M');
        return g_strconcat(thousandstring, r_hundred[hundreds], r_ten[tens], r_one[ones], NULL);
    }
    case NUMBER_roman:
        if (number < 1)
            break;
    {
        const char *r_hundred[] = { "", "c", "cc", "ccc", "cd", "d", "dc", "dcc", "dccc", "cm" };
        const char *r_ten[] = { "", "x", "xx", "xxx", "xl", "l", "lx", "lxx", "lxxx", "xc" };
        const char *r_one[] = { "", "i", "ii", "iii", "iv", "v", "vi", "vii", "viii", "ix" };
        int thousands = number / 1000;
        int hundreds = (number % 1000) / 100;
        int tens = (number % 100) / 10;
        int ones = number % 10;
        g_autofree char *thousandstring = g_strnfill(thousands, 'm');
        return g_strconcat(thousandstring, r_hundred[hundreds], r_ten[tens], r_one[ones], NULL);
    }
    case NUMBER_CIRCLENUM:
        if (number >= 1 && number <= 20)
            return g_strdup_printf("\xE2\x91%c", 0x9F + number);
        break;
    case NUMBER_DECIMAL_ENCLOSED_PERIOD:
        if (number >= 1 && number <= 20)
            return g_strdup_printf("\xE2\x92%c", 0x87 + number);
        break;
    case NUMBER_DECIMAL_ENCLOSED_PARENTHESES:
        if (number >= 1 && number <= 12)
            return g_strdup_printf("\xE2\x91%c", 0xB3 + number);
        if (number >= 13 && number <= 20)
            return g_strdup_printf("\xE2\x92%c", 0x73 + number);
        break;
    case NUMBER_ARABIC:
    default:
        break;
    }
    return g_strdup_printf("%d", number);
}

static bool
field_fldrslt(ParserContext *ctx, FieldState *state, GError **error)
{
    if (state->ignore_field_result) {
        push_new_destination(ctx, &ignore_destination, NULL);
    } else {
        Destination *outerdest = g_queue_peek_nth(ctx->destination_stack, 1);
        Attributes *attr = g_queue_peek_head(outerdest->state_stack);
        push_new_destination(ctx, &field_result_destination, attr);
    }
    return true;
}
