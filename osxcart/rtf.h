#ifndef __OSXCART_RTF_H__
#define __OSXCART_RTF_H__

/* Copyright 2009, 2011 P. F. Chimento
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

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

/**
 * RtfError:
 * @RTF_ERROR_FAILED: A generic error.
 * @RTF_ERROR_INVALID_RTF: The file was not correct RTF.
 * @RTF_ERROR_MISSING_PARAMETER: A numerical parameter was missing from a
 * control word which requires one.
 * @RTF_ERROR_MISSING_BRACE: Not all groups were closed before the end of the
 * file.
 * @RTF_ERROR_EXTRA_CHARACTERS: There was junk after the last '}'.
 * @RTF_ERROR_BAD_VERSION: The RTF file was an incompatible version.
 * @RTF_ERROR_UNDEFINED_COLOR: A color was used which was not defined in the
 * color table.
 * @RTF_ERROR_UNDEFINED_FONT: A font was used which was not defined in the font
 * table.
 * @RTF_ERROR_UNDEFINED_STYLE: A style was used which was not defined in the
 * stylesheet.
 * @RTF_ERROR_BAD_HEX_CODE: Incorrect characters were encountered when expecting
 * hexadecimal digits (0-9, A-F)
 * @RTF_ERROR_BAD_PICT_TYPE: An invalid type of bitmap was specified.
 * @RTF_ERROR_BAD_FONT_SIZE: A negative font size was specified.
 * @RTF_ERROR_UNSUPPORTED_CHARSET: A character set with no iconv equivalent was
 * specified.
 *
 * The different codes which can be thrown in the #RTF_ERROR domain.
 */
typedef enum {
	RTF_ERROR_FAILED,
	RTF_ERROR_INVALID_RTF,
	RTF_ERROR_MISSING_PARAMETER,
	RTF_ERROR_MISSING_BRACE,
	RTF_ERROR_EXTRA_CHARACTERS,
	RTF_ERROR_BAD_VERSION,
	RTF_ERROR_UNDEFINED_COLOR,
	RTF_ERROR_UNDEFINED_FONT,
	RTF_ERROR_UNDEFINED_STYLE,
	RTF_ERROR_BAD_HEX_CODE,
	RTF_ERROR_BAD_PICT_TYPE,
	RTF_ERROR_BAD_FONT_SIZE,
	RTF_ERROR_UNSUPPORTED_CHARSET
} RtfError;

/**
 * RTF_ERROR:
 *
 * The domain of errors raised by RTF processing in Osxcart.
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
