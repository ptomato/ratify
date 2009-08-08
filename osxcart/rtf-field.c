#include <string.h>
#include <glib.h>
#include <config.h>
#include <glib/gi18n-lib.h>
#include "rtf-deserialize.h"
#include "rtf-document.h"
#include "rtf-ignore.h"

typedef enum {
	FIELD_TYPE_HYPERLINK,
	FIELD_TYPE_INCLUDEPICTURE,
	FIELD_TYPE_PAGE
} FieldType;

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
	gchar *argument;
	gchar *date_format;
	gchar *numeric_format;
} FieldInstructionState;

static void field_instruction_text(ParserContext *ctx);
static FieldInstructionState *fldinst_state_new(void);
static FieldInstructionState *fldinst_state_copy(FieldInstructionState *state);
static void fldinst_state_free(FieldInstructionState *state);
static void field_instruction_end(ParserContext *ctx);
static gchar *format_integer(gint number, GeneralNumberFormat format);

const ControlWord field_instruction_word_table[] = { 
	{ "\\", SPECIAL_CHARACTER, FALSE, NULL, 0, "\\" },
	{ NULL }
};

const DestinationInfo field_instruction_destination = {
	field_instruction_word_table,
	field_instruction_text,
	(StateNewFunc *)fldinst_state_new,
	(StateCopyFunc *)fldinst_state_copy,
	(StateFreeFunc *)fldinst_state_free,
	field_instruction_end
};

const ControlWord field_result_word_table[] = {
	DOCUMENT_TEXT_CONTROL_WORDS,
	{ NULL }
};

const DestinationInfo field_result_destination = {
	field_result_word_table,
	document_text,
	(StateNewFunc *)attributes_new,
	(StateCopyFunc *)attributes_copy,
	(StateFreeFunc *)attributes_free
};

const ControlWord field_word_table[] = {
	{ "*fldinst", DESTINATION, FALSE, NULL, 0, NULL, &field_instruction_destination },
	{ "fldrslt", DESTINATION, FALSE, NULL, 0, NULL, &field_result_destination },
	{ NULL }
};

const DestinationInfo field_destination = {
    field_word_table,
    ignore_pending_text,
    ignore_state_new,
    ignore_state_copy,
    ignore_state_free
};

static void
field_instruction_text(ParserContext *ctx)
{
	FieldInstructionState *state = get_state(ctx);
	
	g_string_append(state->scanbuffer, ctx->text->str);
	g_string_truncate(ctx->text, 0);
}

const GScannerConfig field_parser = {
	/* Character sets */
	" \t\n", /* cset_skip_characters */
	G_CSET_A_2_Z G_CSET_a_2_z "\\", /* cset_identifier_first */
	G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "#@*", /* cset_identifier_nth */
	NULL, /* cpair_comment_single */
	
	/* Should symbol lookup work case sensitive? */
	TRUE, /* case_sensitive */
  
	/* Boolean values to be adjusted "on the fly" to configure scanning behaviour. */
	FALSE, /* skip_comment_multi */
	FALSE, /* skip_comment_single */
	FALSE, /* scan_comment_multi */
	TRUE,  /* scan_identifier */
	FALSE, /* scan_identifier_1char */
	FALSE, /* scan_identifier_NULL */
	FALSE, /* scan_symbols */
	FALSE, /* scan_binary */
	FALSE, /* scan_octal */
	FALSE, /* scan_float */
	FALSE, /* scan_hex */
	FALSE, /* scan_hex_dollar */
	FALSE, /* scan_string_sq */
	TRUE,  /* scan_string_dq */
	TRUE,  /* numbers_2_int */
	FALSE, /* int_2_float */
	TRUE,  /* identifier_2_string */
	FALSE, /* char_2_token */
	FALSE, /* symbol_2_token */
	FALSE, /* scope_0_fallback */
	FALSE  /* store_int64 */
};

static FieldInstructionState *
fldinst_state_new(void)
{
	FieldInstructionState *retval = g_new0(FieldInstructionState, 1);
	retval->scanbuffer = g_string_new("");
	retval->general_number_format = NUMBER_ARABIC;
	return retval;
}

static FieldInstructionState *
fldinst_state_copy(FieldInstructionState *state)
{
	FieldInstructionState *retval = g_new0(FieldInstructionState, 1);
	/* Copy the same GString, do not create a new one */
    *retval = *state;
    return retval;
}

static void fldinst_state_free(FieldInstructionState *state)
{
	/* Do not free the GString, as it is shared between all the states on the stack */
	g_free(state);
}

