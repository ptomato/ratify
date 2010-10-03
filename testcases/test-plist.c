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
	PlistObject *list, *obj;
	
	list = plist_read("oneofeach.plist", &error);
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
}
