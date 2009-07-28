#ifndef __OSXCART_RTF_DOCUMENT_H__
#define __OSXCART_RTF_DOCUMENT_H__

#include <glib.h>
#include <pango/pango.h>
#include "rtf-deserialize.h"
#include "rtf-ignore.h"

extern const DestinationInfo document_destination;

typedef struct {
	gint style; /* Index into style sheet */
	
	/* Paragraph formatting */
	
	GtkJustification justification;
	GtkTextDirection pardirection;
	gint space_before;
	gint space_after;
	gboolean ignore_space_before;
	gboolean ignore_space_after;
	PangoTabArray *tabs;
	gint left_margin;
	gint right_margin;
	gint indent;
	
	/* Character formatting */
	
	gint foreground; /* Index into the color table */
	gint background;
	gint highlight;
	gint font; /* Index into the font table */
	gint size;
	gboolean italic;
	gboolean bold;
	gboolean smallcaps;
	gboolean strikethrough;
	gboolean subscript;
	gboolean superscript;
	gboolean invisible;
	PangoUnderline underline;
    GtkTextDirection chardirection;
	gint language;
	gint rise;

	/* Number of characters to skip after \u */
	gint unicode_skip;
} Attributes;

Attributes *attributes_new(void);
Attributes *attributes_copy(Attributes *attr);
void attributes_free(Attributes *attr);
void apply_attributes(ParserContext *ctx, Attributes *attr, GtkTextIter *start, GtkTextIter *end);
void document_text(ParserContext *ctx);

typedef gboolean DocFunc(ParserContext *, Attributes *, GError **);
typedef gboolean DocParamFunc(ParserContext *, Attributes *, gint32, GError **);

/* Document destination functions usable in other destinations */
DocFunc doc_chftn, doc_ltrch, doc_ltrpar, doc_nosupersub, doc_pard, doc_plain, 
        doc_qc, doc_qj, doc_ql, doc_qr, doc_rtlch, doc_rtlpar, doc_sub, 
        doc_super, doc_ulnone;
DocParamFunc doc_b, doc_cb, doc_cf, doc_dn, doc_f, doc_fi, doc_fs, 
             doc_highlight, doc_i, doc_lang, doc_li, doc_ri, doc_s, doc_sa, 
             doc_saauto, doc_sb, doc_sbauto, doc_scaps, doc_strike, doc_tx, 
             doc_u, doc_uc, doc_ul, doc_uldb, doc_ulwave, doc_up, doc_v;

extern const DestinationInfo shppict_destination;

