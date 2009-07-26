#ifndef __OSXCART_RTF_IGNORE_H__
#define __OSXCART_RTF_IGNORE_H__

#include <glib.h>
#include "rtf-deserialize.h"

void ignore_pending_text(ParserContext *ctx);
gpointer ignore_state_new(void);
gpointer ignore_state_copy(gpointer state);
void ignore_state_free(gpointer state);

extern const DestinationInfo ignore_destination;

#endif /* __OSXCART_RTF_IGNORE_H__ */