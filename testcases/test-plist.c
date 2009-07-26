#include <glib.h>
#include <osxcart/plist.h>

static void
plist_fail_case(gconstpointer filename)
{
	GError *error = NULL;
	PlistObject *plist = plist_read((const gchar *)filename, &error);
	g_assert(error != NULL);
	g_test_message("Error message: %s", error->message);
	g_error_free(error);
	g_assert(plist == NULL);
}

static void
plist_pass_case(gconstpointer filename)
{
	GError *error = NULL;
	PlistObject *plist = plist_read((const gchar *)filename, &error);
	g_assert(error == NULL);
	g_assert(plist != NULL);
	plist_object_free(plist);
}

static void
plist_compare_case(gconstpointer filename)
{
	GError *error = NULL;
	PlistObject *plist = plist_read((const gchar *)filename, &error);
	g_assert(error == NULL);
	
	gchar *correctstring;
	g_assert(g_file_get_contents((const gchar *)filename, &correctstring, NULL, NULL));
	
	gchar *actualstring = plist_write_to_string(plist);
	plist_object_free(plist);
	g_assert_cmpstr(actualstring, ==, correctstring);
	g_free(actualstring);
	g_free(correctstring);
}

void
add_plist_tests()
{
	/* Nonexistent filename case */
	g_test_add_data_func("/plist/fail/Nonexistent filename", "", plist_fail_case);
	/* Fail cases with badly formed files */
	g_test_add_data_func("/plist/fail/Badly formed XML", "badlyformed.fail.plist", plist_fail_case);
	g_test_add_data_func("/plist/fail/Incomplete XML elements", "incomplete.fail.plist", plist_fail_case);
	g_test_add_data_func("/plist/fail/Incorrect attribute in plist element", "wrong_attribute.fail.plist", plist_fail_case);
	g_test_add_data_func("/plist/fail/Plist version not specified", "missing_version.fail.plist", plist_fail_case);
	g_test_add_data_func("/plist/fail/Invalid plist version", "wrong_version.fail.plist", plist_fail_case);
	g_test_add_data_func("/plist/fail/No enclosing plist element", "no_plist.fail.plist", plist_fail_case);
	g_test_add_data_func("/plist/fail/No objects in plist", "no_data.fail.plist", plist_fail_case);
	g_test_add_data_func("/plist/fail/More than one object outside of a container", "extraneous_object.fail.plist", plist_fail_case);
	g_test_add_data_func("/plist/fail/Missing key element in dict", "missing_key.fail.plist", plist_fail_case);
	g_test_add_data_func("/plist/fail/key element outside dict", "extraneous_key.fail.plist", plist_fail_case);
	
	/* Pass cases, from Apple's plist man pages */
	g_test_add_data_func("/plist/pass/Apple example 1", "appledoc1.plist", plist_pass_case);
	g_test_add_data_func("/plist/pass/Apple example 2", "appledoc2.plist", plist_pass_case);
	g_test_add_data_func("/plist/pass/Apple example 3", "appledoc3.plist", plist_pass_case);
	g_test_add_data_func("/plist/pass/Apple example 4", "appledoc4.plist", plist_pass_case);
	g_test_add_data_func("/plist/pass/Apple example 5", "appledoc5.plist", plist_pass_case);
		
	/* Cases where the output should be exactly the same as the input */
	/* Remember to specify <real>s to 14 decimal points and to alphabetize
	<dict> keys */
	g_test_add_data_func("/plist/compare/One of each element", "oneofeach.plist", plist_compare_case);
}
