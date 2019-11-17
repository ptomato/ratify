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

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include "rtf.h"
#include "rtf-deserialize.h"
#include "rtf-ignore.h"

/* rtf-picture.c - All destinations dealing with inserting graphics into the
document: \pict, \shppict, \NeXTgraphic. */

typedef enum {
	PICT_TYPE_EMF,
	PICT_TYPE_PNG,
	PICT_TYPE_JPEG,
	PICT_TYPE_MAC,
	PICT_TYPE_OS2,
	PICT_TYPE_WMF,
	PICT_TYPE_DIB,
	PICT_TYPE_BMP
} PictType;

typedef struct {
	PictType type;
	gint type_param;
	GdkPixbufLoader *loader;
	gboolean error;
	
	glong width;
	glong height;
	glong width_goal;
	glong height_goal;
	gint xscale;
	gint yscale;
} PictState;

typedef struct {
    glong width;
    glong height;
} NeXTGraphicState;

#define PICT_STATE_INIT \
    state->type = PICT_TYPE_WMF; \
	state->type_param = 1; \
	state->xscale = state->yscale = 100; \
	state->width = state->height = state->width_goal = state->height_goal = -1;
DEFINE_STATE_FUNCTIONS_WITH_INIT(PictState, pict, PICT_STATE_INIT)
#define NEXTGRAPHIC_STATE_INIT state->width = state->height = -1;
DEFINE_STATE_FUNCTIONS_WITH_INIT(NeXTGraphicState, nextgraphic, NEXTGRAPHIC_STATE_INIT)

/* Forward declarations */
static void pict_text(ParserContext *ctx);
static void pict_end(ParserContext *ctx);
static void nextgraphic_text(ParserContext *ctx);
static void nextgraphic_end(ParserContext *ctx);
static gint nextgraphic_get_codepage(ParserContext *ctx);

typedef gboolean PictFunc(ParserContext *, PictState *, GError **);
typedef gboolean PictParamFunc(ParserContext *, PictState *, gint32, GError **);

static PictFunc pic_emfblip, pic_jpegblip, pic_macpict, pic_pngblip;
static PictParamFunc pic_dibitmap, pic_pich, pic_pichgoal, pic_picscalex,
                     pic_picscaley, pic_picw, pic_picwgoal, pic_pmmetafile,
                     pic_wbitmap, pic_wmetafile;

typedef gboolean NeXTGraphicParamFunc(ParserContext *, NeXTGraphicState *, gint32, GError **);

static NeXTGraphicParamFunc ng_height, ng_width;

const ControlWord pict_word_table[] = {
	{ "dibitmap", REQUIRED_PARAMETER, FALSE, pic_dibitmap },
	{ "emfblip", NO_PARAMETER, FALSE, pic_emfblip },
	{ "jpegblip", NO_PARAMETER, FALSE, pic_jpegblip },
	{ "macpict", NO_PARAMETER, FALSE, pic_macpict },
	{ "pich", REQUIRED_PARAMETER, FALSE, pic_pich },
	{ "pichgoal", REQUIRED_PARAMETER, FALSE, pic_pichgoal },
	{ "picscalex", OPTIONAL_PARAMETER, FALSE, pic_picscalex, 100 },
	{ "picscaley", OPTIONAL_PARAMETER, FALSE, pic_picscaley, 100 },
	{ "picw", REQUIRED_PARAMETER, FALSE, pic_picw },
	{ "picwgoal", REQUIRED_PARAMETER, FALSE, pic_picwgoal },
	{ "pmmetafile", REQUIRED_PARAMETER, FALSE, pic_pmmetafile },
	{ "pngblip", NO_PARAMETER, FALSE, pic_pngblip },
	{ "wbitmap", REQUIRED_PARAMETER, FALSE, pic_wbitmap },
	{ "wmetafile", OPTIONAL_PARAMETER, FALSE, pic_wmetafile, 1 },
	{ NULL }
};

const DestinationInfo pict_destination = {
	pict_word_table,
	pict_text,
	pict_state_new,
	pict_state_copy,
	pict_state_free,
	pict_end
};

const ControlWord nextgraphic_word_table[] = {
    { "height", REQUIRED_PARAMETER, FALSE, ng_height },
    { "width", REQUIRED_PARAMETER, FALSE, ng_width },
    { NULL }
};

const DestinationInfo nextgraphic_destination = {
    nextgraphic_word_table,
    nextgraphic_text,
    nextgraphic_state_new,
    nextgraphic_state_copy,
    nextgraphic_state_free,
    nextgraphic_end,
    nextgraphic_get_codepage
};

const ControlWord shppict_word_table[] = {
	{ "pict", DESTINATION, FALSE, NULL, 0, NULL, &pict_destination },
	{ NULL }
};

const DestinationInfo shppict_destination = {
    shppict_word_table,
    ignore_pending_text,
    ignore_state_new,
	ignore_state_copy,
	ignore_state_free
};

