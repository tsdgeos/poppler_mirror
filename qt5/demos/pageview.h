/*
 * Copyright (C) 2008-2009, Pino Toscano <pino@kde.org>
 * Copyright (C) 2013, Fabio D'Urso <fabiodurso@hotmail.it>
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

#ifndef PAGEVIEW_H
#define PAGEVIEW_H

#include <QtWidgets/QScrollArea>

#include "documentobserver.h"

class QLabel;

class PageView : public QScrollArea, public DocumentObserver
{
    Q_OBJECT

public:
    explicit PageView(QWidget *parent = nullptr);
    ~PageView() override;

    void documentLoaded() override;
    void documentClosed() override;
    void pageChanged(int page) override;

    void slotZoomChanged(qreal value);
    void slotRotationChanged(int value);

private:
    QLabel *m_imageLabel;
    qreal m_zoom;
    int m_rotation;
    int m_dpiX;
    int m_dpiY;
};

#endif
