/* Copyright 2009, 2011, 2012 P. F. Chimento
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

#include <stdarg.h>
#include <glib.h>
#include <gio/gio.h>
#include <config.h>
#include <glib/gi18n-lib.h>
#include <osxcart/plist.h>
#include "init.h"

/**
 * SECTION:plist
 * @short_description: Tools for manipulating property lists
 * @stability: Unstable
 * @include: osxcart/plist.h
 *
 * Property lists are used in Mac OS X, NeXTSTEP, and GNUstep to store 
 * serialized objects. Mac OS X uses an XML format to store property lists in
 * files with the extension <quote>.plist</quote>. This module reads and writes
 * property lists in the XML format. For more information on the format, see the
 * <ulink 
 * link="http://developer.apple.com/documentation/Darwin/Reference/ManPages/man5/plist.5.html">
 * Apple developer documentation</ulink>.
 * Instead of deserializing the property list into Core Foundation types as in
 * Mac OS X, the property list is represented using a hierarchical structure of
 * #PlistObject<!---->s, lightweight objects that can contain any type of data.
 *
 * Each property list object type has a corresponding #PlistObject structure.
 * For completeness, the data types are listed here:
 *
 * <informaltable>
 *   <tgroup cols='3'>
 *     <thead>
 *       <row>
 *         <entry>XML Element</entry>
 *         <entry>Core Foundation data type</entry>
 *         <entry>#PlistObject struct</entry>
 *       </row>
 *     </thead>
 *     <tbody>
 *       <row>
 *         <entry><code>true</code>, <code>false</code></entry>
 *         <entry><code>CFBoolean</code></entry>
 *         <entry>#PlistObjectBoolean</entry>
 *       </row>
 *       <row>
 *         <entry><code>integer</code></entry>
 *         <entry><code>CFNumber</code></entry>
 *         <entry>#PlistObjectInteger</entry>
 *       </row>
 *       <row>
 *         <entry><code>real</code></entry>
 *         <entry><code>CFNumber</code></entry>
 *         <entry>#PlistObjectReal</entry>
 *       </row>
 *       <row>
 *         <entry><code>string</code></entry>
 *         <entry><code>CFString</code></entry>
 *         <entry>#PlistObjectString</entry>
 *       </row>
 *       <row>
 *         <entry><code>date</code></entry>
 *         <entry><code>CFDate</code></entry>
 *         <entry>#PlistObjectDate</entry>
 *       </row>
 *       <row>
 *         <entry><code>data</code></entry>
 *         <entry><code>CFData</code></entry>
 *         <entry>#PlistObjectData</entry>
 *       </row>
 *       <row>
 *         <entry><code>array</code></entry>
 *         <entry><code>CFArray</code></entry>
 *         <entry>#PlistObjectArray</entry>
 *       </row>
 *       <row>
 *         <entry><code>dict</code></entry>
 *         <entry><code>CFDictionary</code></entry>
 *         <entry>#PlistObjectDict</entry>
 *       </row>
 *     </tbody>
 *   </tgroup>
 * </informaltable>
 */

/**
 * plist_error_quark:
 *
 * The error domain for property list errors.
 *
 * Returns: (transfer none): The string <quote>plist-error-quark</quote> as a
 * <link linkend="GQuark">GQuark</link>.
 */
GQuark
plist_error_quark(void)
{
	osxcart_init();
	return g_quark_from_static_string("plist-error-quark");
}

G_DEFINE_BOXED_TYPE(PlistObject, plist_object, plist_object_copy, plist_object_free);

/**
 * plist_object_new:
 * @type: The type of #PlistObject to create.
 * 
 * Allocates a #PlistObject, with its value initialized to zero in whatever way
 * is appropriate for @type.
 *
 * Returns: (transfer full): a newly-allocated #PlistObject.
 */
PlistObject *
plist_object_new(const PlistObjectType type)
{
	PlistObject *retval;

	osxcart_init();
	
	retval = g_slice_new0(PlistObject);
	retval->type = type;
	
	switch(type) {
		case PLIST_OBJECT_REAL:
			/* Explicitly initialize PLIST_OBJECT_REAL to 0.0, since that might
			not	be all zero bytes */
			retval->real.val = 0.0;
			break;
		case PLIST_OBJECT_DICT:
			/* New GHashTable, with strings as keys, and destroy notify 
			functions so the keys and values get freed automatically when 
			destroyed */
			retval->dict.val = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)plist_object_free);
			break;
		default:
		    ;
	}
	
	return retval;
}

/* Helper function: for each element of a PlistObjectArray, copy the element and
insert it into a new list */
void
insert_copied_element(PlistObject *object, GList **new_list)
{
	*new_list = g_list_prepend(*new_list, plist_object_copy(object));
}