/* Insert picture into text buffer at current insertion mark */
static void
insert_picture_into_textbuffer(ParserContext *ctx, GdkPixbuf *pixbuf)
{
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_mark(ctx->textbuffer, &iter, ctx->endmark);
    gtk_text_buffer_insert_pixbuf(ctx->textbuffer, &iter, pixbuf);
}

/* Send a message to the GdkPixbufLoader to change its expected size after
parsing a width or height declaration */
static void
adjust_loader_size(PictState *state)
{
	if(state->loader && (state->width != -1 || state->width_goal != -1) && (state->height != -1 || state->height_goal != -1))
		gdk_pixbuf_loader_set_size(state->loader, 
		                           (state->width_goal != -1)? state->width_goal : state->width, 
		                           (state->height_goal != -1)? state->height_goal : state->height);
}

/* The "text" in a \pict destination is the picture, expressed as a long string
of hexadecimal digits */
static void
pict_text(ParserContext *ctx)
{
	GError *error = NULL;
	PictState *state = get_state(ctx);
	guchar *writebuffer;
	gint count;
	const char *mimetypes[] = {
		"image/x-emf", "image/png", "image/jpeg", "image/x-pict", 
		"OS/2 Presentation Manager", "image/x-wmf", "image/x-bmp", "image-x-bmp"
	}; /* "OS/2 Presentation Manager" isn't supported */

	if(state->error)
		return;

	if(strlen(ctx->text->str) == 0)
		return;
	
	/* If no GdkPixbufLoader has been initialized yet, then do that */
	if(!state->loader)
	{
		GSList *formats = gdk_pixbuf_get_formats();
		GSList *iter;
		gchar **mimes;
		
		/* Make sure the MIME type we want to load is present in the list of
		formats compiled into our GdkPixbuf library */
	 	for(iter = formats; iter && !state->loader; iter = g_slist_next(iter)) 
		{
			int i;
			mimes = gdk_pixbuf_format_get_mime_types(iter->data);
			
		 	for(i = 0; mimes[i] != NULL; i++)
		 	if(g_ascii_strcasecmp(mimes[i], mimetypes[state->type]) == 0) 
			{
			 	state->loader = gdk_pixbuf_loader_new_with_mime_type(mimetypes[state->type], &error);
				if(!state->loader)
				{
					g_warning(_("Error loading picture of MIME type '%s': %s"), mimetypes[state->type], error->message);
					state->error = TRUE;
				}
			 	break;
		 	}
	 	}
	 	if(!state->loader && !state->error)
		{
			g_warning(_("Module for loading MIME type '%s' not found"), mimetypes[state->type]);
			state->error = TRUE;
		}
		
	 	g_slist_free(formats); 

		if(state->error)
			return;

		adjust_loader_size(state);
	}
	
	/* Convert the "text" into binary data */
	writebuffer = g_new0(guchar, strlen(ctx->text->str));
	for(count = 0; ctx->text->str[count * 2] && ctx->text->str[count * 2 + 1]; count++)
	{
		gchar buf[3];
		guchar byte;

		memcpy(buf, ctx->text->str + (count * 2), 2);
		buf[2] = '\0';
		errno = 0;
		byte = (guchar)strtol(buf, NULL, 16);	
		if(errno)
		{
			g_warning(_("Error in \\pict data: '%s'"), buf);
			state->error = TRUE;
			g_free(writebuffer);
			return;
		}
		writebuffer[count] = byte;
	}
    /* Write the "text" into the GdkPixbufLoader */
	if(!gdk_pixbuf_loader_write(state->loader, writebuffer, count, &error))
	{
		g_warning(_("Error reading \\pict data: %s"), error->message);
		state->error = TRUE;
	}

	g_free(writebuffer);
	g_string_truncate(ctx->text, 0);
}

/* When the destination is closed, then there is no more picture data, so close
the GdkPixbufLoader and load the picture */
static void
pict_end(ParserContext *ctx)
{
	GError *error = NULL;
	PictState *state = get_state(ctx);

	if(!state->error)
	{
		if(state->loader && !gdk_pixbuf_loader_close(state->loader, &error))
			g_warning(_("Error closing pixbuf loader: %s"), error->message);
		GdkPixbuf *picture = gdk_pixbuf_loader_get_pixbuf(state->loader);
		if(!picture)
			g_warning(_("Error loading picture"));
		else
		{
			/* Scale picture if needed */
			if(state->xscale != 100 || state->yscale != 100)
			{
				int newwidth = gdk_pixbuf_get_width(picture) * state->xscale / 100;
				int newheight = gdk_pixbuf_get_height(picture) * state->yscale / 100;
				GdkPixbuf *newpicture = gdk_pixbuf_scale_simple(picture, newwidth, newheight, GDK_INTERP_BILINEAR);
				g_object_unref(picture);
				picture = newpicture;
			}
			insert_picture_into_textbuffer(ctx, picture);
		}
	}
}

