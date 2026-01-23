/*
 * Copyright (C) 2009-2010, Pino Toscano <pino@kde.org>
 * Copyright (C) 2017-2020, 2025, 2026, Albert Astals Cid <aacid@kde.org>
 * Copyright (C) 2017, Jason Alan Palmer <jalanpalmer@gmail.com>
 * Copyright (C) 2018, 2020, Suzuki Toshiya <mpsuzuki@hiroshima-u.ac.jp>
 * Copyright (C) 2018, 2020, Adam Reichold <adam.reichold@t-online.de>
 * Copyright (C) 2018, Zsombor Hollay-Horvath <hollay.horvath@gmail.com>
 * Copyright (C) 2018, Aleksey Nikolaev <nae202@gmail.com>
 * Copyright (C) 2020, Jiri Jakes <freedesktop@jirijakes.eu>
 * Copyright (C) 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
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

/**
 \file poppler-page.h
 */
#include "poppler-page.h"
#include "poppler-page-transition.h"

#include "poppler-document-private.h"
#include "poppler-page-private.h"
#include "poppler-private.h"
#include "poppler-font-private.h"
#include "poppler-font.h"

#include "TextOutputDev.h"

#include <memory>
#include <utility>

using namespace poppler;

page_private::page_private(document_private *_doc, int _index) : doc(_doc), page(doc->doc->getCatalog()->getPage(_index + 1)), index(_index) { }

page_private::~page_private()
{
    delete transition;
}

void page_private::init_font_info_cache()
{
    if (font_info_cache_initialized) {
        return;
    }

    poppler::font_iterator it(index, doc);

    if (it.has_next()) {
        font_info_cache = it.next();
    }

    font_info_cache_initialized = true;
}

/**
 \class poppler::page poppler-page.h "poppler/cpp/poppler-page.h"

 A page in a PDF %document.
 */

/**
 \enum poppler::page::orientation_enum

 The possible orientation of a page.
*/

/**
 \enum poppler::page::search_direction_enum

 The direction/action to follow when performing a text search.
*/

/**
 \enum poppler::page::text_layout_enum

 A layout of the text of a page.
*/

page::page(document_private *doc, int index) : d(new page_private(doc, index)) { }

/**
 Destructor.
 */
page::~page()
{
    delete d;
}

/**
 \returns the orientation of the page
 */
page::orientation_enum page::orientation() const
{
    const int rotation = d->page->getRotate();
    switch (rotation) {
    case 90:
        return landscape;
        break;
    case 180:
        return upside_down;
        break;
    case 270:
        return seascape;
        break;
    default:
        return portrait;
    }
}

/**
 The eventual duration the page can be hinted to be shown in a presentation.

 If this value is positive (usually different than -1) then a PDF viewer, when
 showing the page in a presentation, should show the page for at most for this
 number of seconds, and then switch to the next page (if any). Note this is
 purely a presentation attribute, it has no influence on the behaviour.

 \returns the duration time (in seconds) of the page
 */
double page::duration() const
{
    return d->page->getDuration();
}

/**
 Returns the size of one rect of the page.

 \returns the size of the specified page rect
 */
rectf page::page_rect(page_box_enum box) const
{
    const PDFRectangle *r = nullptr;
    switch (box) {
    case media_box:
        r = d->page->getMediaBox();
        break;
    case crop_box:
        r = d->page->getCropBox();
        break;
    case bleed_box:
        r = d->page->getBleedBox();
        break;
    case trim_box:
        r = d->page->getTrimBox();
        break;
    case art_box:
        r = d->page->getArtBox();
        break;
    }
    if (r) {
        return detail::pdfrectangle_to_rectf(*r);
    }
    return rectf();
}

/**
 \returns the label of the page, if any
 */
ustring page::label() const
{
    GooString goo;
    if (!d->doc->doc->getCatalog()->indexToLabel(d->index, &goo)) {
        return ustring();
    }

    return detail::unicode_GooString_to_ustring(&goo);
}

/**
 The transition from this page to the next one.

 If it is set, then a PDF viewer in a presentation should perform the
 specified transition effect when switching from this page to the next one.

 \returns the transition effect for the switch to the next page, if any
 */
page_transition *page::transition() const
{
    if (!d->transition) {
        Object o = d->page->getTrans();
        if (o.isDict()) {
            d->transition = new page_transition(&o);
        }
    }
    return d->transition;
}

/**
 Search the page for some text.

 \param text the text to search
 \param[in,out] r the area where to start search, which will be set to the area
                  of the match (if any)
 \param direction in which direction search for text
 \param case_sensitivity whether search in a case sensitive way
 \param rotation the rotation assumed for the page
 */
