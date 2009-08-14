#include <glib.h>
#include <osxcart/plist.h>

/* This test tries to read a malformed plist, and succeeds if the operation
failed. */
static void
plist_fail_case(gconstpointer filename)
{
	GError *error = NULL;
	PlistObject *plist;
	
	plist = plist_read(filename, &error);
	g_assert(error != NULL);
	g_test_message("Error message: %s", error->message);
	g_error_free(error);
	g_assert(plist == NULL);
}

/* This test tries to read a plist, and succeeds if the operation succeeded. */
static void
plist_pass_case(gconstpointer filename)
{
	GError *error = NULL;
	PlistObject *plist;
	
	plist = plist_read(filename, &error);
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
plist_compare_case(gconstpointer filename)
{
	GError *error = NULL;
	PlistObject *plist;
	gchar *correctstring, *actualstring;
	
	plist = plist_read(filename, &error);
	g_assert(error == NULL);
	g_assert(g_file_get_contents(filename, &correctstring, NULL, &error));
	g_assert(error == NULL);
	actualstring = plist_write_to_string(plist);
	plist_object_free(plist);
	g_assert_cmpstr(actualstring, ==, correctstring);
	g_free(actualstring);
	g_free(correctstring);
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

/* This function adds the plist tests to the test suite. */
void
add_plist_tests()
{
    const gchar **ptr;
    gchar *testname;
    
    /* Fail cases */
    for(ptr = failcases; *ptr; ptr += 2)
	{
		testname = g_strconcat("/plist/fail/", ptr[0], NULL);
		g_test_add_data_func(testname, ptr[1], plist_fail_case);
		g_free(testname);
	}
	
	/* Pass cases, from Apple's plist man pages */
	for(ptr = passcases; *ptr; ptr += 2)
	{
	    testname = g_strconcat("/plist/pass/", ptr[0], NULL);
	    g_test_add_data_func(testname, ptr[1], plist_pass_case);
	    g_free(testname);
	}
	
	/* Cases where the output should be exactly the same as the input */
	/* Remember to specify <real>s to 14 decimal points and to alphabetize
	<dict> keys */
	for(ptr = passcases; *ptr; ptr += 2)
	{
	    testname = g_strconcat("/plist/compare/", ptr[0], NULL);
	    g_test_add_data_func(testname, ptr[1], plist_compare_case);
	    g_free(testname);
    }
}
