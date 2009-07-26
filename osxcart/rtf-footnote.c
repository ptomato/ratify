#include <string.h>
#include <glib.h>
#include "rtf-deserialize.h"
#include "rtf-document.h"

static void footnote_text(ParserContext *ctx);
static void footnote_end(ParserContext *ctx);

const ControlWord footnote_word_table[] = {
	DOCUMENT_TEXT_CONTROL_WORDS,
	{ NULL }
};

const DestinationInfo footnote_destination = {
    footnote_word_table,
    footnote_text,
    (StateNewFunc *)attributes_new,
    (StateCopyFunc *)attributes_copy,
    (StateFreeFunc *)attributes_free,
	footnote_end
};

static void
footnote_text(ParserContext *ctx)
{
	GtkTextIter start, end;
	GtkTextMark *placeholder;
	Attributes *attr; 
	int length;
	gchar *text;

	g_assert(ctx != NULL);
	
	attr = (Attributes *)get_state(ctx);
    text = ctx->text->str;
    
	if(text[0] == '\0') 
		return;

	length = strlen(text) - 1;
	if(!ctx->group_nesting_level && text[length] == '\n')
		text[length] = '\0';

	g_string_append(ctx->plaintext, text);

	gtk_text_buffer_get_end_iter(ctx->textbuffer, &end);
	placeholder = gtk_text_buffer_create_mark(ctx->textbuffer, NULL, &end, TRUE);
	gtk_text_buffer_insert(ctx->textbuffer, &end, text, -1);
	gtk_text_buffer_get_iter_at_mark(ctx->textbuffer, &start, placeholder);
	gtk_text_buffer_get_end_iter(ctx->textbuffer, &end);

	apply_attributes(ctx, attr, &start, &end);

	gtk_text_buffer_delete_mark(ctx->textbuffer, placeholder);
	
	g_string_truncate(ctx->text, 0);

	/* Move the regular document endmark back to the startmark so the footnotes
	 appear after the document */
	gtk_text_buffer_get_iter_at_mark(ctx->textbuffer, &start, ctx->startmark);
	gtk_text_buffer_move_mark(ctx->textbuffer, ctx->endmark, &start);
}

static void
footnote_end(ParserContext *ctx)
{
	ctx->footnote_number++;
}