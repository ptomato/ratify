#include <string.h>
#include <glib.h>
#include <config.h>
#include <glib/gi18n-lib.h>
#include "rtf-deserialize.h"

typedef enum {
	FONT_FAMILY_NIL,
	FONT_FAMILY_ROMAN,
	FONT_FAMILY_SWISS,
	FONT_FAMILY_MODERN,
	FONT_FAMILY_SCRIPT,
	FONT_FAMILY_DECORATIVE,
	FONT_FAMILY_TECH,
	FONT_FAMILY_BIDI
} FontFamily;

typedef struct {
    gint index;
	gint codepage;
	FontFamily family;
	gchar *name;
} FontTableState;

static void font_table_text(ParserContext *ctx);
static FontTableState *fonttbl_state_new(void);
static FontTableState *fonttbl_state_copy(FontTableState *state);
static void fonttbl_state_free(FontTableState *state);
static gint font_table_get_codepage(ParserContext *ctx);

typedef gboolean FontFunc(ParserContext *, FontTableState *, GError **);
typedef gboolean FontParamFunc(ParserContext *, FontTableState *, gint32, GError **);

/* Font table destination functions */
static FontFunc ft_fbidi, ft_fdecor, ft_fmodern, ft_fnil, ft_froman, ft_fscript, 
                ft_fswiss, ft_ftech;
static FontParamFunc ft_f, ft_fcharset;
extern const DestinationInfo ignore_destination;

const ControlWord fonttbl_word_table[] = {
	{ "f", REQUIRED_PARAMETER, TRUE, ft_f },
	{ "fbidi", NO_PARAMETER, TRUE, ft_fbidi },
	{ "fcharset", REQUIRED_PARAMETER, TRUE, ft_fcharset },
	{ "fdecor", NO_PARAMETER, TRUE, ft_fdecor },
	{ "fmodern", NO_PARAMETER, TRUE, ft_fmodern },
	{ "fnil", NO_PARAMETER, TRUE, ft_fnil },
	{ "froman", NO_PARAMETER, TRUE, ft_froman },
	{ "fscript", NO_PARAMETER, TRUE, ft_fscript },
	{ "fswiss", NO_PARAMETER, TRUE, ft_fswiss },
	{ "ftech", NO_PARAMETER, TRUE, ft_ftech },
	{ NULL }
};

const DestinationInfo fonttbl_destination = {
    fonttbl_word_table,
    font_table_text,
    (StateNewFunc *)fonttbl_state_new,
    (StateCopyFunc *)fonttbl_state_copy,
    (StateFreeFunc *)fonttbl_state_free,
	NULL, /* cleanup */
	font_table_get_codepage
};

static void
font_table_text(ParserContext *ctx)
{
	gchar *name, *semicolon;
	FontProperties *fontprop;
	FontTableState *state = (FontTableState *)get_state(ctx);
	static gchar *font_suggestions[] = { /* From RTF spec */
		"", 
		"Serif,Times New Roman,Palatino",
		"Sans,Arial,Helvetica",
		"Monospace,Courier New,Pica",
		"Cursive",
		"Old English,Zapf Chancery",
		"Symbol,Wingdings",
		"Miriam"
	};
	
	name = g_strdup(ctx->text->str);
	semicolon = strchr(name, ';');
	if(!semicolon)
	{
		gchar *newname = g_strconcat(state->name, name, NULL);
		g_free(state->name);
		state->name = newname;
		g_string_truncate(ctx->text, 0);
		return;
	}
	g_string_assign(ctx->text, semicolon + 1); /* Leave the text after the semicolon in the buffer */
	*semicolon = '\0';
	
	fontprop = g_new0(FontProperties, 1);
	fontprop->index = state->index;
	fontprop->codepage = state->codepage;
	fontprop->font_name = g_strconcat(state->name, name, NULL);
	ctx->font_table = g_slist_prepend(ctx->font_table, fontprop);
	
	gchar *tagname = g_strdup_printf("osxcart-rtf-font-%i", state->index);
	GtkTextTag *tag;
    if((tag = gtk_text_tag_table_lookup(ctx->tags, tagname)))
        gtk_text_tag_table_remove(ctx->tags, tag);
        
    gchar *fontstring = NULL;
    tag = gtk_text_tag_new(tagname);

    if(fontprop->font_name && state->family != FONT_FAMILY_NIL)
        fontstring = g_strconcat(fontprop->font_name, 
                                 ",", 
                                 font_suggestions[state->family], 
                                 NULL);
    else if(fontprop->font_name)
        fontstring = g_strdup(fontprop->font_name);
    else if(state->family != FONT_FAMILY_NIL)
        fontstring = g_strdup(font_suggestions[state->family]);

    if(fontstring)
    {
        g_object_set(tag, 
                     "family", fontstring, 
                     "family-set", TRUE,
                     NULL);
        g_free(fontstring);
    }
    gtk_text_tag_table_add(ctx->tags, tag);
	g_free(tagname);

	g_free(state->name);
	state->index = 0;
	state->family = FONT_FAMILY_NIL;
	state->codepage = -1;
	state->name = g_strdup("");
}

