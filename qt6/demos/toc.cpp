/*
 * Copyright (C) 2008, Pino Toscano <pino@kde.org>
 * Copyright (C) 2018, Adam Reichold <adam.reichold@t-online.de>
 * Copyright (C) 2019, 2025, Albert Astals Cid <aacid@kde.org>
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

#include <poppler-qt6.h>

#include <QtGui/QStandardItemModel>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTreeWidget>

#include <QDebug>

struct Node
{
    Node(Poppler::OutlineItem &&item, int row, Node *parent) : m_row(row), m_parent(parent), m_item(std::move(item)) { }

    ~Node() { qDeleteAll(m_children); }

    Node(const Node &) = delete;
    Node &operator=(const Node &) = delete;

    int m_row;
    Node *m_parent;
    Poppler::OutlineItem m_item;
    QVector<Node *> m_children;
};

class TocModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    TocModel(QVector<Poppler::OutlineItem> &&items, QObject *parent) : QAbstractItemModel(parent)
    {
        for (int i = 0; i < items.count(); ++i) {
            m_topItems << new Node(std::move(items[i]), i, nullptr);
        }
    }

    ~TocModel() override { qDeleteAll(m_topItems); }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (role != Qt::DisplayRole) {
            return {};
        }

        Node *n = static_cast<Node *>(index.internalPointer());
        return n->m_item.name();
    }

    QModelIndex index(int row, int column, const QModelIndex &parent) const override
    {
        Node *p = static_cast<Node *>(parent.internalPointer());
        const QVector<Node *> &children = p ? p->m_children : m_topItems;

        return createIndex(row, column, children[row]);
    }

    QModelIndex parent(const QModelIndex &child) const override
    {
        Node *n = static_cast<Node *>(child.internalPointer());
        if (n->m_parent == nullptr) {
            return QModelIndex();
        }
        return createIndex(n->m_parent->m_row, 0, n->m_parent);
    }

    int rowCount(const QModelIndex &parent) const override
    {
        Node *n = static_cast<Node *>(parent.internalPointer());
        if (!n) {
            return m_topItems.count();
        }

        if (n->m_children.isEmpty() && !n->m_item.isNull()) {
            QVector<Poppler::OutlineItem> items = n->m_item.children();
            for (int i = 0; i < items.count(); ++i) {
                n->m_children << new Node(std::move(items[i]), i, n);
            }
        }

        return n->m_children.count();
    }

    bool hasChildren(const QModelIndex &parent) const override
    {
        Node *n = static_cast<Node *>(parent.internalPointer());
        if (!n) {
            return true;
        }

        return n->m_item.hasChildren();
    }

    int columnCount(const QModelIndex & /*parent*/) const override { return 1; }

private:
    QVector<Node *> m_topItems;
};

TocDock::TocDock(QWidget *parent) : AbstractInfoDock(parent)
{
    m_tree = new QTreeView(this);
    setWidget(m_tree);
    m_tree->setAlternatingRowColors(true);
    m_tree->header()->hide();
    setWindowTitle(tr("TOC"));
    m_tree->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
}

TocDock::~TocDock() = default;

void TocDock::expandItemModels(const QModelIndex &parent)
{
    auto *model = static_cast<TocModel *>(m_tree->model());
    for (int i = 0; i < model->rowCount(parent); ++i) {
        QModelIndex index = model->index(i, 0, parent);
        Node *n = static_cast<Node *>(index.internalPointer());
        if (n->m_item.isOpen()) {
            m_tree->setExpanded(index, true);
            expandItemModels(index);
        }
    }
}

void TocDock::fillInfo()
{
    auto outline = document()->outline();
    if (!outline.isEmpty()) {
        auto *model = new TocModel(std::move(outline), this);
        m_tree->setModel(model);

        expandItemModels(QModelIndex());
    } else {
        auto *model = new QStandardItemModel(this);
        auto *item = new QStandardItem(tr("No TOC"));
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        model->appendRow(item);
        m_tree->setModel(model);
    }
}

void TocDock::documentClosed()
{
    m_tree->setModel(nullptr);
    AbstractInfoDock::documentClosed();
}

#include "toc.moc"
