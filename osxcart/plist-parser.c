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

#include <stdlib.h>
#include <glib.h>
#include <config.h>
#include <glib/gi18n-lib.h>
#include <osxcart/plist.h>
#include "init.h"

/* plist-parser.c - This is a plist parser implemented in GLib's GMarkup SAX
parser API. It requires Glib 2.18 or later. */

typedef enum {
	STATE_ROOT_OBJECT,
	STATE_ARRAY_OBJECT,
	STATE_DICT_OBJECT
} ParserState;

typedef struct {
	ParserState state;    /* Current state of state machine */
	GList *array;         /* <array> to add objects, if in STATE_ARRAY_OBJECT */
	GHashTable *dict;     /* <dict> to add objects, if in STATE_DICT_OBJECT */
	gchar *key;	          /* key of current object, if in STATE_DICT_OBJECT */
	PlistObject *current; /* Object currently being parsed */
} ParseData;

/* Forward declarations */
static void element_start(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error);
static void element_end(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error);
static void element_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error);
static void plist_start(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error);
static void plist_end(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error);

/* Callback functions to deal with the opening and closing tags and the content
of XML elements representing PlistObjects */
static GMarkupParser element_parser = { 
	element_start, element_end, element_text, NULL, NULL
};

/* Callback functions to deal with the <plist> root element opening and closing
tags */
static GMarkupParser plist_parser = { 
	plist_start, plist_end, NULL, NULL, NULL
};


/* Custom string equality function, for typing convenience */
static inline gboolean 
str_eq(const gchar *s1, const gchar *s2)
{
	return (g_ascii_strcasecmp(s1, s2) == 0);
}

/* Check the root element of the plist. Make sure it is named <plist>, and that
it is version 1.0 */
static void
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
static void
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

/* Allocate a new PlistObject depending on the <name> of the XML element being
processed. We do not know what the content of the element is, so it is not
assigned a value until fill_object(); the exception is <true> and <false>
elements, whose content is already obvious. */
static PlistObject *
start_new_object(const gchar *element_name)
{
	/* true - assign val here */
	if(str_eq(element_name, "true")) {
		PlistObject *retval = plist_object_new(PLIST_OBJECT_BOOLEAN);
		retval->boolean.val = TRUE;
		return retval;
	}
	
	/* false - assign val here */
	if(str_eq(element_name, "false")) {
		PlistObject *retval = plist_object_new(PLIST_OBJECT_BOOLEAN);
		retval->boolean.val = FALSE;
		return retval;
	}
	
	if(str_eq(element_name, "real"))
		return plist_object_new(PLIST_OBJECT_REAL);
	if(str_eq(element_name, "integer"))
		return plist_object_new(PLIST_OBJECT_INTEGER);
	if(str_eq(element_name, "string"))
		return plist_object_new(PLIST_OBJECT_STRING);
	if(str_eq(element_name, "date"))
		return plist_object_new(PLIST_OBJECT_DATE);
	if(str_eq(element_name, "data"))
		return plist_object_new(PLIST_OBJECT_DATA);
	if(str_eq(element_name, "array"))
		return plist_object_new(PLIST_OBJECT_ARRAY);
	if(str_eq(element_name, "dict"))
		return plist_object_new(PLIST_OBJECT_DICT);
	return NULL;
}

/* Callback for processing an opening XML element */
static void
element_start(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error)
{
	ParseData *data = (ParseData *)user_data;
	
	if(data->current) {
		g_set_error(error, PLIST_ERROR, PLIST_ERROR_UNEXPECTED_OBJECT, _("Unexpected object <%s>; subsequent objects ought to be enclosed in an <array> or <dict>"), element_name);
		return;
	}
	
	if(data->state == STATE_DICT_OBJECT && data->key == NULL && !str_eq(element_name, "key")) {
		g_set_error(error, PLIST_ERROR, PLIST_ERROR_MISSING_KEY, _("Missing <key> for object <%s> in <dict>"), element_name);
		return;
	}
	
	data->current = start_new_object(element_name);
	
	/* <array> - create a new ParseData with a parent array and push this parser context 	onto the stack once more with the new ParseData */
	if(str_eq(element_name, "array")) {
		ParseData *new_data = g_slice_new0(ParseData);
		new_data->state = STATE_ARRAY_OBJECT;
		new_data->array = data->current->array.val;
		g_markup_parse_context_push(context, &element_parser, new_data);
	}
	
	/* <dict> - create a new ParseData with a parent hashtable and push this
	parser context onto the stack once more with the new ParseData */
	else if(str_eq(element_name, "dict")) {
		ParseData *new_data = g_slice_new0(ParseData);
		new_data->state = STATE_DICT_OBJECT;
		new_data->dict = data->current->dict.val;
		g_markup_parse_context_push(context, &element_parser, new_data);
	}

	/* <key> - invalid if not in a <dict> */
	else if(str_eq(element_name, "key")) {
		if(data->state != STATE_DICT_OBJECT)
			g_set_error(error, PLIST_ERROR, PLIST_ERROR_EXTRANEOUS_KEY, _("<key> element found outside of <dict>"));
	} 
}	