#define DOCUMENT_TEXT_CONTROL_WORDS \
	{ "\n", SPECIAL_CHARACTER, FALSE, NULL, 0, "\n" }, \
	{ "-", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xC2\xAD" }, /* U+00AD Soft hyphen */ \
	{ "\\", SPECIAL_CHARACTER, FALSE, NULL, 0, "\\" }, \
	{ "_", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x91" }, /* U+2011 NBhyphen */ \
	{ "{", SPECIAL_CHARACTER, FALSE, NULL, 0, "{" }, \
	{ "}", SPECIAL_CHARACTER, FALSE, NULL, 0, "}" }, \
	{ "~", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xC2\xA0" }, /* U+00A0 NBSP */ \
    { "b", OPTIONAL_PARAMETER, TRUE, doc_b, 1 }, \
	{ "bullet", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\xA2" }, /* U+2022 Bullet */ \
	{ "cb", OPTIONAL_PARAMETER, TRUE, doc_cb, 0 }, \
	{ "cf", OPTIONAL_PARAMETER, TRUE, doc_cf, 0 }, \
	{ "chcbpat", OPTIONAL_PARAMETER, TRUE, doc_cb, 0 }, \
	{ "chftn", NO_PARAMETER, FALSE, doc_chftn }, \
	{ "cs", REQUIRED_PARAMETER, TRUE, doc_s }, \
	{ "dn", OPTIONAL_PARAMETER, TRUE, doc_dn, 6 }, \
	{ "ds", REQUIRED_PARAMETER, TRUE, doc_s }, \
	{ "emdash", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x94" }, /* U+2014 em dash */ \
	{ "emspace", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x83" }, /* U+2003 em space */ \
	{ "endash", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x93" }, /* U+2013 en dash */ \
	{ "enspace", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x82" }, /* U+2002 en space */ \
	{ "f", REQUIRED_PARAMETER, TRUE, doc_f }, \
	{ "fi", OPTIONAL_PARAMETER, TRUE, doc_fi, 0 }, \
	{ "fs", OPTIONAL_PARAMETER, TRUE, doc_fs, 24 }, \
	{ "highlight", REQUIRED_PARAMETER, TRUE, doc_highlight }, \
	{ "i", OPTIONAL_PARAMETER, TRUE, doc_i, 1 }, \
	{ "lang", REQUIRED_PARAMETER, TRUE, doc_lang }, \
	{ "li", OPTIONAL_PARAMETER, TRUE, doc_li, 0 }, \
	{ "line", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\xA8" }, /* U+2028 Line separator */ \
	{ "ldblquote", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x9C" }, /* U+201C Left double quote */ \
	{ "lquote", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x98" }, /* U+2018 Left single quote */ \
	{ "ltrch", NO_PARAMETER, TRUE, doc_ltrch }, \
	{ "ltrmark", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x8E" }, /* U+200E Left-to-right mark */ \
	{ "ltrpar", NO_PARAMETER, TRUE, doc_ltrpar }, \
	{ "nonshppict", DESTINATION, FALSE, NULL, 0, NULL, &ignore_destination }, \
	{ "nosupersub", NO_PARAMETER, TRUE, doc_nosupersub }, \
	{ "par", SPECIAL_CHARACTER, FALSE, NULL, 0, "\n" }, \
	{ "pard", NO_PARAMETER, TRUE, doc_pard }, \
	{ "plain", NO_PARAMETER, TRUE, doc_plain }, \
	{ "qc", NO_PARAMETER, TRUE, doc_qc }, \
	{ "qj", NO_PARAMETER, TRUE, doc_qj }, \
	{ "ql", NO_PARAMETER, TRUE, doc_ql }, \
	{ "qmspace", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x85" }, /* U+2005 4 per em space */ \
	{ "qr", NO_PARAMETER, TRUE, doc_qr }, \
	{ "ri", OPTIONAL_PARAMETER, TRUE, doc_ri, 0 }, \
	{ "rdblquote", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x9D" }, /* U+201D Right double quote */ \
	{ "rquote", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x99" }, /* U+2019 Right single quote */ \
	{ "rtlch", NO_PARAMETER, TRUE, doc_rtlch }, \
	{ "rtlmark", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x8F" }, /* U+200F Right-to-left mark */ \
	{ "rtlpar", NO_PARAMETER, TRUE, doc_rtlpar }, \
	{ "s", REQUIRED_PARAMETER, TRUE, doc_s }, \
	{ "sa", OPTIONAL_PARAMETER, TRUE, doc_sa, 0 }, \
	{ "saauto", OPTIONAL_PARAMETER, TRUE, doc_saauto, 0 }, \
	{ "sb", OPTIONAL_PARAMETER, TRUE, doc_sb, 0 }, \
	{ "sbauto", OPTIONAL_PARAMETER, TRUE, doc_sbauto, 0 }, \
	{ "scaps", OPTIONAL_PARAMETER, TRUE, doc_scaps, 1 }, \
	{ "*shppict", DESTINATION, TRUE, NULL, 0, NULL, &shppict_destination }, \
	{ "strike", OPTIONAL_PARAMETER, TRUE, doc_strike, 1 }, \
	{ "sub", NO_PARAMETER, TRUE, doc_sub }, \
	{ "super", NO_PARAMETER, TRUE, doc_super }, \
	{ "tab", SPECIAL_CHARACTER, FALSE, NULL, 0, "\t" }, \
	{ "ts", REQUIRED_PARAMETER, TRUE, doc_s }, \
	{ "tx", REQUIRED_PARAMETER, FALSE, doc_tx }, \
	{ "u", REQUIRED_PARAMETER, FALSE, doc_u }, \
	{ "uc", REQUIRED_PARAMETER, FALSE, doc_uc }, \
	{ "ul", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 }, \
	{ "uld", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 }, /* Treat unsupported types */ \
	{ "uldash", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 }, /* of underlining as */ \
	{ "uldashd", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 }, /* regular underlining */ \
	{ "uldashdd", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 }, \
	{ "uldb", OPTIONAL_PARAMETER, TRUE, doc_uldb, 1 }, \
	{ "ulhwave", OPTIONAL_PARAMETER, TRUE, doc_ulwave, 1 }, \
	{ "ulldash", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 }, \
	{ "ulnone", NO_PARAMETER, TRUE, doc_ulnone }, \
	{ "ulth", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 }, \
	{ "ulthd", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 }, \
	{ "ulthdash", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 }, \
	{ "ulthdashd", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 }, \
	{ "ulthdashdd", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 }, \
	{ "ulthldash", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 }, \
	{ "ululdbwave", OPTIONAL_PARAMETER, TRUE, doc_ulwave, 1 }, \
	{ "ulw", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 }, \
	{ "ulwave", OPTIONAL_PARAMETER, TRUE, doc_ulwave, 1 }, \
	{ "up", OPTIONAL_PARAMETER, TRUE, doc_up, 6 }, \
	{ "v", OPTIONAL_PARAMETER, TRUE, doc_v, 1 }, \
	{ "zwbo", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x8B" }, /* U+200B zero width space */ \
	{ "zwj", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x8D" }, /* U+200D zero width joiner */ \
	{ "zwnj", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x8C" } /* U+200C zero width non joiner */

#endif /* __OSXCART_RTF_DOCUMENT_H__ */