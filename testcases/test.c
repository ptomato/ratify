#include <glib.h>
#include <gtk/gtk.h>

extern void add_plist_tests(void);
extern void add_rtf_tests(void);

int
main(int argc, char **argv)
{
	g_test_init(&argc, &argv, NULL);
	gtk_init(&argc, &argv);

	add_plist_tests();
	add_rtf_tests();

	return g_test_run();
}