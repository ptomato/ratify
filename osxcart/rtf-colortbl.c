#include <string.h>
#include <glib.h>
#include "rtf-deserialize.h"

typedef struct {
    gint red;
    gint green;
    gint blue;
} ColorTableState;

static void color_table_text(ParserContext *ctx);
static ColorTableState *colortbl_state_new(void);
static ColorTableState *colortbl_state_copy(ColorTableState *state);

typedef gboolean ColorParamFunc(ParserContext *, ColorTableState *, gint32, GError **);

/* Color table destination functions */
static ColorParamFunc ct_red, ct_green, ct_blue;
extern const DestinationInfo ignore_destination;

const ControlWord colortbl_word_table[] = {
	{ "red", REQUIRED_PARAMETER, TRUE, ct_red },
	{ "green", REQUIRED_PARAMETER, TRUE, ct_green },
	{ "blue", REQUIRED_PARAMETER, TRUE, ct_blue },
	{ NULL }
};

const DestinationInfo colortbl_destination = {
    colortbl_word_table,
    color_table_text,
    (StateNewFunc *)colortbl_state_new,
    (StateCopyFunc *)colortbl_state_copy,
    g_free
};

static void
color_table_text(ParserContext *ctx)
{
    ColorTableState *state = (ColorTableState *)get_state(ctx);
	if(strchr(ctx->text->str, ';'))
	{
		gchar *color = g_strdup_printf("#%02x%02x%02x", state->red, state->green, state->blue);
		ctx->color_table = g_slist_append(ctx->color_table, color);
		state->red = state->green = state->blue = 0;
	}
	g_string_truncate(ctx->text, 0);
}

static ColorTableState *
colortbl_state_new(void)
{
    return g_new0(ColorTableState, 1);
}

static ColorTableState *
colortbl_state_copy(ColorTableState *state)
{
    ColorTableState *retval = colortbl_state_new();
    *retval = *state;
    return retval;
}

static gboolean
ct_red(ParserContext *ctx, ColorTableState *state, gint32 param, GError **error)
{
	state->red = param;
	return TRUE;
}

static gboolean
ct_green(ParserContext *ctx, ColorTableState *state, gint32 param, GError **error)
{
	state->green = param;
	return TRUE;
}

static gboolean
ct_blue(ParserContext *ctx, ColorTableState *state, gint32 param, GError **error)
{
	state->blue = param;
	return TRUE;
}
