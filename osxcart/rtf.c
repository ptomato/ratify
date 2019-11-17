/* Copyright 2009, 2011, 2012, 2015, 2019 P. F. Chimento
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
#include <stdbool.h>
#include <string.h>

#include <gdk/gdk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include "init.h"
#include "rtf.h"
#include "rtf-serialize.h"
#include "rtf-deserialize.h"

/**
 * SECTION:rtf
 * @title: RTF tools
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
    osxcart_init();

    g_return_val_if_fail(buffer != NULL, GDK_NONE);
    g_return_val_if_fail(GTK_IS_TEXT_BUFFER(buffer), GDK_NONE);

    GdkAtom format = gtk_text_buffer_register_deserialize_format(buffer, "text/rtf", (GtkTextBufferDeserializeFunc)rtf_deserialize, NULL, NULL);
    gtk_text_buffer_deserialize_set_can_create_tags(buffer, format, true);
    return format;
}

typedef char Pushd;

static Pushd *
push_cwd(const char *newdir, GError **error)
{
    g_autofree char *cwd = g_get_current_dir();
    if (g_chdir(newdir) == -1) {
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno), _("Could not change directory to '%s': %s"), newdir, g_strerror(errno));
        return NULL;
    }
    return g_steal_pointer(&cwd);
}

static void
pop_cwd(Pushd *old_cwd)
{
    if (g_chdir(old_cwd) == -1)
        g_warning(_("Could not restore current directory: %s"), g_strerror(errno));
    g_free(old_cwd);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC(Pushd, pop_cwd);

/**
 * rtf_text_buffer_import_file:
 * @buffer: the text buffer into which to import text
 * @file: a #GFile pointing to an RTF text file
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: return location for an error, or %NULL
 *
 * Deserializes the contents of @file to @buffer. Only a small subset of RTF
 * features are supported: those corresponding to features of <link
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
 * If @cancellable is triggered from another thread, the operation is cancelled.
 *
 * Returns: %TRUE if the operation was successful, %FALSE if not, in which case
 * @error is set.
 *
 * Since: 1.1
 */
gboolean
rtf_text_buffer_import_file(GtkTextBuffer *buffer, GFile *file, GCancellable *cancellable, GError **error)
{
    osxcart_init();

    g_return_val_if_fail(buffer != NULL, false);
    g_return_val_if_fail(GTK_IS_TEXT_BUFFER(buffer), false);
    g_return_val_if_fail(file != NULL, false);
    g_return_val_if_fail(G_IS_FILE(file), false);
    g_return_val_if_fail(cancellable == NULL || G_IS_CANCELLABLE(cancellable), false);
    g_return_val_if_fail(error == NULL || *error == NULL, false);

    /* Check whether this is an RTFD package */
    g_autofree char *basename = g_file_get_basename(file);
    g_autofree char *tmpstr = g_ascii_strdown(basename, -1);
    g_autoptr(GFile) check_file = g_file_get_child(file, "TXT.rtf");
    g_autoptr(GFile) real_file = NULL;
    if (g_str_has_suffix(tmpstr, ".rtfd") &&
        g_file_query_file_type(file, G_FILE_QUERY_INFO_NONE, NULL) == G_FILE_TYPE_DIRECTORY &&
        g_file_query_exists(check_file, NULL)) {
        /* cd to the package directory and open TXT.rtf */
        real_file = g_object_ref(check_file);
    } else {
        real_file = g_object_ref(file);
    }

    /* Change directory */
    g_autoptr(GFile) parent = g_file_get_parent(real_file);
    g_autofree char *newdir = g_file_get_path(parent);
    /* Save the current directory for later, because the RTF file may refer to
    other files relative to its own path */
    g_autoptr(Pushd) olddir = push_cwd(newdir, error);
    if (olddir == NULL)
        return false;

    g_autofree char *contents = NULL;
    if (!g_file_load_contents(real_file, cancellable, &contents, NULL, NULL, error))
        return false;

    return rtf_text_buffer_import_from_string(buffer, contents, error);
}

/**
 * rtf_text_buffer_import:
 * @buffer: the text buffer into which to import text
 * @filename: path to an RTF text file
 * @error: return location for an error, or %NULL
 *
 * Deserializes the contents of @filename to @buffer. For more information, see
 * rtf_text_buffer_import_file().
 *
 * Returns: %TRUE if the operation was successful, %FALSE if not, in which case
 * @error is set.
 */
gboolean
rtf_text_buffer_import(GtkTextBuffer *buffer, const char *filename, GError **error)
{
    osxcart_init();
    g_return_val_if_fail(buffer != NULL, false);
    g_return_val_if_fail(GTK_IS_TEXT_BUFFER(buffer), false);
    g_return_val_if_fail(filename != NULL, false);
    g_return_val_if_fail(error == NULL || *error == NULL, false);

    g_autoptr(GFile) file = g_file_new_for_path(filename);
    return rtf_text_buffer_import_file(buffer, file, NULL, error);
}

