/* PageTransition.cc
 * Copyright (C) 2005, Net Integration Technologies, Inc.
 * Copyright (C) 2010, 2017, 2020, 2026, Albert Astals Cid <aacid@kde.org>
 * Copyright (C) 2013 Adrian Johnson <ajohnson@redneon.com>
 * Copyright (C) 2015, Arseniy Lartsev <arseniy@alumni.chalmers.se>
 * Copyright (C) 2026 Stefan Brüns <stefan.bruens@rwth-aachen.de>
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

#include "PageTransition.h"

#include "Object.h"

//------------------------------------------------------------------------
// PageTransition
//------------------------------------------------------------------------

PageTransition::PageTransition(Object *trans)
{
    Object obj;
    Dict *dict;

    type = transitionReplace;
    duration = 1;
    alignment = transitionHorizontal;
    direction = transitionInward;
    angle = 0;
    scale = 1.0;
    ok = true;

    if (!trans || !trans->isDict()) {
        ok = false;
        return;
    }

    dict = trans->getDict();

    // get type
    obj = dict->lookup("S");
    if (obj.isName()) {
        const std::string &s = obj.getNameString();

        if (s == "R") {
            type = transitionReplace;
        } else if (s == "Split") {
            type = transitionSplit;
        } else if (s == "Blinds") {
            type = transitionBlinds;
        } else if (s == "Box") {
            type = transitionBox;
        } else if (s == "Wipe") {
            type = transitionWipe;
        } else if (s == "Dissolve") {
            type = transitionDissolve;
        } else if (s == "Glitter") {
            type = transitionGlitter;
        } else if (s == "Fly") {
            type = transitionFly;
        } else if (s == "Push") {
            type = transitionPush;
        } else if (s == "Cover") {
            type = transitionCover;
        } else if (s == "Uncover") {
            type = transitionUncover;
        } else if (s == "Fade") {
            type = transitionFade;
        }
    }

    // get duration
    obj = dict->lookup("D");
    if (obj.isNum()) {
        duration = obj.getNum();
    }

    // get alignment
    obj = dict->lookup("Dm");
    if (obj.isName()) {
        const std::string &dm = obj.getNameString();

        if (dm == "H") {
            alignment = transitionHorizontal;
        } else if (dm == "V") {
            alignment = transitionVertical;
        }
    }

    // get direction
    obj = dict->lookup("M");
    if (obj.isName()) {
        const std::string &m = obj.getNameString();

        if (m == "I") {
            direction = transitionInward;
        } else if (m == "O") {
            direction = transitionOutward;
        }
    }

    // get angle
    obj = dict->lookup("Di");
    if (obj.isInt()) {
        angle = obj.getInt();
    }

    obj = dict->lookup("Di");
    if (obj.isName()) {
        if (obj.getNameString() == "None") {
            angle = 0;
        }
    }

    // get scale
    obj = dict->lookup("SS");
    if (obj.isNum()) {
        scale = obj.getNum();
    }

    // get rectangular
    rectangular = dict->lookup("B").getBoolWithDefaultValue(false);
}
