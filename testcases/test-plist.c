#include <string.h>
#include <glib.h>
#include <osxcart/plist.h>

/* Convenience function */
static gchar *
build_filename(const gchar *name)
{
    return g_build_filename(TESTFILEDIR, name, NULL);
}

/* This test tries to read a malformed plist, and succeeds if the operation
failed. */
static void
plist_fail_case(gconstpointer name)
{
	GError *error = NULL;
	PlistObject *plist;
	gchar *filename = build_filename(name);
	
	plist = plist_read(filename, &error);
	g_free(filename);
	g_assert(error != NULL);
	g_test_message("Error message: %s", error->message);
	g_error_free(error);
	g_assert(plist == NULL);
}

/* This test tries to read a plist, and succeeds if the operation succeeded. */
static void
plist_pass_case(gconstpointer name)
{
	GError *error = NULL;
	PlistObject *plist;
	gchar *filename = build_filename(name);
	
	plist = plist_read(filename, &error);
	g_free(filename);
	g_assert(error == NULL);
	g_assert(plist != NULL);
	plist_object_free(plist);
}

/* This test reads a plist and writes it to a string, and compares that string
to the original contents of the plist file. It succeeds if the two strings
compare exactly. It fails if the read operation fails, or if the file contents
could not be read, or if the two strings are different.
In this implementation of writing plists, <real> elements are specified to 14
decimal places, and <dict> entries are alphabetized by their <key>. The input
file must also follow these conventions if the two strings are to be equal. */
static void
plist_compare_case(gconstpointer name)
{
	GError *error = NULL;
	PlistObject *plist;
	gchar *correctstring, *actualstring, *filename = build_filename(name);
	
	plist = plist_read(filename, &error);
	g_assert(error == NULL);
	g_assert(g_file_get_contents(filename, &correctstring, NULL, &error));
	g_free(filename);
	g_assert(error == NULL);
	actualstring = plist_write_to_string(plist);
	plist_object_free(plist);
	g_assert_cmpstr(actualstring, ==, correctstring);
	g_free(actualstring);
	g_free(correctstring);
}

static void
plist_lookup_test()
{
	GError *error = NULL;
	PlistObject *list = NULL, *obj = NULL;
	char *filename = build_filename("oneofeach.plist");

	list = plist_read(filename, &error);
	g_free(filename);
	g_assert(error == NULL);
	g_assert(list);
	
	/* Look up the list itself */
	obj = plist_object_lookup(list, -1);
	g_assert(obj == list);
	
	/* Look up the first element of the "Array" key */
	obj = plist_object_lookup(list, "Array", 0, -1);
	g_assert_cmpint(obj->integer.val, ==, 1);
	
	/* Look up the "String" key of the "Dict" key */
	obj = plist_object_lookup(list, "Dict", "String", -1);
	g_assert_cmpstr(obj->string.val, ==, "3");
	
	/* Look up the "True value" key */
	obj = plist_object_lookup(list, "True value", -1);
	g_assert(obj->boolean.val);
	
	plist_object_free(list);
}

