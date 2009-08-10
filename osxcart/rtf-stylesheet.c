#include <string.h>
#include <glib.h>
#include <config.h>
#include <glib/gi18n-lib.h>
#include <osxcart/rtf.h>
#include "rtf-deserialize.h"
#include "rtf-document.h"

typedef enum {
	STYLE_PARAGRAPH,
	STYLE_CHARACTER,
	STYLE_SECTION,
	STYLE_TABLE
} StyleType;

typedef struct {
	gint index;
	StyleType type;
	Attributes *attr;
} StylesheetState;

static void stylesheet_text(ParserContext *ctx);
static StylesheetState *stylesheet_state_new(void);
static StylesheetState *stylesheet_state_copy(StylesheetState *state);
static void stylesheet_state_free(StylesheetState *state);

typedef gboolean StyleFunc(ParserContext *, StylesheetState *, GError **);
typedef gboolean StyleParamFunc(ParserContext *, StylesheetState *, gint32, GError **);

static StyleFunc sty_ltrch, sty_ltrpar, sty_qc, sty_qj, sty_ql,
                 sty_qr, sty_rtlch, sty_rtlpar, sty_sub, sty_super, sty_ulnone;
static StyleParamFunc sty_b, sty_cb, sty_cf, sty_cs, sty_dn, sty_ds, sty_f,
                      sty_fi, sty_fs, sty_fsmilli, sty_highlight, sty_i, sty_li,
                      sty_ri, sty_s, sty_sa, sty_saauto, sty_sb, sty_sbauto, 
                      sty_scaps, sty_slleading, sty_strike, sty_ts, sty_tx, 
                      sty_ul, sty_uldb, styl_ulstyle, sty_ulwave, sty_up, sty_v;

const ControlWord stylesheet_word_table[] = { 
	{ "b", OPTIONAL_PARAMETER, TRUE, sty_b, 1 },
	{ "cb", OPTIONAL_PARAMETER, TRUE, sty_cb, 0 },
	{ "cf", OPTIONAL_PARAMETER, TRUE, sty_cf, 0 },
	{ "chcbpat", OPTIONAL_PARAMETER, TRUE, sty_cb, 0 },
	{ "*cs", REQUIRED_PARAMETER, TRUE, sty_cs },
	{ "dn", OPTIONAL_PARAMETER, TRUE, sty_dn, 6 },
	{ "*ds", REQUIRED_PARAMETER, TRUE, sty_ds },
	{ "f", REQUIRED_PARAMETER, TRUE, sty_f },
	{ "fi", OPTIONAL_PARAMETER, TRUE, sty_fi, 0 },
	{ "fs", OPTIONAL_PARAMETER, TRUE, sty_fs, 24 },
	{ "fsmilli", REQUIRED_PARAMETER, TRUE, sty_fsmilli },
	{ "highlight", REQUIRED_PARAMETER, TRUE, sty_highlight }, 
	{ "i", OPTIONAL_PARAMETER, TRUE, sty_i, 1 },
	{ "li", OPTIONAL_PARAMETER, TRUE, sty_li, 1 },
	{ "ltrch", NO_PARAMETER, TRUE, sty_ltrch },
	{ "ltrpar", NO_PARAMETER, TRUE, sty_ltrpar },
	{ "qc", NO_PARAMETER, TRUE, sty_qc },
	{ "qj", NO_PARAMETER, TRUE, sty_qj },
	{ "ql", NO_PARAMETER, TRUE, sty_ql },
	{ "qr", NO_PARAMETER, TRUE, sty_qr },
	{ "ri", OPTIONAL_PARAMETER, TRUE, sty_ri, 0 },
	{ "rtlch", NO_PARAMETER, TRUE, sty_rtlch },
	{ "rtlpar", NO_PARAMETER, TRUE, sty_rtlpar },
	{ "s", OPTIONAL_PARAMETER, TRUE, sty_s, 0 },
	{ "sa", OPTIONAL_PARAMETER, TRUE, sty_sa, 0 },
	{ "saauto", OPTIONAL_PARAMETER, TRUE, sty_saauto, 0 },
	{ "sb", OPTIONAL_PARAMETER, TRUE, sty_sb, 0 },
	{ "sbauto", OPTIONAL_PARAMETER, TRUE, sty_sbauto, 0 },
	{ "scaps", OPTIONAL_PARAMETER, TRUE, sty_scaps, 1 },
	{ "slleading", OPTIONAL_PARAMETER, TRUE, sty_slleading, 0 },
	{ "strike", OPTIONAL_PARAMETER, TRUE, sty_strike, 1 },
	{ "sub", NO_PARAMETER, TRUE, sty_sub },
	{ "super", NO_PARAMETER, TRUE, sty_super },
	{ "*ts", REQUIRED_PARAMETER, TRUE, sty_ts },
	{ "tx", REQUIRED_PARAMETER, FALSE, sty_tx },
	{ "ul", OPTIONAL_PARAMETER, TRUE, sty_ul, 1 },
	{ "uld", OPTIONAL_PARAMETER, TRUE, sty_ul, 1 }, /* Treat unsupported types */
	{ "uldash", OPTIONAL_PARAMETER, TRUE, sty_ul, 1 }, /* of underlining as */
	{ "uldashd", OPTIONAL_PARAMETER, TRUE, sty_ul, 1 }, /* regular underlining */
	{ "uldashdd", OPTIONAL_PARAMETER, TRUE, sty_ul, 1 },
	{ "uldb", OPTIONAL_PARAMETER, TRUE, sty_uldb, 1 },
	{ "ulhwave", OPTIONAL_PARAMETER, TRUE, sty_ulwave, 1 },
	{ "ulldash", OPTIONAL_PARAMETER, TRUE, sty_ul, 1 },
	{ "ulnone", NO_PARAMETER, TRUE, sty_ulnone },
	{ "ulth", OPTIONAL_PARAMETER, TRUE, sty_ul, 1 },
	{ "ulthd", OPTIONAL_PARAMETER, TRUE, sty_ul, 1 },
	{ "ulthdash", OPTIONAL_PARAMETER, TRUE, sty_ul, 1 },
	{ "ulthdashd", OPTIONAL_PARAMETER, TRUE, sty_ul, 1 },
	{ "ulthdashdd", OPTIONAL_PARAMETER, TRUE, sty_ul, 1 },
	{ "ulthldash", OPTIONAL_PARAMETER, TRUE, sty_ul, 1 },
	{ "ululdbwave", OPTIONAL_PARAMETER, TRUE, sty_ulwave, 1 },
	{ "ulw", OPTIONAL_PARAMETER, TRUE, sty_ul, 1 },
	{ "ulwave", OPTIONAL_PARAMETER, TRUE, sty_ulwave, 1 },
	{ "up", OPTIONAL_PARAMETER, TRUE, sty_up, 6 },
	{ "v", OPTIONAL_PARAMETER, TRUE, sty_v, 1 },
	{ NULL }
};