static FontTableState *
fonttbl_state_new(void)
{
    FontTableState *retval = g_new0(FontTableState, 1);
	retval->codepage = -1;
	retval->name = g_strdup("");
	return retval;
}

static FontTableState *
fonttbl_state_copy(FontTableState *state)
{
    FontTableState *retval = fonttbl_state_new();
    *retval = *state;
	retval->name = g_strdup(state->name);
    return retval;
}

static void
fonttbl_state_free(FontTableState *state)
{
	g_free(state->name);
	g_free(state);
}

/*
 * Parses the charset
 */
static gint
fcharset_to_codepage(gint charset)
{
    switch(charset)
    {
        case 0:   return 1252;  /* "ANSI" */
        case 1:   return -1;    /* default */
		case 2:   return -1;    /* Symbol; only works in Symbol font of course */
		case 77:  return 10000; /* Mac Roman */
		case 78:  return 10001; /* Mac Shift JIS */
		case 79:  return 10003; /* Mac Hangul */
		case 80:  return 10008; /* Mac GB2312 */
		case 81:  return 10002; /* Mac Big5 */
		case 83:  return 10005; /* Mac Hebrew */
		case 84:  return 10004; /* Mac Arabic */
		case 85:  return 10006; /* Mac Greek */
		case 86:  return 10081; /* Mac Turkish */
		case 87:  return 10021; /* Mac Thai */
		case 88:  return 10029; /* Mac East Europe */
		case 89:  return 10007; /* Mac Cyrillic */
        case 128: return 943;   /* ShiftJIS */
		case 129: return 949;   /* Hangul */
        case 130: return 1361;  /* Johab */
        case 134: return 936;   /* GB2312 */
		case 136: return 950;   /* Chinese Big5 */
        case 161: return 1253;  /* Greek */
        case 162: return 1254;  /* Turkish */
        case 163: return 1258;  /* Vietnamese */
        case 177: return 1255;  /* Hebrew */
	    case 178: return 1256;  /* Arabic */
        case 181: return 862;   /* Hebrew user */
        case 186: return 1257;  /* Baltic */
        case 204: return 1251;  /* Russian */
        case 222: return 874;   /* Thai */
        case 238: return 1250;  /* Eastern European */
        case 254: return 437;   /* PC 437 */
        case 255: return 850;   /* OEM */

		case 82:  /* Mac Johab (old) */
		case 179: /* Arabic traditional (old) */
		case 180: /* Arabic user (old) */
			g_warning(_("Character set %d not supported"), charset);
			return -1;
        default:
            g_warning(_("Unknown character set %d"), charset);
            return -1;
    }
}

static gint 
font_table_get_codepage(ParserContext *ctx)
{
	FontTableState *state = (FontTableState *)get_state(ctx);
	return state->codepage;
}

static gboolean
ft_f(ParserContext *ctx, FontTableState *state, gint32 index, GError **error)
{
	state->index = index;
	return TRUE;
}

static gboolean
ft_fcharset(ParserContext *ctx, FontTableState *state, gint32 charset, GError **error)
{
    state->codepage = fcharset_to_codepage(charset);
    return TRUE;
}

#define DEFINE_FONT_FAMILY_FUNCTION(name, type) \
	static gboolean \
	name(ParserContext *ctx, FontTableState *state, GError **error) \
	{ \
		state->family = type; \
		return TRUE; \
	}
DEFINE_FONT_FAMILY_FUNCTION(ft_fbidi,   FONT_FAMILY_BIDI)
DEFINE_FONT_FAMILY_FUNCTION(ft_fdecor,  FONT_FAMILY_DECORATIVE)
DEFINE_FONT_FAMILY_FUNCTION(ft_fmodern, FONT_FAMILY_MODERN)
DEFINE_FONT_FAMILY_FUNCTION(ft_fnil,    FONT_FAMILY_NIL)
DEFINE_FONT_FAMILY_FUNCTION(ft_froman,  FONT_FAMILY_ROMAN)
DEFINE_FONT_FAMILY_FUNCTION(ft_fscript, FONT_FAMILY_SCRIPT)
DEFINE_FONT_FAMILY_FUNCTION(ft_fswiss,  FONT_FAMILY_SWISS)
DEFINE_FONT_FAMILY_FUNCTION(ft_ftech,   FONT_FAMILY_TECH)