/* Copy the "one-of-each" plist and test that each object was copied */
static void
plist_copy_test()
{
	GError *error = NULL;
	PlistObject *list = NULL, *copy = NULL, *obj = NULL;
	char *filename = build_filename("oneofeach.plist");

	list = plist_read(filename, &error);
	g_free(filename);
	g_assert(error == NULL);
	g_assert(list);

	copy = plist_object_copy(list);
	plist_object_free(list);
	g_assert(copy);

	obj = plist_object_lookup(copy, "Array", -1);
	g_assert(obj->type == PLIST_OBJECT_ARRAY);

	obj = plist_object_lookup(copy, "Array", 0, -1);
	g_assert(obj->type == PLIST_OBJECT_INTEGER);
	g_assert_cmpint(obj->integer.val, ==, 1);

	obj = plist_object_lookup(copy, "Array", 1, -1);
	g_assert(obj->type == PLIST_OBJECT_STRING);
	g_assert_cmpstr(obj->string.val, ==, "2");

	obj = plist_object_lookup(copy, "Array", 2, -1);
	g_assert(obj->type == PLIST_OBJECT_REAL);
	g_assert_cmpfloat(obj->real.val, ==, 3.0);

	obj = plist_object_lookup(copy, "Data", -1);
	g_assert(obj->type == PLIST_OBJECT_DATA);
	g_assert_cmpint(obj->data.length, ==, 5);
	g_assert(strncmp((const char *)obj->data.val, "sure.", 5) == 0);

	obj = plist_object_lookup(copy, "Date", -1);
	g_assert(obj->type == PLIST_OBJECT_DATE);
	g_assert_cmpint(obj->date.val.tv_sec, ==, 1240436323);
	g_assert_cmpint(obj->date.val.tv_usec, ==, 501773);

	obj = plist_object_lookup(copy, "Dict", -1);
	g_assert(obj->type == PLIST_OBJECT_DICT);

	obj = plist_object_lookup(copy, "Dict", "Integer", -1);
	g_assert(obj->type == PLIST_OBJECT_INTEGER);
	g_assert_cmpint(obj->integer.val, ==, 1);

	obj = plist_object_lookup(copy, "Dict", "Real", -1);
	g_assert(obj->type == PLIST_OBJECT_REAL);
	g_assert_cmpfloat(obj->real.val, ==, 2.0);

	obj = plist_object_lookup(copy, "Dict", "String", -1);
	g_assert(obj->type == PLIST_OBJECT_STRING);
	g_assert_cmpstr(obj->string.val, ==, "3");

	obj = plist_object_lookup(copy, "False value", -1);
	g_assert(obj->type == PLIST_OBJECT_BOOLEAN);
	g_assert(!obj->boolean.val);

	obj = plist_object_lookup(copy, "Integer", -1);
	g_assert(obj->type == PLIST_OBJECT_INTEGER);
	g_assert_cmpint(obj->integer.val, ==, -1);

	obj = plist_object_lookup(copy, "Real", -1);
	g_assert(obj->type == PLIST_OBJECT_REAL);
	g_assert_cmpfloat(obj->real.val, ==, 3.14159265358979);

	obj = plist_object_lookup(copy, "String", -1);
	g_assert(obj->type == PLIST_OBJECT_STRING);
	g_assert_cmpstr(obj->string.val, ==, "Hello, world");

	obj = plist_object_lookup(copy, "True value", -1);
	g_assert(obj->type == PLIST_OBJECT_BOOLEAN);
	g_assert(obj->boolean.val);

	plist_object_free(copy);
}

/* Test the accessor functions for language bindings */
static void
plist_accessor_test()
{
	GError *error = NULL;
	PlistObject *list = NULL, *obj = NULL;
	const unsigned char *data;
	size_t length;
	GTimeVal timeval;
	char *filename = build_filename("oneofeach.plist");

	list = plist_read(filename, &error);
	g_free(filename);
	g_assert(error == NULL);
	g_assert(list);

	obj = plist_object_lookup(list, "Array", -1);
	g_assert(plist_object_get_array(obj));

	obj = plist_object_lookup(list, "Array", 0, -1);
	g_assert_cmpint(plist_object_get_integer(obj), ==, 1);

	obj = plist_object_lookup(list, "Array", 1, -1);
	g_assert_cmpstr(plist_object_get_string(obj), ==, "2");

	obj = plist_object_lookup(list, "Array", 2, -1);
	g_assert_cmpfloat(plist_object_get_real(obj), ==, 3.0);

	obj = plist_object_lookup(list, "Data", -1);
	data = plist_object_get_data(obj, &length);
	g_assert_cmpstr((const char *)data, ==, "sure.");
	g_assert_cmpint(length, ==, 5);

	obj = plist_object_lookup(list, "Date", -1);
	timeval = plist_object_get_date(obj);
	g_assert_cmpint(timeval.tv_sec, ==, 1240436323);
	g_assert_cmpint(timeval.tv_usec, ==, 501773);

	obj = plist_object_lookup(list, "Dict", -1);
	g_assert(plist_object_get_dict(obj));

	obj = plist_object_lookup(list, "Dict", "Integer", -1);
	g_assert_cmpint(plist_object_get_integer(obj), ==, 1);

	obj = plist_object_lookup(list, "Dict", "Real", -1);
	g_assert_cmpfloat(plist_object_get_real(obj), ==, 2.0);

	obj = plist_object_lookup(list, "Dict", "String", -1);
	g_assert_cmpstr(plist_object_get_string(obj), ==, "3");

	obj = plist_object_lookup(list, "False value", -1);
	g_assert(!plist_object_get_boolean(obj));

	obj = plist_object_lookup(list, "Integer", -1);
	g_assert_cmpint(plist_object_get_integer(obj), ==, -1);

	obj = plist_object_lookup(list, "Real", -1);
	g_assert_cmpfloat(plist_object_get_real(obj), ==, 3.14159265358979);

	obj = plist_object_lookup(list, "String", -1);
	g_assert_cmpstr(plist_object_get_string(obj), ==, "Hello, world");

	obj = plist_object_lookup(list, "True value", -1);
	g_assert(plist_object_get_boolean(obj));

	plist_object_free(list);
}

