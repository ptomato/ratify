#ifndef __OSXCART_RTF_DESERIALIZE_H__
#define __OSXCART_RTF_DESERIALIZE_H__

#include <glib.h>
#include <gtk/gtk.h>

typedef struct _ParserContext ParserContext;
typedef struct _ControlWord ControlWord;
typedef struct _Destination Destination;
typedef struct _DestinationInfo DestinationInfo;
typedef gboolean CWFunc(ParserContext *, gpointer, GError **);
typedef gboolean CWParamFunc(ParserContext *, gpointer, gint32, GError **);
typedef gpointer StateNewFunc(void);
typedef gpointer StateCopyFunc(gpointer);
typedef void StateFreeFunc(gpointer);

#define POINTS_TO_PANGO(pts) (pts * PANGO_SCALE)
#define HALF_POINTS_TO_PANGO(halfpts) (halfpts * PANGO_SCALE / 2)
#define TWIPS_TO_PANGO(twips) (twips * PANGO_SCALE / 20)

struct _ParserContext {
	/* Header information */
	gint codepage;
	gint default_codepage;
	gint default_font;
	gint default_language;
	
	/* Destination stack management */
	gint group_nesting_level;
	GQueue *destination_stack;

	/* Tables */
	GSList *color_table;
	GSList *font_table;

	/* Other document attributes */
	gint footnote_number;

	/* Text information */
	const gchar *rtftext;
	const gchar *pos;
	GString *plaintext;
	/* Text waiting for insertion */
	GString *text;

	/* Output references */
	GtkTextBuffer *textbuffer;
	GtkTextTagTable *tags;
	GtkTextMark *startmark;
	GtkTextMark *endmark;
};

struct _DestinationInfo {
    const ControlWord *word_table;
    void (*flush)(ParserContext *);
	StateNewFunc *state_new;
	StateCopyFunc *state_copy;
	StateFreeFunc *state_free;
	void (*cleanup)(ParserContext *);
};

typedef enum {
	NO_PARAMETER,
	OPTIONAL_PARAMETER,
	REQUIRED_PARAMETER,
	SPECIAL_CHARACTER,
	DESTINATION
} ControlWordType;

struct _ControlWord {
	const gchar *word;
	ControlWordType type;
	gboolean flush_buffer;
	gboolean (*action)();
	gint32 defaultparam;
	const gchar *replacetext;
	const DestinationInfo *destinfo;
};

struct _Destination {
	gint nesting_level;
    GQueue *state_stack;
    const DestinationInfo *info;
};

typedef struct {
    gint index;
    gint codepage;
	gchar *font_name;
} FontProperties;

gpointer get_state(ParserContext *ctx);
FontProperties *get_font_properties(ParserContext *ctx, int index);
void flush_text(ParserContext *ctx);
gboolean skip_character_or_control_word(ParserContext *ctx, GError **error);

#endif /* __OSXCART_RTF_DESERIALIZE_H__ */