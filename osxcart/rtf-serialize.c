#include <string.h>
#include <time.h>
#include <glib.h>
#include <config.h>
#include <glib/gi18n-lib.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include "config.h"
#include "rtf-langcode.h"

/* Change this later FIXME */
#define PIXELS_TO_TWIPS(pixels) (pixels * 20)
#define PANGO_TO_HALF_POINTS(pango) (2 * pango / PANGO_SCALE)
#define PANGO_TO_TWIPS(pango) (20 * pango / PANGO_SCALE)

struct _WriterContext {
    GtkTextBuffer *textbuffer;
    const GtkTextIter *start, *end;
    
    /* Tag information */
    GHashTable *tag_codes;
    GList *font_table;
    GList *color_table;
};
typedef struct _WriterContext WriterContext;

static WriterContext *
writer_context_new(void)
{
    WriterContext *ctx = g_slice_new0(WriterContext);
    ctx->tag_codes = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_free);
    ctx->font_table = NULL;
    ctx->color_table = g_list_prepend(NULL, g_strdup("")); /* Color 0 always black */
    return ctx;
}

static void 
writer_context_free(WriterContext *ctx)
{
    g_hash_table_unref(ctx->tag_codes);
    g_list_foreach(ctx->color_table, (GFunc)g_free, NULL);
    g_list_free(ctx->color_table);
    g_slice_free(WriterContext, ctx);
}

static gint
get_color_from_gdk_color(GdkColor *color, WriterContext *ctx)
{
    GList *link;
    gint colornum;
    gchar *colorcode;
    
    if(color->red == 0 && color->green == 0 && color->blue == 0)
	    return 0; /* Color 0 always black in this implementation */
    
    colorcode = g_strdup_printf("\\red%d\\green%d\\blue%d", color->red >> 8, color->green >> 8, color->blue >> 8);
    if(!(link = g_list_find_custom(ctx->color_table, colorcode, (GCompareFunc)strcmp)))
    {
        colornum = g_list_length(ctx->color_table);
        ctx->color_table = g_list_append(ctx->color_table, g_strdup(colorcode));   
    }
    else
        colornum = g_list_position(ctx->color_table, link);
    g_free(colorcode);
    
    g_assert(colornum > 0 && colornum < 256);
    return colornum;
}