/* Helper function: for each key and value of a PlistObjectDict, copy them and
insert them into a new dict */
void
insert_copied_key_and_value(char *key, PlistObject *value, GHashTable *new_dict)
{
	g_hash_table_insert(new_dict, g_strdup(key), plist_object_copy(value));
}

/**
 * plist_object_copy: (skip):
 * @object: The #PlistObject to copy.
 *
 * Makes a copy of a #PlistObject.
 *
 * Returns: (transfer full): a newly-allocated #PlistObject.
 */
PlistObject *
plist_object_copy(PlistObject *object)
{
	PlistObject *retval;

	g_return_val_if_fail(object != NULL, NULL);

	retval = plist_object_new(object->type);

	switch(object->type) {
		case PLIST_OBJECT_BOOLEAN:
			retval->boolean.val = object->boolean.val;
			break;
		case PLIST_OBJECT_REAL:
			retval->real.val = object->real.val;
			break;
		case PLIST_OBJECT_INTEGER:
			retval->integer.val = object->integer.val;
			break;
		case PLIST_OBJECT_STRING:
			retval->string.val = g_strdup(object->string.val);
			break;
		case PLIST_OBJECT_DATE:
			retval->date.val = object->date.val;
			break;
		case PLIST_OBJECT_ARRAY:
			retval->array.val = NULL;
			g_list_foreach(object->array.val, (GFunc)insert_copied_element, &(retval->array.val));
			retval->array.val = g_list_reverse(retval->array.val);
			break;
		case PLIST_OBJECT_DICT:
			g_hash_table_foreach(object->dict.val, (GHFunc)insert_copied_key_and_value, retval->dict.val);
			break;
		case PLIST_OBJECT_DATA:
			retval->data.length = object->data.length;
			retval->data.val = g_malloc(retval->data.length);
			memcpy(retval->data.val, object->data.val, object->data.length);
			break;
		default:
			g_assert_not_reached();
	}

	return retval;
}

/**
 * plist_object_free: (skip):
 * @object: The #PlistObject to free.
 *
 * Deallocates a #PlistObject. If @object is a container type, also deallocates
 * all of the #PlistObject<!---->s inside it.
 */
void
plist_object_free(PlistObject *object)
{
	osxcart_init();
	
	if(object == NULL)
		return;
	
	switch(object->type) {
		case PLIST_OBJECT_STRING:
			g_free(object->string.val);
			break;
		case PLIST_OBJECT_ARRAY:
			g_list_foreach(object->array.val, (GFunc)plist_object_free, NULL);
			g_list_free(object->array.val);
			break;
		case PLIST_OBJECT_DICT:
			g_hash_table_destroy(object->dict.val);
			break;
		case PLIST_OBJECT_DATA:
			g_free(object->data.val);
			break;
		default:
		    ;
	}
	
	g_slice_free(PlistObject, object);
}

/**
 * plist_object_get_boolean:
 * @object: a #PlistObject holding a boolean
 *
 * This function is intended for bindings to other programming languages; in C,
 * you can simply use <code>object->boolean.val</code>.
 *
 * Returns: the boolean value held by @object.
 */
gboolean
plist_object_get_boolean(PlistObject *object)
{
	return object->boolean.val;
}

/**
 * plist_object_get_real:
 * @object: a #PlistObject holding a real value
 *
 * This function is intended for bindings to other programming languages; in C,
 * you can simply use <code>object->real.val</code>.
 *
 * Returns: the real value held by @object.
 */
double
plist_object_get_real(PlistObject *object)
{
	return object->real.val;
}

/**
 * plist_object_get_integer:
 * @object: a #PlistObject holding an integer
 *
 * This function is intended for bindings to other programming languages; in C,
 * you can simply use <code>object->integer.val</code>.
 *
 * Returns: the integer value held by @object.
 */
int
plist_object_get_integer(PlistObject *object)
{
	return object->integer.val;
}

/**
 * plist_object_get_string:
 * @object: a #PlistObject holding a string
 *
 * This function is intended for bindings to other programming languages; in C,
 * you can simply use <code>object->string.val</code>.
 *
 * Returns: (transfer none): the string value held by @object.
 */
const char *
plist_object_get_string(PlistObject *object)
{
	return object->string.val;
}

/* See explanation in plist.h */
#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#pragma GCC diagnostic push
#endif /* GCC 4.6 */
#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3))
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#endif /* GCC 4.3 */
/**
 * plist_object_get_date:
 * @object: a #PlistObject holding a date
 *
 * This function is intended for bindings to other programming languages; in C,
 * you can simply use <code>object->date.val</code>.
 *
 * Returns: (transfer none): the date value held by @object as a #GTimeVal.
 */