/**
 * rtf_text_buffer_import_from_string:
 * @buffer: the text buffer into which to import text
 * @string: a string containing an RTF document
 * @error: return location for an error, or %NULL
 *
 * Deserializes the contents of @string to @buffer. See
 * rtf_text_buffer_import_file() for details.
 *
 * <note><para>
 *   If @string contains references to external files, such as images, then
 *   these will be resolved relative to the current working directory.
 *   That's usually not what you want.
 *   If you want to load images, then you must change the current working
 *   directory appropriately before calling this function, or embed the images
 *   in the RTF code, or use rtf_text_buffer_import() or
 *   rtf_text_buffer_import_file().
 * </para></note>
 *
 * Returns: %TRUE if the operation was successful, %FALSE if not, in which case
 * @error is set.
 */
gboolean
rtf_text_buffer_import_from_string(GtkTextBuffer *buffer, const char *string, GError **error)
{
    osxcart_init();

    g_return_val_if_fail(buffer != NULL, false);
    g_return_val_if_fail(GTK_IS_TEXT_BUFFER(buffer), false);
    g_return_val_if_fail(string != NULL, false);
    g_return_val_if_fail(error == NULL || *error == NULL, false);

    gtk_text_buffer_set_text(buffer, "", -1);
    GtkTextIter start;
    gtk_text_buffer_get_start_iter(buffer, &start);

    GdkAtom format = rtf_register_deserialize_format(buffer);
    bool retval = gtk_text_buffer_deserialize(buffer, buffer, format, &start, (uint8_t *)string, strlen(string), error);
    gtk_text_buffer_unregister_deserialize_format(buffer, format);

    return retval;
}

/**
 * rtf_text_buffer_export_file:
 * @buffer: the text buffer to export
 * @file: a #GFile to export to
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: return location for an error, or %NULL
 *
 * Serializes the contents of @buffer to an RTF text file, @file. Any formatting
 * and embedded pixbufs in @buffer are preserved, but embedded widgets will not
 * be. If the buffer was imported from RTF text written by another application,
 * the result will be quite different; none of the advanced formatting features
 * that RTF is capable of representing, such as styles, are preserved across
 * loading and saving.
 *
 * This function automatically registers the RTF serialization format and
 * deregisters it afterwards, so there is no need to call
 * rtf_register_serialize_format().
 *
 * The operation can be cancelled by triggering @cancellable from another
 * thread.
 *
 * Returns: %TRUE if the operation succeeded, %FALSE if not, in which case
 * @error is set.
 *
 * Since: 1.1
 */
gboolean
rtf_text_buffer_export_file(GtkTextBuffer *buffer, GFile *file, GCancellable *cancellable, GError **error)
{
    osxcart_init();

    g_return_val_if_fail(buffer != NULL, false);
    g_return_val_if_fail(GTK_IS_TEXT_BUFFER(buffer), false);
    g_return_val_if_fail(file != NULL, false);
    g_return_val_if_fail(G_IS_FILE(file), false);
    g_return_val_if_fail(cancellable == NULL || G_IS_CANCELLABLE(cancellable), false);
    g_return_val_if_fail(error == NULL || *error == NULL, false);

    g_autofree char *string = rtf_text_buffer_export_to_string(buffer);
    return g_file_replace_contents(file, string, strlen(string), NULL, false, G_FILE_CREATE_NONE, NULL, cancellable, error);
}

/**
 * rtf_text_buffer_export:
 * @buffer: the text buffer to export
 * @filename: a filename to export to
 * @error: return location for an error, or %NULL
 *
 * Serializes the contents of @buffer to an RTF text file, @filename. See
 * rtf_text_buffer_export_file() for more information.
 *
 * Returns: %TRUE if the operation succeeded, %FALSE if not, in which case
 * @error is set.
 */
gboolean
rtf_text_buffer_export(GtkTextBuffer *buffer, const char *filename, GError **error)
{
    osxcart_init();

    g_return_val_if_fail(buffer != NULL, false);
    g_return_val_if_fail(GTK_IS_TEXT_BUFFER(buffer), false);
    g_return_val_if_fail(filename != NULL, false);
    g_return_val_if_fail(error == NULL || *error == NULL, false);

    g_autofree char *string = rtf_text_buffer_export_to_string(buffer);
    return g_file_set_contents(filename, string, -1, error);
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
char *
rtf_text_buffer_export_to_string(GtkTextBuffer *buffer)
{
    osxcart_init();

    g_return_val_if_fail(buffer != NULL, NULL);

    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);

    GdkAtom format = rtf_register_serialize_format(buffer);
    size_t length;
    char *string = (char *)gtk_text_buffer_serialize(buffer, buffer, format, &start, &end, &length);
    gtk_text_buffer_unregister_serialize_format(buffer, format);

    return string;
}
