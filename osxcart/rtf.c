/* Copyright 2009 P. F. Chimento
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

#include <string.h>
#include <errno.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <config.h>
#include <glib/gi18n-lib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <osxcart/rtf.h>
#include "init.h"
#include "rtf-serialize.h"
#include "rtf-deserialize.h"

/**
 * SECTION:rtf
 * @short_description: Tools for reading and writing Rich Text Format
 * @stability: Unstable
 * @include: osxcart/rtf.h
 *
 * Rich Text Format is a file format for storing marked-up text. It has been
 * around for more than twenty years at the time of writing, but is still used
 * as a native format by various programs and controls on Mac OS X, Windows, and
 * NeXTSTEP.
 *
 * For more information, see the <ulink 
 * link="http://en.wikipedia.org/wiki/Rich_Text_Format">Wikipedia page on 
 * RTF</ulink>.
 */

/**
 * rtf_error_quark:
 *
 * The error domain for RTF serializer errors.
 *
 * Returns: (transfer none): The string <quote>rtf-error-quark</quote> as a
 * <link linkend="GQuark">GQuark</link>.
 */
GQuark
rtf_error_quark(void)
{
	osxcart_init();
	return g_quark_from_static_string("rtf-error-quark");
}

/**
 * rtf_register_serialize_format:
 * @buffer: a text buffer
 *
 * Registers the RTF text serialization format with @buffer. This allows the
 * contents of @buffer to be exported to Rich Text Format (MIME type text/rtf).
 *
 * Returns: (transfer none): a <link linkend="GdkAtom">GdkAtom</link>
 * representing the serialization format, to be passed to
 * gtk_text_buffer_serialize().
 */
GdkAtom
rtf_register_serialize_format(GtkTextBuffer *buffer)
{
	osxcart_init();
	
	g_return_val_if_fail(buffer != NULL, GDK_NONE);
	g_return_val_if_fail(GTK_IS_TEXT_BUFFER(buffer), GDK_NONE);
	
	return gtk_text_buffer_register_serialize_format(buffer, "text/rtf", (GtkTextBufferSerializeFunc)rtf_serialize, NULL, NULL);
}

/**
 * rtf_register_deserialize_format:
 * @buffer: a text buffer
 *
 * Registers the RTF text deserialization format with @buffer. This allows
 * Rich Text Format files to be imported into @buffer. See 
 * rtf_register_serialize_format().
 * 
 * Returns: (transfer none): a <link linkend="GdkAtom">GdkAtom</link>
 * representing the deserialization format, to be passed to
 * gtk_text_buffer_deserialize().
 */
GdkAtom
rtf_register_deserialize_format(GtkTextBuffer *buffer)
{
	GdkAtom format;
	
	osxcart_init();
	
	g_return_val_if_fail(buffer != NULL, GDK_NONE);
	g_return_val_if_fail(GTK_IS_TEXT_BUFFER(buffer), GDK_NONE);
	
	format = gtk_text_buffer_register_deserialize_format(buffer, "text/rtf", (GtkTextBufferDeserializeFunc)rtf_deserialize, NULL, NULL);
	gtk_text_buffer_deserialize_set_can_create_tags(buffer, format, TRUE);
	return format;
}

/**
 * rtf_text_buffer_import:
 * @buffer: the text buffer into which to import text
 * @filename: an RTF text file
 * @error: return location for an error, or %NULL
 *
 * Deserializes the contents of @filename to @buffer. Only a small subset of
 * RTF features are supported: those corresponding to features of <link
 * linkend="GtkTextBuffer">GtkTextBuffer</link> or those that can be emulated in
 * a <link linkend="GtkTextBuffer">GtkTextBuffer</link>. All unsupported 
 * features are ignored.
 *
 * This function automatically registers the RTF deserialization format and
 * deregisters it afterwards, so there is no need to call 
 * rtf_register_deserialize_format().
 *
 * <note><para>
 *  This function also supports OS X and NeXTSTEP's RTFD packages. If @filename
 *  ends in <quote>.rtfd</quote>, is a directory, and contains a file called
 *  <quote>TXT.rtf</quote>, then it is assumed to be an RTFD package.
 * </para></note>
 *
 * Returns: %TRUE if the operation was successful, %FALSE if not, in which case
 * @error is set.
 */ 