const GTimeVal
plist_object_get_date(PlistObject *object)
{
	return object->date.val;
}
#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#pragma GCC diagnostic pop
#endif /* GCC 4.6 */

/**
 * plist_object_get_array:
 * @object: a #PlistObject holding an array
 *
 * This function is intended for bindings to other programming languages; in C,
 * you can simply use <code>object->array.val</code>.
 *
 * Returns: (transfer none) (element-type PlistObject): the #GList held by
 * @object.
 */
GList *
plist_object_get_array(PlistObject *object)
{
	return object->array.val;
}

/**
 * plist_object_get_dict:
 * @object: a #PlistObject holding a dictionary
 *
 * This function is intended for bindings to other programming languages; in C,
 * you can simply use <code>object->dict.val</code>.
 *
 * Returns: (transfer none) (element-type char* PlistObject): the #GHashTable
 * held by @object.
 */
GHashTable *
plist_object_get_dict(PlistObject *object)
{
	return object->dict.val;
}

/**
 * plist_object_get_data:
 * @object: a #PlistObject holding binary data
 * @length: (out): return location for the length of the data
 *
 * This function is intended for bindings to other programming languages; in C,
 * you can simply use <code>object->data.val</code> and
 * <code>object->data.length</code>.
 *
 * Returns: (transfer none) (array length=length): a pointer to the data held by
 * @object
 */
unsigned char *
plist_object_get_data(PlistObject *object, size_t *length)
{
	g_return_val_if_fail(length != NULL, NULL);
	*length = object->data.length;
	return object->data.val;
}

/**
 * plist_object_lookup:
 * @tree: The root object of the plist
 * @...: A path consisting of dictionary keys and array indices, terminated
 * by -1
 *
 * Convenience function for looking up an object that exists at a certain path
 * within the plist. The variable argument list can consist of either strings 
 * (dictionary keys, if the object at that point in the path is a dict) or 
 * integers (array indices, if the object at that point in the path is an 
 * array.) 
 * 
 * The variable argument list must be terminated by -1. 
 * 
 * For example, given the following plist: 
 * |[&lt;plist version="1.0"&gt; 
 * &lt;dict&gt; 
 *   &lt;key&gt;Array&lt;/key&gt; 
 *   &lt;array&gt; 
 *     &lt;integer&gt;1&lt;/integer&gt;
 *     &lt;string&gt;2&lt;/string&gt;
 *     &lt;real&gt;3.0&lt;/real&gt; 
 *   &lt;/array&gt; 
 *   &lt;key&gt;Dict&lt;/key&gt;
 *   &lt;dict&gt;
 *     &lt;key&gt;Integer&lt;/key&gt;
 *     &lt;integer&gt;1&lt;/integer&gt;
 *     &lt;key&gt;Real&lt;/key&gt;
 *     &lt;real&gt;2.0&lt;/real&gt;
 *     &lt;key&gt;String&lt;/key&gt;
 *     &lt;string&gt;3&lt;/string&gt;
 *   &lt;/dict&gt;
 * &lt;/plist&gt;]| 
 * then the following code: 
 * |[PlistObject *obj1 = plist_object_lookup(plist, "Array", 0, -1); 
 * PlistObject *obj2 = plist_object_lookup(plist, "Dict", "Integer", -1);]| 
 * will place in @obj1 and @obj2 two identical #PlistObjects containing
 * the integer 1, although they will both point to two different spots in the
 * @plist tree. 
 * 
 * Returns: (transfer none): The requested #PlistObject, or %NULL if the path
 * did not exist. The returned object is a pointer to the object within the
 * original @tree, and is not copied. Therefore, it should not be freed
 * separately from @tree.
 */
PlistObject *
plist_object_lookup(PlistObject *tree, ...)
{
	g_return_val_if_fail(tree, NULL);
	
	va_list ap;
	gpointer arg;
	
	va_start(ap, tree);
	for(arg = va_arg(ap, gpointer); GPOINTER_TO_INT(arg) != -1; arg = va_arg(ap, gpointer)) {
		if(tree->type == PLIST_OBJECT_DICT)
			tree = g_hash_table_lookup(tree->dict.val, (const gchar *)arg);
		else if(tree->type == PLIST_OBJECT_ARRAY)
			tree = g_list_nth_data(tree->array.val, GPOINTER_TO_UINT(arg));
		else {
			g_critical("%s: %s", __func__, _("Tried to look up a child of an "
				"object that wasn't a dict or array"));
			return tree;
		}
		/* Return NULL if one of the keys or indices wasn't found */
		if(tree == NULL)
			break;
	}
	va_end(ap);
	
	return tree;
}

