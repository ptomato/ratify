#ifndef __OSXCART_RTF_IGNORE_H__
#define __OSXCART_RTF_IGNORE_H__

#include <glib.h>
#include "rtf-deserialize.h"

G_GNUC_INTERNAL void ignore_pending_text(ParserContext *ctx);
G_GNUC_INTERNAL gpointer ignore_state_new(void);
G_GNUC_INTERNAL gpointer ignore_state_copy(gconstpointer state);
G_GNUC_INTERNAL void ignore_state_free(gpointer state);

extern const DestinationInfo ignore_destination;

#endif /* __OSXCART_RTF_IGNORE_H__ */