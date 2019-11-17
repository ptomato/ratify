#ifndef __OSXCART_RTF_DOCUMENT_H__
#define __OSXCART_RTF_DOCUMENT_H__

/* Copyright 2009, 2012 P. F. Chimento
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
#include <pango/pango.h>
#include "rtf-deserialize.h"
#include "rtf-ignore.h"
#include "rtf-state.h"

extern const DestinationInfo document_destination;

G_GNUC_INTERNAL void apply_attributes(ParserContext *ctx, Attributes *attr, GtkTextIter *start, GtkTextIter *end);
G_GNUC_INTERNAL void document_text(ParserContext *ctx);
G_GNUC_INTERNAL gint document_get_codepage(ParserContext *ctx);

typedef gboolean DocFunc(ParserContext *, Attributes *, GError **);
typedef gboolean DocParamFunc(ParserContext *, Attributes *, gint32, GError **);

/* Document destination functions usable in other destinations */
G_GNUC_INTERNAL DocFunc doc_chftn, doc_ltrch, doc_ltrpar, doc_nosupersub,
    doc_pard, doc_plain, doc_qc, doc_qj, doc_ql, doc_qr, doc_rtlch, doc_rtlpar,
    doc_sub, doc_super, doc_ud, doc_ulnone, doc_upr;
G_GNUC_INTERNAL DocParamFunc doc_b, doc_cb, doc_cf, doc_charscalex, doc_dn,
    doc_f, doc_fi, doc_fs, doc_fsmilli, doc_highlight, doc_i, doc_lang, doc_li,
    doc_ri, doc_s, doc_sa, doc_saauto, doc_sb, doc_sbauto, doc_scaps,
    doc_slleading, doc_strike, doc_tx, doc_u, doc_uc, doc_ul, doc_uldb,
    doc_ulstyle, doc_ulwave, doc_up, doc_v;

extern const DestinationInfo shppict_destination;

/* Text formatting control words usable in other destinations */
#define SPECIAL_CHARACTER_CONTROL_WORDS \
    { "\n", SPECIAL_CHARACTER, FALSE, NULL, 0, "\n" }, \
    { "\r", SPECIAL_CHARACTER, FALSE, NULL, 0, "\n" }, \
    { "-", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xC2\xAD" }, /* U+00AD Soft hyphen */ \
    { "\\", SPECIAL_CHARACTER, FALSE, NULL, 0, "\\" }, \
    { "_", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x91" }, /* U+2011 NBhyphen */ \
    { "{", SPECIAL_CHARACTER, FALSE, NULL, 0, "{" }, \
    { "}", SPECIAL_CHARACTER, FALSE, NULL, 0, "}" }, \
    { "~", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xC2\xA0" }, /* U+00A0 NBSP */ \
    { "bullet", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\xA2" }, /* U+2022 Bullet */ \
    { "emdash", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x94" }, /* U+2014 em dash */ \
    { "emspace", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x83" }, /* U+2003 em space */ \
    { "endash", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x93" }, /* U+2013 en dash */ \
    { "enspace", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x82" }, /* U+2002 en space */ \
    { "line", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\xA8" }, /* U+2028 Line separator */ \
    { "ldblquote", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x9C" }, /* U+201C Left double quote */ \
    { "lquote", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x98" }, /* U+2018 Left single quote */ \
    { "ltrmark", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x8E" }, /* U+200E Left-to-right mark */ \
    { "par", SPECIAL_CHARACTER, FALSE, NULL, 0, "\n" }, \
    { "qmspace", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x85" }, /* U+2005 4 per em space */ \
    { "rdblquote", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x9D" }, /* U+201D Right double quote */ \
    { "rquote", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x99" }, /* U+2019 Right single quote */ \
    { "rtlmark", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x8F" }, /* U+200F Right-to-left mark */ \
    { "tab", SPECIAL_CHARACTER, FALSE, NULL, 0, "\t" }, \
    { "u", REQUIRED_PARAMETER, FALSE, doc_u }, \
    { "uc", REQUIRED_PARAMETER, FALSE, doc_uc }, \
    { "zwbo", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x8B" }, /* U+200B zero width space */ \
    { "zwj", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x8D" }, /* U+200D zero width joiner */ \
    { "zwnj", SPECIAL_CHARACTER, FALSE, NULL, 0, "\xE2\x80\x8C" } /* U+200C zero width non joiner */

