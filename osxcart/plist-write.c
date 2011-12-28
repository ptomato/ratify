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

#include <string.h>
#include <glib.h>
#include <gio/gio.h>
#include <osxcart/plist.h>
#include "init.h"

/* plist-write.c - Simple recursive plist writer */

typedef struct {
	GString *buffer;
	gint num_indents;
	GHashTable *hashtable;
} PlistDumpContext;

/* Forward declarations */
static void plist_dump(PlistObject *object, PlistDumpContext *context);

/* Output a key and its corresponding object */
static void
dump_key_value_pair(const gchar *key, PlistDumpContext *context)
{
	GHashTable *hashtable = context->hashtable;
	gchar *tabs = g_strnfill(context->num_indents, '\t');
	PlistObject *value = g_hash_table_lookup(hashtable, key);
	
	g_string_append_printf(context->buffer, "%s<key>%s</key>\n", tabs, key);
	g_free(tabs);
	plist_dump(value, context);
	context->hashtable = hashtable;
}

/* Output an object; recurse if the object is a container */
static void
plist_dump(PlistObject *object, PlistDumpContext *context)
{
	gchar *tempstr, *tabs;

	if(object == NULL)
		return;

	tabs = g_strnfill(context->num_indents, '\t');
	g_string_append(context->buffer, tabs);

	switch(object->type) {
	case PLIST_OBJECT_BOOLEAN:
		g_string_append(context->buffer, object->boolean.val? "<true/>\n" : "<false/>\n");
		break;
	
	case PLIST_OBJECT_REAL:
		g_string_append_printf(context->buffer, "<real>%.14f</real>\n", object->real.val);
		break;
	
	case PLIST_OBJECT_INTEGER:
		g_string_append_printf(context->buffer, "<integer>%d</integer>\n", object->integer.val);
		break;
		
	case PLIST_OBJECT_STRING:
		if(object->string.val == NULL || strlen(object->string.val) == 0)
			g_string_append(context->buffer, "<string></string>\n");
		else {
			tempstr = g_markup_escape_text(object->string.val, -1);
			g_string_append_printf(context->buffer, "<string>%s</string>\n", tempstr);
			g_free(tempstr);
		}
		break;
		
	case PLIST_OBJECT_DATE:
		tempstr = g_time_val_to_iso8601(&(object->date.val));
		g_string_append_printf(context->buffer, "<date>%s</date>\n", tempstr);
		g_free(tempstr);
		break;
	
	case PLIST_OBJECT_ARRAY:
		if(object->array.val) {
			g_string_append(context->buffer, "<array>\n");
			context->num_indents++;
			g_list_foreach(object->array.val, (GFunc)plist_dump, context);
			context->num_indents--;
			g_string_append_printf(context->buffer, "%s</array>\n", tabs);
		} else
			g_string_append(context->buffer, "<array/>\n");
		break;
	
	case PLIST_OBJECT_DICT:
		if(g_hash_table_size(object->dict.val) != 0) {
			GList *keys = g_list_sort(g_hash_table_get_keys(object->dict.val), (GCompareFunc)strcmp);
						
			g_string_append(context->buffer, "<dict>\n");
			context->num_indents++;
			context->hashtable = object->dict.val;
			g_list_foreach(keys, (GFunc)dump_key_value_pair, context);
			context->num_indents--;
			context->hashtable = NULL;
			g_string_append_printf(context->buffer, "%s</dict>\n", tabs);
		} else
			g_string_append(context->buffer, "<dict/>\n");
		break;
	
	case PLIST_OBJECT_DATA:
		tempstr = g_base64_encode(object->data.val, object->data.length);
		g_string_append_printf(context->buffer, "<data>%s</data>\n", tempstr);
		g_free(tempstr);
		break;
	}
	g_free(tabs);
}

/**
 * plist_write:
 * @plist: A property list object.
 * @filename: The filename to write to.
 * @error: Return location for an error, or %NULL.
 *
 * Writes the property list @plist to a file in XML format. If @filename exists,
 * it will be overwritten.
 *
 * Returns: %TRUE if the operation succeeded, %FALSE if not, in which case
 * @error is set.
 */
gboolean
plist_write(PlistObject *plist, const gchar *filename, GError **error)
{
	gchar *string;
	gboolean retval;

	osxcart_init();
	
	g_return_val_if_fail(plist != NULL, FALSE);
	g_return_val_if_fail(filename != NULL, FALSE);
	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

	string = plist_write_to_string(plist);
	retval = g_file_set_contents(filename, string, -1, error);
	g_free(string);
	return retval;
}

/**
 * plist_write_file:
 * @plist: A property list object.
 * @file: A #GFile to write to.
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL.
 * @error: Return location for an error, or %NULL.
 *
 * Writes the property list @plist to a file in XML format. If @file exists,
 * it will be overwritten. The operation will be cancelled if @cancellable is
 * triggered from another thread.
 *
 * Returns: %TRUE if the operation succeeded, %FALSE if not, in which case
 * @error is set.
 */
gboolean
plist_write_file(PlistObject *plist, GFile *file, GCancellable *cancellable, GError **error)
{
	gchar *string;
	gboolean retval;

	osxcart_init();

	g_return_val_if_fail(plist != NULL, FALSE);
	g_return_val_if_fail(file != NULL, FALSE);
	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

	string = plist_write_to_string(plist);
	retval = g_file_replace_contents(file, string, -1, NULL, FALSE, G_FILE_CREATE_NONE, NULL, cancellable, error);
	g_free(string);
	return retval;
}

/**
 * plist_write_to_string:
 * @plist: A property list object.
 * 
 * Writes the property list @plist to a string in XML format.
 *
 * Returns: a newly-allocated string containing an XML property list. The string
 * must be freed with <link linkend="glib-Memory-Allocation">g_free()</link> 
 * when you are done with it.
 */
gchar *
plist_write_to_string(PlistObject *plist)
{
	PlistDumpContext *context;
	GString *buffer;

	osxcart_init();
	
	g_return_val_if_fail(plist != NULL, NULL);

	buffer = g_string_new(
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" "
		"\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
		"<plist version=\"1.0\">\n");
	context = g_slice_new0(PlistDumpContext);	
	context->buffer = buffer;
	plist_dump(plist, context);
	g_slice_free(PlistDumpContext, context);
	g_string_append(buffer, "</plist>\n");
	return g_string_free(buffer, FALSE);
}
