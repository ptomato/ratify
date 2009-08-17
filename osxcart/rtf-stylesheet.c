#include <string.h>
#include <glib.h>
#include <config.h>
#include <glib/gi18n-lib.h>
#include <osxcart/rtf.h>
#include "rtf-deserialize.h"
#include "rtf-document.h"
#include "rtf-state.h"

/* rtf-stylesheet.c - Implementation of style sheets. */

typedef enum {
	STYLE_PARAGRAPH,
	STYLE_CHARACTER,
	STYLE_SECTION,
	STYLE_TABLE
} StyleType;

typedef struct {
    Attributes attr; /* must be first */
	gint index;
	StyleType type;
} StylesheetState;

/* Forward declarations */
static void stylesheet_text(ParserContext *ctx);

#define DEFINE_STYLE_FUNCTION(fn, styletype) \
    static gboolean \
    fn(ParserContext *ctx, StylesheetState *state, gint32 param, GError **error) \
    { \
        state->index = param; \
        state->type = styletype; \
        return TRUE; \
    }
DEFINE_STYLE_FUNCTION(sty_cs, STYLE_CHARACTER)
DEFINE_STYLE_FUNCTION(sty_ds, STYLE_SECTION)
DEFINE_STYLE_FUNCTION(sty_s, STYLE_PARAGRAPH)
DEFINE_STYLE_FUNCTION(sty_ts, STYLE_TABLE)

const ControlWord stylesheet_word_table[] = { 
	{ "b", OPTIONAL_PARAMETER, TRUE, doc_b, 1 },
	{ "cb", OPTIONAL_PARAMETER, TRUE, doc_cb, 0 },
	{ "cf", OPTIONAL_PARAMETER, TRUE, doc_cf, 0 },
	{ "chcbpat", OPTIONAL_PARAMETER, TRUE, doc_cb, 0 },
	{ "*cs", REQUIRED_PARAMETER, TRUE, sty_cs },
	{ "dn", OPTIONAL_PARAMETER, TRUE, doc_dn, 6 },
	{ "*ds", REQUIRED_PARAMETER, TRUE, sty_ds },
	{ "f", REQUIRED_PARAMETER, TRUE, doc_f },
	{ "fi", OPTIONAL_PARAMETER, TRUE, doc_fi, 0 },
	{ "fs", OPTIONAL_PARAMETER, TRUE, doc_fs, 24 },
	{ "fsmilli", REQUIRED_PARAMETER, TRUE, doc_fsmilli }, /* Apple extension */
	{ "highlight", REQUIRED_PARAMETER, TRUE, doc_highlight }, 
	{ "i", OPTIONAL_PARAMETER, TRUE, doc_i, 1 },
	{ "li", OPTIONAL_PARAMETER, TRUE, doc_li, 1 },
	{ "ltrch", NO_PARAMETER, TRUE, doc_ltrch },
	{ "ltrpar", NO_PARAMETER, TRUE, doc_ltrpar },
	{ "qc", NO_PARAMETER, TRUE, doc_qc },
	{ "qj", NO_PARAMETER, TRUE, doc_qj },
	{ "ql", NO_PARAMETER, TRUE, doc_ql },
	{ "qr", NO_PARAMETER, TRUE, doc_qr },
	{ "ri", OPTIONAL_PARAMETER, TRUE, doc_ri, 0 },
	{ "rtlch", NO_PARAMETER, TRUE, doc_rtlch },
	{ "rtlpar", NO_PARAMETER, TRUE, doc_rtlpar },
	{ "s", OPTIONAL_PARAMETER, TRUE, sty_s, 0 },
	{ "sa", OPTIONAL_PARAMETER, TRUE, doc_sa, 0 },
	{ "saauto", OPTIONAL_PARAMETER, TRUE, doc_saauto, 0 },
	{ "sb", OPTIONAL_PARAMETER, TRUE, doc_sb, 0 },
	{ "sbauto", OPTIONAL_PARAMETER, TRUE, doc_sbauto, 0 },
	{ "scaps", OPTIONAL_PARAMETER, TRUE, doc_scaps, 1 },
	{ "slleading", OPTIONAL_PARAMETER, TRUE, doc_slleading, 0 }, /* Apple extension */
	{ "strike", OPTIONAL_PARAMETER, TRUE, doc_strike, 1 },
	{ "sub", NO_PARAMETER, TRUE, doc_sub },
	{ "super", NO_PARAMETER, TRUE, doc_super },
	{ "*ts", REQUIRED_PARAMETER, TRUE, sty_ts },
	{ "tx", REQUIRED_PARAMETER, FALSE, doc_tx },
	{ "ul", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 },
	{ "uld", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 }, /* Treat unsupported types */
	{ "uldash", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 }, /* of underlining as */
	{ "uldashd", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 }, /* regular underlining */
	{ "uldashdd", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 },
	{ "uldb", OPTIONAL_PARAMETER, TRUE, doc_uldb, 1 },
	{ "ulhwave", OPTIONAL_PARAMETER, TRUE, doc_ulwave, 1 },
	{ "ulldash", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 },
	{ "ulnone", NO_PARAMETER, TRUE, doc_ulnone },
	{ "ulstyle", REQUIRED_PARAMETER, TRUE, doc_ulstyle }, /* Apple extension */
	{ "ulth", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 },
	{ "ulthd", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 },
	{ "ulthdash", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 },
	{ "ulthdashd", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 },
	{ "ulthdashdd", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 },
	{ "ulthldash", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 },
	{ "ululdbwave", OPTIONAL_PARAMETER, TRUE, doc_ulwave, 1 },
	{ "ulw", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 },
	{ "ulwave", OPTIONAL_PARAMETER, TRUE, doc_ulwave, 1 },
	{ "up", OPTIONAL_PARAMETER, TRUE, doc_up, 6 },
	{ "v", OPTIONAL_PARAMETER, TRUE, doc_v, 1 },
	{ NULL }
};

