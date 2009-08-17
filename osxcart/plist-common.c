#include <stdlib.h>
#include <glib.h>
#include <config.h>
#include <glib/gi18n-lib.h>
#include <osxcart/plist.h>
#include "plist-common.h"
#include "init.h"

/* plist-common.c -- plist parser functions used in both the pre-GLib-2.18 and
post-2.18 parsers */

/* Custom string equality function, for typing convenience */
inline gboolean 
str_eq(const gchar *s1, const gchar *s2)
{
	return (g_ascii_strcasecmp(s1, s2) == 0);
}

/* Check the root element of the plist. Make sure it is named <plist>, and that
it is version 1.0 */
void
check_plist_element(const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, GError **error)
{
    const gchar *version_string;

	if(!str_eq(element_name, "plist")) {
		g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT, _("<plist> root element not found; got <%s> instead"), element_name);
		return;
	}
	
	if(!g_markup_collect_attributes(element_name, attribute_names, attribute_values, error,
	   G_MARKUP_COLLECT_STRING, "version", &version_string,
	   G_MARKUP_COLLECT_INVALID))
	    return;
	/* Don't free version_string */
	
	if(!str_eq(version_string, "1.0"))
		g_set_error(error, PLIST_ERROR, PLIST_ERROR_BAD_VERSION, _("Unsupported plist version '%s'"), version_string);
}

/* Assign a value to an already-allocated PlistObject, from an XML element
<name> with content 'text' */
void
fill_object(PlistObject *current, const gchar *name, const gchar *text, GError **error)
{
	/* There should be no text in <true> or <false> */
	if(str_eq(name, "true") || str_eq(name, "false"))
		g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT, _("<%s> should have no content, but found '%s'"), name, text);

	/* <real> - this assumes that property lists do NOT contain localized 
	representation of numbers */
	else if(str_eq(name, "real"))
		current->real.val = g_ascii_strtod(text, NULL);
	
	else if(str_eq(name, "integer"))
		current->integer.val = atoi(text);
	
	else if(str_eq(name, "string"))
		current->string.val = g_strdup(text);

	else if(str_eq(name, "date")) {
		if(!g_time_val_from_iso8601(text, &(current->date.val)))
			g_set_error(error, PLIST_ERROR, PLIST_ERROR_BAD_DATE, _("Could not parse date '%s'"), text);
	}
	
	else if(str_eq(name, "data"))
		current->data.val = g_base64_decode(text, &(current->data.length));

	/* else - just ignore text in <array> and <dict>, because it could be 
	whitespace */	

	return;
}

/**
 * plist_read:
 * @filename: The path to a file containing a property list in XML format.
 * @error: Return location for an error, or %NULL.
 *
 * Reads a property list in XML format from @filename and returns a #PlistObject 
 * representing the property list.
 *
 * Returns: the property list, or %NULL if an error occurred, in which case
 * @error is set. The property list must be freed with plist_object_free() after
 * use.
 */
PlistObject *
plist_read(const gchar *filename, GError **error)
{
	gchar *contents;
	PlistObject *retval;

	osxcart_init();
	
	g_return_val_if_fail(filename != NULL, NULL);
	g_return_val_if_fail(error == NULL || *error == NULL, NULL);

	if(!g_file_get_contents(filename, &contents, NULL, error))
		return NULL;
	retval = plist_read_from_string(contents, error);
	g_free(contents);
	return retval;
}

/**
 * plist_read_from_string:
 * @string: A string containing a property list in XML format.
 * @error: Return location for an error, or %NULL.
 *
 * Reads a property list in XML format from @string and returns a #PlistObject
 * representing the property list.
 *
 * Returns: the property list, or %NULL if an error occurred, in which case
 * @error is set. The property list must be freed with plist_object_free() after
 * use.
 */
PlistObject *
plist_read_from_string(const gchar *string, GError **error)
{
	osxcart_init();
	return plist_read_from_string_real(string, error);
}