/* Test the setter functions */
static void
plist_set_accessor_test()
{
	PlistObject *obj = NULL;
	GHashTable *hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)plist_object_free);

	obj = plist_object_new(PLIST_OBJECT_BOOLEAN);
	plist_object_set_boolean(obj, TRUE);
	g_assert(obj->boolean.val);
	plist_object_free(obj);

	obj = plist_object_new(PLIST_OBJECT_REAL);
	plist_object_set_real(obj, 2.718281828);
	g_assert_cmpfloat(obj->real.val, ==, 2.718281828);
	plist_object_free(obj);

	obj = plist_object_new(PLIST_OBJECT_INTEGER);
	plist_object_set_integer(obj, -1);
	g_assert_cmpint(obj->integer.val, ==, -1);
	plist_object_free(obj);

	obj = plist_object_new(PLIST_OBJECT_DATE);
	plist_object_set_date(obj, (GTimeVal){ 1234567, 123456 });
	g_assert_cmpint(obj->date.val.tv_sec, ==, 1234567);
	g_assert_cmpint(obj->date.val.tv_usec, ==, 123456);
	plist_object_free(obj);

	obj = plist_object_new(PLIST_OBJECT_STRING);
	plist_object_set_string(obj, "Now is the time for all");
	g_assert_cmpstr(obj->string.val, ==, "Now is the time for all");
	plist_object_free(obj);

	obj = plist_object_new(PLIST_OBJECT_ARRAY);
	plist_object_set_array(obj, NULL);
	g_assert_cmpuint(g_list_length(obj->array.val), ==, 0);
	plist_object_free(obj);

	obj = plist_object_new(PLIST_OBJECT_DICT);
	plist_object_set_dict(obj, hash);
	g_assert_cmpuint(g_hash_table_size(obj->dict.val), ==, 0);
	plist_object_free(obj);

	obj = plist_object_new(PLIST_OBJECT_DATA);
	plist_object_set_data(obj, (unsigned char *)"01234567", 5);
	g_assert_cmpuint(obj->data.length, ==, 5);
	g_assert_cmpint(strncmp((char *)obj->data.val, "01234", 5), ==, 0);
	plist_object_free(obj);
}

const gchar *failcases[] = {
    "Nonexistent filename", "",
    "Badly formed XML", "badlyformed.fail.plist",
    "Incomplete XML elements", "incomplete.fail.plist",
    "Incorrect attribute in plist element", "wrong_attribute.fail.plist",
    "Plist version not specified", "missing_version.fail.plist",
    "Invalid plist version", "wrong_version.fail.plist",
    "No enclosing plist element", "no_plist.fail.plist",
    "No objects in plist", "no_data.fail.plist",
    "More than one object outside of a container", "extraneous_object.fail.plist",
    "Missing key element in dict", "missing_key.fail.plist",
    "Key element outside dict", "extraneous_key.fail.plist",
    NULL, NULL
};

const gchar *passcases[] = {
    "Inform 7 settings", "Settings.plist",
    "Inform 7 manifest", "manifest.plist",
    "One of each element", "oneofeach.plist",
    NULL, NULL
};

static void
add_tests(const gchar **testlist, const gchar *path, void (test_func)(gconstpointer))
{
    const gchar **ptr;
    gchar *testname;
    
    for(ptr = testlist; *ptr; ptr += 2)
	{
		testname = g_strconcat(path, ptr[0], NULL);
		g_test_add_data_func(testname, ptr[1], test_func);
		g_free(testname);
	}
}

/* This function adds the plist tests to the test suite. */
void
add_plist_tests()
{
    /* Fail cases */
    add_tests(failcases, "/plist/fail/", plist_fail_case);
	
	/* Pass cases */
	add_tests(passcases, "/plist/pass/", plist_pass_case);
	/* Cases where the output should be exactly the same as the input */
	/* Remember to specify <real>s to 14 decimal points and to alphabetize
	<dict> keys */
	add_tests(passcases, "/plist/compare/", plist_compare_case);
	/* Other cases */
	g_test_add_func("/plist/misc/Lookup test", plist_lookup_test);
	g_test_add_func("/plist/misc/Copy test", plist_copy_test);
	g_test_add_func("/plist/misc/Accessor test", plist_accessor_test);
	g_test_add_func("/plist/misc/Accessor set test", plist_set_accessor_test);
}