bool page::search(const ustring &text, rectf &r, search_direction_enum direction, case_sensitivity_enum case_sensitivity, rotation_enum rotation) const
{
    const size_t len = text.length();

    if (len == 0) {
        return false;
    }

    std::vector<Unicode> u(len);
    for (size_t i = 0; i < len; ++i) {
        u[i] = text[i];
    }

    const bool sCase = case_sensitivity == case_sensitive;
    const int rotation_value = (int)rotation * 90;

    bool found = false;
    double rect_left = r.left();
    double rect_top = r.top();
    double rect_right = r.right();
    double rect_bottom = r.bottom();

    TextOutputDev td(nullptr, true, 0, false, false);
    d->doc->doc->displayPage(&td, d->index + 1, 72, 72, rotation_value, false, true, false);
    std::unique_ptr<TextPage> text_page = td.takeText();

    switch (direction) {
    case search_from_top:
        found = text_page->findText(u.data(), len, true, true, false, false, sCase, false, false, &rect_left, &rect_top, &rect_right, &rect_bottom);
        break;
    case search_next_result:
        found = text_page->findText(u.data(), len, false, true, true, false, sCase, false, false, &rect_left, &rect_top, &rect_right, &rect_bottom);
        break;
    case search_previous_result:
        found = text_page->findText(u.data(), len, false, true, true, false, sCase, true, false, &rect_left, &rect_top, &rect_right, &rect_bottom);
        break;
    }

    r.set_left(rect_left);
    r.set_top(rect_top);
    r.set_right(rect_right);
    r.set_bottom(rect_bottom);

    return found;
}

/**
 Returns the text in the page, in its physical layout.

 \param r if not empty, it will be extracted the text in it; otherwise, the
          text of the whole page

 \returns the text of the page in the specified rect or in the whole page
 */
ustring page::text(const rectf &r) const
{
    return text(r, physical_layout);
}

static void appendToGooString(void *stream, const char *text, int len)
{
    ((GooString *)stream)->append(text, len);
}

/**
 Returns the text in the page.

 \param rect if not empty, it will be extracted the text in it; otherwise, the
             text of the whole page
 \param layout_mode the layout of the text. non_raw_non_physical_layout is not
                    supported together with a non empty rect

 \returns the text of the page in the specified rect or in the whole page

 \since 0.16
 */
ustring page::text(const rectf &r, text_layout_enum layout_mode) const
{
    std::unique_ptr<GooString> out(new GooString());
    const bool use_raw_order = (layout_mode == raw_order_layout);
    const bool use_physical_layout = (layout_mode == physical_layout);
    TextOutputDev td(&appendToGooString, out.get(), use_physical_layout, 0, use_raw_order, false);
    if (r.is_empty()) {
        d->doc->doc->displayPage(&td, d->index + 1, 72, 72, 0, false, true, false);
    } else {
        if (layout_mode == non_raw_non_physical_layout) {
            detail::user_debug_function("non_raw_non_physical_layout is not supported together with a non empty rect", detail::debug_closure);
            return {};
        }
        d->doc->doc->displayPageSlice(&td, d->index + 1, 72, 72, 0, false, true, false, r.left(), r.top(), r.width(), r.height());
    }
    return ustring::from_utf8(out->c_str());
}

/*
 * text_box_font_info object for text_box
 */
text_box_font_info_data::~text_box_font_info_data() = default;

/*
 * text_box object for page::text_list()
 */
text_box_data::~text_box_data() = default;

text_box::~text_box() = default;

text_box &text_box::operator=(text_box &&a) noexcept = default;
text_box::text_box(text_box &&a) noexcept = default;

text_box::text_box(text_box_data *data) : m_data { data } { }

ustring text_box::text() const
{
    return m_data->text;
}

rectf text_box::bbox() const
{
    return m_data->bbox;
}

int text_box::rotation() const
{
    return m_data->rotation;
}

rectf text_box::char_bbox(size_t i) const
{
    if (i < m_data->char_bboxes.size()) {
        return m_data->char_bboxes[i];
    }
    return rectf(0, 0, 0, 0);
}

bool text_box::has_space_after() const
{
    return m_data->has_space_after;
}

bool text_box::has_font_info() const
{
    return (m_data->text_box_font != nullptr);
}

text_box::writing_mode_enum text_box::get_wmode(int i) const
{
    if (this->has_font_info()) {
        return m_data->text_box_font->wmodes[i];
    }
    return text_box::invalid_wmode;
}

