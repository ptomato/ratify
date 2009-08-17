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
	attr->italic = FALSE;
	attr->bold = FALSE;
	attr->smallcaps = FALSE;
	attr->strikethrough = FALSE;
	attr->subscript = FALSE;
	attr->superscript = FALSE;
	attr->underline = -1;
	attr->chardirection = -1;
	attr->language = 1024;
	attr->rise = 0;
}

void
set_default_paragraph_attributes(Attributes *attr)
{
    attr->justification = -1;
	attr->pardirection = -1;
	attr->space_before = 0;
	attr->space_after = 0;
	attr->ignore_space_before = FALSE;
	attr->ignore_space_after = FALSE;
	attr->tabs = NULL;
	attr->left_margin = 0;
	attr->right_margin = 0;
	attr->indent = 0;
	attr->leading = 0;
}
