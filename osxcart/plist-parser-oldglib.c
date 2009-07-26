#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <osxcart/plist.h>
#include "plist-common.h"

typedef enum {
	STATE_PLIST,
	STATE_ROOT_OBJECT,
	STATE_ARRAY_OBJECT,
	STATE_DICT_OBJECT,
	STATE_DICT_KEY
} ParserState;

typedef struct {
	ParserState state;
	PlistObject *current;
	PlistObject *retval;
	GQueue *container_stack;
	GQueue *key_stack;
} ParseData;

/* Forward declarations */
static void plist_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error);
static void plist_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error);
static void plist_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error);

static GMarkupParser plist_parser = { 
	plist_start_element, plist_end_element, plist_text, NULL, NULL
};

static ParseData *
parse_data_new(void)
{
	ParseData *data = g_slice_new0(ParseData);
	data->state = STATE_PLIST;
	data->container_stack = g_queue_new();
	data->key_stack = g_queue_new();
	return data;
}

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

static void
insert_top_object(ParseData *data)
{
	PlistObject *obj = g_queue_pop_head(data->container_stack);
	PlistObject *top;
	
	/* Reverse the array now that we are done filling it */
	if(obj->type == PLIST_OBJECT_ARRAY)
		obj->array.val = g_list_reverse(obj->array.val);
	
	if(g_queue_is_empty(data->container_stack)) {
		data->retval = obj;
		data->state = STATE_PLIST;
		return;
	}
	
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

static void
plist_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error)
{
	ParseData *data = (ParseData *)user_data;

	switch(data->state) {
	case STATE_PLIST:
		check_plist_element(element_name, attribute_names, attribute_values, error);
		data->state = STATE_ROOT_OBJECT;
		return;
	
	case STATE_ROOT_OBJECT:
	case STATE_ARRAY_OBJECT:
	case STATE_DICT_OBJECT:
		if(data->current) {
			g_set_error(error, PLIST_ERROR, PLIST_ERROR_UNEXPECTED_ELEMENT, _("Unexpected object <%s>"), element_name);
			return;
		}
		
		if((data->current = start_new_object(element_name, error)) == NULL)
			return;

		/* If the element being opened is an array or dict object, push it on
		to the stack and expect a new object or key */
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
		if(!str_eq(element_name, "key"))
			g_set_error(error, PLIST_ERROR, PLIST_ERROR_MISSING_KEY, _("Missing <key> for object <%s> in <dict>"), element_name);
		return;
	}
	g_assert_not_reached();
}

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
		
		top = g_queue_peek_head(data->container_stack);
		top->array.val = g_list_prepend(top->array.val, data->current);
		data->current = NULL;
		return;
		
	case STATE_DICT_OBJECT:
		/* Add object to dict */
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
		
		/* Otherwise it was </key> */
		data->state = STATE_DICT_OBJECT;
		return;
	}
	g_assert_not_reached();
}

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