static gchar *
get_string_token(GScanner *tokenizer)
{
	GTokenType token;
	
	if(g_scanner_eof(tokenizer))
	{
		g_warning(_("Unexpected end of field instructions"));
		g_scanner_destroy(tokenizer);
		return NULL;
	}
	token = g_scanner_get_next_token(tokenizer);
	if(token != G_TOKEN_STRING)
	{
		g_warning(_("Expected a string in field instructions"));
		g_scanner_destroy(tokenizer);
		return NULL;
	}
	return tokenizer->value.v_string;
}

typedef struct {
	gchar *switchname;
	gchar *switcharg;
} SwitchInfo;

static void
free_switch_info(SwitchInfo *info)
{
	g_free(info->switchname);
	g_free(info->switcharg);
	g_slice_free(SwitchInfo, info);
}

static GSList *
get_switches(GScanner *tokenizer, GSList *switcheslist, const gchar *switches, const gchar *argswitches, const gchar *wideswitches, const gchar *wideargswitches)
{
	gboolean found = FALSE;
	
	g_assert(strlen(wideswitches) % 2 == 0);
	g_assert(strlen(wideargswitches) % 2 == 0);

	/* Parse switches until an argument without '\' or unexpected switch */
	while(!g_scanner_eof(tokenizer))
	{
		GTokenType token = g_scanner_peek_next_token(tokenizer);
		found = FALSE;
		
		if(token != G_TOKEN_STRING)
			break;
		if(tokenizer->next_value.v_string[0] == '\\')
		{
			const gchar *ptr;
			for(ptr = switches; *ptr && !found; ptr++)
				if(tokenizer->next_value.v_string[1] == ptr[0] && tokenizer->next_value.v_string[2] == '\0')
				{
					SwitchInfo *info = g_slice_new0(SwitchInfo);
					info->switchname = g_strdup(tokenizer->next_value.v_string + 1);
					info->switcharg = NULL;
					g_scanner_get_next_token(tokenizer);
					switcheslist = g_slist_prepend(switcheslist, info);
					found = TRUE;
				}
			for(ptr = argswitches; *ptr && !found; ptr++)
				if(tokenizer->next_value.v_string[1] == ptr[0] && tokenizer->next_value.v_string[2] == '\0')
				{
					SwitchInfo *info = g_slice_new0(SwitchInfo);
					info->switchname = g_strdup(tokenizer->next_value.v_string + 1);
					g_scanner_get_next_token(tokenizer);
					info->switcharg = g_strdup(get_string_token(tokenizer));
					switcheslist = g_slist_prepend(switcheslist, info);
					found = TRUE;
				}
			for(ptr = wideswitches; *ptr && !found; ptr++)
				if(tokenizer->next_value.v_string[1] == ptr[0] && tokenizer->next_value.v_string[2] == ptr[1] && tokenizer->next_value.v_string[3] == '\0')
				{
					SwitchInfo *info = g_slice_new0(SwitchInfo);
					info->switchname = g_strdup(tokenizer->next_value.v_string + 1);
					info->switcharg = NULL;
					g_scanner_get_next_token(tokenizer);
					switcheslist = g_slist_prepend(switcheslist, info);
					found = TRUE;
				}
			for(ptr = wideargswitches; *ptr && !found; ptr += 2)
				if(tokenizer->next_value.v_string[1] == ptr[0] && tokenizer->next_value.v_string[2] == ptr[1] && tokenizer->next_value.v_string[3] == '\0')
				{
					SwitchInfo *info = g_slice_new0(SwitchInfo);
					info->switchname = g_strdup(tokenizer->next_value.v_string + 1);
					g_scanner_get_next_token(tokenizer);
					info->switcharg = g_strdup(get_string_token(tokenizer));
					switcheslist = g_slist_prepend(switcheslist, info);
					found = TRUE;
				}
		}
		else
			break;

		if(!found)
			break;
		/* Unexpected switch, so it must belong to the next part of the field */
	}

	return switcheslist;
}