const DestinationInfo stylesheet_destination = {
	stylesheet_word_table,
	stylesheet_text,
	(StateNewFunc *)stylesheet_state_new,
	(StateCopyFunc *)stylesheet_state_copy,
	(StateFreeFunc *)stylesheet_state_free
};

static StylesheetState *
stylesheet_state_new(void)
{
	StylesheetState *retval = g_new0(StylesheetState, 1);
	retval->attr = attributes_new();
	return retval;
}

static StylesheetState *
stylesheet_state_copy(StylesheetState *state)
{
	StylesheetState *retval = g_new0(StylesheetState, 1);
	*retval = *state;
	retval->attr = attributes_copy(state->attr);
	return retval;
}

static void
stylesheet_state_free(StylesheetState *state)
{
	attributes_free(state->attr);
	g_free(state);
}

static void
stylesheet_text(ParserContext *ctx)
{
	gchar *semicolon, *tagname;
	StylesheetState *state = (StylesheetState *)get_state(ctx);
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
    if(state->attr->justification != -1)
		g_object_set(tag, 
		             "justification", state->attr->justification,
		             "justification-set", TRUE,
		             NULL);
	if(state->attr->pardirection != -1)
		g_object_set(tag, "direction", state->attr->pardirection, NULL);
	if(state->attr->space_before != 0 && !state->attr->ignore_space_before)
		g_object_set(tag,
		             "pixels-above-lines", PANGO_PIXELS(TWIPS_TO_PANGO(state->attr->space_before)),
		             "pixels-above-lines-set", TRUE,
		             NULL);
	if(state->attr->space_after != 0 && !state->attr->ignore_space_after)
		g_object_set(tag,
		             "pixels-below-lines", PANGO_PIXELS(TWIPS_TO_PANGO(state->attr->space_after)),
		             "pixels-below-lines-set", TRUE,
		             NULL);
	if(state->attr->tabs)
		g_object_set(tag, 
		             "tabs", state->attr->tabs,
		             "tabs-set", TRUE,
		             NULL);
	if(state->attr->left_margin)
		g_object_set(tag,
		             "left-margin", PANGO_PIXELS(TWIPS_TO_PANGO(state->attr->left_margin)),
		             "left-margin-set", TRUE,
		             NULL);
	if(state->attr->right_margin)
		g_object_set(tag,
		             "right-margin", PANGO_PIXELS(TWIPS_TO_PANGO(state->attr->right_margin)),
		             "right-margin-set", TRUE,
		             NULL);
	if(state->attr->indent)
		g_object_set(tag,
		             "indent", PANGO_PIXELS(TWIPS_TO_PANGO(state->attr->indent)),
		             "indent-set", TRUE,
		             NULL);

	/* Add each character attribute to the tag */
	if(state->attr->foreground != -1)
	{
		gchar *color = g_slist_nth_data(ctx->color_table, state->attr->foreground);
		/* color must exist, because that was already checked when executing
		 the \cf command */
        g_object_set(tag, 
                     "foreground", color, 
                     "foreground-set", TRUE, 
                     NULL);
	}
	if(state->attr->background != -1)
	{
		gchar *color = g_slist_nth_data(ctx->color_table, state->attr->background);
		/* color must exist, because that was already checked when executing
		 the \cf command */
        g_object_set(tag, 
                     "background", color, 
                     "background-set", TRUE, 
                     NULL);
	}
	if(state->attr->highlight != -1)
	{
		gchar *color = g_slist_nth_data(ctx->color_table, state->attr->highlight);
		/* color must exist, because that was already checked when executing
		 the \cf command */
        g_object_set(tag, 
                     "paragraph-background", color, 
                     "paragraph-background-set", TRUE, 
                     NULL);
	}
	if(state->attr->font != -1)
	{
		gchar *tagname = g_strdup_printf("osxcart-rtf-font-%d", state->attr->font);
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
	if(state->attr->size != 0.0)
		g_object_set(tag,
		             "size", POINTS_TO_PANGO(state->attr->size),
		             "size-set", TRUE,
		             NULL);
	if(state->attr->italic)
		g_object_set(tag,
		             "style", PANGO_STYLE_ITALIC,
		             "style-set", TRUE,
		             NULL);
	if(state->attr->bold)
		g_object_set(tag,
		             "weight", PANGO_WEIGHT_BOLD,
		             "weight-set", TRUE,
		             NULL);
	if(state->attr->smallcaps)
		g_object_set(tag,
		             "variant", PANGO_VARIANT_SMALL_CAPS,
		             "variant-set", TRUE,
		             NULL);
	if(state->attr->strikethrough)
		g_object_set(tag,
		             "strikethrough", TRUE,
		             "strikethrough-set", TRUE,
		             NULL);
	if(state->attr->subscript)
		g_object_set(tag,
		             "rise", POINTS_TO_PANGO(-6),
		             "rise-set", TRUE,
		             "scale", PANGO_SCALE_X_SMALL,
		             "scale-set", TRUE,
		             NULL);
	if(state->attr->superscript)
		g_object_set(tag,
		             "rise", POINTS_TO_PANGO(6),
		             "rise-set", TRUE,
		             "scale", PANGO_SCALE_X_SMALL,
		             "scale-set", TRUE,
		             NULL);
	if(state->attr->invisible)
		g_object_set(tag,
		             "invisible", TRUE,
		             "invisible-set", TRUE,
		             NULL);
	if(state->attr->underline != -1)
		g_object_set(tag,
		             "underline", state->attr->underline,
		             "underline-set", TRUE,
		             NULL);
	if(state->attr->chardirection != -1)
		g_object_set(tag, "direction", state->attr->chardirection, NULL);
	if(state->attr->rise != 0)
		g_object_set(tag,
		             "rise", HALF_POINTS_TO_PANGO(state->attr->rise),
		             "rise-set", TRUE,
		             NULL);
       
    gtk_text_tag_table_add(ctx->tags, tag);
	g_free(tagname);

	state->index = 0;
	state->type = STYLE_PARAGRAPH;
	attributes_free(state->attr);
	state->attr = attributes_new();
}

static gboolean
sty_cb(ParserContext *ctx, StylesheetState *state, gint32 param, GError **error)
{
	if(g_slist_nth_data(ctx->color_table, param) == NULL)
	{
		g_set_error(error, RTF_ERROR, RTF_ERROR_UNDEFINED_COLOR, _("Color '%i' undefined"), param);
		return FALSE;
	}
	state->attr->background = param;
	return TRUE;
}

static gboolean
sty_cf(ParserContext *ctx, StylesheetState *state, gint32 param, GError **error)
{
	if(g_slist_nth_data(ctx->color_table, param) == NULL)
	{
		g_set_error(error, RTF_ERROR, RTF_ERROR_UNDEFINED_COLOR, _("Color '%i' undefined"), param);
		return FALSE;
	}
	state->attr->foreground = param;
	return TRUE;
}

static gboolean
sty_cs(ParserContext *ctx, StylesheetState *state, gint32 param, GError **error)
{
	state->index = param;
	state->type = STYLE_CHARACTER;
	return TRUE;
}

static gboolean
sty_dn(ParserContext *ctx, StylesheetState *state, gint32 param, GError **error)
{
	state->attr->rise = -param;
	return TRUE;
}

static gboolean
sty_ds(ParserContext *ctx, StylesheetState *state, gint32 param, GError **error)
{
	state->index = param;
	state->type = STYLE_SECTION;
	return TRUE;
}

static gboolean
sty_f(ParserContext *ctx, StylesheetState *state, gint32 param, GError **error)
{
	if(get_font_properties(ctx, param) == NULL)
	{
		g_set_error(error, RTF_ERROR, RTF_ERROR_UNDEFINED_FONT, _("Font '%i' undefined"), param);
		return FALSE;
	}
	state->attr->font = param;
	return TRUE;
}

static gboolean 
sty_fs(ParserContext *ctx, StylesheetState *state, gint32 halfpoints, GError **error)
{
    if(halfpoints <= 0)
	{
		g_set_error(error, RTF_ERROR, RTF_ERROR_BAD_FONT_SIZE, _("\\fs%d is invalid, negative or zero font sizes not allowed"), halfpoints);
		return FALSE; 
	}
    state->attr->size = halfpoints / 2.0;
    return TRUE;
}

static gboolean 
sty_fsmilli(ParserContext *ctx, StylesheetState *state, gint32 milli, GError **error)
{
    if(milli <= 0)
	{
		g_set_error(error, RTF_ERROR, RTF_ERROR_BAD_FONT_SIZE, _("\\fsmilli%d is invalid, negative or zero font sizes not allowed"), milli);
		return FALSE; 
	}
    state->attr->size = milli / 1000.0;
    return TRUE;
}

static gboolean
sty_highlight(ParserContext *ctx, StylesheetState *state, gint32 param, GError **error)
{
	if(g_slist_nth_data(ctx->color_table, param) == NULL)
	{
		g_set_error(error, RTF_ERROR, RTF_ERROR_UNDEFINED_COLOR, _("Color '%i' undefined"), param);
		return FALSE;
	}
	state->attr->highlight = param;
	return TRUE;
}

static gboolean
sty_s(ParserContext *ctx, StylesheetState *state, gint32 param, GError **error)
{
	state->index = param;
	state->type = STYLE_PARAGRAPH;
	return TRUE;
}

static gboolean
sty_ts(ParserContext *ctx, StylesheetState *state, gint32 param, GError **error)
{
	state->index = param;
	state->type = STYLE_TABLE;
	return TRUE;
}

static gboolean
sty_tx(ParserContext *ctx, StylesheetState *state, gint32 twips, GError **error)
{
    gint tab_index;
    
    if(state->attr->tabs == NULL)
    {
        state->attr->tabs = pango_tab_array_new(1, FALSE);
        tab_index = 0;
    }
    else
    {
        tab_index = pango_tab_array_get_size(state->attr->tabs);
        pango_tab_array_resize(state->attr->tabs, tab_index + 1);
    }
    
    pango_tab_array_set_tab(state->attr->tabs, tab_index, PANGO_TAB_LEFT, TWIPS_TO_PANGO(twips));
        
    return TRUE;
}

static gboolean
sty_ulstyle(ParserContext *ctx, StylesheetState *state, gint32 param, GError **error)
{
    switch(param & 0xF)
    {
        case 1:
            return sty_ul(ctx, state, 1, error);
        case 9:
            return sty_uldb(ctx, state, 1, error);
    }
    return sty_ulnone(ctx, state, error);
}

#define DEFINE_STYLE_FLAG_FUNCTION(name, attribute) \
	static gboolean \
	name(ParserContext *ctx, StylesheetState *state, gint32 param, GError **error) \
	{ \
		state->attr->attribute = (param != 0); \
		return TRUE; \
	}
DEFINE_STYLE_FLAG_FUNCTION(sty_b,      bold)
DEFINE_STYLE_FLAG_FUNCTION(sty_i,      italic)
DEFINE_STYLE_FLAG_FUNCTION(sty_saauto, ignore_space_after)
DEFINE_STYLE_FLAG_FUNCTION(sty_sbauto, ignore_space_before)
DEFINE_STYLE_FLAG_FUNCTION(sty_scaps,  smallcaps)
DEFINE_STYLE_FLAG_FUNCTION(sty_strike, strikethrough)
DEFINE_STYLE_FLAG_FUNCTION(sty_v,      invisible)

#define DEFINE_STYLE_PARAM_FUNCTION(name, attribute) \
	static gboolean \
	name(ParserContext *ctx, StylesheetState *state, gint32 param, GError **error) \
	{ \
		state->attr->attribute = param; \
		return TRUE; \
	}
DEFINE_STYLE_PARAM_FUNCTION(sty_fi,        indent)
DEFINE_STYLE_PARAM_FUNCTION(sty_li,        left_margin)
DEFINE_STYLE_PARAM_FUNCTION(sty_ri,        right_margin)
DEFINE_STYLE_PARAM_FUNCTION(sty_sa,        space_after)
DEFINE_STYLE_PARAM_FUNCTION(sty_sb,        space_before)
DEFINE_STYLE_PARAM_FUNCTION(sty_slleading, leading)
DEFINE_STYLE_PARAM_FUNCTION(sty_up,        rise)

#define DEFINE_STYLE_VALUE_FUNCTION(name, attribute, value) \
	static gboolean \
	name(ParserContext *ctx, StylesheetState *state, GError **error) \
	{ \
		state->attr->attribute = value; \
		return TRUE; \
	}
DEFINE_STYLE_VALUE_FUNCTION(sty_ltrch,  chardirection, GTK_TEXT_DIR_LTR)
DEFINE_STYLE_VALUE_FUNCTION(sty_ltrpar, pardirection,  GTK_TEXT_DIR_LTR)
DEFINE_STYLE_VALUE_FUNCTION(sty_qc,     justification, GTK_JUSTIFY_CENTER)
DEFINE_STYLE_VALUE_FUNCTION(sty_qj,     justification, GTK_JUSTIFY_FILL)
DEFINE_STYLE_VALUE_FUNCTION(sty_ql,     justification, GTK_JUSTIFY_LEFT)
DEFINE_STYLE_VALUE_FUNCTION(sty_qr,     justification, GTK_JUSTIFY_RIGHT)
DEFINE_STYLE_VALUE_FUNCTION(sty_rtlch,  chardirection, GTK_TEXT_DIR_RTL)
DEFINE_STYLE_VALUE_FUNCTION(sty_rtlpar, pardirection,  GTK_TEXT_DIR_RTL)
DEFINE_STYLE_VALUE_FUNCTION(sty_sub,    subscript,     TRUE)
DEFINE_STYLE_VALUE_FUNCTION(sty_super,  superscript,   TRUE)
DEFINE_STYLE_VALUE_FUNCTION(sty_ulnone, underline,     PANGO_UNDERLINE_NONE)

#define DEFINE_STYLE_VALUE_FLAG_FUNCTION(name, attribute, value1, value2) \
	static gboolean \
	name(ParserContext *ctx, StylesheetState *state, gint32 param, GError **error) \
	{ \
		state->attr->attribute = param? value1 : value2; \
		return TRUE; \
	}
DEFINE_STYLE_VALUE_FLAG_FUNCTION(sty_ul,     underline, PANGO_UNDERLINE_SINGLE, PANGO_UNDERLINE_NONE)
DEFINE_STYLE_VALUE_FLAG_FUNCTION(sty_uldb,   underline, PANGO_UNDERLINE_DOUBLE, PANGO_UNDERLINE_NONE)
DEFINE_STYLE_VALUE_FLAG_FUNCTION(sty_ulwave, underline, PANGO_UNDERLINE_ERROR,  PANGO_UNDERLINE_NONE)
