//*********************************************************************************
//                               Rendition.cc
//---------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------
// Hugo Mercier <hmercier31[at]gmail.com> (c) 2008
// Pino Toscano <pino@kde.org> (c) 2008
// Carlos Garcia Campos <carlosgc@gnome.org> (c) 2010
// Tobias Koenig <tobias.koenig@kdab.com> (c) 2012
// Albert Astals Cid <aacid@kde.org> (C) 2017, 2018, 2024, 2025
// g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk> (C) 2025
// 2025 Arnav V <arnav0872@gmail.com> (C) 2025
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//*********************************************************************************

#include "Rendition.h"

MediaWindowParameters::MediaWindowParameters()
{
    // default values
    type = windowEmbedded;
    width = -1;
    height = -1;
    relativeTo = windowRelativeToDocument;
    XPosition = 0.5;
    YPosition = 0.5;
    hasTitleBar = true;
    hasCloseButton = true;
    isResizeable = true;
}

void MediaWindowParameters::parseFWParams(const Dict &params)
{
    Object tmp = params.lookup("D");
    if (tmp.isArray()) {
        Array *dim = tmp.getArray();

        if (dim->getLength() >= 2) {
            Object dd = dim->get(0);
            if (dd.isInt()) {
                width = dd.getInt();
            }

            dd = dim->get(1);
            if (dd.isInt()) {
                height = dd.getInt();
            }
        }
    }

    tmp = params.lookup("RT");
    if (tmp.isInt()) {
        int t = tmp.getInt();
        switch (t) {
        case 0:
            relativeTo = windowRelativeToDocument;
            break;
        case 1:
            relativeTo = windowRelativeToApplication;
            break;
        case 2:
            relativeTo = windowRelativeToDesktop;
            break;
        }
    }

    tmp = params.lookup("P");
    if (tmp.isInt()) {
        int t = tmp.getInt();

        switch (t) {
        case 0: // Upper left
            XPosition = 0.0;
            YPosition = 0.0;
            break;
        case 1: // Upper Center
            XPosition = 0.5;
            YPosition = 0.0;
            break;
        case 2: // Upper Right
            XPosition = 1.0;
            YPosition = 0.0;
            break;
        case 3: // Center Left
            XPosition = 0.0;
            YPosition = 0.5;
            break;
        case 4: // Center
            XPosition = 0.5;
            YPosition = 0.5;
            break;
        case 5: // Center Right
            XPosition = 1.0;
            YPosition = 0.5;
            break;
        case 6: // Lower Left
            XPosition = 0.0;
            YPosition = 1.0;
            break;
        case 7: // Lower Center
            XPosition = 0.5;
            YPosition = 1.0;
            break;
        case 8: // Lower Right
            XPosition = 1.0;
            YPosition = 1.0;
            break;
        }
    }

    tmp = params.lookup("T");
    if (tmp.isBool()) {
        hasTitleBar = tmp.getBool();
    }
    tmp = params.lookup("UC");
    if (tmp.isBool()) {
        hasCloseButton = tmp.getBool();
    }
    tmp = params.lookup("R");
    if (tmp.isInt()) {
        isResizeable = (tmp.getInt() != 0);
    }
}

MediaParameters::MediaParameters()
{
    // instanciate to default values

    volume = 100;
    fittingPolicy = fittingUndefined;
    autoPlay = true;
    repeatCount = 1.0;
    opacity = 1.0;
    showControls = false;
    duration = 0;
}

void MediaParameters::parseMediaPlayParameters(const Dict &playDict)
{
    Object tmp = playDict.lookup("V");
    if (tmp.isInt()) {
        volume = tmp.getInt();
    }

    tmp = playDict.lookup("C");
    if (tmp.isBool()) {
        showControls = tmp.getBool();
    }

    tmp = playDict.lookup("F");
    if (tmp.isInt()) {
        int t = tmp.getInt();

        switch (t) {
        case 0:
            fittingPolicy = fittingMeet;
            break;
        case 1:
            fittingPolicy = fittingSlice;
            break;
        case 2:
            fittingPolicy = fittingFill;
            break;
        case 3:
            fittingPolicy = fittingScroll;
            break;
        case 4:
            fittingPolicy = fittingHidden;
            break;
        case 5:
            fittingPolicy = fittingUndefined;
            break;
        }
    }

    // duration parsing
    // duration's default value is set to 0, which means : intrinsinc media duration
    tmp = playDict.lookup("D");
    if (tmp.isDict()) {
        Object oname = tmp.dictLookup("S");
        if (oname.isName()) {
            const char *name = oname.getName();
            if (!strcmp(name, "F")) {
                duration = -1; // infinity
            } else if (!strcmp(name, "T")) {
                Object ddict = tmp.dictLookup("T");
                if (ddict.isDict()) {
                    Object tmp2 = ddict.dictLookup("V");
                    if (tmp2.isNum()) {
                        duration = (unsigned long)(tmp2.getNum());
                    }
                }
            }
        }
    }

    tmp = playDict.lookup("A");
    if (tmp.isBool()) {
        autoPlay = tmp.getBool();
    }

    tmp = playDict.lookup("RC");
    if (tmp.isNum()) {
        repeatCount = tmp.getNum();
    }
}

