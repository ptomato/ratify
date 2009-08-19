#ifndef __OSXCART_PLIST_COMMON_H__
#define __OSXCART_PLIST_COMMON_H__

/* Copyright 2009 P. F. Chimento
This file is part of Osxcart.

Osxcart is free software: you can redistribute it and/or modify it under the
terms of the GNU Lesser General Public License as published by the Free Software 
Foundation, either version 3 of the License, or (at your option) any later 
version.

Osxcart is distributed in the hope that it will be useful, but WITHOUT ANY 
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along 
with Osxcart.  If not, see <http://www.gnu.org/licenses/>. */

#include <glib.h>
#include <osxcart/plist.h>

G_GNUC_INTERNAL inline gboolean str_eq(const gchar *s1, const gchar *s2);
G_GNUC_INTERNAL void check_plist_element(const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, GError **error);
G_GNUC_INTERNAL void fill_object(PlistObject *current, const gchar *name, const gchar *text, GError **error);
G_GNUC_INTERNAL PlistObject *plist_read_from_string_real(const gchar *string, GError **error);

#endif /* __OSXCART_PLIST_COMMON_H__ */
