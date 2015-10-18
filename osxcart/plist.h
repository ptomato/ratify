#ifndef __OSXCART_PLIST_H__
#define __OSXCART_PLIST_H__

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

#include <glib.h>
#include <gio/gio.h>
#include <stdarg.h>

G_BEGIN_DECLS

/**
 * PlistObjectType:
 * @PLIST_OBJECT_BOOLEAN: A #PlistObjectBoolean.
 * @PLIST_OBJECT_REAL: A #PlistObjectReal.
 * @PLIST_OBJECT_INTEGER: A #PlistObjectInteger.
 * @PLIST_OBJECT_STRING: A #PlistObjectString.
 * @PLIST_OBJECT_DATE: A #PlistObjectDate.
 * @PLIST_OBJECT_ARRAY: A #PlistObjectArray.
 * @PLIST_OBJECT_DICT: A #PlistObjectDict.
 * @PLIST_OBJECT_DATA: A #PlistObjectData.
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
 * PlistObjectBoolean: (skip)
 * @type: Must be %PLIST_OBJECT_BOOLEAN
 * @val: The value
 *
 * A #PlistObject which contains a boolean, similar to <code>CFBoolean</code>.
 */
typedef struct {
	PlistObjectType type;
	gboolean val;
} PlistObjectBoolean;

/**
 * PlistObjectReal: (skip)
 * @type: Must be %PLIST_OBJECT_REAL
 * @val: The value
 *
 * A #PlistObject which contains a double-precision floating point number.
 * <code>CFNumber</code> is used to represent these in CoreFoundation.
 */
typedef struct {
	PlistObjectType type;
	gdouble val;
} PlistObjectReal;

/**
 * PlistObjectInteger: (skip)
 * @type: Must be %PLIST_OBJECT_INTEGER
 * @val: The value
 *
 * A #PlistObject which contains an integer. <code>CFNumber</code> is used to
 * represent these in CoreFoundation.
 */
typedef struct {
	PlistObjectType type;
	gint val;
} PlistObjectInteger;

/**
 * PlistObjectString: (skip)
 * @type: Must be %PLIST_OBJECT_STRING
 * @val: The string
 *
 * A #PlistObject which contains a string, similar to <code>CFString</code>.
 */
typedef struct {
	PlistObjectType type;
	gchar *val;
} PlistObjectString;

/**
 * PlistObjectDate: (skip)
 * @type: Must be %PLIST_OBJECT_DATE
 * @val: The date
 *
 * A #PlistObject which contains a date in GLib's timeval format, similar to 
 * <code>CFDate</code>.
 */
typedef struct {
	PlistObjectType type;
	GTimeVal val;
} PlistObjectDate;

/**
 * PlistObjectArray: (skip)
 * @type: Must be %PLIST_OBJECT_ARRAY
 * @val: A list of #PlistObject<!---->s
 *
 * A #PlistObject which contains any number of child #PlistObject<!---->s, 
 * similar to <code>CFArray</code>.
 */
typedef struct {
	PlistObjectType type;
	GList *val;
} PlistObjectArray;

/**
 * PlistObjectDict: (skip)
 * @type: Must be %PLIST_OBJECT_DICT
 * @val: A hash table of #PlistObject<!---->s
 *
 * A #PlistObject which contains a dictionary of child #PlistObject<!---->s 
 * accessed by string keys, similar to <code>CFDictionary</code>.
 */
typedef struct {
	PlistObjectType type;
	GHashTable *val;
} PlistObjectDict;

/**
 * PlistObjectData: (skip)
 * @type: Must be %PLIST_OBJECT_DATA
 * @val: The data
 * @length: The length of the data
 *
 * A #PlistObject which contains arbitary binary data, similar to
 * <code>CFData</code>.
 */
typedef struct {
	PlistObjectType type;
	guchar *val;
	gsize length;
} PlistObjectData;

