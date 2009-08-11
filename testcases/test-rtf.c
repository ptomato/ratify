#include <stdlib.h>
#include <string.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
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
	if(!rtf_text_buffer_import(buffer, filename, &error))
		g_test_message("Error message: %s", error->message);
	g_object_unref(buffer);
	g_assert(error == NULL);
}

static void
rtf_parse_human_approval_case(gconstpointer filename)
{
    GError *error = NULL;
	
	GtkWidget *label = gtk_label_new("Is the RTF code rendered correctly?");
	GtkWidget *pane = gtk_hpaned_new();
	GtkWidget *codescroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(codescroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	GtkWidget *codeview = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(codeview), GTK_WRAP_CHAR);
	GtkWidget *rtfscroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(rtfscroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	GtkWidget *rtfview = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(rtfview), GTK_WRAP_WORD);
	
	gtk_container_add(GTK_CONTAINER(codescroll), codeview);
	gtk_container_add(GTK_CONTAINER(rtfscroll), rtfview);
	gtk_paned_add1(GTK_PANED(pane), codescroll);
	gtk_paned_add2(GTK_PANED(pane), rtfscroll);
	
	GtkWidget *dialog = gtk_dialog_new_with_buttons(filename, NULL, 0,
	    GTK_STOCK_YES, GTK_RESPONSE_YES,
	    GTK_STOCK_NO,  GTK_RESPONSE_NO,
	    NULL);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 1000, 400);
	GtkWidget *vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_box_pack_start(GTK_BOX(vbox), pane, TRUE, TRUE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 6);
	gtk_paned_set_position(GTK_PANED(pane), 500);
	gtk_widget_show_all(vbox);
	
	gchar *text;
	if(!g_file_get_contents(filename, &text, NULL, &error))
	    g_test_message("Error message: %s", error->message);
	gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(codeview)), text, -1);
	g_assert(error == NULL);
    if(!rtf_text_buffer_import_from_string(gtk_text_view_get_buffer(GTK_TEXT_VIEW(rtfview)), text, &error))
	    g_test_message("Error message: %s", error->message);
	g_free(text);
	g_assert(error == NULL);
	
	gint response = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	
	g_assert(response == GTK_RESPONSE_YES);
}

static void
rtf_write_pass_case(gconstpointer filename)
{
    GError *error = NULL;
    GtkTextBuffer *buffer1 = gtk_text_buffer_new(NULL);
    GtkTextBuffer *buffer2 = gtk_text_buffer_new(NULL);
	if(!rtf_text_buffer_import(buffer1, filename, &error))
	    g_test_message("Import error message: %s", error->message);
	g_assert(error == NULL);
	gchar *string = rtf_text_buffer_export_to_string(buffer1);
	if(!rtf_text_buffer_import_from_string(buffer2, string, &error))
	    g_test_message("Export error message: %s", error->message);
	g_assert(error == NULL);
	    
	GtkTextIter start, end;
	gtk_text_buffer_get_bounds(buffer1, &start, &end);
	gchar *text1 = gtk_text_buffer_get_slice(buffer1, &start, &end, TRUE);
	gtk_text_buffer_get_bounds(buffer2, &start, &end);
	gchar *text2 = gtk_text_buffer_get_slice(buffer2, &start, &end, TRUE);
	g_assert_cmpstr(text1, ==, text2);
	
	g_free(text1);
	g_free(text2);
	g_object_unref(buffer1);
	g_object_unref(buffer2);
	g_free(string);
}

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

const gchar *codeprojectpasscases[] = {
	"Formatting 1", "DefaultText.rtf",
	"Bare minimum file", "minimal.rtf",
	"Hello World 1", "RtfInterpreterTest_0.rtf",
	"High-bit characters", "RtfInterpreterTest_1.rtf",
	"Formatting and unicode", "RtfInterpreterTest_2.rtf",
	"Justification", "RtfInterpreterTest_3.rtf",
	"JPEG image", "RtfInterpreterTest_4.rtf",
	"PNG image", "RtfInterpreterTest_5.rtf",
	"WMF image 1", "RtfInterpreterTest_6.rtf",
	"EMF image 1", "RtfInterpreterTest_7.rtf",
	"EMF image 2", "RtfInterpreterTest_8.rtf",
	"Lists", "RtfInterpreterTest_9.rtf",
	"Nested lists", "RtfInterpreterTest_10.rtf",
	"Formatting 2", "RtfInterpreterTest_11.rtf",
	"Wide characters 1", "RtfInterpreterTest_12.rtf",
	"Wide characters 2", "RtfInterpreterTest_13.rtf",
	"Wide characters 3", "RtfInterpreterTest_14.rtf",
	"Single letter 'a'", "RtfInterpreterTest_15.rtf",
	"Japanese Test 1", "RtfInterpreterTest_16.rtf",
	"Japanese Test 2", "RtfInterpreterTest_17.rtf",
	"Wide characters 4", "RtfInterpreterTest_18.rtf",
	"Unicode and PNG image", "RtfInterpreterTest_19.rtf",
	"Formatting 3", "RtfInterpreterTest_20.rtf",
	"Wide characters 5", "RtfInterpreterTest_21.rtf",
	"WMF image 2", "RtfInterpreterTest_22.rtf",
	"Hello World 2", "RtfParserTest_0.rtf",
	"Hello World 3", "RtfParserTest_1.rtf",
	"Hello World 4", "RtfParserTest_2.rtf",
	"Hello World 5", "RtfParserTest_3.rtf",
	NULL, NULL
};
const gchar *codeprojectfailcases[] = {
	"Empty document group", "RtfInterpreterTest_fail_0.rtf",
	"Missing version", "RtfInterpreterTest_fail_1.rtf",
	"Unsupported version", "RtfInterpreterTest_fail_2.rtf",
	"No \\rtf control word", "RtfInterpreterTest_fail_3.rtf",
	"Unknown version", "RtfInterpreterTest_fail_4.rtf",
	"Text before document group", "RtfParserTest_fail_0.rtf",
	"Text after document group", "RtfParserTest_fail_1.rtf",
	"Too many closing braces", "RtfParserTest_fail_2.rtf",
	"Not enough closing braces", "RtfParserTest_fail_3.rtf",
	"No document group 1", "RtfParserTest_fail_5.rtf", /* 4 was just an empty file */
	"No document group 2", "RtfParserTest_fail_6.rtf",
	NULL, NULL
};