/* Callback for processing content of XML elements */
static void
element_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{	
	ParseData *data = (ParseData *)user_data;

	const gchar *name = g_markup_parse_context_get_element(context);
	
	fill_object(data->current, name, text, error);
	
	/* key */
	if(str_eq(name, "key"))
		data->key = g_strdup(text);
}

/* Callback for processing closing XML elements */
static void
element_end(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error)
{	
	ParseData *data = (ParseData *)user_data;

	/* </array> - store the subparser's new head of the list in this parser's 
	parser data */
	if(str_eq(element_name, "array")) {
		ParseData *sub_data = g_markup_parse_context_pop(context);
		data->current->array.val = g_list_reverse(sub_data->array);
		g_slice_free(ParseData, sub_data);
	}
	
	/* </dict> - don't have to do anything, our copy of the hashtable is ok */
	else if(str_eq(element_name, "dict")) {
		ParseData *sub_data = g_markup_parse_context_pop(context);
		g_slice_free(ParseData, sub_data);
	}
	
	/* If the element is still NULL, that means that the tag we have been 
	handling wasn't recognized. Unless it was <key>. */
	if(data->current == NULL && !str_eq(element_name, "key")) {
		g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT, _("Unknown object <%s>"), element_name);
		return;
	}

	/* any element while in array - store the element in the array and go 
	round again */
	if(data->state == STATE_ARRAY_OBJECT) {
		data->array = g_list_prepend(data->array, data->current);
		data->current = NULL;
		return;
	}
	
	/* other element while in dict - store the element in the hash table, blank 
	the key, rinse, lather, repeat */
	if(data->state == STATE_DICT_OBJECT && data->key != NULL && data->current != NULL) {
		g_hash_table_insert(data->dict, data->key, data->current);
		data->key = NULL;
		data->current = NULL;
		return;
	}
	
	/* other element while in root <plist> element - leave it where it is */
}

/* Callback for processing opening tag of root <plist> element */
static void
plist_start(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error)
{
	ParseData *parse_data;
	
	check_plist_element(element_name, attribute_names, attribute_values, error);
	if(*error)
		return;
	
	parse_data = g_slice_new0(ParseData);
	parse_data->state = STATE_ROOT_OBJECT;
	g_markup_parse_context_push(context, &element_parser, parse_data);
}

/* Callback for processing closing </plist> tag */
static void
plist_end(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error)
{
	PlistObject **data = (PlistObject **)user_data;
	
	ParseData *parse_data = g_markup_parse_context_pop(context);
	if(parse_data->current == NULL) {
		g_set_error(error, PLIST_ERROR, PLIST_ERROR_NO_ELEMENTS, _("No objects found within <plist> root element"));
		return;
	}
	*data = parse_data->current;
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
	GMarkupParseContext *context;
	PlistObject *plist = NULL;
	
	osxcart_init();

	g_return_val_if_fail(string != NULL, NULL);
	g_return_val_if_fail(error == NULL || *error == NULL, NULL);
	
	context = g_markup_parse_context_new(&plist_parser, G_MARKUP_PREFIX_ERROR_POSITION, &plist, NULL);
	if(!g_markup_parse_context_parse(context, string, -1, error) || !g_markup_parse_context_end_parse(context, error)) {
		g_markup_parse_context_free(context);
		plist_object_free(plist);
		return NULL;
	}
	g_markup_parse_context_free(context);
	return plist;
}
