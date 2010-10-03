#ifndef __OSXCART_PLIST_H__
#define __OSXCART_PLIST_H__

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

#include <glib.h>

G_BEGIN_DECLS

/**
 * PlistObjectType:
 *
 * The type of value stored in a #PlistObject.
 */
typedef enum {
	PLIST_OBJECT_BOOLEAN,
	PLIST_OBJECT_REAL,
	PLIST_OBJECT_INTEGER,
	PLIST_OBJECT_STRING,
	PLIST_OBJECT_DATE,
	PLIST_OBJECT_ARRAY,
	PLIST_OBJECT_DICT,
	PLIST_OBJECT_DATA
} PlistObjectType;

/**
 * PlistObjectBoolean:
 * @type: Must be %PLIST_OBJECT_BOOLEAN
 * @val: The value
 *
 * A #PlistObject which contains a boolean, similar to #CFBoolean.
 */
typedef struct _PlistObjectBoolean {
	PlistObjectType type;
	gboolean val;
} PlistObjectBoolean;

/**
 * PlistObjectReal:
 * @type: Must be %PLIST_OBJECT_REAL
 * @val: The value
 *
 * A #PlistObject which contains a double-precision floating point number.
 * #CFNumber is used to represent these in CoreFoundation.
 */
typedef struct _PlistObjectReal {
	PlistObjectType type;
	gdouble val;
} PlistObjectReal;

/**
 * PlistObjectInteger:
 * @type: Must be %PLIST_OBJECT_INTEGER
 * @val: The value
 *
 * A #PlistObject which contains an integer. #CFNumber is used to represent
 * these in CoreFoundation.
 */
typedef struct _PlistObjectInteger {
	PlistObjectType type;
	gint val;
} PlistObjectInteger;

/**
 * PlistObjectString:
 * @type: Must be %PLIST_OBJECT_STRING
 * @val: The string
 *
 * A #PlistObject which contains a string, similar to #CFString.
 */
typedef struct _PlistObjectString {
	PlistObjectType type;
	gchar *val;
} PlistObjectString;

/**
 * PlistObjectDate:
 * @type: Must be %PLIST_OBJECT_DATE
 * @val: The date
 *
 * A #PlistObject which contains a date in GLib's timeval format, similar to 
 * #CFDate.
 */
typedef struct _PlistObjectDate {
	PlistObjectType type;
	GTimeVal val;
} PlistObjectDate;

/**
 * PlistObjectArray:
 * @type: Must be %PLIST_OBJECT_ARRAY
 * @val: A list of #PlistObject<!---->s
 *
 * A #PlistObject which contains any number of child #PlistObject<!---->s, 
 * similar to #CFArray.
 */
typedef struct _PlistObjectArray {
	PlistObjectType type;
	GList *val;
} PlistObjectArray;

/**
 * PlistObjectDict:
 * @type: Must be %PLIST_OBJECT_DICT
 * @val: A hash table of #PlistObject<!---->s
 *
 * A #PlistObject which contains a dictionary of child #PlistObject<!---->s 
 * accessed by string keys, similar to #CFDictionary.
 */
typedef struct _PlistObjectDict {
	PlistObjectType type;
	GHashTable *val;
} PlistObjectDict;

/**
 * PlistObjectData:
 * @type: Must be %PLIST_OBJECT_DATA
 * @val: The data
 * @length: The length of the data
 *
 * A #PlistObject which contains arbitary binary data, similar to #CFData.
 */
typedef struct _PlistObjectData {
	PlistObjectType type;
	guchar *val;
	gsize length;
} PlistObjectData;

/**
 * PlistObject:
 * 
 * The #PlistObject type is a union of all the types that can be stored in a
 * property list. It is similar to a #NSValue or #CFValue. In this library,
 * #PlistObject<!---->s provide a lightweight interface for storing and 
 * manipulating property lists so that the library doesn't have to depend on 
 * CoreFoundation.
 *
 * Similar to <link linkend="GdkEvent">GdkEvent</link>, the @type field is
 * always the first member of the structure, so that the type of value can
 * always be determined.
 *
 * Here is an example:
 * |[PlistObject *obj;
 * if(obj->type == PLIST_OBJECT_ARRAY)
 *     g_list_foreach(obj->array.val, some_function, NULL);]|
 */
typedef union _PlistObject {
	PlistObjectType    type;
	PlistObjectBoolean boolean;
	PlistObjectReal    real;
	PlistObjectInteger integer;
	PlistObjectString  string;
	PlistObjectDate    date;
	PlistObjectArray   array;
	PlistObjectDict    dict;
	PlistObjectData    data;
} PlistObject;

/**
 * PlistError:
 *
 * The different error codes which can be thrown in the #PLIST_ERROR domain.
 */
typedef enum {
	PLIST_ERROR_FAILED,            /* Generic error */
	PLIST_ERROR_BAD_VERSION,       /* The plist was an incompatible version */
	PLIST_ERROR_UNEXPECTED_OBJECT, /* An object was out of place in the plist */
	PLIST_ERROR_EXTRANEOUS_KEY,    /* A <key> element was encountered outside a
	                                  <dict> object */
	PLIST_ERROR_MISSING_KEY,       /* A <dict> object was missing a <key> */
	PLIST_ERROR_BAD_DATE,          /* A <date> object contained incorrect
	                                  formatting */
	PLIST_ERROR_NO_ELEMENTS        /* The plist was empty */
} PlistError;

/**
 * PLIST_ERROR:
 *
 * The domain of errors raised by property list processing in Osxcart.
 */
#define PLIST_ERROR plist_error_quark()

GQuark plist_error_quark(void);
PlistObject *plist_object_new(const PlistObjectType type);
void plist_object_free(PlistObject *object);
PlistObject *plist_read(const gchar *filename, GError **error);
PlistObject *plist_read_from_string(const gchar *string, GError **error);
gboolean plist_write(PlistObject *plist, const gchar *filename, GError **error);
gchar *plist_write_to_string(PlistObject *plist);

G_END_DECLS

#endif /* __LIBMAC_PLIST_H__ */
