#include <gtk/gtk.h>
#include <osxcart/rtf.h>

static void
rtf_fail_case(gconstpointer filename)
{
	GError *error = NULL;
	GtkTextBuffer *buffer = gtk_text_buffer_new(NULL);
	g_assert(!rtf_text_buffer_import(buffer, filename, &error));
	g_assert(error != NULL);
	g_test_message("Error message: %s", error->message);
	g_error_free(error);
	g_object_unref(buffer);
}

static void
rtf_parse_pass_case(gconstpointer filename)
{
	GError *error = NULL;
	GtkTextBuffer *buffer = gtk_text_buffer_new(NULL);
	g_assert(rtf_text_buffer_import(buffer, filename, &error));
	g_assert(error == NULL);
	g_object_unref(buffer);
}

#if 0
	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_size_request(window, 500, 300);
	g_signal_connect(window, "delete-event", gtk_main_quit, NULL);
	GtkWidget *scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
	GtkWidget *textview = gtk_text_view_new_with_buffer(buffer);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview), GTK_WRAP_WORD);
	gtk_container_add(GTK_CONTAINER(scrolledwindow), textview);
	gtk_container_add(GTK_CONTAINER(window), scrolledwindow);
	gtk_widget_show_all(window);
	gtk_main();
#endif

const gchar *rtfbookexamples[] = {
	"Hello world 1", "p004_hello_world.rtf",
	"Hello world 2", "p005a_hello_world.rtf",
	"Hello world 3", "p005b_hello_world.rtf",
	"Hello world 4", "p005c_hello_world.rtf",
	"Hello world 5", "p006a_hello_world.rtf",
	"Hello world 6", "p006b_hello_world.rtf",
	"Latin-1 characters", "p007_salvete_omnes.rtf",
	"Paragraph formatting 1", "p014_annalum.rtf",
	"Paragraph formatting 2", "p015_annalum.rtf",
	"Paragraph formatting 3", "p018_annalum.rtf",
	"Paragraph formatting 4", "p023_martial.rtf",
	"Paragraph formatting 5", "p024_annalum.rtf",
	"Paragraph formatting 6", "p025_annalum.rtf",
	"Fonts", "p027_grep.rtf",
	"Style nesting 1", "p032a_scream.rtf",
	"Style nesting 2", "p032b_scream.rtf",
	"Unicode characters", "p034_daodejing.rtf",
	"Info destination 1", "p044_vote.rtf",
	"Info destination 2", "p045_vote.rtf",
	"Header destination", "p046_vote_with_prelim.rtf",
	"Nonexistent control words", "p048_zimgir.rtf",
	"Nonexistent ignorable destination 1", "p049a_nondualist.rtf",
	"Nonexistent ignorable destination 2", "p049b_supermegacomment.rtf",
	"Language formatting 1", "p050_seti_loewe.rtf",
	"Language formatting 2", "p051a_mysterious.rtf",
	"Proofreading 1", "p051b_chaucer.rtf",
	"Proofreading 2", "p052a_chaucer.rtf",
	"Columns 1", "p053_columns.rtf",
	"Columns 2", "p055_columns.rtf",
	"Footnotes", "p056_footnotes.rtf",
	"Colors 1", "p057_yow.rtf",
	"Colors 2", "p058a_yow.rtf",
	"Hyperlinks 1", "p058b_link.rtf",
	"Hyperlinks 2", "p058c_link.rtf",
	"Page formatting 1", "p059_margins.rtf",
	"Drawing objects", "p060_horizrule.rtf",
	"Page formatting 2", "p060_landscape_a4.rtf",
	"Page formatting 3", "p060_landscape_usletter.rtf",
	"Page formatting 4", "p060_twoup_a4.rtf",
	"Page formatting 5", "p060_twoup_us.rtf",
	"Paragraph borders 1", "p061_horizrule.rtf",
	"Paragraph borders 2", "p062_horizrule.rtf",
	"Image 1", "p063a_image.rtf",
	"Image 2", "p063b_image.rtf",
	"Image 3", "p063c_image.rtf",
	"Page formatting 6", "p065_vhcenter.rtf",
	"Styles", "p069_styles.rtf",
	"Tables 1", "p076a_table_single_cell_no_border.rtf",
	"Tables 2", "p076b_table_2x1_no_border.rtf",
	"Tables 3", "p076c_table_2x2_no_border.rtf",
	"Tables 4", "p077_table_2x2_groups_no_border.rtf",
	"Tables 5", "p078_table_cell_stretches_down_still_no_border.rtf",
	"Tables 6", "p082_table_all_borders.rtf",
	"Tables 7", "p083_table_some_borders.rtf",
	"Tables 8", "p085a_table_v_alignment.rtf",
	"Tables 9", "p085b_table_h_alignment.rtf",
	"Tables 10", "p086a_table_all_alignments.rtf",
	"Tables 11", "p086b_table_all_alignments_with_borders.rtf",
	NULL, NULL
};

void
add_rtf_tests(void)
{
	/* Nonexistent filename case */
	g_test_add_data_func("/rtf/fail/Nonexistent filename", "", rtf_fail_case);
	/* Fail cases with badly formed files */
	g_test_add_data_func("/rtf/fail/RTF version not specified", "missing_version.fail.rtf", rtf_fail_case);
	g_test_add_data_func("/rtf/fail/Invalid RTF version", "wrong_version.fail.rtf", rtf_fail_case);

	/* Pass cases, all examples from 'RTF Pocket Guide' by Sean M. Burke */
	/* These must all parse correctly */
	const gchar **ptr = rtfbookexamples;
	while(*ptr)
	{
		gchar *testname = g_strconcat("/rtf/pass/", *ptr++, NULL);
		g_test_add_data_func(testname, *ptr++, rtf_parse_pass_case);
		g_free(testname);
	}
}
