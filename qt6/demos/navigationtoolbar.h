/*
 * Copyright (C) 2008-2009, Pino Toscano <pino@kde.org>
 * Copyright (C) 2013, Fabio D'Urso <fabiodurso@hotmail.it>
 * Copyright (C) 2019, 2021, Albert Astals Cid <aacid@kde.org>
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

#ifndef NAVIGATIONTOOLBAR_H
#define NAVIGATIONTOOLBAR_H

#include <QtWidgets/QToolBar>

#include "documentobserver.h"

class QAction;
class QComboBox;

class NavigationToolBar : public QToolBar, public DocumentObserver
{
    Q_OBJECT

public:
    explicit NavigationToolBar(QWidget *parent = nullptr);
    ~NavigationToolBar() override;

    void documentLoaded() override;
    void documentClosed() override;
    void pageChanged(int page) override;

Q_SIGNALS:
    void zoomChanged(qreal value); // NOLINT(readability-inconsistent-declaration-parameter-name)
    void rotationChanged(int rotation); // NOLINT(readability-inconsistent-declaration-parameter-name)

private Q_SLOTS:
    void slotGoFirst();
    void slotGoPrev();
    void slotGoNext();
    void slotGoLast();
    void slotComboActivated(int index);
    void slotZoomComboActivated(int index);
    void slotRotationComboChanged(int idx);

private:
    QAction *m_firstAct;
    QAction *m_prevAct;
    QComboBox *m_pageCombo;
    QAction *m_nextAct;
    QAction *m_lastAct;
    QComboBox *m_zoomCombo;
    QComboBox *m_rotationCombo;
};

#endif
