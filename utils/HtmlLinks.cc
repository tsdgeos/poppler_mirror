//========================================================================
//
// This file comes from pdftohtml project
// http://pdftohtml.sourceforge.net
//
// Copyright from:
// Gueorgui Ovtcharov
// Rainer Dorsch <http://www.ra.informatik.uni-stuttgart.de/~rainer/>
// Mikhail Kruk <meshko@cs.brandeis.edu>
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2008 Boris Toloknov <tlknv@yandex.ru>
// Copyright (C) 2010, 2021, 2022, 2024 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2013 Julien Nabet <serval2412@yahoo.fr>
// Copyright (C) 2025, 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include "HtmlLinks.h"

extern bool xml;

HtmlLink::HtmlLink(const HtmlLink &x)
{
    Xmin = x.Xmin;
    Ymin = x.Ymin;
    Xmax = x.Xmax;
    Ymax = x.Ymax;
    dest = x.dest->copy();
}

HtmlLink::HtmlLink(double xmin, double ymin, double xmax, double ymax, std::unique_ptr<GooString> &&_dest) : dest(std::move(_dest))
{
    if (xmin < xmax) {
        Xmin = xmin;
        Xmax = xmax;
    } else {
        Xmin = xmax;
        Xmax = xmin;
    }
    if (ymin < ymax) {
        Ymin = ymin;
        Ymax = ymax;
    } else {
        Ymin = ymax;
        Ymax = ymin;
    }
}

HtmlLink::~HtmlLink() = default;

bool HtmlLink::isEqualDest(const HtmlLink &x) const
{
    return (!strcmp(dest->c_str(), x.dest->c_str()));
}

bool HtmlLink::inLink(double xmin, double ymin, double xmax, double ymax) const
{
    double y = (ymin + ymax) / 2;
    if (y > Ymax) {
        return false;
    }
    return (y > Ymin) && (xmin < Xmax) && (xmax > Xmin);
}

// returns nullptr if same as input string (to avoid copying)
static std::unique_ptr<GooString> EscapeSpecialChars(GooString *s)
{
    std::unique_ptr<GooString> tmp;
    for (size_t i = 0, j = 0; i < s->size(); i++, j++) {
        const char *replace = nullptr;
        switch (s->getChar(i)) {
        case '"':
            replace = "&quot;";
            break;
        case '&':
            replace = "&amp;";
            break;
        case '<':
            replace = "&lt;";
            break;
        case '>':
            replace = "&gt;";
            break;
        default:
            continue;
        }
        if (replace) {
            if (!tmp) {
                tmp = s->copy();
            }
            if (tmp) {
                tmp->erase(j, 1);
                int l = strlen(replace);
                tmp->insert(j, replace, l);
                j += l - 1;
            }
        }
    }
    return tmp;
}

std::unique_ptr<GooString> HtmlLink::getLinkStart() const
{
    std::unique_ptr<GooString> res = std::make_unique<GooString>("<a href=\"");
    std::unique_ptr<GooString> d = xml ? EscapeSpecialChars(dest.get()) : nullptr;
    res->append(d ? d->toStr() : dest->toStr());
    res->append("\">");
    return res;
}

/*GooString* HtmlLink::Link(GooString* content){
  //GooString* _dest=new GooString(dest);
  GooString *tmp=new GooString("<a href=\"");
  tmp->append(dest);
  tmp->append("\">");
  tmp->append(content);
  tmp->append("</a>");
  //delete _dest;
  return tmp;
  }*/

HtmlLinks::HtmlLinks() = default;

HtmlLinks::~HtmlLinks() = default;

bool HtmlLinks::inLink(double xmin, double ymin, double xmax, double ymax, size_t &p) const
{

    for (auto i = accu.begin(); i != accu.end(); ++i) {
        if (i->inLink(xmin, ymin, xmax, ymax)) {
            p = (i - accu.begin());
            return true;
        }
    }
    return false;
}

const HtmlLink *HtmlLinks::getLink(size_t i) const
{
    return &accu[i];
}
