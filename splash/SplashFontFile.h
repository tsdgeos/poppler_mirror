//========================================================================
//
// SplashFontFile.h
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2006 Takashi Iwai <tiwai@suse.de>
// Copyright (C) 2008, 2010, 2018, 2024-2026 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2022 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright (C) 2024, 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#ifndef SPLASHFONTFILE_H
#define SPLASHFONTFILE_H

#include <string>
#include <variant>
#include <vector>
#include <memory>

#include "SplashTypes.h"
#include "poppler_private_export.h"

class SplashFontEngine;
class SplashFont;
class SplashFontFileID;

//------------------------------------------------------------------------
// SplashFontFile
//------------------------------------------------------------------------

class POPPLER_PRIVATE_EXPORT SplashFontSrc
{
public:
    explicit SplashFontSrc(const std::string &file);
    explicit SplashFontSrc(std::vector<unsigned char> &&data);

    SplashFontSrc(const SplashFontSrc &) = delete;
    SplashFontSrc &operator=(const SplashFontSrc &) = delete;

    const std::vector<unsigned char> &buf() const { return std::get<std::vector<unsigned char>>(m_data); }

    const std::string &fileName() const { return std::get<std::string>(m_data); }
    bool isFile() const { return std::holds_alternative<std::string>(m_data); }
    ~SplashFontSrc();

private:
    const std::variant<std::string, std::vector<unsigned char>> m_data;
};

class POPPLER_PRIVATE_EXPORT SplashFontFile
{
public:
    virtual ~SplashFontFile();

    SplashFontFile(const SplashFontFile &) = delete;
    SplashFontFile &operator=(const SplashFontFile &) = delete;

    // Create a new SplashFont, i.e., a scaled instance of this font
    // file.
    virtual SplashFont *makeFont(const std::array<SplashCoord, 4> &mat, const std::array<SplashCoord, 4> &textMat) = 0;

    // Get the font file ID.
    const SplashFontFileID &getID() const { return *id; }

    bool doAdjustMatrix;

protected:
    SplashFontFile(std::unique_ptr<SplashFontFileID> idA, std::unique_ptr<SplashFontSrc> srcA);

    std::unique_ptr<SplashFontFileID> id;
    const std::unique_ptr<SplashFontSrc> src;

    friend class SplashFontEngine;
};

#endif
