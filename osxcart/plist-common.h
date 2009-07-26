#ifndef __OSXCART_PLIST_COMMON_H__
#define __OSXCART_PLIST_COMMON_H__

#include <glib.h>
#include <osxcart/plist.h>

G_GNUC_INTERNAL inline gboolean str_eq(const gchar *s1, const gchar *s2);
G_GNUC_INTERNAL void check_plist_element(const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, GError **error);
G_GNUC_INTERNAL void fill_object(PlistObject *current, const gchar *name, const gchar *text, GError **error);
G_GNUC_INTERNAL PlistObject *plist_read_from_string_real(const gchar *string, GError **error);

#endif /* __OSXCART_PLIST_COMMON_H__ */
