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

#include <stdarg.h>
#include <glib.h>
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
 * Returns: The string <quote>plist-error-quark</quote> as a <link 
 * linkend="GQuark">GQuark</link>.
 */
GQuark
plist_error_quark(void)
{
	osxcart_init();
	return g_quark_from_static_string("plist-error-quark");
}

/**
 * plist_object_new:
 * @type: The type of #PlistObject to create.
 * 
 * Allocates a #PlistObject, with its value initialized to zero in whatever way
 * is appropriate for @type.
 *
 * Returns: a newly-allocated #PlistObject.
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

/**
 * plist_object_free:
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
 * plist_object_lookup:
 * @tree: The root object of the plist
 * @Varargs: A path consisting of dictionary keys and array indices, terminated
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
 * Returns: The requested #PlistObject, or %NULL if the path did not exist. The 
 * returned object is a pointer to the object within the original @tree, and is 
 * not copied. Therefore, it should not be freed separately from @tree.
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

