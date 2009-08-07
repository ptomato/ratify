#include <glib.h>
#include <gtk/gtk.h>
#include <osxcart/rtf.h>

int
main(int argc, char **argv)
{
	gtk_init(&argc, &argv);
	
	GError *error = NULL;
	GtkTextBuffer *buffer = gtk_text_buffer_new(NULL);
	g_assert(rtf_text_buffer_import(buffer, "RtfInterpreterTest_20.rtf", &error));
	g_assert(error == NULL);
	gchar *string = rtf_text_buffer_export_to_string(buffer);
	g_print("%s\n", string);
	g_object_unref(buffer);
	g_free(string);
	return 0;
}
