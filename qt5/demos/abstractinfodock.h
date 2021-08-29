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

#ifndef ABSTRACTINFODOCK_H
#define ABSTRACTINFODOCK_H

#include <QtWidgets/QDockWidget>

#include "documentobserver.h"

class AbstractInfoDock : public QDockWidget, public DocumentObserver
{
    Q_OBJECT

public:
    explicit AbstractInfoDock(QWidget *parent = nullptr);
    ~AbstractInfoDock() override;

    void documentLoaded() override;
    void documentClosed() override;
    void pageChanged(int page) override;

protected:
    virtual void fillInfo() = 0;

private Q_SLOTS:
    void slotVisibilityChanged(bool visible);

private:
    bool m_filled;
};

#endif