static void field_instruction_end(ParserContext *ctx)
{
	FieldInstructionState *state = get_state(ctx);
	gchar *field_type, *buffer;
	GSList *switches = NULL, *formatswitches = NULL, *iter;
	GScanner *tokenizer = g_scanner_new(&field_parser);
	g_scanner_input_text(tokenizer, state->scanbuffer->str, strlen(state->scanbuffer->str));

	/*while(!g_scanner_eof(tokenizer))
	{
		GTokenType token = g_scanner_get_next_token(tokenizer);
		g_printerr("Token type: %d\n", token);
		if(token == G_TOKEN_STRING)
			g_printerr("String value: %s\n", tokenizer->value.v_string);
		else if(token == G_TOKEN_CHAR)
			g_printerr("Character value: %c\n", tokenizer->value.v_char);
	}
	return;*/
	
	/* Get field type */
	if(!(field_type = get_string_token(tokenizer)))
		return;

	/* Determine if field type is supported */
	if(strcmp(field_type, "HYPERLINK") == 0)
	{
		state->type = FIELD_TYPE_HYPERLINK;
		switches = get_switches(tokenizer, switches, "mn", "lot", "", "");
		if(!(buffer = get_string_token(tokenizer)))
			return;
		state->argument = g_strdup(buffer);
		switches = get_switches(tokenizer, switches, "mn", "lot", "", "");
		/* Ignore switches */
	}
	else if(strcmp(field_type, "INCLUDEPICTURE") == 0)
	{
		state->type = FIELD_TYPE_INCLUDEPICTURE;
		switches = get_switches(tokenizer, switches, "d", "c", "", "");
		if(!(buffer = get_string_token(tokenizer)))
			return;
		state->argument = g_strdup(buffer);
		switches = get_switches(tokenizer, switches, "d", "c", "", "");
		/* Ignore switches */
	}
	else if(strcmp(field_type, "PAGE") == 0)
	{
		state->type = FIELD_TYPE_PAGE;
		/* No switches or arguments */
	}
	else
	{
		g_warning(_("'%s' field not supported"), field_type);
		g_scanner_destroy(tokenizer);
		return;
	}

	formatswitches = get_switches(tokenizer, formatswitches, "", "@#*", "", "");
	for(iter = formatswitches; iter; iter = g_slist_next(iter))
	{
		SwitchInfo *info = (SwitchInfo *)iter->data;
		/* Parse a date format consisting of \@ and a string */
		if(strcmp(info->switchname, "@") == 0)
			state->date_format = g_strdup(info->switcharg);
		/* Parse a numeric format consisting of \# and a string */
		else if(strcmp(info->switchname, "#") == 0)
			state->numeric_format = g_strdup(info->switcharg);
		/* Parse a general format consisting of \* and a keyword */
		else if(strcmp(info->switchname, "*") == 0)
		{
			if(strcmp(info->switcharg, "ALPHABETIC") == 0)
				state->general_number_format = NUMBER_ALPHABETIC;
			else if(strcmp(info->switcharg, "alphabetic") == 0)
				state->general_number_format = NUMBER_alphabetic;
			else if(strcmp(info->switcharg, "Arabic") == 0)
				state->general_number_format = NUMBER_ARABIC;
			else if(strcmp(info->switcharg, "ArabicDash") == 0)
				state->general_number_format = NUMBER_ARABIC_DASH;
			else if(strcmp(info->switcharg, "CIRCLENUM") == 0)
				state->general_number_format = NUMBER_CIRCLENUM;
			else if(strcmp(info->switcharg, "GB1") == 0)
				state->general_number_format = NUMBER_DECIMAL_ENCLOSED_PERIOD;
			else if(strcmp(info->switcharg, "GB2") == 0)
				state->general_number_format = NUMBER_DECIMAL_ENCLOSED_PARENTHESES;
			else if(strcmp(info->switcharg, "Hex") == 0)
				state->general_number_format = NUMBER_HEX;
			else if(strcmp(info->switcharg, "MERGEFORMATINET") == 0)
				; /* ignore */
			else if(strcmp(info->switcharg, "Ordinal") == 0)
				state->general_number_format = NUMBER_ORDINAL;
			else if(strcmp(info->switcharg, "Roman") == 0)
				state->general_number_format = NUMBER_ROMAN;
			else if(strcmp(info->switcharg, "roman") == 0)
				state->general_number_format = NUMBER_roman;
			else
				g_warning(_("Format '%s' not supported."), buffer);
				/* Just continue */
		}
	}

	switch(state->type)
	{
		case FIELD_TYPE_HYPERLINK:
			break;
			/* ignore */

		case FIELD_TYPE_INCLUDEPICTURE:
		{
			GError *error = NULL;
			gchar **pathcomponents = g_strsplit(state->argument, "\\", 0);
			gchar *realfilename = g_build_filenamev(pathcomponents);
			
			g_strfreev(pathcomponents);
			GdkPixbuf *picture = gdk_pixbuf_new_from_file(realfilename, &error);
			if(!picture)
				g_warning(_("Error loading picture from file '%s': %s"), realfilename, error->message);
			else
			{
				/* Insert picture into text buffer */
				GtkTextIter iter;
				gtk_text_buffer_get_iter_at_mark(ctx->textbuffer, &iter, ctx->endmark);
				gtk_text_buffer_insert_pixbuf(ctx->textbuffer, &iter, picture);
				g_object_unref(picture);
			}
			g_free(realfilename);
		}
			break;

		case FIELD_TYPE_PAGE:
		{
			gchar *output = format_integer(1, state->general_number_format);
			GtkTextIter iter;
			gtk_text_buffer_get_iter_at_mark(ctx->textbuffer, &iter, ctx->endmark);
			gtk_text_buffer_insert(ctx->textbuffer, &iter, output, -1);
			g_free(output);
		}
			break;
			
		default:
			g_assert_not_reached();
	}
	
	g_free(state->argument);
	g_free(state->date_format);
	g_free(state->numeric_format);
	g_slist_foreach(switches, (GFunc)free_switch_info, NULL);
	g_slist_foreach(formatswitches, (GFunc)free_switch_info, NULL);
	g_scanner_destroy(tokenizer);
}

