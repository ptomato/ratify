#include <glib.h>
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
			/* Explicitly initialize PLIST_OBJECT_REAL to 0.0, since that might not
			be all zero bytes */
			retval->real.val = 0.0;
			break;
		case PLIST_OBJECT_DICT:
			/* New GHashTable, with strings as keys, and destroy notify functions so
			the keys and values get freed automatically when destroyed */
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

