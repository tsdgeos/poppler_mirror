/* poppler-media.cc: qt interface to poppler
 * Copyright (C) 2012 Guillermo A. Amaral B. <gamaral@kde.org>
 * Copyright (C) 2013, 2018, 2021, 2024 Albert Astals Cid <aacid@kde.org>
 * Copyright (C) 2025 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
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

#include "poppler-media.h"

#include "Rendition.h"

#include "poppler-private.h"

#include <QtCore/QBuffer>

#define BUFFER_MAX 4096

namespace Poppler {

class MediaRenditionPrivate
{
public:
    explicit MediaRenditionPrivate(std::unique_ptr<::MediaRendition> &&renditionA) : rendition(std::move(renditionA)) { }

    ~MediaRenditionPrivate() = default;

    MediaRenditionPrivate(const MediaRenditionPrivate &) = delete;
    MediaRenditionPrivate &operator=(const MediaRenditionPrivate &) = delete;

    std::unique_ptr<::MediaRendition> rendition;
};

MediaRendition::MediaRendition(::MediaRendition *rendition) : MediaRendition(std::unique_ptr<::MediaRendition>(rendition)) { }

MediaRendition::MediaRendition(std::unique_ptr<::MediaRendition> &&rendition) : d_ptr(new MediaRenditionPrivate(std::move(rendition))) { }

MediaRendition::~MediaRendition()
{
    delete d_ptr;
}

bool MediaRendition::isValid() const
{
    Q_D(const MediaRendition);
    return d->rendition && d->rendition->isOk();
}

QString MediaRendition::contentType() const
{
    Q_ASSERT(isValid() && "Invalid media rendition.");
    Q_D(const MediaRendition);
    return UnicodeParsedString(d->rendition->getContentType());
}

QString MediaRendition::fileName() const
{
    Q_ASSERT(isValid() && "Invalid media rendition.");
    Q_D(const MediaRendition);
    return UnicodeParsedString(d->rendition->getFileName());
}

bool MediaRendition::isEmbedded() const
{
    Q_ASSERT(isValid() && "Invalid media rendition.");
    Q_D(const MediaRendition);
    return d->rendition->getIsEmbedded();
}

QByteArray MediaRendition::data() const
{
    Q_ASSERT(isValid() && "Invalid media rendition.");
    Q_D(const MediaRendition);

    Stream *s = d->rendition->getEmbbededStream();
    if (!s) {
        return QByteArray();
    }

    if (!s->reset()) {
        return QByteArray {};
    }

    QBuffer buffer;
    unsigned char data[BUFFER_MAX];
    int bread;

    buffer.open(QIODevice::WriteOnly);
    while ((bread = s->doGetChars(BUFFER_MAX, data)) != 0) {
        buffer.write(reinterpret_cast<const char *>(data), bread);
    }
    buffer.close();

    return buffer.data();
}

bool MediaRendition::autoPlay() const
{
    Q_D(const MediaRendition);
    if (d->rendition->getBEParameters()) {
        return d->rendition->getBEParameters()->autoPlay;
    } else if (d->rendition->getMHParameters()) {
        return d->rendition->getMHParameters()->autoPlay;
    } else {
        qDebug("No BE or MH parameters to reference!");
    }
    return false;
}

bool MediaRendition::showControls() const
{
    Q_D(const MediaRendition);
    if (d->rendition->getBEParameters()) {
        return d->rendition->getBEParameters()->showControls;
    } else if (d->rendition->getMHParameters()) {
        return d->rendition->getMHParameters()->showControls;
    } else {
        qDebug("No BE or MH parameters to reference!");
    }
    return false;
}

float MediaRendition::repeatCount() const
{
    Q_D(const MediaRendition);
    if (d->rendition->getBEParameters()) {
        return d->rendition->getBEParameters()->repeatCount;
    } else if (d->rendition->getMHParameters()) {
        return d->rendition->getMHParameters()->repeatCount;
    } else {
        qDebug("No BE or MH parameters to reference!");
    }
    return 1.f;
}

QSize MediaRendition::size() const
{
    Q_D(const MediaRendition);
    const MediaParameters *mp = nullptr;

    if (d->rendition->getBEParameters()) {
        mp = d->rendition->getBEParameters();
    } else if (d->rendition->getMHParameters()) {
        mp = d->rendition->getMHParameters();
    } else {
        qDebug("No BE or MH parameters to reference!");
    }

    if (mp) {
        return QSize(mp->windowParams.width, mp->windowParams.height);
    }
    return QSize();
}

}
