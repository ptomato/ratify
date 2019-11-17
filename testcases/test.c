#include <stdlib.h>
#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <osxcart/rtf.h>

/* Convenience function */
static gchar *
build_filename(const gchar *name)
{
    return g_build_filename(TESTFILEDIR, name, NULL);
}

/* This test tries to import a malformed RTF file, and succeeds if the import
failed. */
static void
rtf_fail_case(gconstpointer name)
{
    GError *error = NULL;
    GtkTextBuffer *buffer = gtk_text_buffer_new(NULL);
    gchar *filename = build_filename(name);

    g_assert(!rtf_text_buffer_import(buffer, filename, &error));
    g_free(filename);
    g_object_unref(buffer);
    g_assert(error != NULL);
    g_test_message("Error message: %s", error->message);
    g_error_free(error);
}

/* This test tries to import an RTF file, and succeeds if the import succeeded. */
static void
rtf_parse_pass_case(gconstpointer name)
{
    GError *error = NULL;
    GtkTextBuffer *buffer = gtk_text_buffer_new(NULL);
    gchar *filename = build_filename(name);

    if(!rtf_text_buffer_import(buffer, filename, &error))
        g_test_message("Error message: %s", error->message);
    g_free(filename);
    g_object_unref(buffer);
    g_assert(error == NULL);
}

/* This test is commented out. */
#if 0
static void
rtf_write_case(gcontpointer name)
{
    GError *error = NULL;
    GtkTextBuffer *buffer = gtk_text_buffer_new(NULL);
    gchar *filename = build_filename(name);

    g_assert(rtf_text_buffer_import(buffer, filename, &error));
    g_free(filename);
    g_assert(error == NULL);
    gchar *string = rtf_text_buffer_export_to_string(buffer);
    g_object_unref(buffer);
    g_print("%s\n", string);
    g_free(string);
}
#endif /* rtf_write_case */

/* This test imports an RTF file, exports it, and imports it again. The export
operation cannot fail, but if either import operation fails, the test fails. It
then compares the plaintext of the two GtkTextBuffers, and if they differ, the
test fails. Otherwise, the test succeeds.
Comparing the plaintext is for lack of a better way to compare the text buffers'
formatting. */
static void
rtf_write_pass_case(gconstpointer name)
{
    GError *error = NULL;
    GtkTextBuffer *buffer1 = gtk_text_buffer_new(NULL);
    GtkTextBuffer *buffer2 = gtk_text_buffer_new(NULL);
    gchar *filename = build_filename(name);

    if(!rtf_text_buffer_import(buffer1, filename, &error))
        g_test_message("Import error message: %s", error->message);
    g_free(filename);
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

static void
yes_clicked(GtkButton *button, gboolean *was_correct)
{
    *was_correct = TRUE;
    gtk_main_quit();
}

static gboolean
yes_not_clicked(void)
{
    gtk_main_quit();
    return GDK_EVENT_STOP;  /* we'll destroy the window after main loop exits */
}

/* This test reads an RTF file into a string, and imports the RTF string into a
GtkTextBuffer, failing if either of these operations fail. It then displays the
RTF code and its rendered result side by side, asking the user whether the RTF
code is rendered correctly. The test succeeds if the user answers Yes, and fails
if the user answers No. */
static void
rtf_parse_human_approval_case(gconstpointer name)
{
    GError *error = NULL;
    GtkWidget *label, *pane, *codescroll, *codeview, *rtfscroll, *rtfview,
        *window, *vbox, *buttons, *yes, *no;
    GtkTextBuffer *rtfbuffer = gtk_text_buffer_new(NULL);
    gchar *text, *filename = build_filename(name);
    gboolean was_correct = FALSE;
    GFile *file = g_file_new_for_path(filename);

    /* Get RTF code */
    if(!g_file_get_contents(filename, &text, NULL, &error))
        g_test_message("Error message: %s", error->message);
    g_assert(error == NULL);

    /* Import RTF code into text buffer. Import it from a file, even though we
    have already loaded the RTF code to display in the left-hand pane, because
    the RTF code may contain references to images. */
    if(!rtf_text_buffer_import_file(rtfbuffer, file, NULL, &error))
        g_test_message("Error message: %s", error->message);
    g_assert(error == NULL);
    g_object_unref(file);

    /* Build the interface widgets */
    label = gtk_label_new("Is the RTF code rendered correctly?");
    pane = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    codescroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(codescroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    codeview = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(codeview), GTK_WRAP_CHAR);
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(codeview)), text, -1);
    rtfscroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(rtfscroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    rtfview = gtk_text_view_new_with_buffer(rtfbuffer);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(rtfview), GTK_WRAP_WORD);
    buttons = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(buttons), GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(buttons), 6);
    yes = gtk_button_new_with_mnemonic("_Yes");
    no = gtk_button_new_with_mnemonic("_No");
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    /* Pack everything into containers */
    gtk_container_add(GTK_CONTAINER(codescroll), codeview);
    gtk_container_add(GTK_CONTAINER(rtfscroll), rtfview);
    gtk_paned_add1(GTK_PANED(pane), codescroll);
    gtk_paned_add2(GTK_PANED(pane), rtfscroll);
    gtk_container_add(GTK_CONTAINER(buttons), yes);
    gtk_container_add(GTK_CONTAINER(buttons), no);
    gtk_box_pack_start(GTK_BOX(vbox), pane, TRUE, TRUE, 6);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(vbox), buttons, FALSE, FALSE, 6);
    /* Build window */
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), filename);
    gtk_window_set_modal(GTK_WINDOW(window), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 400);
    gtk_paned_set_position(GTK_PANED(pane), 500);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    /* Connect signals */
    g_signal_connect(yes, "clicked", G_CALLBACK(yes_clicked), &was_correct);
    g_signal_connect(no, "clicked", G_CALLBACK(yes_not_clicked), NULL);
    g_signal_connect(window, "delete-event", G_CALLBACK(yes_not_clicked), NULL);
    gtk_widget_show_all(window);
    g_free(filename);

    /* Run it */
    gtk_main();
    gtk_widget_destroy(window);

    g_free(text);
    g_assert(was_correct);
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
    "Crazy font table", "crazy_fonttable.rtf",
    "General 1", "outtake_crazy_fonttable.rtf",
    "General 2", "outtake_latin.rtf",
    "General 3", "outtake_simplicity.rtf",
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
    /* 4 was just an empty file, which is caught by GTK's serialize code */
    "No document group 1", "RtfParserTest_fail_5.rtf",
    "No document group 2", "RtfParserTest_fail_6.rtf",
    NULL, NULL
};

