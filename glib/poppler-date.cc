/* poppler-date.cc: glib interface to poppler
 *
 * Copyright (C) 2009 Carlos Garcia Campos <carlosgc@gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <goo/glibc.h>
#include <DateInfo.h>

#include "poppler-date.h"

/**
 * poppler_date_parse:
 * @date: string to parse
 * @timet: an uninitialized #time_t
 *
 * Parses a PDF format date string and converts it to a #time_t. Returns #FALSE
 * if the parsing fails or the input string is not a valid PDF format date string
 *
 * Return value: #TRUE, if @timet was set
 *
 * Since: 0.12
 **/
gboolean poppler_date_parse(const gchar *date, time_t *timet)
{
    time_t t;
    GooString dateString(date);
    t = dateStringToTime(&dateString);
    if (t == (time_t)-1) {
        return FALSE;
    }

    *timet = t;
    return TRUE;
}