gboolean
rtf_text_buffer_import(GtkTextBuffer *buffer, const gchar *filename, GError **error)
{
	gchar *cwd, *contents, *tmpstr, *tmpfilename, *realfilename, *newdir;
	gboolean retval;

	osxcart_init();
	
	g_return_val_if_fail(buffer != NULL, FALSE);
	g_return_val_if_fail(GTK_IS_TEXT_BUFFER(buffer), FALSE);
	g_return_val_if_fail(filename != NULL, FALSE);
	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    /* Save the current directory for later, because the RTF file may refer to
    other files relative to its own path */
	cwd = g_get_current_dir();

    /* Check whether this is an RTFD package */
    tmpstr = g_ascii_strdown(filename, -1);
    tmpfilename = g_build_filename(filename, "TXT.rtf", NULL);
    if(g_str_has_suffix(tmpstr, ".rtfd") && g_file_test(filename, G_FILE_TEST_IS_DIR) && g_file_test(tmpfilename, G_FILE_TEST_EXISTS))
    {
        /* cd to the package directory and open TXT.rtf */
        newdir = g_strdup(filename);
        realfilename = g_strdup("TXT.rtf");
    }
    else
    {
        newdir = g_path_get_dirname(filename);
        realfilename = g_path_get_basename(filename);
    }
    g_free(tmpstr);
    g_free(tmpfilename);

    /* Change directory */
    if(g_chdir(newdir) == -1)
    {
        g_free(newdir);
        g_free(realfilename);
		g_free(cwd);
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno), _("Could not change directory to '%s': %s"), newdir, g_strerror(errno));
        return FALSE;
    }
    g_free(newdir);

	if(!g_file_get_contents(realfilename, &contents, NULL, error))
	{
	    g_free(realfilename);
		if(g_chdir(cwd) == -1)
	    g_warning(_("Could not restore current directory: %s"), g_strerror(errno));
			g_free(cwd);
		return FALSE;
    }
    g_free(realfilename);
	retval = rtf_text_buffer_import_from_string(buffer, contents, error);
	g_free(contents);
	
	/* Change the directory back */
    if(g_chdir(cwd) == -1)
	    g_warning(_("Could not restore current directory: %s"), g_strerror(errno));
	g_free(cwd);
	
	return retval;
}

/**
 * rtf_text_buffer_import_from_string:
 * @buffer: the text buffer into which to import text
 * @string: a string containing an RTF document
 * @error: return location for an error, or %NULL
 *
 * Deserializes the contents of @string to @buffer. See 
 * rtf_text_buffer_import() for details.
 * 
 * Returns: %TRUE if the operation was successful, %FALSE if not, in which case
 * @error is set.
 */
gboolean
rtf_text_buffer_import_from_string(GtkTextBuffer *buffer, const gchar *string, GError **error)
{
	GdkAtom format;
	GtkTextIter start;
	gboolean retval;

	osxcart_init();
	
	g_return_val_if_fail(buffer != NULL, FALSE);
	g_return_val_if_fail(GTK_IS_TEXT_BUFFER(buffer), FALSE);
	g_return_val_if_fail(string != NULL, FALSE);
	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
	
	gtk_text_buffer_set_text(buffer, "", -1);
	gtk_text_buffer_get_start_iter(buffer, &start);
	
	format = rtf_register_deserialize_format(buffer);
	retval = gtk_text_buffer_deserialize(buffer, buffer, format, &start, (guint8 *)string, strlen(string), error);
	gtk_text_buffer_unregister_deserialize_format(buffer, format);
	
	return retval;
}

/**
 * rtf_text_buffer_export:
 * @buffer: the text buffer to export
 * @filename: filename to export to
 * @error: return location for an error, or %NULL
 *
 * Serializes the contents of @buffer to an RTF text file. Any formatting and 
 * embedded pixbufs in @buffer are preserved, but embedded widgets will not be. 
 * If the buffer was imported from RTF text written by another application, the
 * result will be quite different; none of the advanced formatting features that
 * RTF is capable of representing, such as styles, are preserved across loading 
 * and saving.
 *
 * This function automatically registers the RTF serialization format and
 * deregisters it afterwards, so there is no need to call 
 * rtf_register_serialize_format().
 *
 * Returns: %TRUE if the operation succeeded, %FALSE if not, in which case 
 * @error is set.
 */
gboolean
rtf_text_buffer_export(GtkTextBuffer *buffer, const gchar *filename, GError **error)
{
	gchar *string;
	gboolean retval;

	osxcart_init();
	
	g_return_val_if_fail(buffer != NULL, FALSE);
	g_return_val_if_fail(GTK_IS_TEXT_BUFFER(buffer), FALSE);
	g_return_val_if_fail(filename != NULL, FALSE);
	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
	
	string = rtf_text_buffer_export_to_string(buffer);
	retval = g_file_set_contents(filename, string, -1, error);
	g_free(string);
	return retval;
}

/**
 * rtf_text_buffer_export_to_string:
 * @buffer: the text buffer to export
 *
 * Serializes the contents of @buffer to a string in RTF format. See
 * rtf_text_buffer_export() for details.
 *
 * Returns: a string containing RTF text. The string must be freed with <link
 * linkend="glib-Memory-Allocation">g_free()</link> when you are done with it.
 */
gchar *
rtf_text_buffer_export_to_string(GtkTextBuffer *buffer)
{
	GdkAtom format;
	GtkTextIter start, end;
	gchar *string;
	gsize length;

	osxcart_init();
	
	g_return_val_if_fail(buffer != NULL, NULL);
	
	gtk_text_buffer_get_bounds(buffer, &start, &end);
	
	format = rtf_register_serialize_format(buffer);
	string = (gchar *)gtk_text_buffer_serialize(buffer, buffer, format, &start, &end, &length);	
	gtk_text_buffer_unregister_serialize_format(buffer, format);

	return string;	
}
