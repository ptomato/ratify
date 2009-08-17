#ifndef __OSXCART_RTF_SERIALIZE_H__
#define __OSXCART_RTF_SERIALIZE_H__

G_GNUC_INTERNAL guint8 *rtf_serialize(GtkTextBuffer *register_buffer, GtkTextBuffer *content_buffer, const GtkTextIter *start, const GtkTextIter *end, gsize *length);

#endif /* __OSXCART_RTF_SERIALIZE_H__ */