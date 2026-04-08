//========================================================================
//
// FoFiType1.cc
//
// Copyright 1999-2003 Glyph & Cog, LLC
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2005, 2008, 2010, 2018, 2021-2026 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2005 Kristian Høgsberg <krh@redhat.com>
// Copyright (C) 2010 Jakub Wilk <jwilk@jwilk.net>
// Copyright (C) 2014 Carlos Garcia Campos <carlosgc@gnome.org>
// Copyright (C) 2017 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2017 Jean Ghali <jghali@libertysurf.fr>
// Copyright (C) 2022 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright (C) 2025 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#include <charconv>
#include <optional>

#include <cstring>
#include "goo/gmem.h"
#include "FoFiEncodings.h"
#include "FoFiType1.h"

//------------------------------------------------------------------------
// FoFiType1
//------------------------------------------------------------------------

std::unique_ptr<FoFiType1> FoFiType1::make(std::vector<unsigned char> &&fileA)
{
    return std::make_unique<FoFiType1>(std::move(fileA));
}

FoFiType1::FoFiType1(std::vector<unsigned char> &&fileA, PrivateTag /*unused*/) : FoFiBase(std::move(fileA))
{
    encoding = nullptr;
    parsed = false;
    undoPFB();
}

FoFiType1::~FoFiType1()
{
    if (encoding && encoding != &fofiType1StandardEncoding) {
        for (const char *glyphName : *encoding) {
            gfree(const_cast<char *>(glyphName));
        }
        delete encoding;
    }
}

std::string FoFiType1::getName()
{
    if (!parsed) {
        parse();
    }
    return name;
}

const std::array<const char *, 256> *FoFiType1::getEncodingA()
{
    if (!parsed) {
        parse();
    }
    return encoding;
}

const char *FoFiType1::getNextLine(const char *line) const
{
    const char *begin = reinterpret_cast<const char *>(file.data());
    const char *end = begin + file.size();

    while (line < end && *line != '\x0a' && *line != '\x0d') {
        ++line;
    }
    if (line < end && *line == '\x0d') {
        ++line;
    }
    if (line < end && *line == '\x0a') {
        ++line;
    }
    if (line >= end) {
        return nullptr;
    }
    return line;
}

static const char tokenSeparators[] = " \t\n\r";

class FoFiType1Tokenizer
{
public:
    explicit FoFiType1Tokenizer(std::string_view &&stringViewA) : stringView(stringViewA) { }

    std::optional<std::string_view> getToken()
    {
        const auto length = stringView.length();
        if (currentPos >= length) {
            return {};
        }

        std::string_view::size_type pos = stringView.find_first_of(tokenSeparators, currentPos);
        while (pos == currentPos) {
            // skip multiple contiguous separators
            ++currentPos;
            pos = stringView.find_first_of(tokenSeparators, currentPos);
        }
        if (pos == std::string_view::npos) {
            const auto tokenLength = length - currentPos;
            if (tokenLength > 0) {
                std::string_view token = stringView.substr(currentPos, tokenLength);
                currentPos = length;
                return token;
            }
            currentPos = length;
            return {};
        }

        std::string_view token = stringView.substr(currentPos, pos - currentPos);

        currentPos = pos + 1;

        return token;
    }

private:
    std::string_view::size_type currentPos = 0;
    const std::string_view stringView;
};

void FoFiType1::parse()
{
    FoFiType1Tokenizer tokenizer(std::string_view(reinterpret_cast<const char *>(file.data()), file.size()));
    while (name.empty() || !encoding) {
        const std::optional<std::string_view> token = tokenizer.getToken();

        if (!token) {
            break;
        }

        if (name.empty() && token == "/FontName") {
            const std::optional<std::string_view> fontNameToken = tokenizer.getToken();
            if (!fontNameToken) {
                break;
            }

            // Skip the /
            name = fontNameToken->substr(1);

        } else if (!encoding && token == "/Encoding") {
            const std::optional<std::string_view> token2 = tokenizer.getToken();
            if (!token2) {
                break;
            }

            const std::optional<std::string_view> token3 = tokenizer.getToken();
            if (!token3) {
                break;
            }

            if (token2 == "StandardEncoding" && token3 == "def") {
                encoding = &fofiType1StandardEncoding;
            } else if (token2 == "256" && token3 == "array") {
                auto *customEncoding = new std::array<const char *, 256>();
                customEncoding->fill(nullptr);

                while (true) {
                    const std::optional<std::string_view> encodingToken = tokenizer.getToken();
                    if (!encodingToken) {
                        break;
                    }

                    if (encodingToken == "dup") {
                        std::optional<std::string_view> codeToken = tokenizer.getToken();
                        if (!codeToken) {
                            break;
                        }

                        std::optional<std::string_view> nameToken;
                        // Sometimes font data has code and name together without spacing i.e. 33/exclam
                        // if that happens don't call getToken again and just split codeToken in 2
                        const auto slashPositionInCodeToken = codeToken->find('/');
                        if (slashPositionInCodeToken != std::string_view::npos) {
                            nameToken = codeToken->substr(slashPositionInCodeToken, codeToken->length() - slashPositionInCodeToken);
                            codeToken = codeToken->substr(0, slashPositionInCodeToken);
                        } else {
                            nameToken = tokenizer.getToken();
                        }

                        if (!nameToken) {
                            break;
                        }

                        int code = 0;
                        if (codeToken->length() > 2 && codeToken->at(0) == '8' && codeToken->at(1) == '#') {
                            std::from_chars(codeToken->data() + 2, codeToken->data() + codeToken->length(), code, 8);
                        } else {
                            std::from_chars(codeToken->data(), codeToken->data() + codeToken->length(), code);
                        }

                        if (code >= 0 && code < 256 && nameToken->length() > 1) {
                            gfree(const_cast<char *>((*customEncoding)[code]));
                            (*customEncoding)[code] = copyString(nameToken->data() + 1, nameToken->length() - 1);
                        }

                    } else if (encodingToken == "def") {
                        break;
                    }
                }

                encoding = customEncoding;
            }
        }
    }

    parsed = true;
}

// Undo the PFB encoding, i.e., remove the PFB headers.
void FoFiType1::undoPFB()
{
    bool ok;
    int pos1, pos2, type;
    unsigned int segLen;

    ok = true;
    if (getU8(0, &ok) != 0x80 || !ok) {
        return;
    }
    std::vector<unsigned char> file2;
    file2.resize(file.size());
    pos1 = pos2 = 0;
    while (getU8(pos1, &ok) == 0x80 && ok) {
        type = getU8(pos1 + 1, &ok);
        if (type < 1 || type > 2 || !ok) {
            break;
        }
        segLen = getU32LE(pos1 + 2, &ok);
        pos1 += 6;
        if (!ok || !checkRegion(pos1, segLen)) {
            break;
        }
        memcpy(file2.data() + pos2, file.data() + pos1, segLen);
        pos1 += segLen;
        pos2 += segLen;
    }
    file2.resize(pos2);
    fileOwner = file2;
    file = fileOwner;
}