static void 
convert_tag_to_code(GtkTextTag *tag, WriterContext *ctx)
{
    gboolean val;
    gint pixels, pango, colornum;
    gdouble factor, points;
    GdkColor *color;
	const gchar *name;
    GString *code;
    
    /* First check if this is a osxcart named tag that doesn't have a direct 
	 Pango attributes equivalent, such as superscript or subscript. */
	g_object_get(tag, "name", &name, NULL);
	if(strcmp(name, "osxcart-rtf-superscript") == 0)
	{
		g_hash_table_insert(ctx->tag_codes, tag, g_strdup("\\super"));
		return;
	}
	else if(strcmp(name, "osxcart-rtf-subscript") == 0)
	{
		g_hash_table_insert(ctx->tag_codes, tag, g_strdup("\\sub"));
		return;
	}

	/* Otherwise, read the attributes one by one and add RTF code for them */
	code = g_string_new("");
	
	g_object_get(tag, "background-set", &val, NULL);
	if(val)
	{
	    g_object_get(tag, "background-gdk", &color, NULL);
	    colornum = get_color_from_gdk_color(color, ctx);
	    g_string_append_printf(code, "\\chshdng0\\chcbpat%d\\cb%d", colornum, colornum);
	}
	
	g_object_get(tag, "family-set", &val, NULL);
	if(val)
	{
	    const gchar *family;
	    GList *link;
	    gint fontnum;
	    g_object_get(tag, "family", &family, NULL);
	    if(!(link = g_list_find_custom(ctx->font_table, family, (GCompareFunc)strcmp)))
        {
            fontnum = g_list_length(ctx->font_table);
            ctx->font_table = g_list_append(ctx->font_table, g_strdup(family));   
        }
        else
            fontnum = g_list_position(ctx->font_table, link);
        g_string_append_printf(code, "\\f%d", fontnum);
    }
	
	g_object_get(tag, "foreground-set", &val, NULL);
	if(val)
	{
	    g_object_get(tag, "foreground-gdk", &color, NULL);
	    colornum = get_color_from_gdk_color(color, ctx);
	    g_string_append_printf(code, "\\cf%d", colornum);
	}
	
    g_object_get(tag, "indent-set", &val, NULL);
    if(val)
    {
        g_object_get(tag, "indent", &pixels, NULL);
        g_string_append_printf(code, "\\fi%d", PIXELS_TO_TWIPS(pixels));
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

	g_object_get(tag, "language-set", &val, NULL);
	if(val)
	{
		gchar *isocode;
		g_object_get(tag, "language", &isocode, NULL);
		g_string_append_printf(code, "\\lang%d", language_to_wincode(isocode));
		g_free(isocode);
	}
	
    g_object_get(tag, "left-margin-set", &val, NULL);
    if(val)
    {
        g_object_get(tag, "left-margin", &pixels, NULL);
        g_string_append_printf(code, "\\li%d", PIXELS_TO_TWIPS(pixels));
    }
    
    g_object_get(tag, "paragraph-background-set", &val, NULL);
	if(val)
	{
	    g_object_get(tag, "paragraph-background-gdk", &color, NULL);
	    colornum = get_color_from_gdk_color(color, ctx);
	    g_string_append_printf(code, "\\highlight%d", colornum);
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
    
    g_object_get(tag, "pixels-inside-wrap-set", &val, NULL);
    if(val)
    {
        g_object_get(tag, "pixels-inside-wrap", &pixels, NULL);
        g_string_append_printf(code, "\\slleading%d", PIXELS_TO_TWIPS(pixels));
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
analyze_buffer(WriterContext *ctx, GtkTextBuffer *textbuffer, const GtkTextIter *start, const GtkTextIter *end)
{
    GtkTextTagTable *tagtable = gtk_text_buffer_get_tag_table(textbuffer);
    gtk_text_tag_table_foreach(tagtable, (GtkTextTagTableForeach)convert_tag_to_code, ctx);
    
    ctx->textbuffer = textbuffer;
    ctx->start = start;
    ctx->end = end;
}

static void
write_rtf_text(GString *output, const gchar *text)
{
    const gchar *ptr;
    for(ptr = text; *ptr; ptr = g_utf8_next_char(ptr))
    {
        gunichar ch = g_utf8_get_char(ptr);
        if(ch == 0x09)
            g_string_append(output, "\\tab ");
        else if(ch == '\n') /* whatever value that is */
            g_string_append(output, "\\par ");
        else if(ch == 0x5C)
            g_string_append(output, "\\\\");
        else if(ch == 0x7B)
            g_string_append(output, "\\{");
        else if(ch == 0x7D)
            g_string_append(output, "\\}");
        else if(ch > 0 && ch < 0x80)
            g_string_append_c(output, (gchar)ch);
        else if(ch == 0xA0)
            g_string_append(output, "\\~");
        else if(ch == 0xAD)
            g_string_append(output, "\\-");
        else if(ch >= 0xA1 && ch <= 0xFF)
            g_string_append_printf(output, "\\'%2X", ch);
        else if(ch == 0x2002)
            g_string_append(output, "\\enspace ");
        else if(ch == 0x2003)
            g_string_append(output, "\\emspace ");
        else if(ch == 0x2005)
            g_string_append(output, "\\qmspace ");
        else if(ch == 0x200B)
            g_string_append(output, "\\zwbo ");
        else if(ch == 0x200C)
            g_string_append(output, "\\zwnj ");
        else if(ch == 0x200D)
            g_string_append(output, "\\zwj ");
        else if(ch == 0x200E)
            g_string_append(output, "\\ltrmark ");
        else if(ch == 0x200F)
            g_string_append(output, "\\rtlmark ");
        else if(ch == 0x2011)
            g_string_append(output, "\\_");
        else if(ch == 0x2013)
            g_string_append(output, "\\endash ");
        else if(ch == 0x2014)
            g_string_append(output, "\\emdash ");
        else if(ch == 0x2018)
            g_string_append(output, "\\lquote ");
        else if(ch == 0x2019)
            g_string_append(output, "\\rquote ");
        else if(ch == 0x201C)
            g_string_append(output, "\\ldblquote ");
        else if(ch == 0x201D)
            g_string_append(output, "\\rdblquote ");
        else if(ch == 0x2022)
            g_string_append(output, "\\bullet ");
        else if(ch == 0x2028)
            g_string_append(output, "\\line ");
        else
            g_string_append_printf(output, "\\u%d ", ch);
    }
}

/* Analyze a segment of text in which there are no tag flips, but possibly embedded pictures */
static void
write_rtf_text_and_pictures(WriterContext *ctx, const GtkTextIter *start, const GtkTextIter *end, GString *output)
{
    GtkTextIter iter;
    GdkPixbuf *pixbuf = NULL;
    gchar *text, *pngbuffer;
    gsize bufsize;
    GError *error = NULL;

    for(iter = *start; !gtk_text_iter_equal(&iter, end); gtk_text_iter_forward_char(&iter))
    {
        if((pixbuf = gtk_text_iter_get_pixbuf(&iter)))
            break;
    }
    
    if(!pixbuf)
    {
        text = gtk_text_buffer_get_text(ctx->textbuffer, start, end, TRUE);
        write_rtf_text(output, text);
        g_free(text);
        return;
    }

    /* Write the text before the pixbuf, insert a \pict destination into the document, and recurse on the text after */
    text = gtk_text_buffer_get_text(ctx->textbuffer, start, &iter, TRUE);
    write_rtf_text(output, text);
    g_free(text);
    
    if(gdk_pixbuf_save_to_buffer(pixbuf, &pngbuffer, &bufsize, "png", &error, "compression", "9", NULL))
    {
        int count;
        g_string_append_printf(output, "{\\pict\\pngblip\\picw%d\\pich%d", gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf));
        for(count = 0; count < bufsize; count++)
        {
            if(count % 40 == 0)
                g_string_append_c(output, '\n');
            g_string_append_printf(output, "%02X", (unsigned char)pngbuffer[count]);
        }
        g_string_append(output, "\n}");
        g_free(pngbuffer);
    }
    else
        g_warning(_("Could not serialize picture, skipping: %s"), error->message);
    
    gtk_text_iter_forward_char(&iter);
    write_rtf_text_and_pictures(ctx, &iter, end, output);
}
        
static void
write_rtf_paragraphs(WriterContext *ctx, GString *output)
{
    GtkTextIter start, end, linestart, lineend;
    GSList *taglist, *iter;

    linestart = *(ctx->start);
    lineend = linestart;
    
    while(gtk_text_iter_in_range(&lineend, ctx->start, ctx->end))
    {
        /* Begin the paragraph by resetting the paragraph properties */
        g_string_append(output, "{\\pard ");
        
        /* Get the next paragraph of text */
        gtk_text_iter_forward_to_line_end(&lineend);
        /* Skip to the end of any clump of paragraph separators */
        while(gtk_text_iter_ends_line(&lineend) && !gtk_text_iter_is_end(&lineend))
            gtk_text_iter_forward_char(&lineend);
        if(gtk_text_iter_compare(&lineend, ctx->end) > 0)
            lineend = *(ctx->end);
        taglist = gtk_text_iter_get_tags(&linestart);
        for(iter = taglist; iter; iter = g_slist_next(iter))
            g_string_append(output, g_hash_table_lookup(ctx->tag_codes, iter->data));
        if(taglist)
            g_string_append_c(output, ' ');
        
        start = linestart;
        end = start;
        while(gtk_text_iter_in_range(&end, &linestart, &lineend))
        {
            gtk_text_iter_forward_to_tag_toggle(&end, NULL);
            if(gtk_text_iter_compare(&end, &lineend) > 0)
                end = lineend;
            
            gchar *text = gtk_text_buffer_get_text(ctx->textbuffer, &start, &end, TRUE);
            write_rtf_text_and_pictures(ctx, &start, &end, output);
            g_free(text);
            start = end;
            
            if(gtk_text_iter_ends_tag(&start, NULL))
            {
                g_string_append(output, "}{");
                taglist = gtk_text_iter_get_tags(&start);
            }
            else
                taglist = gtk_text_iter_get_toggled_tags(&start, TRUE);
            
            for(iter = taglist; iter; iter = g_slist_next(iter))
                g_string_append(output, g_hash_table_lookup(ctx->tag_codes, iter->data));
            if(taglist)
                g_string_append_c(output, ' ');
        }
        
        linestart = lineend;
        g_string_append(output, "}\n");   
    }
}

static void
write_color_table_entry(const gchar *colorcode, GString *output)
{
    g_string_append_printf(output, "%s;\n", colorcode);
}

static gchar *
write_rtf(WriterContext *ctx)
{
    /* Header */
    GString *output = g_string_new("{\\rtf1\\ansi\\deff0\\uc0\n{\\fonttbl\n");
    GList *iter;
    int count;
    
    /* Font table */
    for(count = 0, iter = ctx->font_table; iter; iter = g_list_next(iter), count++)
    {
        gchar **fontnames = g_strsplit(iter->data, ",", 2);
        g_string_append_printf(output, "{\\f%d\\fnil %s;}\n", count, fontnames[0]);
        g_strfreev(fontnames);
    }
    
    /* Color table */
    g_string_append(output, "}\n{\\colortbl\n");
    g_list_foreach(ctx->color_table, (GFunc)write_color_table_entry, output);
    
    /* Metadata (provide default values because Word will overwrite if missing) */
    g_string_append(output, "}\n{\\info\n{\\author .}\n{\\company .}\n{\\title .}\n");
    gchar buffer[29];
    time_t timer = time(NULL);
    if(strftime(buffer, 29, "\\yr%Y\\mo%m\\dy%d\\hr%H\\min%M", localtime(&timer)))
        g_string_append_printf(output, "{\\creatim%s}\n", buffer);
    g_string_append_printf(output, "{\\doccomm\nCreated by %s %s on %s}\n}\n", PACKAGE_NAME, PACKAGE_VERSION, ctime(&timer));
    g_string_append_printf(output, "{\\*\\generator %s %s}\n", PACKAGE_NAME, PACKAGE_VERSION);
    
    /* Preliminary formatting */
    g_string_append_printf(output, "\\deflang%d", language_to_wincode(pango_language_to_string(pango_language_get_default())));
    g_string_append(output, "\\plain\\widowctrl\\hyphauto\n");
    
    /* Document body */
    write_rtf_paragraphs(ctx, output);
    
    g_string_append_c(output, '}');
    return g_string_free(output, FALSE);
}

guint8 *
rtf_serialize(GtkTextBuffer *register_buffer, GtkTextBuffer *content_buffer, const GtkTextIter *start, const GtkTextIter *end, gsize *length)
{
    WriterContext *ctx = writer_context_new();
    analyze_buffer(ctx, content_buffer, start, end);
	gchar *contents = write_rtf(ctx);
	*length = strlen(contents);
	writer_context_free(ctx);
	return (guint8 *)contents;
}
