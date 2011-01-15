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

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <osxcart/plist.h>
#include "plist-common.h"

/* plist-parser-oldglib.c - This is a plist parser implemented in GLib's
GMarkup SAX parser API, only BEFORE it included a stack. (Since 2.18.) The code
in plist-parser.c was written first and less hastily. However, 2.18 is not in
Debian stable at the time of writing. This whole source file is highly 
deprecated and will be removed from the distribution as soon as Debian stable
gets GLib 2.18, probably within 10 years. */

typedef enum {
	STATE_PLIST,
	STATE_ROOT_OBJECT,
	STATE_ARRAY_OBJECT,
	STATE_DICT_OBJECT,
	STATE_DICT_KEY
} ParserState;

/* Plist parser context */
typedef struct {
	ParserState state;       /* Current state of state machine */
	PlistObject *current;    /* CFValue we are currently parsing, NULL if none*/
	PlistObject *retval;     /* Outermost container, to be returned */
	GQueue *container_stack; /* Containers which we are nested inside,
	                            including currently active one */
	GQueue *key_stack;       /* Keys of <dict> entries which are themselves on
	                            the container stack */
} ParseData;

/* Forward declarations */
static void plist_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error);
static void plist_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error);
static void plist_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error);

/* Callback functions which deal with the start of an XML element, the end of
one, and the content inside it. We have one GMarkupParser for every parser
state here, instead of different ones as in plist-parser.c */
static GMarkupParser plist_parser = { 
	plist_start_element, plist_end_element, plist_text, NULL, NULL
};

/* Allocate parser context */
static ParseData *
parse_data_new(void)
{
	ParseData *data = g_slice_new0(ParseData);
	data->state = STATE_PLIST;
	data->container_stack = g_queue_new();
	data->key_stack = g_queue_new();
	return data;
}

/* Free parser context */
static void
parse_data_free(ParseData *data)
{
	/* Don't free retval */
	plist_object_free(data->current);
	g_queue_foreach(data->container_stack, (GFunc)plist_object_free, NULL);
	g_queue_free(data->container_stack);
	g_queue_foreach(data->key_stack, (GFunc)g_free, NULL);
	g_queue_free(data->key_stack);
	g_slice_free(ParseData, data);
}

/* Allocate a new PlistObject depending on the <name> of the XML element being
processed. We do not know what the content of the element is, so it is not
assigned a value until fill_object(); the exception is <true> and <false>
elements, whose content is already obvious. */
static PlistObject *
start_new_object(const gchar *element_name, GError **error)
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
		
	/* key invalid where object expected */
	if(str_eq(element_name, "key")) {
		g_set_error(error, PLIST_ERROR, PLIST_ERROR_EXTRANEOUS_KEY, _("<key> element found outside of <dict>"));
		return NULL;
	}
	
	g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT, _("Unknown element <%s>"), element_name);
	return NULL;
}

/* This function is called after the last PlistObject is inserted into a
container PlistObject, i.e. the container was closed with an </array> or 
</dict>. It determines what parser state to go to next. */
static void
insert_top_object(ParseData *data)
{
	PlistObject *obj = g_queue_pop_head(data->container_stack);
	PlistObject *top;
	
	/* Arrays are filled with prepend(), so reverse the array now that we are 
	done filling it */
	if(obj->type == PLIST_OBJECT_ARRAY)
		obj->array.val = g_list_reverse(obj->array.val);
	
	/* If this was the outermost container, then it will be the parser's return
	value; go to STATE_PLIST and expect a closing </plist> */
	if(g_queue_is_empty(data->container_stack)) {
		data->retval = obj;
		data->state = STATE_PLIST;
		return;
	}
	
	/* Look at the new top of the container stack; if it's an <array>, then
	expect array elements, if it's a <dict>, then expect the next <key>. In
	either case, insert the newly-completed container into the now-current
	container. */
	top = g_queue_peek_head(data->container_stack);
	if(top->type == PLIST_OBJECT_ARRAY) {
		top->array.val = g_list_prepend(top->array.val, obj);
		data->state = STATE_ARRAY_OBJECT;
		return;
	}
	if(top->type == PLIST_OBJECT_DICT) {
		g_hash_table_insert(top->dict.val, g_queue_pop_head(data->key_stack), obj);
		data->state = STATE_DICT_KEY;
		return;
	}

	g_assert_not_reached();
}