double text_box::get_font_size() const
{
    if (this->has_font_info()) {
        return m_data->text_box_font->font_size;
    }
    return -1;
}

std::string text_box::get_font_name(int i) const
{
    if (!this->has_font_info()) {
        return std::string("*ignored*");
    }

    int j = m_data->text_box_font->glyph_to_cache_index[i];
    if (j < 0) {
        return std::string("");
    }
    return m_data->text_box_font->font_info_cache[j].name();
}

std::vector<text_box> page::text_list(int opt_flag) const
{
    std::vector<text_box> output_list;

    /* config values are same with Qt5 Page::TextList() */
    auto output_dev = std::make_unique<TextOutputDev>(nullptr, /* char* fileName */
                                                      false, /* bool physLayoutA */
                                                      0, /* double fixedPitchA */
                                                      false, /* bool rawOrderA */
                                                      false /* bool append */
    );

    /*
     * config values are same with Qt5 Page::TextList(),
     * but rotation is fixed to zero.
     * Few people use non-zero values.
     */
    d->doc->doc->displayPageSlice(output_dev.get(), d->index + 1, /* page */
                                  72, 72, 0, /* hDPI, vDPI, rot */
                                  false, false, false, /* useMediaBox, crop, printing */
                                  -1, -1, -1, -1, /* sliceX, sliceY, sliceW, sliceH */
                                  nullptr, nullptr, /* abortCheckCbk(), abortCheckCbkData */
                                  nullptr, nullptr, /* annotDisplayDecideCbk(), annotDisplayDecideCbkData */
                                  true); /* copyXRef */

    const std::unique_ptr<TextWordList> word_list = output_dev->makeWordList();

    const std::vector<TextWord *> &words = word_list->getWords();
    output_list.reserve(words.size());
    for (const TextWord *word : words) {
        const std::unique_ptr<std::string> wordText = word->getText();
        const ustring ustr = ustring::from_utf8(wordText->c_str());

        double xMin, yMin, xMax, yMax;
        word->getBBox(&xMin, &yMin, &xMax, &yMax);

        text_box tb { new text_box_data { .text = ustr, .bbox = { xMin, yMin, xMax - xMin, yMax - yMin }, .rotation = word->getRotation(), .char_bboxes = {}, .has_space_after = word->hasSpaceAfter(), .text_box_font = nullptr } };

        std::unique_ptr<text_box_font_info_data> tb_font_info = nullptr;
        if (opt_flag & page::text_list_include_font) {
            d->init_font_info_cache();

            std::unique_ptr<text_box_font_info_data> tb_font { new text_box_font_info_data {
                    .font_size = word->getFontSize(), // double font_size
                    .wmodes = {}, // std::vector<text_box::writing_mode> wmodes;
                    .font_info_cache = d->font_info_cache, // std::vector<font_info> font_info_cache;
                    .glyph_to_cache_index = {} // std::vector<int> glyph_to_cache_index;
            } };

            tb_font_info = std::move(tb_font);
        };

        tb.m_data->char_bboxes.reserve(word->getLength());
        for (int j = 0; j < word->getLength(); j++) {
            word->getCharBBox(j, &xMin, &yMin, &xMax, &yMax);
            tb.m_data->char_bboxes.emplace_back(xMin, yMin, xMax - xMin, yMax - yMin);
        }

        if (tb_font_info && d->font_info_cache_initialized) {
            tb_font_info->glyph_to_cache_index.reserve(word->getLength());
            for (int j = 0; j < word->getLength(); j++) {
                const TextFontInfo *cur_text_font_info = word->getFontInfo(j);

                switch (cur_text_font_info->getWMode()) {
                case GfxFont::WritingMode::Horizontal:
                    tb_font_info->wmodes.push_back(text_box::horizontal_wmode);
                    break;
                case GfxFont::WritingMode::Vertical:
                    tb_font_info->wmodes.push_back(text_box::vertical_wmode);
                    break;
                };

                tb_font_info->glyph_to_cache_index.push_back(-1);
                for (size_t k = 0; k < tb_font_info->font_info_cache.size(); k++) {
                    if (cur_text_font_info->matches(&(tb_font_info->font_info_cache[k].d->ref))) {
                        tb_font_info->glyph_to_cache_index[j] = k;
                        break;
                    }
                }
            }
            tb.m_data->text_box_font = std::move(tb_font_info);
        }

        output_list.push_back(std::move(tb));
    }

    return output_list;
}

std::vector<text_box> page::text_list() const
{
    return text_list(0);
}