static gboolean
pic_dibitmap(ParserContext *ctx, PictState *state, gint32 param, GError **error)
{
	if(param != 0)
	{
		g_set_error(error, RTF_ERROR, RTF_ERROR_BAD_PICT_TYPE, _("Invalid bitmap type '%i' for \\dibitmap"), param);
		return FALSE;
	}
	state->type = PICT_TYPE_DIB;
	state->type_param = 0;
	return TRUE;
}

static gboolean
pic_emfblip(ParserContext *ctx, PictState *state, GError **error)
{
	state->type = PICT_TYPE_EMF;
	return TRUE;
}

static gboolean
pic_jpegblip(ParserContext *ctx, PictState *state, GError **error)
{
	state->type = PICT_TYPE_JPEG;
	return TRUE;
}

static gboolean
pic_macpict(ParserContext *ctx, PictState *state, GError **error)
{
	state->type = PICT_TYPE_MAC;
	return TRUE;
}

static gboolean
pic_pich(ParserContext *ctx, PictState *state, gint32 pixels, GError **error)
{
	state->height = pixels;
	adjust_loader_size(state);
	return TRUE;
}

static gboolean
pic_pichgoal(ParserContext *ctx, PictState *state, gint32 twips, GError **error)
{
	state->height_goal = PANGO_PIXELS(TWIPS_TO_PANGO(twips));
	adjust_loader_size(state);
	return TRUE;
}

static gboolean
pic_picscalex(ParserContext *ctx, PictState *state, gint32 percent, GError **error)
{
	state->xscale = percent;
	return TRUE;
}

static gboolean
pic_picscaley(ParserContext *ctx, PictState *state, gint32 percent, GError **error)
{
	state->yscale = percent;
	return TRUE;
}

static gboolean
pic_picw(ParserContext *ctx, PictState *state, gint32 pixels, GError **error)
{
	state->width = pixels;
	adjust_loader_size(state);
	return TRUE;
}

static gboolean
pic_picwgoal(ParserContext *ctx, PictState *state, gint32 twips, GError **error)
{
	state->width_goal = PANGO_PIXELS(TWIPS_TO_PANGO(twips));
	adjust_loader_size(state);
	return TRUE;
}

static gboolean
pic_pmmetafile(ParserContext *ctx, PictState *state, gint32 param, GError **error)
{
	state->type = PICT_TYPE_OS2;
	state->type_param = param;
	return TRUE;
}

static gboolean
pic_pngblip(ParserContext *ctx, PictState *state, GError **error)
{
	state->type = PICT_TYPE_PNG;
	return TRUE;
}

static gboolean
pic_wbitmap(ParserContext *ctx, PictState *state, gint32 param, GError **error)
{
	if(param != 0)
	{
		g_set_error(error, RTF_ERROR, RTF_ERROR_BAD_PICT_TYPE, _("Invalid bitmap type '%i' for \\wbitmap"), param);
		return FALSE;
	}
	state->type = PICT_TYPE_BMP;
	state->type_param = 0;
	return TRUE;
}

static gboolean
pic_wmetafile(ParserContext *ctx, PictState *state, gint32 param, GError **error)
{
	state->type = PICT_TYPE_WMF;
	state->type_param = param;
	return TRUE;
}

/* Ignore text, but leave it in the pending text buffer, because we use it in
nextgraphic_end() */
static void
nextgraphic_text(ParserContext *ctx)
{
}

/* Load the file from the filename in the pending text buffer */
static void
nextgraphic_end(ParserContext *ctx)
{
    GdkPixbuf *pixbuf;
    gchar *filename;
    NeXTGraphicState *state = get_state(ctx);
    GError *error = NULL;
    
    filename = g_strstrip(g_strdup(ctx->text->str));
    g_string_truncate(ctx->text, 0);
    pixbuf = gdk_pixbuf_new_from_file_at_scale(filename, state->width, state->height, FALSE /* preserve aspect ratio */, &error);
    if(!pixbuf)
    {
        g_warning(_("Error loading picture from file '%s': %s"), filename, error->message);
        return;
    }
    g_free(filename);
    
    insert_picture_into_textbuffer(ctx, pixbuf);
}

static gint
nextgraphic_get_codepage(ParserContext *ctx)
{
    return 65001; /* UTF-8 */
}

static gboolean
ng_height(ParserContext *ctx, NeXTGraphicState *state, gint32 twips, GError **error)
{
    state->height = PANGO_PIXELS(TWIPS_TO_PANGO(twips));
    return TRUE;
}

static gboolean
ng_width(ParserContext *ctx, NeXTGraphicState *state, gint32 twips, GError **error)
{
    state->width = PANGO_PIXELS(TWIPS_TO_PANGO(twips));
    return TRUE;
}