/* Callback for processing an opening XML element */
static void
plist_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error)
{
	ParseData *data = (ParseData *)user_data;

	switch(data->state) {
	case STATE_PLIST:
	    /* Make sure the element is <plist> and its version is 1.0; then start
	    expecting PlistObjects */
		check_plist_element(element_name, attribute_names, attribute_values, error);
		data->state = STATE_ROOT_OBJECT;
		return;
	
	case STATE_ROOT_OBJECT:
	case STATE_ARRAY_OBJECT:
	case STATE_DICT_OBJECT:
	    /* Error if we are already in the middle of parsing an object */
		if(data->current) {
			g_set_error(error, PLIST_ERROR, PLIST_ERROR_UNEXPECTED_OBJECT, _("Unexpected object <%s>"), element_name);
			return;
		}
		
		if((data->current = start_new_object(element_name, error)) == NULL)
			return;

		/* If the element being opened is an <array> or <dict> object, push it
		onto the stack and expect a new object (if in an <array>) or <key> (if 
		<dict>) */
		if(data->current->type == PLIST_OBJECT_ARRAY) {
			g_queue_push_head(data->container_stack, data->current);
			data->current = NULL;
			data->state = STATE_ARRAY_OBJECT;
		} else if(data->current->type == PLIST_OBJECT_DICT) {
			g_queue_push_head(data->container_stack, data->current);
			data->current = NULL;
			data->state = STATE_DICT_KEY;
		}
		
		return;
		
	case STATE_DICT_KEY:
	    /* If we are expecting <key>, then error on anything else */
		if(!str_eq(element_name, "key"))
			g_set_error(error, PLIST_ERROR, PLIST_ERROR_MISSING_KEY, _("Missing <key> for object <%s> in <dict>"), element_name);
		return;
	}
	g_assert_not_reached();
}

/* Callback for processing the content of an XML element */
static void
plist_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	ParseData *data = (ParseData *)user_data;
	const gchar *name = g_markup_parse_context_get_element(context);
	
	switch(data->state) {
	case STATE_PLIST:
		/* ignore whitespace */
		return;
	
	case STATE_ROOT_OBJECT:
	case STATE_ARRAY_OBJECT:
	case STATE_DICT_OBJECT:
	    /* The content contains the object's value, assign it to the object */
		fill_object(data->current, name, text, error);
		return;
	
	case STATE_DICT_KEY:
		if(str_eq(name, "key"))
			g_queue_push_head(data->key_stack, g_strdup(text));
		/* otherwise ignore whitespace */
		return;
	}
	g_assert_not_reached();
}

/* Callback for parsing a closing XML element */
static void
plist_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error)
{
	ParseData *data = (ParseData *)user_data;
	PlistObject *top;
	
	switch(data->state) {
	case STATE_PLIST:
		return;

	case STATE_ROOT_OBJECT:
		data->retval = data->current;
		data->current = NULL;
		data->state = STATE_PLIST;
		if(data->retval == NULL)
			g_set_error(error, PLIST_ERROR, PLIST_ERROR_NO_ELEMENTS, _("No objects found within <plist> root element"));
		return;	
	
	case STATE_ARRAY_OBJECT:
		/* Handle </array> */
		if(str_eq(element_name, "array")) {
			insert_top_object(data);
			return;
		}
		
		/* Add the completed object to the array */
		top = g_queue_peek_head(data->container_stack);
		top->array.val = g_list_prepend(top->array.val, data->current);
		data->current = NULL;
		return;
		
	case STATE_DICT_OBJECT:
		/* Add the completed object to the dict */
		top = g_queue_peek_head(data->container_stack);
		g_hash_table_insert(top->dict.val, g_queue_pop_head(data->key_stack), data->current);
		data->current = NULL;
		data->state = STATE_DICT_KEY;
		return;
		
	case STATE_DICT_KEY:
		/* Handle </dict> */
		if(str_eq(element_name, "dict")) {
			insert_top_object(data);
			return;
		}
		
		/* Otherwise it was </key>; now expect the corresponding object */
		data->state = STATE_DICT_OBJECT;
		return;
	}
	g_assert_not_reached();
}

/* Pre-2.18 implementation of plist_read_from_string() */
PlistObject *
plist_read_from_string_real(const gchar *string, GError **error)
{
	GMarkupParseContext *context;
	ParseData *parse_data;
	PlistObject *retval;

	g_return_val_if_fail(string != NULL, NULL);
	g_return_val_if_fail(error == NULL || *error == NULL, NULL);
	
	parse_data = parse_data_new();
	context = g_markup_parse_context_new(&plist_parser, G_MARKUP_PREFIX_ERROR_POSITION, parse_data, (GDestroyNotify)parse_data_free);
	if(!g_markup_parse_context_parse(context, string, -1, error) || !g_markup_parse_context_end_parse(context, error)) {
		plist_object_free(parse_data->retval);
		g_markup_parse_context_free(context);
		return NULL;
	}
	
	retval = parse_data->retval;
	g_markup_parse_context_free(context);
	return retval;
}
