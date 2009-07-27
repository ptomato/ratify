#ifndef __OSXCART_RTF_LANGCODE_H__
#define __OSXCART_RTF_LANGCODE_H__

#include <glib.h>

G_GNUC_INTERNAL const gchar *language_to_iso(gint wincode);
G_GNUC_INTERNAL gint language_to_wincode(const gchar *isocode);

#endif /* __OSXCART_RTF_LANGCODE_H__ */