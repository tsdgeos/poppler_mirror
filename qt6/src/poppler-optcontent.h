/* poppler-optcontent.h: qt interface to poppler
 *
 * Copyright (C) 2007, Brad Hards <bradh@kde.org>
 * Copyright (C) 2008, Pino Toscano <pino@kde.org>
 * Copyright (C) 2013, Anthony Granger <grangeranthony@gmail.com>
 * Copyright (C) 2016, 2021, Albert Astals Cid <aacid@kde.org>
 * Copyright (C) 2025, g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
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

#ifndef POPPLER_OPTCONTENT_H
#define POPPLER_OPTCONTENT_H

#include <QtCore/QAbstractListModel>

#include "poppler-export.h"
#include "poppler-link.h"

class OCGs;

namespace Poppler {
class Document;
class OptContentModelPrivate;

/**
 * \brief Model for optional content
 *
 * OptContentModel is an item model representing the optional content items
 * that can be found in PDF documents.
 *
 * The model offers a mostly read-only display of the data, allowing to
 * enable/disable some contents setting the Qt::CheckStateRole data role.
 */
class POPPLER_QT6_EXPORT OptContentModel : public QAbstractItemModel
{
    friend class Document;

    Q_OBJECT

public:
    ~OptContentModel() override;

    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    /**
     * Applies the Optional Content Changes specified by that link.
     */
    void applyLink(LinkOCGState *link);

private:
    explicit OptContentModel(const OCGs *optContent, QObject *parent = nullptr);

    friend class OptContentModelPrivate;
    OptContentModelPrivate *d;
};
}

#endif
