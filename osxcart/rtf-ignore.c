#include <glib.h>
#include "rtf-deserialize.h"
#include "rtf-ignore.h"

const ControlWord ignore_word_table[] = {{ NULL }};

const DestinationInfo ignore_destination = {
    ignore_word_table,
    ignore_pending_text,
    ignore_state_new,
    ignore_state_copy,
    ignore_state_free
};

void
ignore_pending_text(ParserContext *ctx)
{
	g_string_truncate(ctx->text, 0);
}


gpointer
ignore_state_new(void)
{
	return NULL;
}

gpointer
ignore_state_copy(gpointer state)
{
    return NULL;
}

void
ignore_state_free(gpointer state)
{
}
