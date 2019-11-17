#pragma once

/* Copyright 2009, 2012, 2019 P. F. Chimento
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

#include <stdbool.h>

#include <glib.h>
#include <pango/pango.h>

#include "rtf-deserialize.h"
#include "rtf-ignore.h"
#include "rtf-state.h"

extern const DestinationInfo document_destination;

G_GNUC_INTERNAL void apply_attributes(ParserContext *ctx, Attributes *attr, GtkTextIter *start, GtkTextIter *end);
G_GNUC_INTERNAL void document_text(ParserContext *ctx);
G_GNUC_INTERNAL int document_get_codepage(ParserContext *ctx);

typedef bool DocFunc(ParserContext *, Attributes *, GError **);
typedef bool DocParamFunc(ParserContext *, Attributes *, int32_t, GError **);

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
    { "\n", SPECIAL_CHARACTER, false, NULL, 0, "\n" }, \
    { "\r", SPECIAL_CHARACTER, false, NULL, 0, "\n" }, \
    { "-", SPECIAL_CHARACTER, false, NULL, 0, "\xC2\xAD" }, /* U+00AD Soft hyphen */ \
    { "\\", SPECIAL_CHARACTER, false, NULL, 0, "\\" }, \
    { "_", SPECIAL_CHARACTER, false, NULL, 0, "\xE2\x80\x91" }, /* U+2011 NBhyphen */ \
    { "{", SPECIAL_CHARACTER, false, NULL, 0, "{" }, \
    { "}", SPECIAL_CHARACTER, false, NULL, 0, "}" }, \
    { "~", SPECIAL_CHARACTER, false, NULL, 0, "\xC2\xA0" }, /* U+00A0 NBSP */ \
    { "bullet", SPECIAL_CHARACTER, false, NULL, 0, "\xE2\x80\xA2" }, /* U+2022 Bullet */ \
    { "emdash", SPECIAL_CHARACTER, false, NULL, 0, "\xE2\x80\x94" }, /* U+2014 em dash */ \
    { "emspace", SPECIAL_CHARACTER, false, NULL, 0, "\xE2\x80\x83" }, /* U+2003 em space */ \
    { "endash", SPECIAL_CHARACTER, false, NULL, 0, "\xE2\x80\x93" }, /* U+2013 en dash */ \
    { "enspace", SPECIAL_CHARACTER, false, NULL, 0, "\xE2\x80\x82" }, /* U+2002 en space */ \
    { "line", SPECIAL_CHARACTER, false, NULL, 0, "\xE2\x80\xA8" }, /* U+2028 Line separator */ \
    { "ldblquote", SPECIAL_CHARACTER, false, NULL, 0, "\xE2\x80\x9C" }, /* U+201C Left double quote */ \
    { "lquote", SPECIAL_CHARACTER, false, NULL, 0, "\xE2\x80\x98" }, /* U+2018 Left single quote */ \
    { "ltrmark", SPECIAL_CHARACTER, false, NULL, 0, "\xE2\x80\x8E" }, /* U+200E Left-to-right mark */ \
    { "par", SPECIAL_CHARACTER, false, NULL, 0, "\n" }, \
    { "qmspace", SPECIAL_CHARACTER, false, NULL, 0, "\xE2\x80\x85" }, /* U+2005 4 per em space */ \
    { "rdblquote", SPECIAL_CHARACTER, false, NULL, 0, "\xE2\x80\x9D" }, /* U+201D Right double quote */ \
    { "rquote", SPECIAL_CHARACTER, false, NULL, 0, "\xE2\x80\x99" }, /* U+2019 Right single quote */ \
    { "rtlmark", SPECIAL_CHARACTER, false, NULL, 0, "\xE2\x80\x8F" }, /* U+200F Right-to-left mark */ \
    { "tab", SPECIAL_CHARACTER, false, NULL, 0, "\t" }, \
    { "u", REQUIRED_PARAMETER, false, doc_u }, \
    { "uc", REQUIRED_PARAMETER, false, doc_uc }, \
    { "zwbo", SPECIAL_CHARACTER, false, NULL, 0, "\xE2\x80\x8B" }, /* U+200B zero width space */ \
    { "zwj", SPECIAL_CHARACTER, false, NULL, 0, "\xE2\x80\x8D" }, /* U+200D zero width joiner */ \
    { "zwnj", SPECIAL_CHARACTER, false, NULL, 0, "\xE2\x80\x8C" } /* U+200C zero width non joiner */