#define FORMATTED_TEXT_CONTROL_WORDS \
    { "b", OPTIONAL_PARAMETER, TRUE, doc_b, 1 }, \
    { "cb", OPTIONAL_PARAMETER, TRUE, doc_cb, 0 }, \
    { "cf", OPTIONAL_PARAMETER, TRUE, doc_cf, 0 }, \
    { "charscalex", OPTIONAL_PARAMETER, TRUE, doc_charscalex, 100 }, \
    { "chcbpat", OPTIONAL_PARAMETER, TRUE, doc_cb, 0 }, \
    { "dn", OPTIONAL_PARAMETER, TRUE, doc_dn, 6 }, \
    { "f", REQUIRED_PARAMETER, TRUE, doc_f }, \
    { "fi", OPTIONAL_PARAMETER, TRUE, doc_fi, 0 }, \
    { "fs", OPTIONAL_PARAMETER, TRUE, doc_fs, 24 }, \
    { "fsmilli", REQUIRED_PARAMETER, TRUE, doc_fsmilli }, /* Apple extension */ \
    { "highlight", REQUIRED_PARAMETER, TRUE, doc_highlight }, \
    { "i", OPTIONAL_PARAMETER, TRUE, doc_i, 1 }, \
    { "lang", REQUIRED_PARAMETER, TRUE, doc_lang }, \
    { "li", OPTIONAL_PARAMETER, TRUE, doc_li, 0 }, \
    { "ltrch", NO_PARAMETER, TRUE, doc_ltrch }, \
    { "ltrpar", NO_PARAMETER, TRUE, doc_ltrpar }, \
    { "nosupersub", NO_PARAMETER, TRUE, doc_nosupersub }, \
    { "pard", NO_PARAMETER, TRUE, doc_pard }, \
    { "plain", NO_PARAMETER, TRUE, doc_plain }, \
    { "qc", NO_PARAMETER, TRUE, doc_qc }, \
    { "qj", NO_PARAMETER, TRUE, doc_qj }, \
    { "ql", NO_PARAMETER, TRUE, doc_ql }, \
    { "qr", NO_PARAMETER, TRUE, doc_qr }, \
    { "ri", OPTIONAL_PARAMETER, TRUE, doc_ri, 0 }, \
    { "rtlch", NO_PARAMETER, TRUE, doc_rtlch }, \
    { "rtlpar", NO_PARAMETER, TRUE, doc_rtlpar }, \
    { "sa", OPTIONAL_PARAMETER, TRUE, doc_sa, 0 }, \
    { "saauto", OPTIONAL_PARAMETER, TRUE, doc_saauto, 0 }, \
    { "sb", OPTIONAL_PARAMETER, TRUE, doc_sb, 0 }, \
    { "sbauto", OPTIONAL_PARAMETER, TRUE, doc_sbauto, 0 }, \
    { "scaps", OPTIONAL_PARAMETER, TRUE, doc_scaps, 1 }, \
    { "slleading", OPTIONAL_PARAMETER, TRUE, doc_slleading, 0 }, /* Apple extension */ \
    { "strike", OPTIONAL_PARAMETER, TRUE, doc_strike, 1 }, \
    { "sub", NO_PARAMETER, TRUE, doc_sub }, \
    { "super", NO_PARAMETER, TRUE, doc_super }, \
    { "tx", REQUIRED_PARAMETER, FALSE, doc_tx }, \
    { "ul", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 }, \
    { "uld", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 }, /* Treat unsupported types */ \
    { "uldash", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 }, /* of underlining as */ \
    { "uldashd", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 }, /* regular underlining */ \
    { "uldashdd", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 }, \
    { "uldb", OPTIONAL_PARAMETER, TRUE, doc_uldb, 1 }, \
    { "ulhwave", OPTIONAL_PARAMETER, TRUE, doc_ulwave, 1 }, \
    { "ulldash", OPTIONAL_PARAMETER, TRUE, doc_ul, 1 }, \
    { "ulnone", NO_PARAMETER, TRUE, doc_ulnone }, \
    { "ulstyle", REQUIRED_PARAMETER, TRUE, doc_ulstyle }, /* Apple extension */ \
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
    { "v", OPTIONAL_PARAMETER, TRUE, doc_v, 1 }

#define DOCUMENT_TEXT_CONTROL_WORDS \
    SPECIAL_CHARACTER_CONTROL_WORDS, \
    FORMATTED_TEXT_CONTROL_WORDS, \
    { "chftn", NO_PARAMETER, FALSE, doc_chftn }, \
    { "cs", REQUIRED_PARAMETER, TRUE, doc_s }, \
    { "ds", REQUIRED_PARAMETER, TRUE, doc_s }, \
    { "nonshppict", DESTINATION, FALSE, NULL, 0, NULL, &ignore_destination }, \
    { "s", REQUIRED_PARAMETER, TRUE, doc_s }, \
    { "*shppict", DESTINATION, TRUE, NULL, 0, NULL, &shppict_destination }, \
    { "ts", REQUIRED_PARAMETER, TRUE, doc_s }, \
    { "*ud", NO_PARAMETER, TRUE, doc_ud }, \
    { "upr", NO_PARAMETER, TRUE, doc_upr }

#endif /* __OSXCART_RTF_DOCUMENT_H__ */
