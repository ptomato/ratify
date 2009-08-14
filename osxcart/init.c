#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

static gboolean osxcart_initialized = FALSE;

/* This function is called at every entry point of the library, as suggested in
chapter 4.10 of the gettext manual. It sets up gettext for the library. */
void
osxcart_init(void)
{
	if(!osxcart_initialized)
	{
		bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
		bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    }
}
