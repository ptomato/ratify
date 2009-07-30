#ifndef __OSXCART_RTF_H__
#define __OSXCART_RTF_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

/**
 * RtfError:
 *
 * The different codes which can be thrown in the #RTF_ERROR domain.
 */
typedef enum {
	RTF_ERROR_FAILED,              /* Generic error */
	RTF_ERROR_INVALID_RTF,         /* The file was not correct RTF */
	RTF_ERROR_MISSING_PARAMETER,   /* A numerical parameter was missing from a
	                                  control word which requires one */
	RTF_ERROR_MISSING_BRACE,       /* Not all groups were closed before EOF */
	RTF_ERROR_EXTRA_CHARACTERS,    /* There was junk after the last '}' */
	RTF_ERROR_BAD_VERSION,         /* The RTF file was an incompatible version*/
	RTF_ERROR_UNDEFINED_COLOR,     /* A color was used which was not defined in
	                                  the color table */
	RTF_ERROR_UNDEFINED_FONT,      /* A font was used which was not defined in
	                                  the font table */
	RTF_ERROR_UNDEFINED_STYLE,     /* A style was used which was not defined in
	                                  the stylesheet */
	RTF_ERROR_BAD_HEX_CODE,        /* Incorrect characters were encountered when
	                                  expecting hexadecimal digits (0-9, A-F) */
	RTF_ERROR_BAD_PICT_TYPE,       /* An invalid type of bitmap was specified */
	RTF_ERROR_BAD_FONT_SIZE,       /* A negative font size was specified */
	RTF_ERROR_UNSUPPORTED_CHARSET  /* A character set with no iconv equivalent
	                                  was specified */
} RtfError;

/**
 * RTF_ERROR:
 *
 * The domain of errors raised by RTF processing in Libmac.
 */
#define RTF_ERROR rtf_error_quark()

GQuark rtf_error_quark(void);
GdkAtom rtf_register_serialize_format(GtkTextBuffer *buffer);
GdkAtom rtf_register_deserialize_format(GtkTextBuffer *buffer);
gboolean rtf_text_buffer_import(GtkTextBuffer *buffer, const gchar *filename, GError **error);
gboolean rtf_text_buffer_import_from_string(GtkTextBuffer *buffer, const gchar *string, GError **error);
gboolean rtf_text_buffer_export(GtkTextBuffer *buffer, const gchar *filename, GError **error);
gchar *rtf_text_buffer_export_to_string(GtkTextBuffer *buffer);

G_END_DECLS

#endif /* __OSXCART_RTF_H__ */
