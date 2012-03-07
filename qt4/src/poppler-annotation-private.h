/* poppler-link-extractor-private.h: qt interface to poppler
 * Copyright (C) 2007, Pino Toscano <pino@kde.org>
 * Copyright (C) 2012, Tobias Koenig <tokoe@kdab.com>
 * Copyright (C) 2012, Fabio D'Urso <fabiodurso@hotmail.it>
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

#ifndef _POPPLER_ANNOTATION_PRIVATE_H_
#define _POPPLER_ANNOTATION_PRIVATE_H_

#include <QtCore/QSharedDataPointer>

#include "poppler-annotation.h"

#include <Object.h>

namespace Poppler
{

class AnnotationPrivate : public QSharedData
{
    public:
        AnnotationPrivate();
        virtual ~AnnotationPrivate();

        /* Returns an Annotation of the right subclass whose d_ptr points to
         * this AnnotationPrivate */
        virtual Annotation * makeAlias() = 0;

        /* properties: contents related */
        QString author;
        QString contents;
        QString uniqueName;
        QDateTime modDate;       // before or equal to currentDateTime()
        QDateTime creationDate;  // before or equal to modifyDate

        /* properties: look/interaction related */
        int flags;
        QRectF boundary;

        /* style and popup */
        Annotation::Style style;
        Annotation::Popup popup;

        /* revisions */
        Annotation::RevScope revisionScope;
        Annotation::RevType revisionType;
        QList<Annotation*> revisions;

        Ref pdfObjectReference;
};

}

#endif