void MediaParameters::parseMediaScreenParameters(const Dict &screenDict)
{
    Object tmp = screenDict.lookup("W");
    if (tmp.isInt()) {
        int t = tmp.getInt();

        switch (t) {
        case 0:
            windowParams.type = MediaWindowParameters::windowFloating;
            break;
        case 1:
            windowParams.type = MediaWindowParameters::windowFullscreen;
            break;
        case 2:
            windowParams.type = MediaWindowParameters::windowHidden;
            break;
        case 3:
            windowParams.type = MediaWindowParameters::windowEmbedded;
            break;
        }
    }

    // background color
    tmp = screenDict.lookup("B");
    if (tmp.isArray()) {
        Array *color = tmp.getArray();

        Object component = color->get(0);
        bgColor.r = component.getNum();

        component = color->get(1);
        bgColor.g = component.getNum();

        component = color->get(2);
        bgColor.b = component.getNum();
    }

    // opacity
    tmp = screenDict.lookup("O");
    if (tmp.isNum()) {
        opacity = tmp.getNum();
    }

    if (windowParams.type == MediaWindowParameters::windowFloating) {
        const Object winDict = screenDict.lookup("F");
        if (winDict.isDict()) {
            windowParams.parseFWParams(*winDict.getDict());
        }
    }
}

MediaRendition::~MediaRendition() = default;

MediaRendition::MediaRendition(const Dict &dict)
{
    bool hasClip = false;

    ok = true;
    isEmbedded = false;

    //
    // Parse media clip data
    //
    Object tmp2 = dict.lookup("C");
    if (tmp2.isDict()) { // media clip
        hasClip = true;
        Object tmp = tmp2.dictLookup("S");
        if (tmp.isName()) {
            if (!strcmp(tmp.getName(), "MCD")) { // media clip data
                Object obj1 = tmp2.dictLookup("D");
                if (obj1.isDict()) {
                    Object obj2 = obj1.dictLookup("F");
                    if (obj2.isString()) {
                        fileName = obj2.takeString();
                    }
                    obj2 = obj1.dictLookup("EF");
                    if (obj2.isDict()) {
                        Object embedded = obj2.dictLookup("F");
                        if (embedded.isStream()) {
                            isEmbedded = true;
                            embeddedStreamObject = embedded.copy();
                        }
                    }

                    // TODO: D might be a form XObject too
                } else {
                    error(errSyntaxError, -1, "Invalid Media Clip Data");
                    ok = false;
                }

                // FIXME: ignore CT if D is a form XObject
                obj1 = tmp2.dictLookup("CT");
                if (obj1.isString()) {
                    contentType = obj1.takeString();
                }
            } else if (!strcmp(tmp.getName(), "MCS")) { // media clip data
                // TODO
            }
        } else {
            error(errSyntaxError, -1, "Invalid Media Clip");
            ok = false;
        }
    }

    if (!ok) {
        return;
    }

    //
    // parse Media Play Parameters
    tmp2 = dict.lookup("P");
    if (tmp2.isDict()) { // media play parameters
        Object params = tmp2.dictLookup("MH");
        if (params.isDict()) {
            MH.parseMediaPlayParameters(*params.getDict());
        }
        params = tmp2.dictLookup("BE");
        if (params.isDict()) {
            BE.parseMediaPlayParameters(*params.getDict());
        }
    } else if (!hasClip) {
        error(errSyntaxError, -1, "Invalid Media Rendition");
        ok = false;
    }

    //
    // parse Media Screen Parameters
    tmp2 = dict.lookup("SP");
    if (tmp2.isDict()) { // media screen parameters
        Object params = tmp2.dictLookup("MH");
        if (params.isDict()) {
            MH.parseMediaScreenParameters(*params.getDict());
        }
        params = tmp2.dictLookup("BE");
        if (params.isDict()) {
            BE.parseMediaScreenParameters(*params.getDict());
        }
    }
}

MediaRendition::MediaRendition(const MediaRendition &other)
{
    ok = other.ok;
    MH = other.MH;
    BE = other.BE;
    isEmbedded = other.isEmbedded;
    embeddedStreamObject = other.embeddedStreamObject.copy();

    if (other.contentType) {
        contentType = other.contentType->copy();
    }

    if (other.fileName) {
        fileName = other.fileName->copy();
    }
}

void MediaRendition::outputToFile(FILE *fp)
{
    if (!isEmbedded) {
        return;
    }

    if (!embeddedStreamObject.streamRewind()) {
        return;
    }

    while (true) {
        int c = embeddedStreamObject.streamGetChar();
        if (c == EOF) {
            break;
        }

        fwrite(&c, 1, 1, fp);
    }
}

std::unique_ptr<MediaRendition> MediaRendition::copy() const
{
    return std::make_unique<MediaRendition>(*this);
}

// TODO: SelectorRendition
