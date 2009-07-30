#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <glib.h>
#include <config.h>
#include <glib/gi18n-lib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include <osxcart/rtf.h>
#include "rtf-deserialize.h"
#include "rtf-ignore.h"

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

static void pict_text(ParserContext *ctx);
static PictState *pict_state_new(void);
static PictState *pict_state_copy(PictState *state);
static void pict_state_free(PictState *state);
static void pict_end(ParserContext *ctx);

typedef gboolean PictFunc(ParserContext *, PictState *, GError **);
typedef gboolean PictParamFunc(ParserContext *, PictState *, gint32, GError **);

static PictFunc pic_emfblip, pic_jpegblip, pic_macpict, pic_pngblip;
static PictParamFunc pic_dibitmap, pic_pich, pic_pichgoal, pic_picscalex,
                     pic_picscaley, pic_picw, 
                     pic_picwgoal, pic_pmmetafile, pic_wbitmap, pic_wmetafile;

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
	(StateNewFunc *)pict_state_new,
	(StateCopyFunc *)pict_state_copy,
	(StateFreeFunc *)pict_state_free,
	pict_end
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

static void
adjust_loader_size(PictState *state)
{
	if(state->loader && (state->width != -1 || state->width_goal != -1) && (state->height != -1 || state->height_goal != -1))
		gdk_pixbuf_loader_set_size(state->loader, 
		                           (state->width_goal != -1)? state->width_goal : state->width, 
		                           (state->height_goal != -1)? state->height_goal : state->height);
}

static void
pict_text(ParserContext *ctx)
{
	GError *error = NULL;
	PictState *state = (PictState *)get_state(ctx);
	guchar *writebuffer;
	gint count;
	const char *mimetypes[] = {
		"image/x-emf", "image/png", "image/jpeg", "image/x-pict", 
		"not supported", "image/x-wmf", "image/x-bmp", "image-x-bmp"
	};

	if(state->error)
		return;

	if(strlen(ctx->text->str) == 0)
		return;
	
	if(!state->loader)
	{
		GSList *formats = gdk_pixbuf_get_formats();
		GSList *iter;
	 	GdkPixbufFormat *info;
		gchar **mimes;
		
	 	for(iter = formats; iter && !state->loader; iter = g_slist_next(iter)) 
		{
			int i;
			info = (GdkPixbufFormat *)iter->data;
			mimes = gdk_pixbuf_format_get_mime_types(info);
			
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
	
	if(!gdk_pixbuf_loader_write(state->loader, writebuffer, count, &error))
	{
		g_warning(_("Error reading \\pict data: %s"), error->message);
		state->error = TRUE;
	}

	g_free(writebuffer);
}

static PictState *
pict_state_new(void)
{
    PictState *retval = g_new0(PictState, 1);
	retval->type = PICT_TYPE_WMF;
	retval->type_param = 1;
	retval->width = -1;
	retval->height = -1;
	retval->width_goal = -1;
	retval->height_goal = -1;
	retval->xscale = 100;
	retval->yscale = 100;
	return retval;
}

static PictState *
pict_state_copy(PictState *state)
{
    PictState *retval = pict_state_new();
    *retval = *state;
    return retval;
}

static void
pict_state_free(PictState *state)
{
	g_free(state);
}

static void
pict_end(ParserContext *ctx)
{
	GError *error = NULL;
	PictState *state = (PictState *)get_state(ctx);

	if(!state->error)
	{
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
			/* Insert picture into text buffer */
			GtkTextIter iter;
			gtk_text_buffer_get_iter_at_mark(ctx->textbuffer, &iter, ctx->endmark);
			gtk_text_buffer_insert_pixbuf(ctx->textbuffer, &iter, picture);
		}
	}
	if(state->loader && !gdk_pixbuf_loader_close(state->loader, &error))
		g_warning(_("Error closing pixbuf loader: %s"), error->message);
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
	state->type = PICT_TYPE_PNG;
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
