/*
 * Copyright (C) 2008, Pino Toscano <pino@kde.org>
 * Copyright (C) 2018 Adam Reichold <adam.reichold@t-online.de>
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

#include "toc.h"

#include <poppler-qt5.h>

#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTreeWidget>

static void fillToc(const QVector<Poppler::OutlineItem> &items, QTreeWidget *tree, QTreeWidgetItem *parentItem)
{
    QTreeWidgetItem *newitem = nullptr;
    for (const auto &item : items) {
        if (item.isNull()) {
            continue;
        }

        if (!parentItem) {
            newitem = new QTreeWidgetItem(tree, newitem);
        } else {
            newitem = new QTreeWidgetItem(parentItem, newitem);
        }
        newitem->setText(0, item.name());

        if (item.isOpen()) {
            tree->expandItem(newitem);
        }

        const auto children = item.children();
        if (!children.isEmpty()) {
            fillToc(children, tree, newitem);
        }
    }
}


TocDock::TocDock(QWidget *parent)
    : AbstractInfoDock(parent)
{
    m_tree = new QTreeWidget(this);
    setWidget(m_tree);
    m_tree->setAlternatingRowColors(true);
    m_tree->header()->hide();
    setWindowTitle(tr("TOC"));
    m_tree->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
}

TocDock::~TocDock()
{
}

void TocDock::fillInfo()
{
    const auto outline = document()->outline();
    if (!outline.isEmpty()) {
        fillToc(outline, m_tree, nullptr);
    } else {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, tr("No TOC"));
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        m_tree->addTopLevelItem(item);
    }
}

void TocDock::documentClosed()
{
    m_tree->clear();
    AbstractInfoDock::documentClosed();
}

