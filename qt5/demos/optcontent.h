/*
 * Copyright (C) 2008, Pino Toscano <pino@kde.org>
 * Copyright (C) 2021, Albert Astals Cid <aacid@kde.org>
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

#ifndef OPTCONTENT_H
#define OPTCONTENT_H

#include "abstractinfodock.h"

class QTreeView;

class OptContentDock : public AbstractInfoDock
{
    Q_OBJECT

public:
    explicit OptContentDock(QWidget *parent = nullptr);
    ~OptContentDock() override;

    void documentLoaded() override;
    void documentClosed() override;

protected:
    void fillInfo() override;

private:
    void reloadImage();

    QTreeView *m_view;
};

#endif
