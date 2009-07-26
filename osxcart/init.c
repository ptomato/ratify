#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

static gboolean osxcart_initialized = FALSE;

void
osxcart_init(void)
{
	if(!osxcart_initialized)
		bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
}
