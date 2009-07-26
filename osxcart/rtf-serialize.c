#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>

/* Change this later FIXME */
#define PIXELS_TO_TWIPS(pixels) (pixels * 20)
#define PANGO_TO_HALF_POINTS(pango) (2 * pango / PANGO_SCALE)
#define PANGO_TO_TWIPS(pango) (20 * pango / PANGO_SCALE)

struct _WriterContext {
    /* Tag information */
    GHashTable *tag_codes;
    GSList *font_table;
    GSList *color_table;
};
typedef struct _WriterContext WriterContext;

static WriterContext *
writer_context_new(void)
{
    WriterContext *ctx = g_slice_new0(WriterContext);
    ctx->tag_codes = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_free);
    return ctx;
}

static void 
writer_context_free(WriterContext *ctx)
{
    g_hash_table_unref(ctx->tag_codes);
    g_slice_free(WriterContext, ctx);
}

static void 
convert_tag_to_code(GtkTextTag *tag, WriterContext *ctx)
{
    gboolean val;
    gint pixels, pango;
    gdouble factor, points;
    GString *code = g_string_new("");
    
    /* g_object_get(tag, "background-set", &val, NULL);
    if(val)
    {
        GdkColor color;*/
        
    g_object_get(tag, "indent-set", &val, NULL);
    if(val)
    {
        g_object_get(tag, "indent", &pixels, NULL);
        g_string_append_printf(code, "\fi%d", PIXELS_TO_TWIPS(pixels));
    }
        
    g_object_get(tag, "invisible-set", &val, NULL);
    if(val)
    {
        g_object_get(tag, "invisible", &val, NULL);
        if(val)
            g_string_append(code, "\\v");
        else
            g_string_append(code, "\\v0");
    }
    
    g_object_get(tag, "justification-set", &val, NULL);
    if(val)
    {
        GtkJustification just;
        g_object_get(tag, "justification", &just, NULL);
        switch(just)
        {
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
    
    g_object_get(tag, "left-margin-set", &val, NULL);
    if(val)
    {
        g_object_get(tag, "left-margin", &pixels, NULL);
        g_string_append_printf(code, "\\li%d", PIXELS_TO_TWIPS(pixels));
    }
    
    g_object_get(tag, "pixels-above-lines-set", &val, NULL);
    if(val)
    {
        g_object_get(tag, "pixels-above-lines", &pixels, NULL);
        g_string_append_printf(code, "\\sb%d", PIXELS_TO_TWIPS(pixels));
    }
    
    g_object_get(tag, "pixels-below-lines-set", &val, NULL);
    if(val)
    {
        g_object_get(tag, "pixels-below-lines", &pixels, NULL);
        g_string_append_printf(code, "\\sa%d", PIXELS_TO_TWIPS(pixels));
    }
    
    g_object_get(tag, "right-margin-set", &val, NULL);
    if(val)
    {
        g_object_get(tag, "right-margin", &pixels, NULL);
        g_string_append_printf(code, "\\ri%d", PIXELS_TO_TWIPS(pixels));
    }
    
    g_object_get(tag, "rise-set", &val, NULL);
    if(val)
    {
        g_object_get(tag, "rise", &pango, NULL);
        if(pango > 0)
            g_string_append_printf(code, "\\up%d", PANGO_TO_HALF_POINTS(pango));
        else if(pango < 0)
            g_string_append_printf(code, "\\dn%d", PANGO_TO_HALF_POINTS(-pango));
        else
            g_string_append(code, "\\up0\\dn0");
    }
    
    g_object_get(tag, "scale-set", &val, NULL);
    if(val)
    {
        g_object_get(tag, "scale", &factor, NULL);
        g_string_append_printf(code, "\\charscalex%d", (gint)(factor * 100));
    }
    
    g_object_get(tag, "size-set", &val, NULL);
    if(val)
    {
        g_object_get(tag, "size-points", &points, NULL);
        g_string_append_printf(code, "\\fs%d", (gint)(points * 2));
    }
        
    g_object_get(tag, "strikethrough-set", &val, NULL);
    if(val)
    {
        g_object_get(tag, "strikethrough", &val, NULL);
        if(val)
            g_string_append(code, "\\strike");
        else
            g_string_append(code, "\\strike0");
    }
    
    g_object_get(tag, "style-set", &val, NULL);
    if(val)
    {
        PangoStyle style;
        g_object_get(tag, "style", &style, NULL);
        switch(style)
        {
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
    if(val)
    {
        PangoTabArray *tabs;
        gboolean in_pixels;
        gint size, count;
        
        g_object_get(tag, "tabs", &tabs, NULL);
        in_pixels = pango_tab_array_get_positions_in_pixels(tabs);
        size = pango_tab_array_get_size(tabs);
        for(count = 0; count < size; count++)
        {
            gint location;
            /* alignment can only be LEFT in the current version of Pango */
            pango_tab_array_get_tab(tabs, count, NULL, &location); 
            g_string_append_printf(code, "\\tx%d", in_pixels? PIXELS_TO_TWIPS(location) : PANGO_TO_TWIPS(location));
        }
    }
    
    g_object_get(tag, "underline-set", &val, NULL);
    if(val)
    {
        PangoUnderline underline;
        g_object_get(tag, "underline", &underline, NULL);
        switch(underline)
        {
            case PANGO_UNDERLINE_NONE:
                g_string_append(code, "\\ul0\\ulnone");
                break;
            case PANGO_UNDERLINE_SINGLE:
            case PANGO_UNDERLINE_LOW:
                g_string_append(code, "\\ul");
                break;
            case PANGO_UNDERLINE_DOUBLE:
                g_string_append(code, "\\uldb");
                break;
            case PANGO_UNDERLINE_ERROR:
                g_string_append(code, "\\ulwave");
                break;
        }
    }
    
    g_object_get(tag, "variant-set", &val, NULL);
    if(val)
    {
        PangoVariant variant;
        g_object_get(tag, "variant", &variant, NULL);
        switch(variant)
        {
            case PANGO_VARIANT_NORMAL:
                g_string_append(code, "\\scaps0");
                break;
            case PANGO_VARIANT_SMALL_CAPS:
                g_string_append(code, "\\scaps");
                break;
        }
    }
    
    g_object_get(tag, "weight-set", &val, NULL);
    if(val)
    {
        gint weight;
        g_object_get(tag, "weight", &weight, NULL);
        if(weight >= PANGO_WEIGHT_BOLD)
            g_string_append(code, "\\b");
        else
            g_string_append(code, "\\b0");
    }
    
    g_hash_table_insert(ctx->tag_codes, tag, g_string_free(code, FALSE));
}

static void
dump(GtkTextTag *tag, const gchar *code)
{
    const gchar *name;
    g_object_get(tag, "name", &name, NULL);
    g_printerr("Tag name: '%s' - RTF code: '%s'\n", name, code);
}

static void 
analyze_buffer(WriterContext *ctx, GtkTextBuffer *textbuffer, const GtkTextIter *start, const GtkTextIter *end)
{
    GtkTextTagTable *tagtable = gtk_text_buffer_get_tag_table(textbuffer);
    gtk_text_tag_table_foreach(tagtable, (GtkTextTagTableForeach)convert_tag_to_code, ctx);
    
    g_hash_table_foreach(ctx->tag_codes, (GHFunc)dump, NULL);
}

guint8 *
rtf_serialize(GtkTextBuffer *register_buffer, GtkTextBuffer *content_buffer, const GtkTextIter *start, const GtkTextIter *end, gsize *length)
{
    WriterContext *ctx = writer_context_new();
    analyze_buffer(ctx, content_buffer, start, end);
/*	gchar *contents = write_rtf(ctx);
	*length = strlen(contents);*/
	writer_context_free(ctx);
/*	return (guint8 *)contents;*/
	return NULL;
}