/**
 * PlistObject:
 * @type: The type of value stored in this object.
 * @boolean: The object as a #PlistObjectBoolean.
 * @real: The object as a #PlistObjectReal.
 * @integer: The object as a #PlistObjectInteger.
 * @string: The object as a #PlistObjectString.
 * @date: The object as a #PlistObjectDate.
 * @array: The object as a #PlistObjectArray.
 * @dict: The object as a #PlistObjectDict.
 * @data: The object as a #PlistObjectData.
 * 
 * The #PlistObject type is a union of all the types that can be stored in a
 * property list. It is similar to a <code>NSValue</code> or
 * <code>CFValue</code>, or an extremely lightweight <code>GValue</code>. In
 * this library, #PlistObject<!---->s provide a lightweight interface for
 * storing and manipulating property lists so that the library doesn't have to
 * depend on CoreFoundation.
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
typedef union {
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
 * @PLIST_ERROR_FAILED: A generic error.
 * @PLIST_ERROR_BAD_VERSION: The plist was an incompatible version.
 * @PLIST_ERROR_UNEXPECTED_OBJECT: An object was out of place in the plist.
 * @PLIST_ERROR_EXTRANEOUS_KEY: A <code>&lt;key&gt;</code> element was
 * encountered outside a <code>&lt;dict&gt;</code> object.
 * @PLIST_ERROR_MISSING_KEY: A <code>&lt;dict&gt;</code> object was missing a
 * <code>&lt;key&gt;</code> element.
 * @PLIST_ERROR_BAD_DATE: A <code>&lt;date&gt;</code> object contained incorrect
 * formatting.
 * @PLIST_ERROR_NO_ELEMENTS: The plist was empty.
 *
 * The different error codes which can be thrown in the #PLIST_ERROR domain.
 */
typedef enum {
	PLIST_ERROR_FAILED,
	PLIST_ERROR_BAD_VERSION,
	PLIST_ERROR_UNEXPECTED_OBJECT,
	PLIST_ERROR_EXTRANEOUS_KEY,
	PLIST_ERROR_MISSING_KEY,
	PLIST_ERROR_BAD_DATE,
	PLIST_ERROR_NO_ELEMENTS
} PlistError;

/**
 * PLIST_ERROR:
 *
 * The domain of errors raised by property list processing in Osxcart.
 */
#define PLIST_ERROR plist_error_quark()

#define PLIST_TYPE_OBJECT (plist_object_get_type())

GQuark plist_error_quark(void);
GType plist_object_get_type(void) G_GNUC_CONST;
PlistObject *plist_object_new(const PlistObjectType type);
PlistObject *plist_object_copy(PlistObject *object);
void plist_object_free(PlistObject *object);

gboolean plist_object_get_boolean(PlistObject *object);
double plist_object_get_real(PlistObject *object);
int plist_object_get_integer(PlistObject *object);
const char *plist_object_get_string(PlistObject *object);
/* We silence a warning about ignored const for plist_object_get_date(); the
const has a meaning to the introspection bindings. The push/pop pragmas are only
supported from GCC 4.6 onwards, and the warning option is only supported from
4.3 onwards, so this is quite messy. */
#ifndef __GTK_DOC_IGNORE__
#define GCC_AT_LEAST(maj, min) defined(__GNUC__) && (__GNUC__ > maj || (__GNUC__ == maj && __GNUC_MINOR__ >= min))
#if GCC_AT_LEAST(4, 6) || defined(__clang__)
#pragma GCC diagnostic push
#endif /* GCC 4.6 or Clang */
#if GCC_AT_LEAST(4, 3) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#endif /* GCC 4.3 or Clang */
#endif /* __GTK_DOC_IGNORE__ */
const GTimeVal plist_object_get_date(PlistObject *object);
#ifndef __GTK_DOC_IGNORE__
#if GCC_AT_LEAST(4, 6) || defined(__clang__)
#pragma GCC diagnostic pop
#endif /* GCC 4.6 or Clang*/
#undef GCC_AT_LEAST
#endif /* __GTK_DOC_IGNORE__ */
GList *plist_object_get_array(PlistObject *object);
GHashTable *plist_object_get_dict(PlistObject *object);
unsigned char *plist_object_get_data(PlistObject *object, size_t *length);

void plist_object_set_boolean(PlistObject *object, gboolean val);
void plist_object_set_real(PlistObject *object, double val);
void plist_object_set_integer(PlistObject *object, int val);
void plist_object_set_string(PlistObject *object, const char *val);
void plist_object_set_date(PlistObject *object, GTimeVal val);
void plist_object_set_array(PlistObject *object, GList *val);
void plist_object_set_dict(PlistObject *object, GHashTable *val);
void plist_object_set_data(PlistObject *object, unsigned char *val, size_t length);

PlistObject *plist_object_lookup(PlistObject *tree, ...);
PlistObject *plist_read(const gchar *filename, GError **error);
PlistObject *plist_read_file(GFile *file, GCancellable *cancellable, GError **error);
PlistObject *plist_read_from_string(const gchar *string, GError **error);
gboolean plist_write(PlistObject *plist, const gchar *filename, GError **error);
gboolean plist_write_file(PlistObject *plist, GFile *file, GCancellable *cancellable, GError **error);
gchar *plist_write_to_string(PlistObject *plist);

G_END_DECLS

#endif /* __OSXCART_PLIST_H__ */