DEFINE_ATTR_STATE_FUNCTIONS(StylesheetState, stylesheet)

const DestinationInfo stylesheet_destination = {
	stylesheet_word_table,
	stylesheet_text,
	stylesheet_state_new,
	stylesheet_state_copy,
	stylesheet_state_free
};

/* Add a style tag to the GtkTextBuffer's tag table with all the attributes of
the current style */
static void
stylesheet_text(ParserContext *ctx)
{
	gchar *semicolon, *tagname;
	StylesheetState *state = get_state(ctx);
	Attributes *attr = (Attributes *)state;
	GtkTextTag *tag;
	
	semicolon = strchr(ctx->text->str, ';');
	if(!semicolon)
	{
		g_string_truncate(ctx->text, 0);
		return;
	}
	g_string_assign(ctx->text, semicolon + 1); /* Leave the text after the semicolon in the buffer */
	
	tagname = g_strdup_printf("osxcart-rtf-style-%i", state->index);
    if((tag = gtk_text_tag_table_lookup(ctx->tags, tagname)))
        gtk_text_tag_table_remove(ctx->tags, tag);
	tag = gtk_text_tag_new(tagname);

	/* Add each paragraph attribute to the tag */
    if(attr->justification != -1)
		g_object_set(tag, 
		             "justification", attr->justification,
		             "justification-set", TRUE,
		             NULL);
	if(attr->pardirection != -1)
		g_object_set(tag, "direction", attr->pardirection, NULL);
	if(attr->space_before != 0 && !attr->ignore_space_before)
		g_object_set(tag,
		             "pixels-above-lines", PANGO_PIXELS(TWIPS_TO_PANGO(attr->space_before)),
		             "pixels-above-lines-set", TRUE,
		             NULL);
	if(attr->space_after != 0 && !attr->ignore_space_after)
		g_object_set(tag,
		             "pixels-below-lines", PANGO_PIXELS(TWIPS_TO_PANGO(attr->space_after)),
		             "pixels-below-lines-set", TRUE,
		             NULL);
	if(attr->tabs)
		g_object_set(tag, 
		             "tabs", attr->tabs,
		             "tabs-set", TRUE,
		             NULL);
	if(attr->left_margin)
		g_object_set(tag,
		             "left-margin", PANGO_PIXELS(TWIPS_TO_PANGO(attr->left_margin)),
		             "left-margin-set", TRUE,
		             NULL);
	if(attr->right_margin)
		g_object_set(tag,
		             "right-margin", PANGO_PIXELS(TWIPS_TO_PANGO(attr->right_margin)),
		             "right-margin-set", TRUE,
		             NULL);
	if(attr->indent)
		g_object_set(tag,
		             "indent", PANGO_PIXELS(TWIPS_TO_PANGO(attr->indent)),
		             "indent-set", TRUE,
		             NULL);

	/* Add each character attribute to the tag */
	if(attr->foreground != -1)
	{
		gchar *color = g_slist_nth_data(ctx->color_table, attr->foreground);
		/* color must exist, because that was already checked when executing
		 the \cf command */
        g_object_set(tag, 
                     "foreground", color, 
                     "foreground-set", TRUE, 
                     NULL);
	}
	if(attr->background != -1)
	{
		gchar *color = g_slist_nth_data(ctx->color_table, attr->background);
		/* color must exist, because that was already checked when executing
		 the \cf command */
        g_object_set(tag, 
                     "background", color, 
                     "background-set", TRUE, 
                     NULL);
	}
	if(attr->highlight != -1)
	{
		gchar *color = g_slist_nth_data(ctx->color_table, attr->highlight);
		/* color must exist, because that was already checked when executing
		 the \cf command */
        g_object_set(tag, 
                     "paragraph-background", color, 
                     "paragraph-background-set", TRUE, 
                     NULL);
	}
	if(attr->font != -1)
	{
		gchar *tagname = g_strdup_printf("osxcart-rtf-font-%d", attr->font);
		GtkTextTag *fonttag = gtk_text_tag_table_lookup(ctx->tags, tagname);
		PangoFontDescription *fontdesc;

		g_object_get(fonttag, "font-desc", &fontdesc, NULL); /* do not free */
		if(fontdesc && pango_font_description_get_family(fontdesc))
		{
			g_object_set(tag, 
			             "family", g_strdup(pango_font_description_get_family(fontdesc)), 
			             "family-set", TRUE,
			             NULL);
		}
		g_free(tagname);
	}
	if(attr->size != 0.0)
		g_object_set(tag,
		             "size", POINTS_TO_PANGO(attr->size),
		             "size-set", TRUE,
		             NULL);
	if(attr->italic)
		g_object_set(tag,
		             "style", PANGO_STYLE_ITALIC,
		             "style-set", TRUE,
		             NULL);
	if(attr->bold)
		g_object_set(tag,
		             "weight", PANGO_WEIGHT_BOLD,
		             "weight-set", TRUE,
		             NULL);
	if(attr->smallcaps)
		g_object_set(tag,
		             "variant", PANGO_VARIANT_SMALL_CAPS,
		             "variant-set", TRUE,
		             NULL);
	if(attr->strikethrough)
		g_object_set(tag,
		             "strikethrough", TRUE,
		             "strikethrough-set", TRUE,
		             NULL);
	if(attr->subscript)
		g_object_set(tag,
		             "rise", POINTS_TO_PANGO(-6),
		             "rise-set", TRUE,
		             "scale", PANGO_SCALE_X_SMALL,
		             "scale-set", TRUE,
		             NULL);
	if(attr->superscript)
		g_object_set(tag,
		             "rise", POINTS_TO_PANGO(6),
		             "rise-set", TRUE,
		             "scale", PANGO_SCALE_X_SMALL,
		             "scale-set", TRUE,
		             NULL);
	if(attr->invisible)
		g_object_set(tag,
		             "invisible", TRUE,
		             "invisible-set", TRUE,
		             NULL);
	if(attr->underline != -1)
		g_object_set(tag,
		             "underline", attr->underline,
		             "underline-set", TRUE,
		             NULL);
	if(attr->chardirection != -1)
		g_object_set(tag, "direction", attr->chardirection, NULL);
	if(attr->rise != 0)
		g_object_set(tag,
		             "rise", HALF_POINTS_TO_PANGO(attr->rise),
		             "rise-set", TRUE,
		             NULL);
       
    gtk_text_tag_table_add(ctx->tags, tag);
	g_free(tagname);

	state->index = 0;
	state->type = STYLE_PARAGRAPH;
	set_default_paragraph_attributes(attr);
	set_default_character_attributes(attr);
}