void
add_rtf_tests(void)
{
    const gchar **ptr;
    gboolean have_wmf = FALSE;
    gboolean have_emf = FALSE;
    GSList *iter, *formats = gdk_pixbuf_get_formats();
    
    /* Determine whether WMF and EMF support were compiled into our GdkPixbuf */
    for(iter = formats; iter; iter = g_slist_next(iter))
    {
        if(strcmp(gdk_pixbuf_format_get_name(iter->data), "wmf") == 0)
            have_wmf = TRUE;
        else if(strcmp(gdk_pixbuf_format_get_name(iter->data), "emf") == 0)
            have_emf = TRUE;
    }
    g_slist_free(formats);
    
	/* Nonexistent filename case */
	g_test_add_data_func("/rtf/parse/fail/Nonexistent filename", "", rtf_fail_case);

	/* Pass cases, all examples from 'RTF Pocket Guide' by Sean M. Burke */
	/* These must all parse correctly */
	for(ptr = rtfbookexamples; *ptr; ptr += 2)
	{
		gchar *testname = g_strconcat("/rtf/parse/pass/pocketguide/", ptr[0], NULL);
		g_test_add_data_func(testname, ptr[1], rtf_parse_pass_case);
		g_free(testname);
	}

	/* Pass cases, from http://www.codeproject.com/KB/recipes/RtfConverter.aspx */
	/* These must all parse correctly */
	for(ptr = codeprojectpasscases; *ptr; ptr += 2)
	{
	    if(strstr(ptr[0], "WMF") && !have_wmf)
	        continue;
	    if(strstr(ptr[0], "EMF") && !have_emf)
	        continue;
		gchar *testname = g_strconcat("/rtf/parse/pass/codeproject/", ptr[0], NULL);
		g_test_add_data_func(testname, ptr[1], rtf_parse_pass_case);
		g_free(testname);
	}

	/* Fail cases, from http://www.codeproject.com/KB/recipes/RtfConverter.aspx */
	/* These must all give an error */
	for(ptr = codeprojectfailcases; *ptr; ptr += 2)
	{
		gchar *testname = g_strconcat("/rtf/parse/fail/codeproject/", ptr[0], NULL);
		g_test_add_data_func(testname, ptr[1], rtf_fail_case);
		g_free(testname);
	}
	
	/* Now export the RTF to a string and re-import it */
	for(ptr = rtfbookexamples; *ptr; ptr += 2)
	{
		gchar *testname = g_strconcat("/rtf/write/pocketguide/", ptr[0], NULL);
		g_test_add_data_func(testname, ptr[1], rtf_write_pass_case);
		g_free(testname);
	}
	for(ptr = codeprojectpasscases; *ptr; ptr += 2)
	{
	    if(strstr(ptr[0], "WMF") && !have_wmf)
	        continue;
	    if(strstr(ptr[0], "EMF") && !have_emf)
	        continue;
		gchar *testname = g_strconcat("/rtf/write/codeproject/", ptr[0], NULL);
		g_test_add_data_func(testname, ptr[1], rtf_write_pass_case);
		g_free(testname);
	}
	
    /* RTFD tests */
    g_test_add_data_func("/rtf/parse/pass/rtfd/RTFD test", "rtfdtest.rtfd", rtf_parse_pass_case);
    g_test_add_data_func("/rtf/write/pass/rtfd/RTFD test", "rtfdtest.rtfd", rtf_write_pass_case);
    
    /* Human tests -- only on thorough testing */
    if(g_test_thorough())
    {
        for(ptr = rtfbookexamples; *ptr; ptr += 2)
        {
            gchar *testname = g_strconcat("/rtf/parse/human/pocketguide/", ptr[0], NULL);
            g_test_add_data_func(testname, ptr[1], rtf_parse_human_approval_case);
            g_free(testname);
        }
    
        /* Pass cases, from http://www.codeproject.com/KB/recipes/RtfConverter.aspx */
        /* These must all parse correctly */
        for(ptr = codeprojectpasscases; *ptr; ptr += 2)
        {
            if(strstr(ptr[0], "WMF") && !have_wmf)
                continue;
            if(strstr(ptr[0], "EMF") && !have_emf)
                continue;
            gchar *testname = g_strconcat("/rtf/parse/human/codeproject/", ptr[0], NULL);
            g_test_add_data_func(testname, ptr[1], rtf_parse_human_approval_case);
            g_free(testname);
        }
    }
    
}
