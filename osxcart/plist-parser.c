#include <stdlib.h>
#include <glib.h>
#include <config.h>
#include <glib/gi18n-lib.h>
#include <osxcart/plist.h>
#include "plist-common.h"

typedef enum {
	STATE_ROOT_OBJECT,
	STATE_ARRAY_OBJECT,
	STATE_DICT_OBJECT
} ParserState;

typedef struct {
	ParserState state;
	GList *array;
	GHashTable *dict;
	gchar *key;	
	PlistObject *current;
} ParseData;

/* Forward declarations */
static void element_start(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error);
static void element_end(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error);
static void element_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error);
static void plist_start(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error);
static void plist_end(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error);

static GMarkupParser element_parser = { 
	element_start, element_end, element_text, NULL, NULL
};

static GMarkupParser plist_parser = { 
	plist_start, plist_end, NULL, NULL, NULL
};

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
	
	/* array - create a new ParseData with a parent array and push this parser
	onto the context once more with the new ParseData */
	if(str_eq(element_name, "array")) {
		ParseData *new_data = g_slice_new0(ParseData);
		new_data->state = STATE_ARRAY_OBJECT;
		new_data->array = data->current->array.val;
		g_markup_parse_context_push(context, &element_parser, new_data);
	}
	
	/* dict - create a new ParseData with a parent hashtable and push this
	parser onto the context once more with the new ParseData */
	else if(str_eq(element_name, "dict")) {
		ParseData *new_data = g_slice_new0(ParseData);
		new_data->state = STATE_DICT_OBJECT;
		new_data->dict = data->current->dict.val;
		g_markup_parse_context_push(context, &element_parser, new_data);
	}

	/* key - invalid if not in a dict */
	else if(str_eq(element_name, "key")) {
		if(data->state != STATE_DICT_OBJECT)
			g_set_error(error, PLIST_ERROR, PLIST_ERROR_EXTRANEOUS_KEY, _("<key> element found outside of <dict>"));
	} 
}	

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


PlistObject *
plist_read_from_string_real(const gchar *string, GError **error)
{
	GMarkupParseContext *context;
	PlistObject *plist = NULL;

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