#define FORMATTED_TEXT_CONTROL_WORDS \
    { "b", OPTIONAL_PARAMETER, true, doc_b, 1 }, \
    { "cb", OPTIONAL_PARAMETER, true, doc_cb, 0 }, \
    { "cf", OPTIONAL_PARAMETER, true, doc_cf, 0 }, \
    { "charscalex", OPTIONAL_PARAMETER, true, doc_charscalex, 100 }, \
    { "chcbpat", OPTIONAL_PARAMETER, true, doc_cb, 0 }, \
    { "dn", OPTIONAL_PARAMETER, true, doc_dn, 6 }, \
    { "f", REQUIRED_PARAMETER, true, doc_f }, \
    { "fi", OPTIONAL_PARAMETER, true, doc_fi, 0 }, \
    { "fs", OPTIONAL_PARAMETER, true, doc_fs, 24 }, \
    { "fsmilli", REQUIRED_PARAMETER, true, doc_fsmilli }, /* Apple extension */ \
    { "highlight", REQUIRED_PARAMETER, true, doc_highlight }, \
    { "i", OPTIONAL_PARAMETER, true, doc_i, 1 }, \
    { "lang", REQUIRED_PARAMETER, true, doc_lang }, \
    { "li", OPTIONAL_PARAMETER, true, doc_li, 0 }, \
    { "ltrch", NO_PARAMETER, true, doc_ltrch }, \
    { "ltrpar", NO_PARAMETER, true, doc_ltrpar }, \
    { "nosupersub", NO_PARAMETER, true, doc_nosupersub }, \
    { "pard", NO_PARAMETER, true, doc_pard }, \
    { "plain", NO_PARAMETER, true, doc_plain }, \
    { "qc", NO_PARAMETER, true, doc_qc }, \
    { "qj", NO_PARAMETER, true, doc_qj }, \
    { "ql", NO_PARAMETER, true, doc_ql }, \
    { "qr", NO_PARAMETER, true, doc_qr }, \
    { "ri", OPTIONAL_PARAMETER, true, doc_ri, 0 }, \
    { "rtlch", NO_PARAMETER, true, doc_rtlch }, \
    { "rtlpar", NO_PARAMETER, true, doc_rtlpar }, \
    { "sa", OPTIONAL_PARAMETER, true, doc_sa, 0 }, \
    { "saauto", OPTIONAL_PARAMETER, true, doc_saauto, 0 }, \
    { "sb", OPTIONAL_PARAMETER, true, doc_sb, 0 }, \
    { "sbauto", OPTIONAL_PARAMETER, true, doc_sbauto, 0 }, \
    { "scaps", OPTIONAL_PARAMETER, true, doc_scaps, 1 }, \
    { "slleading", OPTIONAL_PARAMETER, true, doc_slleading, 0 }, /* Apple extension */ \
    { "strike", OPTIONAL_PARAMETER, true, doc_strike, 1 }, \
    { "sub", NO_PARAMETER, true, doc_sub }, \
    { "super", NO_PARAMETER, true, doc_super }, \
    { "tx", REQUIRED_PARAMETER, false, doc_tx }, \
    { "ul", OPTIONAL_PARAMETER, true, doc_ul, 1 }, \
    { "uld", OPTIONAL_PARAMETER, true, doc_ul, 1 }, /* Treat unsupported types */ \
    { "uldash", OPTIONAL_PARAMETER, true, doc_ul, 1 }, /* of underlining as */ \
    { "uldashd", OPTIONAL_PARAMETER, true, doc_ul, 1 }, /* regular underlining */ \
    { "uldashdd", OPTIONAL_PARAMETER, true, doc_ul, 1 }, \
    { "uldb", OPTIONAL_PARAMETER, true, doc_uldb, 1 }, \
    { "ulhwave", OPTIONAL_PARAMETER, true, doc_ulwave, 1 }, \
    { "ulldash", OPTIONAL_PARAMETER, true, doc_ul, 1 }, \
    { "ulnone", NO_PARAMETER, true, doc_ulnone }, \
    { "ulstyle", REQUIRED_PARAMETER, true, doc_ulstyle }, /* Apple extension */ \
    { "ulth", OPTIONAL_PARAMETER, true, doc_ul, 1 }, \
    { "ulthd", OPTIONAL_PARAMETER, true, doc_ul, 1 }, \
    { "ulthdash", OPTIONAL_PARAMETER, true, doc_ul, 1 }, \
    { "ulthdashd", OPTIONAL_PARAMETER, true, doc_ul, 1 }, \
    { "ulthdashdd", OPTIONAL_PARAMETER, true, doc_ul, 1 }, \
    { "ulthldash", OPTIONAL_PARAMETER, true, doc_ul, 1 }, \
    { "ululdbwave", OPTIONAL_PARAMETER, true, doc_ulwave, 1 }, \
    { "ulw", OPTIONAL_PARAMETER, true, doc_ul, 1 }, \
    { "ulwave", OPTIONAL_PARAMETER, true, doc_ulwave, 1 }, \
    { "up", OPTIONAL_PARAMETER, true, doc_up, 6 }, \
    { "v", OPTIONAL_PARAMETER, true, doc_v, 1 }

#define DOCUMENT_TEXT_CONTROL_WORDS \
    SPECIAL_CHARACTER_CONTROL_WORDS, \
    FORMATTED_TEXT_CONTROL_WORDS, \
    { "chftn", NO_PARAMETER, false, doc_chftn }, \
    { "cs", REQUIRED_PARAMETER, true, doc_s }, \
    { "ds", REQUIRED_PARAMETER, true, doc_s }, \
    { "nonshppict", DESTINATION, false, NULL, 0, NULL, &ignore_destination }, \
    { "s", REQUIRED_PARAMETER, true, doc_s }, \
    { "*shppict", DESTINATION, true, NULL, 0, NULL, &shppict_destination }, \
    { "ts", REQUIRED_PARAMETER, true, doc_s }, \
    { "*ud", NO_PARAMETER, true, doc_ud }, \
    { "upr", NO_PARAMETER, true, doc_upr }
