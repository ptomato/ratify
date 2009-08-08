#include <glib.h>
#include <gtk/gtk.h>
#include <osxcart/rtf.h>

int
main(int argc, char **argv)
{
	gtk_init(&argc, &argv);
	
	GError *error = NULL;
	GtkTextBuffer *buffer = gtk_text_buffer_new(NULL);
	g_assert(rtf_text_buffer_import(buffer, "rtfdtest.rtfd", &error));
	g_assert(error == NULL);
	
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
	
/*	gchar *string = rtf_text_buffer_export_to_string(buffer);
	g_print("%s\n", string);
	g_object_unref(buffer);
	g_free(string);*/
	return 0;
}