/* Free the return value when done with it */
static gchar *
format_integer(gint number, GeneralNumberFormat format)
{
	switch(format)
	{
		case NUMBER_ALPHABETIC:
			if(number < 1)
				break;
			return g_strnfill(number / 26 + 1, number % 26 - 1 + 'A');
		case NUMBER_alphabetic:
			if(number < 1)
				break;
			return g_strnfill(number / 26 + 1, number % 26 - 1 + 'a');
		case NUMBER_ARABIC_DASH:
			return g_strdup_printf("- %d -", number);
		case NUMBER_HEX:
			return g_strdup_printf("%X", number);
		case NUMBER_ORDINAL:
			if(number % 10 == 1 && number % 100 != 11)
				return g_strdup_printf("%dst", number);
			if(number % 10 == 2 && number % 100 != 12)
				return g_strdup_printf("%dnd", number);
			if(number % 10 == 3 && number % 100 != 13);
				return g_strdup_printf("%drd", number);
			return g_strdup_printf("%dth", number);
		case NUMBER_ROMAN:
			if(number < 1)
				break;
		{
			const gchar *r_hundred[] = { "", "C", "CC", "CCC", "CD", "D", "DC", "DCC", "DCCC", "CM" };
			const gchar *r_ten[] = { "", "X", "XX", "XXX", "XL", "L", "LX", "LXX", "LXXX", "XC" };
			const gchar *r_one[] = { "", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX" };
			gint thousands = number / 1000;
			gint hundreds = (number % 1000) / 100;
			gint tens = (number % 100) / 10;
			gint ones = number % 10;
			gchar *thousandstring = g_strnfill(thousands, 'M');
			gchar *retval = g_strconcat(thousandstring, r_hundred[hundreds], r_ten[tens], r_one[ones], NULL);
			g_free(thousandstring);
			return retval;
		}
		case NUMBER_roman:
			if(number < 1)
				break;
		{
			const gchar *r_hundred[] = { "", "c", "cc", "ccc", "cd", "d", "dc", "dcc", "dccc", "cm" };
			const gchar *r_ten[] = { "", "x", "xx", "xxx", "xl", "l", "lx", "lxx", "lxxx", "xc" };
			const gchar *r_one[] = { "", "i", "ii", "iii", "iv", "v", "vi", "vii", "viii", "ix" };
			gint thousands = number / 1000;
			gint hundreds = (number % 1000) / 100;
			gint tens = (number % 100) / 10;
			gint ones = number % 10;
			gchar *thousandstring = g_strnfill(thousands, 'm');
			gchar *retval = g_strconcat(thousandstring, r_hundred[hundreds], r_ten[tens], r_one[ones], NULL);
			g_free(thousandstring);
			return retval;
		}
		case NUMBER_CIRCLENUM:
			if(number >= 1 && number <= 20)
				return g_strdup_printf("\xE2\x91%c", 0x9F + number);
			break;
		case NUMBER_DECIMAL_ENCLOSED_PERIOD:
			if(number >= 1 && number <= 20)
				return g_strdup_printf("\xE2\x92%c", 0x87 + number);
			break;
		case NUMBER_DECIMAL_ENCLOSED_PARENTHESES:
			if(number >= 1 && number <= 12)
				return g_strdup_printf("\xE2\x91%c", 0xB3 + number);
			if(number >= 13 && number <= 20)
				return g_strdup_printf("\xE2\x92%c", 0x73 + number);
			break;
		case NUMBER_ARABIC:
		default:
			break;
	}
	return g_strdup_printf("%d", number);
}