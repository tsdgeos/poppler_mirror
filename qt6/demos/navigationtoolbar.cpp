/*
 * Copyright (C) 2008-2009, Pino Toscano <pino@kde.org>
 * Copyright (C) 2013, Fabio D'Urso <fabiodurso@hotmail.it>
 * Copyright (C) 2019, 2025, Albert Astals Cid <aacid@kde.org>
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

#include "navigationtoolbar.h"

#include <poppler-qt6.h>

#include <QAction>
#include <QComboBox>
#include <QDebug>

NavigationToolBar::NavigationToolBar(QWidget *parent) : QToolBar(parent)
{
    m_firstAct = addAction(tr("First"), this, &NavigationToolBar::slotGoFirst);
    m_prevAct = addAction(tr("Previous"), this, &NavigationToolBar::slotGoPrev);
    m_pageCombo = new QComboBox(this);
    connect(m_pageCombo, &QComboBox::activated, this, &NavigationToolBar::slotComboActivated);
    addWidget(m_pageCombo);
    m_nextAct = addAction(tr("Next"), this, &NavigationToolBar::slotGoNext);
    m_lastAct = addAction(tr("Last"), this, &NavigationToolBar::slotGoLast);

    addSeparator();

    m_zoomCombo = new QComboBox(this);
    m_zoomCombo->setEditable(true);
    m_zoomCombo->addItem(tr("10%"));
    m_zoomCombo->addItem(tr("25%"));
    m_zoomCombo->addItem(tr("33%"));
    m_zoomCombo->addItem(tr("50%"));
    m_zoomCombo->addItem(tr("66%"));
    m_zoomCombo->addItem(tr("75%"));
    m_zoomCombo->addItem(tr("100%"));
    m_zoomCombo->addItem(tr("125%"));
    m_zoomCombo->addItem(tr("150%"));
    m_zoomCombo->addItem(tr("200%"));
    m_zoomCombo->addItem(tr("300%"));
    m_zoomCombo->addItem(tr("400%"));
    m_zoomCombo->setCurrentIndex(6); // "100%"
    connect(m_zoomCombo, &QComboBox::activated, this, &NavigationToolBar::slotZoomComboActivated);
    addWidget(m_zoomCombo);

    m_rotationCombo = new QComboBox(this);
    // NOTE: \302\260 = degree symbol
    m_rotationCombo->addItem(tr("0\302\260"));
    m_rotationCombo->addItem(tr("90\302\260"));
    m_rotationCombo->addItem(tr("180\302\260"));
    m_rotationCombo->addItem(tr("270\302\260"));
    connect(m_rotationCombo, &QComboBox::currentIndexChanged, this, &NavigationToolBar::slotRotationComboChanged);
    addWidget(m_rotationCombo);

    documentClosed();
}

NavigationToolBar::~NavigationToolBar() = default;

void NavigationToolBar::documentLoaded()
{
    const int pageCount = document()->numPages();
    for (int i = 0; i < pageCount; ++i) {
        m_pageCombo->addItem(QString::number(i + 1));
    }
    m_pageCombo->setEnabled(true);
}

void NavigationToolBar::documentClosed()
{
    m_firstAct->setEnabled(false);
    m_prevAct->setEnabled(false);
    m_nextAct->setEnabled(false);
    m_lastAct->setEnabled(false);
    m_pageCombo->clear();
    m_pageCombo->setEnabled(false);
}

void NavigationToolBar::pageChanged(int page)
{
    const int pageCount = document()->numPages();
    m_firstAct->setEnabled(page > 0);
    m_prevAct->setEnabled(page > 0);
    m_nextAct->setEnabled(page < (pageCount - 1));
    m_lastAct->setEnabled(page < (pageCount - 1));
    m_pageCombo->setCurrentIndex(page);
}

void NavigationToolBar::slotGoFirst()
{
    setPage(0);
}

void NavigationToolBar::slotGoPrev()
{
    setPage(page() - 1);
}

void NavigationToolBar::slotGoNext()
{
    setPage(page() + 1);
}

void NavigationToolBar::slotGoLast()
{
    setPage(document()->numPages() - 1);
}

void NavigationToolBar::slotComboActivated(int index)
{
    setPage(index);
}

void NavigationToolBar::slotZoomComboActivated(int /*index*/)
{
    QString text = m_zoomCombo->currentText();
    text.remove(QLatin1Char('%'));
    bool ok = false;
    int value = text.toInt(&ok);
    if (ok && value >= 10) {
        Q_EMIT zoomChanged(qreal(value) / 100);
    }
}

void NavigationToolBar::slotRotationComboChanged(int idx)
{
    Q_EMIT rotationChanged(idx * 90);
}
