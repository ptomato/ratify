/* Copyright 2009, 2012, 2019 P. F. Chimento
This file is part of Ratify.

Ratify is free software: you can redistribute it and/or modify it under the
terms of the GNU Lesser General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

Ratify is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with Ratify.  If not, see <http://www.gnu.org/licenses/>. */

#include "config.h"

#include "rtf-state.h"

void
set_default_character_attributes(Attributes *attr)
{
    attr->style = -1;
    attr->background = -1;
    attr->foreground = -1;
    attr->highlight = -1;
    attr->font = -1;
    attr->size = 0.0;
    attr->italic = false;
    attr->bold = false;
    attr->smallcaps = false;
    attr->strikethrough = false;
    attr->subscript = false;
    attr->superscript = false;
    attr->underline = -1;
    attr->chardirection = -1;
    attr->language = 1024;
    attr->rise = 0;
    attr->scale = 100;
}

void
set_default_paragraph_attributes(Attributes *attr)
{
    attr->justification = -1;
    attr->pardirection = -1;
    attr->space_before = 0;
    attr->space_after = 0;
    attr->ignore_space_before = false;
    attr->ignore_space_after = false;
    attr->tabs = NULL;
    attr->left_margin = 0;
    attr->right_margin = 0;
    attr->indent = 0;
    attr->leading = 0;
}
