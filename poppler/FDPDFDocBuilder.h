//========================================================================
//
// FileDescriptorPDFDocBuilder.h
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2010 Hib Eris <hib@hiberis.nl>
// Copyright 2010, 2018, 2022, 2025 Albert Astals Cid <aacid@kde.org>
// Copyright 2021 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright 2021 Christian Persch <chpe@src.gnome.org>
// Copyright 2025 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
//========================================================================

#ifndef FDPDFDOCBUILDER_H
#define FDPDFDOCBUILDER_H

#include "PDFDocBuilder.h"

//------------------------------------------------------------------------
// FileDescriptorPDFDocBuilder
//
// The FileDescriptorPDFDocBuilder implements a PDFDocBuilder that read from a file descriptor.
//------------------------------------------------------------------------

class FileDescriptorPDFDocBuilder : public PDFDocBuilder
{

public:
    std::unique_ptr<PDFDoc> buildPDFDoc(const GooString &uri, const std::optional<GooString> &ownerPassword = {}, const std::optional<GooString> &userPassword = {}) override;
    bool supports(const GooString &uri) override;

private:
    static int parseFdFromUri(const std::string &uri);
};

#endif /* FDPDFDOCBUILDER_H */