const char *variouspasscases[] = {
    "Character scaling", "charscalex.rtf",
    NULL, NULL
};

const char *variousfailcases[] = {
    "Incorrect character scaling", "charscalexfail.rtf",
    NULL, NULL
};

/* Whether WMF and EMF loading is available */
gboolean have_wmf = FALSE;
gboolean have_emf = FALSE;

/* Determine whether WMF and EMF support were compiled into our GdkPixbuf */
static void
check_wmf_and_emf(void)
{
    GSList *iter, *formats = gdk_pixbuf_get_formats();

    for(iter = formats; iter; iter = g_slist_next(iter))
    {
        if(strcmp(gdk_pixbuf_format_get_name(iter->data), "wmf") == 0)
            have_wmf = TRUE;
        else if(strcmp(gdk_pixbuf_format_get_name(iter->data), "emf") == 0)
            have_emf = TRUE;
    }
    g_slist_free(formats);
}

static void
add_tests(const gchar **testlist, const gchar *path, void (test_func)(gconstpointer))
{
    const gchar **ptr;
    gchar *testname;

    for(ptr = testlist; *ptr; ptr += 2)
    {
        if((!have_wmf && strstr(ptr[0], "WMF")) || (!have_emf && strstr(ptr[0], "EMF")))
            continue;
        testname = g_strconcat(path, ptr[0], NULL);
        g_test_add_data_func(testname, ptr[1], test_func);
        g_free(testname);
    }
}

/* The main function adds all the RTF tests to the test suite. It skips tests
that load WMF and EMF files if those modules are not compiled into GdkPixbuf.
The human tests are only added if the test suite was invoked with '-m=perf
-m=thorough'.*/

int
main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    gtk_init(&argc, &argv);

    check_wmf_and_emf();

    /* Fail cases */
    /* Nonexistent filename case */
    g_test_add_data_func("/rtf/parse/fail/Nonexistent filename", "", rtf_fail_case);
    /* Cases from http://www.codeproject.com/KB/recipes/RtfConverter.aspx */
    add_tests(codeprojectfailcases, "/rtf/parse/fail/", rtf_fail_case);
    /* Other */
    add_tests(variousfailcases, "/rtf/parse/fail/", rtf_fail_case);

    /* Pass cases */
    /* Examples from 'RTF Pocket Guide' by Sean M. Burke */
    add_tests(rtfbookexamples, "/rtf/parse/pass/", rtf_parse_pass_case);
    /* From http://www.codeproject.com/KB/recipes/RtfConverter.aspx */
    add_tests(codeprojectpasscases, "/rtf/parse/pass/", rtf_parse_pass_case);
    /* Other */
    add_tests(variouspasscases, "/rtf/parse/pass/", rtf_parse_pass_case);
    /* These tests export the RTF to a string and re-import it */
    add_tests(rtfbookexamples, "/rtf/write/", rtf_write_pass_case);
    add_tests(codeprojectpasscases, "/rtf/write/", rtf_write_pass_case);
    add_tests(variouspasscases, "/rtf/write/", rtf_write_pass_case);
    /* RTFD tests */
    g_test_add_data_func("/rtf/parse/pass/RTFD test", "rtfdtest.rtfd", rtf_parse_pass_case);
    g_test_add_data_func("/rtf/write/RTFD test", "rtfdtest.rtfd", rtf_write_pass_case);

    /* Human tests -- only on thorough testing */
    if(g_test_thorough())
    {
        add_tests(rtfbookexamples, "/rtf/parse/human/", rtf_parse_human_approval_case);
        add_tests(codeprojectpasscases, "/rtf/parse/human/", rtf_parse_human_approval_case);
        add_tests(variouspasscases, "/rtf/parse/human/", rtf_parse_human_approval_case);
    }

    return g_test_run();
}
