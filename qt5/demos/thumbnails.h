/*
 * Copyright (C) 2009, Shawn Rutledge <shawn.t.rutledge@gmail.com>
 * Copyright (C) 2009, Pino Toscano <pino@kde.org>
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

#ifndef THUMBNAILS_H
#define THUMBNAILS_H

#include "abstractinfodock.h"

class QListWidget;
class QListWidgetItem;

class ThumbnailsDock : public AbstractInfoDock
{
    Q_OBJECT

public:
    explicit ThumbnailsDock(QWidget *parent = nullptr);
    ~ThumbnailsDock() override;

    void documentClosed() override;

protected:
    void fillInfo() override;

private:
    void slotItemActivated(QListWidgetItem *item);

    QListWidget *m_list;
};

#endif
