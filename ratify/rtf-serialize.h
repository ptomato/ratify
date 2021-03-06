#pragma once

/* Copyright 2009, 2019 P. F. Chimento
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

#include <stdint.h>

#include <gtk/gtk.h>

uint8_t *rtf_serialize(GtkTextBuffer *register_buffer, GtkTextBuffer *content_buffer, const GtkTextIter *start, const GtkTextIter *end, size_t *length);
