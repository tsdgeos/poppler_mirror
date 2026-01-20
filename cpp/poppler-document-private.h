/*
 * Copyright (C) 2009-2011, Pino Toscano <pino@kde.org>
 * Copyright (C) 2018, 2020, 2022, 2025, Albert Astals Cid <aacid@kde.org>
 * Copyright (C) 2018, 2020, Adam Reichold <adam.reichold@t-online.de>
 * Copyright (C) 2024, 2026, g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
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

#ifndef POPPLER_DOCUMENT_PRIVATE_H
#define POPPLER_DOCUMENT_PRIVATE_H

#include "poppler-global.h"

#include "goo/GooString.h"
#include "PDFDoc.h"
#include "GlobalParams.h"

#include <vector>

namespace poppler {

class document;
class embedded_file;

class document_private : private GlobalParamsIniter
{
public:
    document_private(std::unique_ptr<GooString> &&file_path, const std::string &owner_password, const std::string &user_password);
    document_private(byte_array *file_data, const std::string &owner_password, const std::string &user_password);
    document_private(const char *file_data, int file_data_length, const std::string &owner_password, const std::string &user_password);
    ~document_private();

    static document *check_document(document_private *doc, byte_array *file_data);

    std::unique_ptr<PDFDoc> doc;
    byte_array doc_data;
    const char *raw_doc_data;
    int raw_doc_data_length;
    bool is_locked;
    std::vector<embedded_file *> embedded_files;

private:
    document_private();
};

}

#endif
