/* poppler-annotation.cc: qt interface to poppler
 * Copyright (C) 2006, 2009, 2012-2015, 2018-2022, 2024 Albert Astals Cid <aacid@kde.org>
 * Copyright (C) 2006, 2008, 2010 Pino Toscano <pino@kde.org>
 * Copyright (C) 2012, Guillermo A. Amaral B. <gamaral@kde.org>
 * Copyright (C) 2012-2014 Fabio D'Urso <fabiodurso@hotmail.it>
 * Copyright (C) 2012, 2015, Tobias Koenig <tobias.koenig@kdab.com>
 * Copyright (C) 2018 Adam Reichold <adam.reichold@t-online.de>
 * Copyright (C) 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by the LiMux project of the city of Munich
 * Copyright (C) 2018 Intevation GmbH <intevation@intevation.de>
 * Copyright (C) 2018 Dileep Sankhla <sankhla.dileep96@gmail.com>
 * Copyright (C) 2018, 2019 Tobias Deiminger <haxtibal@posteo.de>
 * Copyright (C) 2018 Carlos Garcia Campos <carlosgc@gnome.org>
 * Copyright (C) 2020, 2022 Oliver Sander <oliver.sander@tu-dresden.de>
 * Copyright (C) 2020 Katarina Behrens <Katarina.Behrens@cib.de>
 * Copyright (C) 2020 Thorsten Behrens <Thorsten.Behrens@CIB.de>
 * Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by Technische Universität Dresden
 * Copyright (C) 2021 Mahmoud Ahmed Khalil <mahmoudkhalil11@gmail.com>
 * Copyright (C) 2024 Pratham Gandhi <ppg.1382@gmail.com>
 * Copyright (C) 2024, 2025 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
 * Adapting code from
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>
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

// qt/kde includes
#include <QtCore/QRegExp>
#include <QtCore/QtAlgorithms>
#include <QtXml/QDomElement>
#include <QtGui/QColor>
#include <QtGui/QTransform>
#include <QImage>

// local includes
#include "poppler-annotation.h"
#include "poppler-link.h"
#include "poppler-qt5.h"
#include "poppler-annotation-helper.h"
#include "poppler-annotation-private.h"
#include "poppler-page-private.h"
#include "poppler-private.h"

// poppler includes
#include <Page.h>
#include <Annot.h>
#include <Gfx.h>
#include <Error.h>
#include <FileSpec.h>
#include <Link.h>
#include <DateInfo.h>

/* Almost all getters directly query the underlying poppler annotation, with
 * the exceptions of link, file attachment, sound, movie and screen annotations,
 * Whose data retrieval logic has not been moved yet. Their getters return
 * static data set at creation time by findAnnotations
 */

template<typename T, typename U>
static std::unique_ptr<T> static_pointer_cast(std::unique_ptr<U> &&in)
{
    return std::unique_ptr<T>(static_cast<std::add_pointer_t<T>>(in.release()));
}

namespace Poppler {

// BEGIN AnnotationUtils implementation
Annotation *AnnotationUtils::createAnnotation(const QDomElement &annElement)
{
    // safety check on annotation element
    if (!annElement.hasAttribute(QStringLiteral("type"))) {
        return nullptr;
    }

    // build annotation of given type
    Annotation *annotation = nullptr;
    int typeNumber = annElement.attribute(QStringLiteral("type")).toInt();
    switch (typeNumber) {
    case Annotation::AText:
        annotation = new TextAnnotation(annElement);
        break;
    case Annotation::ALine:
        annotation = new LineAnnotation(annElement);
        break;
    case Annotation::AGeom:
        annotation = new GeomAnnotation(annElement);
        break;
    case Annotation::AHighlight:
        annotation = new HighlightAnnotation(annElement);
        break;
    case Annotation::AStamp:
        annotation = new StampAnnotation(annElement);
        break;
    case Annotation::AInk:
        annotation = new InkAnnotation(annElement);
        break;
    case Annotation::ACaret:
        annotation = new CaretAnnotation(annElement);
        break;
    }

    // return created annotation
    return annotation;
}

void AnnotationUtils::storeAnnotation(const Annotation *ann, QDomElement &annElement, QDomDocument &document)
{
    // save annotation's type as element's attribute
    annElement.setAttribute(QStringLiteral("type"), (uint)ann->subType());

    // append all annotation data as children of this node
    ann->store(annElement, document);
}

QDomElement AnnotationUtils::findChildElement(const QDomNode &parentNode, const QString &name)
{
    // loop through the whole children and return a 'name' named element
    QDomNode subNode = parentNode.firstChild();
    while (subNode.isElement()) {
        QDomElement element = subNode.toElement();
        if (element.tagName() == name) {
            return element;
        }
        subNode = subNode.nextSibling();
    }
    // if the name can't be found, return a dummy null element
    return QDomElement();
}
// END AnnotationUtils implementation

// BEGIN AnnotationAppearancePrivate implementation
AnnotationAppearancePrivate::AnnotationAppearancePrivate(Annot *annot)
{
    if (annot) {
        appearance = annot->getAppearance();
    } else {
        appearance.setToNull();
    }
}
// END AnnotationAppearancePrivate implementation

// BEGIN AnnotationAppearance implementation
AnnotationAppearance::AnnotationAppearance(AnnotationAppearancePrivate *annotationAppearancePrivate) : d(annotationAppearancePrivate) { }

AnnotationAppearance::~AnnotationAppearance()
{
    delete d;
}
// END AnnotationAppearance implementation

// BEGIN Annotation implementation
AnnotationPrivate::AnnotationPrivate() : flags(0), revisionScope(Annotation::Root), revisionType(Annotation::None), pdfAnnot(nullptr), pdfPage(nullptr), parentDoc(nullptr) { }

void getRawDataFromQImage(const QImage &qimg, int bitsPerPixel, QByteArray *data, QByteArray *sMaskData)
{
    const int height = qimg.height();
    const int width = qimg.width();

    switch (bitsPerPixel) {
    case 1:
        for (int line = 0; line < height; line++) {
            const char *lineData = reinterpret_cast<const char *>(qimg.scanLine(line));
            for (int offset = 0; offset < (width + 7) / 8; offset++) {
                data->append(lineData[offset]);
            }
        }
        break;
    case 8:
    case 24:
        data->append((const char *)qimg.bits(), static_cast<int>(qimg.sizeInBytes()));
        break;
    case 32:
        for (int line = 0; line < height; line++) {
            const QRgb *lineData = reinterpret_cast<const QRgb *>(qimg.scanLine(line));
            for (int offset = 0; offset < width; offset++) {
                char a = (char)qAlpha(lineData[offset]);
                char r = (char)qRed(lineData[offset]);
                char g = (char)qGreen(lineData[offset]);
                char b = (char)qBlue(lineData[offset]);

                data->append(r);
                data->append(g);
                data->append(b);

                sMaskData->append(a);
            }
        }
        break;
    }
}

void AnnotationPrivate::addRevision(Annotation *ann, Annotation::RevScope scope, Annotation::RevType type)
{
    /* Since ownership stays with the caller, create an alias of ann */
    revisions.push_back(ann->d_ptr->makeAlias());

    /* Set revision properties */
    revisionScope = scope;
    revisionType = type;
}

AnnotationPrivate::~AnnotationPrivate() = default;

void AnnotationPrivate::tieToNativeAnnot(std::shared_ptr<Annot> ann, ::Page *page, Poppler::DocumentData *doc)
{
    if (pdfAnnot) {
        error(errIO, -1, "Annotation is already tied");
        return;
    }

    pdfAnnot = std::move(ann);
    pdfPage = page;
    parentDoc = doc;
}

/* This method is called when a new annotation is created, after pdfAnnot and
 * pdfPage have been set */
void AnnotationPrivate::flushBaseAnnotationProperties()
{
    Q_ASSERT(pdfPage);

    std::unique_ptr<Annotation> q = makeAlias(); // Setters are defined in the public class

    // Since pdfAnnot has been set, this calls will write in the Annot object
    q->setAuthor(author);
    q->setContents(contents);
    q->setUniqueName(uniqueName);
    q->setModificationDate(modDate);
    q->setCreationDate(creationDate);
    q->setFlags(flags);
    // q->setBoundary(boundary); -- already set by subclass-specific code
    q->setStyle(style);
    q->setPopup(popup);

    // Flush revisions
    revisions.clear();

    // Clear some members to save memory
    author.clear();
    contents.clear();
    uniqueName.clear();
    revisions.clear();
}

// Returns matrix to convert from user space coords (oriented according to the
// specified rotation) to normalized coords
static void fillNormalizationMTX(::Page *pdfPage, double MTX[6], int pageRotation)
{
    Q_ASSERT(pdfPage);

    // build a normalized transform matrix for this page at 100% scale
    GfxState *gfxState = new GfxState(72.0, 72.0, pdfPage->getCropBox(), pageRotation, true);
    const double *gfxCTM = gfxState->getCTM();

    double w = pdfPage->getCropWidth();
    double h = pdfPage->getCropHeight();

    // Swap width and height if the page is rotated landscape or seascape
    if (pageRotation == 90 || pageRotation == 270) {
        double t = w;
        w = h;
        h = t;
    }

    for (int i = 0; i < 6; i += 2) {
        MTX[i] = gfxCTM[i] / w;
        MTX[i + 1] = gfxCTM[i + 1] / h;
    }
    delete gfxState;
}

// Returns matrix to convert from user space coords (i.e. those that are stored
// in the PDF file) to normalized coords (i.e. those that we expose to clients).
// This method also applies a rotation around the top-left corner if the
// FixedRotation flag is set.
void AnnotationPrivate::fillTransformationMTX(double MTX[6]) const
{
    Q_ASSERT(pdfPage);
    Q_ASSERT(pdfAnnot);

    const int pageRotate = pdfPage->getRotate();

    if (pageRotate == 0 || (pdfAnnot->getFlags() & Annot::flagNoRotate) == 0) {
        // Use the normalization matrix for this page's rotation
        fillNormalizationMTX(pdfPage, MTX, pageRotate);
    } else {
        // Clients expect coordinates relative to this page's rotation, but
        // FixedRotation annotations internally use unrotated coordinates:
        // construct matrix to both normalize and rotate coordinates using the
        // top-left corner as rotation pivot

        double MTXnorm[6];
        fillNormalizationMTX(pdfPage, MTXnorm, pageRotate);

        QTransform transform(MTXnorm[0], MTXnorm[1], MTXnorm[2], MTXnorm[3], MTXnorm[4], MTXnorm[5]);
        transform.translate(+pdfAnnot->getXMin(), +pdfAnnot->getYMax());
        transform.rotate(pageRotate);
        transform.translate(-pdfAnnot->getXMin(), -pdfAnnot->getYMax());

        MTX[0] = transform.m11();
        MTX[1] = transform.m12();
        MTX[2] = transform.m21();
        MTX[3] = transform.m22();
        MTX[4] = transform.dx();
        MTX[5] = transform.dy();
    }
}

QRectF AnnotationPrivate::fromPdfRectangle(const PDFRectangle &r) const
{
    double swp, MTX[6];
    fillTransformationMTX(MTX);

    QPointF p1, p2;
    XPDFReader::transform(MTX, r.x1, r.y1, p1);
    XPDFReader::transform(MTX, r.x2, r.y2, p2);

    double tl_x = p1.x();
    double tl_y = p1.y();
    double br_x = p2.x();
    double br_y = p2.y();

    if (tl_x > br_x) {
        swp = tl_x;
        tl_x = br_x;
        br_x = swp;
    }

    if (tl_y > br_y) {
        swp = tl_y;
        tl_y = br_y;
        br_y = swp;
    }

    return QRectF(QPointF(tl_x, tl_y), QPointF(br_x, br_y));
}

// This function converts a boundary QRectF in normalized coords to a
// PDFRectangle in user coords. If the FixedRotation flag is set, this function
// also applies a rotation around the top-left corner: it's the inverse of
// the transformation produced by fillTransformationMTX, but we can't use
// fillTransformationMTX here because it relies on the native annotation
// object's boundary rect to be already set up.
PDFRectangle boundaryToPdfRectangle(::Page *pdfPage, const QRectF &r, int rFlags)
{
    Q_ASSERT(pdfPage);

    const double w = pdfPage->getCropWidth();
    const double h = pdfPage->getCropHeight();

    if (w == 0 || h == 0) {
        // page is broken, there's nothing to transform
        return {};
    }

    const int pageRotate = pdfPage->getRotate();

    double MTX[6];
    fillNormalizationMTX(pdfPage, MTX, pageRotate);

    double tl_x, tl_y, br_x, br_y, swp;
    XPDFReader::invTransform(MTX, r.topLeft(), tl_x, tl_y);
    XPDFReader::invTransform(MTX, r.bottomRight(), br_x, br_y);

    if (tl_x > br_x) {
        swp = tl_x;
        tl_x = br_x;
        br_x = swp;
    }

    if (tl_y > br_y) {
        swp = tl_y;
        tl_y = br_y;
        br_y = swp;
    }

    const int rotationFixUp = (rFlags & Annotation::FixedRotation) ? pageRotate : 0;
    const double width = br_x - tl_x;
    const double height = br_y - tl_y;

    if (rotationFixUp == 0) {
        return PDFRectangle(tl_x, tl_y, br_x, br_y);
    } else if (rotationFixUp == 90) {
        return PDFRectangle(tl_x, tl_y - width, tl_x + height, tl_y);
    } else if (rotationFixUp == 180) {
        return PDFRectangle(br_x, tl_y - height, br_x + width, tl_y);
    } else { // rotationFixUp == 270
        return PDFRectangle(br_x, br_y - width, br_x + height, br_y);
    }
}

PDFRectangle AnnotationPrivate::boundaryToPdfRectangle(const QRectF &r, int rFlags) const
{
    return Poppler::boundaryToPdfRectangle(pdfPage, r, rFlags);
}

std::unique_ptr<AnnotPath> AnnotationPrivate::toAnnotPath(const QLinkedList<QPointF> &list) const
{
    const int count = list.size();
    std::vector<AnnotCoord> ac;
    ac.reserve(count);

    double MTX[6];
    fillTransformationMTX(MTX);

    Q_FOREACH (const QPointF &p, list) {
        double x, y;
        XPDFReader::invTransform(MTX, p, x, y);
        ac.emplace_back(x, y);
    }

    return std::make_unique<AnnotPath>(std::move(ac));
}

QList<Annotation *> AnnotationPrivate::findAnnotations(::Page *pdfPage, DocumentData *doc, const QSet<Annotation::SubType> &subtypes, int parentID)
{
    Annots *annots = pdfPage->getAnnots();

    const bool wantTextAnnotations = subtypes.isEmpty() || subtypes.contains(Annotation::AText);
    const bool wantLineAnnotations = subtypes.isEmpty() || subtypes.contains(Annotation::ALine);
    const bool wantGeomAnnotations = subtypes.isEmpty() || subtypes.contains(Annotation::AGeom);
    const bool wantHighlightAnnotations = subtypes.isEmpty() || subtypes.contains(Annotation::AHighlight);
    const bool wantStampAnnotations = subtypes.isEmpty() || subtypes.contains(Annotation::AStamp);
    const bool wantInkAnnotations = subtypes.isEmpty() || subtypes.contains(Annotation::AInk);
    const bool wantLinkAnnotations = subtypes.isEmpty() || subtypes.contains(Annotation::ALink);
    const bool wantCaretAnnotations = subtypes.isEmpty() || subtypes.contains(Annotation::ACaret);
    const bool wantFileAttachmentAnnotations = subtypes.isEmpty() || subtypes.contains(Annotation::AFileAttachment);
    const bool wantSoundAnnotations = subtypes.isEmpty() || subtypes.contains(Annotation::ASound);
    const bool wantMovieAnnotations = subtypes.isEmpty() || subtypes.contains(Annotation::AMovie);
    const bool wantScreenAnnotations = subtypes.isEmpty() || subtypes.contains(Annotation::AScreen);
    const bool wantWidgetAnnotations = subtypes.isEmpty() || subtypes.contains(Annotation::AWidget);

    // Create Annotation objects and tie to their native Annot
    QList<Annotation *> res;
    for (const std::shared_ptr<Annot> &ann : annots->getAnnots()) {
        if (!ann) {
            error(errInternal, -1, "Annot is null");
            continue;
        }

        // Check parent annotation
        AnnotMarkup *markupann = dynamic_cast<AnnotMarkup *>(ann.get());
        if (!markupann) {
            // Assume it's a root annotation, and skip if user didn't request it
            if (parentID != -1) {
                continue;
            }
        } else if (markupann->getInReplyToID() != parentID) {
            continue;
        }

        /* Create Annotation of the right subclass */
        Annotation *annotation = nullptr;
        Annot::AnnotSubtype subType = ann->getType();

        switch (subType) {
        case Annot::typeText:
            if (!wantTextAnnotations) {
                continue;
            }
            annotation = new TextAnnotation(TextAnnotation::Linked);
            break;
        case Annot::typeFreeText:
            if (!wantTextAnnotations) {
                continue;
            }
            annotation = new TextAnnotation(TextAnnotation::InPlace);
            break;
        case Annot::typeLine:
            if (!wantLineAnnotations) {
                continue;
            }
            annotation = new LineAnnotation(LineAnnotation::StraightLine);
            break;
        case Annot::typePolygon:
        case Annot::typePolyLine:
            if (!wantLineAnnotations) {
                continue;
            }
            annotation = new LineAnnotation(LineAnnotation::Polyline);
            break;
        case Annot::typeSquare:
        case Annot::typeCircle:
            if (!wantGeomAnnotations) {
                continue;
            }
            annotation = new GeomAnnotation();
            break;
        case Annot::typeHighlight:
        case Annot::typeUnderline:
        case Annot::typeSquiggly:
        case Annot::typeStrikeOut:
            if (!wantHighlightAnnotations) {
                continue;
            }
            annotation = new HighlightAnnotation();
            break;
        case Annot::typeStamp:
            if (!wantStampAnnotations) {
                continue;
            }
            annotation = new StampAnnotation();
            break;
        case Annot::typeInk:
            if (!wantInkAnnotations) {
                continue;
            }
            annotation = new InkAnnotation();
            break;
        case Annot::typeLink: /* TODO: Move logic to getters */
        {
            if (!wantLinkAnnotations) {
                continue;
            }
            // parse Link params
            AnnotLink *linkann = static_cast<AnnotLink *>(ann.get());
            LinkAnnotation *l = new LinkAnnotation();
            annotation = l;

            // -> hlMode
            l->setLinkHighlightMode((LinkAnnotation::HighlightMode)linkann->getLinkEffect());

            // -> link region
            // TODO

            // reading link action
            if (linkann->getAction()) {
                Link *popplerLink = PageData::convertLinkActionToLink(linkann->getAction(), doc, QRectF());
                if (popplerLink) {
                    l->setLinkDestination(popplerLink);
                }
            }
            break;
        }
        case Annot::typeCaret:
            if (!wantCaretAnnotations) {
                continue;
            }
            annotation = new CaretAnnotation();
            break;
        case Annot::typeFileAttachment: /* TODO: Move logic to getters */
        {
            if (!wantFileAttachmentAnnotations) {
                continue;
            }
            AnnotFileAttachment *attachann = static_cast<AnnotFileAttachment *>(ann.get());
            FileAttachmentAnnotation *f = new FileAttachmentAnnotation();
            annotation = f;
            // -> fileIcon
            f->setFileIconName(QString::fromLatin1(attachann->getName()->c_str()));
            // -> embeddedFile
            auto filespec = std::make_unique<FileSpec>(attachann->getFile());
            f->setEmbeddedFile(new EmbeddedFile(*new EmbeddedFileData(std::move(filespec))));
            break;
        }
        case Annot::typeSound: /* TODO: Move logic to getters */
        {
            if (!wantSoundAnnotations) {
                continue;
            }
            AnnotSound *soundann = static_cast<AnnotSound *>(ann.get());
            SoundAnnotation *s = new SoundAnnotation();
            annotation = s;

            // -> soundIcon
            s->setSoundIconName(QString::fromLatin1(soundann->getName()->c_str()));
            // -> sound
            s->setSound(new SoundObject(soundann->getSound()));
            break;
        }
        case Annot::typeMovie: /* TODO: Move logic to getters */
        {
            if (!wantMovieAnnotations) {
                continue;
            }
            AnnotMovie *movieann = static_cast<AnnotMovie *>(ann.get());
            MovieAnnotation *m = new MovieAnnotation();
            annotation = m;

            // -> movie
            MovieObject *movie = new MovieObject(movieann);
            m->setMovie(movie);
            // -> movieTitle
            const GooString *movietitle = movieann->getTitle();
            if (movietitle) {
                m->setMovieTitle(QString::fromLatin1(movietitle->c_str()));
            }
            break;
        }
        case Annot::typeScreen: {
            if (!wantScreenAnnotations) {
                continue;
            }
            AnnotScreen *screenann = static_cast<AnnotScreen *>(ann.get());
            // TODO Support other link types than Link::Rendition in ScreenAnnotation
            if (!screenann->getAction() || screenann->getAction()->getKind() != actionRendition) {
                continue;
            }
            ScreenAnnotation *s = new ScreenAnnotation();
            annotation = s;

            // -> screen
            Link *popplerLink = PageData::convertLinkActionToLink(screenann->getAction(), doc, QRectF());
            s->setAction(static_cast<Poppler::LinkRendition *>(popplerLink));

            // -> screenTitle
            const GooString *screentitle = screenann->getTitle();
            if (screentitle) {
                s->setScreenTitle(UnicodeParsedString(screentitle));
            }
            break;
        }
        case Annot::typePopup:
            continue; // popups are parsed by Annotation's window() getter
        case Annot::typeUnknown:
            continue; // special case for ignoring unknown annotations
        case Annot::typeWidget:
            if (!wantWidgetAnnotations) {
                continue;
            }
            annotation = new WidgetAnnotation();
            break;
        case Annot::typeRichMedia: {
            const AnnotRichMedia *annotRichMedia = static_cast<AnnotRichMedia *>(ann.get());

            RichMediaAnnotation *richMediaAnnotation = new RichMediaAnnotation;

            const AnnotRichMedia::Settings *annotSettings = annotRichMedia->getSettings();
            if (annotSettings) {
                RichMediaAnnotation::Settings *settings = new RichMediaAnnotation::Settings;

                if (annotSettings->getActivation()) {
                    RichMediaAnnotation::Activation *activation = new RichMediaAnnotation::Activation;

                    switch (annotSettings->getActivation()->getCondition()) {
                    case AnnotRichMedia::Activation::conditionPageOpened:
                        activation->setCondition(RichMediaAnnotation::Activation::PageOpened);
                        break;
                    case AnnotRichMedia::Activation::conditionPageVisible:
                        activation->setCondition(RichMediaAnnotation::Activation::PageVisible);
                        break;
                    case AnnotRichMedia::Activation::conditionUserAction:
                        activation->setCondition(RichMediaAnnotation::Activation::UserAction);
                        break;
                    }

                    settings->setActivation(activation);
                }

                if (annotSettings->getDeactivation()) {
                    RichMediaAnnotation::Deactivation *deactivation = new RichMediaAnnotation::Deactivation;

                    switch (annotSettings->getDeactivation()->getCondition()) {
                    case AnnotRichMedia::Deactivation::conditionPageClosed:
                        deactivation->setCondition(RichMediaAnnotation::Deactivation::PageClosed);
                        break;
                    case AnnotRichMedia::Deactivation::conditionPageInvisible:
                        deactivation->setCondition(RichMediaAnnotation::Deactivation::PageInvisible);
                        break;
                    case AnnotRichMedia::Deactivation::conditionUserAction:
                        deactivation->setCondition(RichMediaAnnotation::Deactivation::UserAction);
                        break;
                    }

                    settings->setDeactivation(deactivation);
                }

                richMediaAnnotation->setSettings(settings);
            }

            const AnnotRichMedia::Content *annotContent = annotRichMedia->getContent();
            if (annotContent) {
                RichMediaAnnotation::Content *content = new RichMediaAnnotation::Content;

                const int configurationsCount = annotContent->getConfigurationsCount();
                if (configurationsCount > 0) {
                    QList<RichMediaAnnotation::Configuration *> configurations;

                    for (int i = 0; i < configurationsCount; ++i) {
                        const AnnotRichMedia::Configuration *annotConfiguration = annotContent->getConfiguration(i);
                        if (!annotConfiguration) {
                            continue;
                        }

                        RichMediaAnnotation::Configuration *configuration = new RichMediaAnnotation::Configuration;

                        if (annotConfiguration->getName()) {
                            configuration->setName(UnicodeParsedString(annotConfiguration->getName()));
                        }

                        switch (annotConfiguration->getType()) {
                        case AnnotRichMedia::Configuration::type3D:
                            configuration->setType(RichMediaAnnotation::Configuration::Type3D);
                            break;
                        case AnnotRichMedia::Configuration::typeFlash:
                            configuration->setType(RichMediaAnnotation::Configuration::TypeFlash);
                            break;
                        case AnnotRichMedia::Configuration::typeSound:
                            configuration->setType(RichMediaAnnotation::Configuration::TypeSound);
                            break;
                        case AnnotRichMedia::Configuration::typeVideo:
                            configuration->setType(RichMediaAnnotation::Configuration::TypeVideo);
                            break;
                        }

                        const int instancesCount = annotConfiguration->getInstancesCount();
                        if (instancesCount > 0) {
                            QList<RichMediaAnnotation::Instance *> instances;

                            for (int j = 0; j < instancesCount; ++j) {
                                const AnnotRichMedia::Instance *annotInstance = annotConfiguration->getInstance(j);
                                if (!annotInstance) {
                                    continue;
                                }

                                RichMediaAnnotation::Instance *instance = new RichMediaAnnotation::Instance;

                                switch (annotInstance->getType()) {
                                case AnnotRichMedia::Instance::type3D:
                                    instance->setType(RichMediaAnnotation::Instance::Type3D);
                                    break;
                                case AnnotRichMedia::Instance::typeFlash:
                                    instance->setType(RichMediaAnnotation::Instance::TypeFlash);
                                    break;
                                case AnnotRichMedia::Instance::typeSound:
                                    instance->setType(RichMediaAnnotation::Instance::TypeSound);
                                    break;
                                case AnnotRichMedia::Instance::typeVideo:
                                    instance->setType(RichMediaAnnotation::Instance::TypeVideo);
                                    break;
                                }

                                const AnnotRichMedia::Params *annotParams = annotInstance->getParams();
                                if (annotParams) {
                                    RichMediaAnnotation::Params *params = new RichMediaAnnotation::Params;

                                    if (annotParams->getFlashVars()) {
                                        params->setFlashVars(UnicodeParsedString(annotParams->getFlashVars()));
                                    }

                                    instance->setParams(params);
                                }

                                instances.append(instance);
                            }

                            configuration->setInstances(instances);
                        }

                        configurations.append(configuration);
                    }

                    content->setConfigurations(configurations);
                }

                const int assetsCount = annotContent->getAssetsCount();
                if (assetsCount > 0) {
                    QList<RichMediaAnnotation::Asset *> assets;

                    for (int i = 0; i < assetsCount; ++i) {
                        const AnnotRichMedia::Asset *annotAsset = annotContent->getAsset(i);
                        if (!annotAsset) {
                            continue;
                        }

                        RichMediaAnnotation::Asset *asset = new RichMediaAnnotation::Asset;

                        if (annotAsset->getName()) {
                            asset->setName(UnicodeParsedString(annotAsset->getName()));
                        }

                        auto fileSpec = std::make_unique<FileSpec>(annotAsset->getFileSpec());
                        asset->setEmbeddedFile(new EmbeddedFile(*new EmbeddedFileData(std::move(fileSpec))));

                        assets.append(asset);
                    }

                    content->setAssets(assets);
                }

                richMediaAnnotation->setContent(content);
            }

            annotation = richMediaAnnotation;

            break;
        }
        default: {
#define CASE_FOR_TYPE(thetype)                                                                                                                                                                                                                 \
    case Annot::type##thetype:                                                                                                                                                                                                                 \
        error(errUnimplemented, -1, "Annotation " #thetype " not supported");                                                                                                                                                                  \
        break;
            switch (subType) {
                CASE_FOR_TYPE(PrinterMark)
                CASE_FOR_TYPE(TrapNet)
                CASE_FOR_TYPE(Watermark)
                CASE_FOR_TYPE(3D)
            default:
                error(errUnimplemented, -1, "Annotation {0:d} not supported", subType);
            }
            continue;
#undef CASE_FOR_TYPE
        }
        }

        annotation->d_ptr->tieToNativeAnnot(ann, pdfPage, doc);
        res.append(annotation);
    }

    return res;
}

Ref AnnotationPrivate::pdfObjectReference() const
{
    if (pdfAnnot == nullptr) {
        return Ref::INVALID();
    }

    return pdfAnnot->getRef();
}

Link *AnnotationPrivate::additionalAction(Annotation::AdditionalActionType type) const
{
    if (pdfAnnot->getType() != Annot::typeScreen && pdfAnnot->getType() != Annot::typeWidget) {
        return nullptr;
    }

    const Annot::AdditionalActionsType actionType = toPopplerAdditionalActionType(type);

    std::unique_ptr<::LinkAction> linkAction = nullptr;
    if (pdfAnnot->getType() == Annot::typeScreen) {
        linkAction = static_cast<AnnotScreen *>(pdfAnnot.get())->getAdditionalAction(actionType);
    } else {
        linkAction = static_cast<AnnotWidget *>(pdfAnnot.get())->getAdditionalAction(actionType);
    }

    Link *link = nullptr;

    if (linkAction) {
        link = PageData::convertLinkActionToLink(linkAction.get(), parentDoc, QRectF());
    }

    return link;
}

void AnnotationPrivate::addAnnotationToPage(::Page *pdfPage, DocumentData *doc, const Annotation *ann)
{
    if (ann->d_ptr->pdfAnnot != nullptr) {
        error(errIO, -1, "Annotation is already tied");
        return;
    }

    // Unimplemented annotations can't be created by the user because their ctor
    // is private. Therefore, createNativeAnnot will never return 0
    std::shared_ptr<Annot> nativeAnnot = ann->d_ptr->createNativeAnnot(pdfPage, doc);
    Q_ASSERT(nativeAnnot);

    if (ann->d_ptr->annotationAppearance.isStream()) {
        nativeAnnot->setNewAppearance(ann->d_ptr->annotationAppearance.copy());
    }

    pdfPage->addAnnot(nativeAnnot);
}

void AnnotationPrivate::removeAnnotationFromPage(::Page *pdfPage, const Annotation *ann)
{
    if (ann->d_ptr->pdfAnnot == nullptr) {
        error(errIO, -1, "Annotation is not tied");
        return;
    }

    if (ann->d_ptr->pdfPage != pdfPage) {
        error(errIO, -1, "Annotation doesn't belong to the specified page");
        return;
    }

    // Remove annotation
    pdfPage->removeAnnot(ann->d_ptr->pdfAnnot);

    // Destroy object
    delete ann;
}

class TextAnnotationPrivate : public AnnotationPrivate
{
public:
    TextAnnotationPrivate();
    std::unique_ptr<Annotation> makeAlias() override;
    std::shared_ptr<Annot> createNativeAnnot(::Page *destPage, DocumentData *doc) override;
    void setDefaultAppearanceToNative();
    std::unique_ptr<DefaultAppearance> getDefaultAppearanceFromNative() const;

    // data fields
    TextAnnotation::TextType textType;
    QString textIcon;
    std::optional<QFont> textFont;
    QColor textColor = Qt::black;
    int inplaceAlign; // 0:left, 1:center, 2:right
    QVector<QPointF> inplaceCallout;
    TextAnnotation::InplaceIntent inplaceIntent;
};

class Annotation::Style::Private : public QSharedData
{
public:
    Private() : opacity(1.0), width(1.0), lineStyle(Solid), xCorners(0.0), yCorners(0.0), lineEffect(NoEffect), effectIntensity(1.0)
    {
        dashArray.resize(1);
        dashArray[0] = 3;
    }

    QColor color;
    double opacity;
    double width;
    Annotation::LineStyle lineStyle;
    double xCorners;
    double yCorners;
    QVector<double> dashArray;
    Annotation::LineEffect lineEffect;
    double effectIntensity;
};

Annotation::Style::Style() : d(new Private) { }

Annotation::Style::Style(const Style &other) = default;

Annotation::Style &Annotation::Style::operator=(const Style &other)
{
    if (this != &other) {
        d = other.d;
    }

    return *this;
}

Annotation::Style::~Style() = default;

QColor Annotation::Style::color() const
{
    return d->color;
}

void Annotation::Style::setColor(const QColor &color)
{
    d->color = color;
}

double Annotation::Style::opacity() const
{
    return d->opacity;
}

void Annotation::Style::setOpacity(double opacity)
{
    d->opacity = opacity;
}

double Annotation::Style::width() const
{
    return d->width;
}

void Annotation::Style::setWidth(double width)
{
    d->width = width;
}

Annotation::LineStyle Annotation::Style::lineStyle() const
{
    return d->lineStyle;
}

void Annotation::Style::setLineStyle(Annotation::LineStyle style)
{
    d->lineStyle = style;
}

double Annotation::Style::xCorners() const
{
    return d->xCorners;
}

void Annotation::Style::setXCorners(double radius)
{
    d->xCorners = radius;
}

double Annotation::Style::yCorners() const
{
    return d->yCorners;
}

void Annotation::Style::setYCorners(double radius)
{
    d->yCorners = radius;
}

const QVector<double> &Annotation::Style::dashArray() const
{
    return d->dashArray;
}

void Annotation::Style::setDashArray(const QVector<double> &array)
{
    d->dashArray = array;
}

Annotation::LineEffect Annotation::Style::lineEffect() const
{
    return d->lineEffect;
}

void Annotation::Style::setLineEffect(Annotation::LineEffect effect)
{
    d->lineEffect = effect;
}

double Annotation::Style::effectIntensity() const
{
    return d->effectIntensity;
}

void Annotation::Style::setEffectIntensity(double intens)
{
    d->effectIntensity = intens;
}

class Annotation::Popup::Private : public QSharedData
{
public:
    Private() : flags(-1) { }

    int flags;
    QRectF geometry;
    QString title;
    QString summary;
    QString text;
};

Annotation::Popup::Popup() : d(new Private) { }

Annotation::Popup::Popup(const Popup &other) = default;

Annotation::Popup &Annotation::Popup::operator=(const Popup &other)
{
    if (this != &other) {
        d = other.d;
    }

    return *this;
}

Annotation::Popup::~Popup() = default;

int Annotation::Popup::flags() const
{
    return d->flags;
}

void Annotation::Popup::setFlags(int flags)
{
    d->flags = flags;
}

QRectF Annotation::Popup::geometry() const
{
    return d->geometry;
}

void Annotation::Popup::setGeometry(const QRectF &geom)
{
    d->geometry = geom;
}

QString Annotation::Popup::title() const
{
    return d->title;
}

void Annotation::Popup::setTitle(const QString &title)
{
    d->title = title;
}

QString Annotation::Popup::summary() const
{
    return d->summary;
}

void Annotation::Popup::setSummary(const QString &summary)
{
    d->summary = summary;
}

QString Annotation::Popup::text() const
{
    return d->text;
}

void Annotation::Popup::setText(const QString &text)
{
    d->text = text;
}

Annotation::Annotation(AnnotationPrivate &dd) : d_ptr(&dd) { }

Annotation::~Annotation() = default;

Annotation::Annotation(AnnotationPrivate &dd, const QDomNode &annNode) : d_ptr(&dd)
{
    Q_D(Annotation);

    // get the [base] element of the annotation node
    QDomElement e = AnnotationUtils::findChildElement(annNode, QStringLiteral("base"));
    if (e.isNull()) {
        return;
    }

    Style s;
    Popup w;

    // parse -contents- attributes
    if (e.hasAttribute(QStringLiteral("author"))) {
        setAuthor(e.attribute(QStringLiteral("author")));
    }
    if (e.hasAttribute(QStringLiteral("contents"))) {
        setContents(e.attribute(QStringLiteral("contents")));
    }
    if (e.hasAttribute(QStringLiteral("uniqueName"))) {
        setUniqueName(e.attribute(QStringLiteral("uniqueName")));
    }
    if (e.hasAttribute(QStringLiteral("modifyDate"))) {
        QDateTime dt = QDateTime::fromString(e.attribute(QStringLiteral("modifyDate")));
        if (!dt.isValid()) {
            dt = QDateTime::fromString(e.attribute(QStringLiteral("modifyDate")), Qt::ISODate);
        }
        setModificationDate(dt);
    }
    if (e.hasAttribute(QStringLiteral("creationDate"))) {
        QDateTime dt = QDateTime::fromString(e.attribute(QStringLiteral("creationDate")));
        if (!dt.isValid()) {
            dt = QDateTime::fromString(e.attribute(QStringLiteral("creationDate")), Qt::ISODate);
        }
        setCreationDate(dt);
    }

    // parse -other- attributes
    if (e.hasAttribute(QStringLiteral("flags"))) {
        setFlags(e.attribute(QStringLiteral("flags")).toInt());
    }
    if (e.hasAttribute(QStringLiteral("color"))) {
        s.setColor(QColor(e.attribute(QStringLiteral("color"))));
    }
    if (e.hasAttribute(QStringLiteral("opacity"))) {
        s.setOpacity(e.attribute(QStringLiteral("opacity")).toDouble());
    }

    // parse -the-subnodes- (describing Style, Window, Revision(s) structures)
    // Note: all subnodes if present must be 'attributes complete'
    QDomNode eSubNode = e.firstChild();
    while (eSubNode.isElement()) {
        QDomElement ee = eSubNode.toElement();
        eSubNode = eSubNode.nextSibling();

        // parse boundary
        if (ee.tagName() == QLatin1String("boundary")) {
            QRectF brect;
            brect.setLeft(ee.attribute(QStringLiteral("l")).toDouble());
            brect.setTop(ee.attribute(QStringLiteral("t")).toDouble());
            brect.setRight(ee.attribute(QStringLiteral("r")).toDouble());
            brect.setBottom(ee.attribute(QStringLiteral("b")).toDouble());
            setBoundary(brect);
        }
        // parse penStyle if not default
        else if (ee.tagName() == QLatin1String("penStyle")) {
            s.setWidth(ee.attribute(QStringLiteral("width")).toDouble());
            s.setLineStyle((LineStyle)ee.attribute(QStringLiteral("style")).toInt());
            s.setXCorners(ee.attribute(QStringLiteral("xcr")).toDouble());
            s.setYCorners(ee.attribute(QStringLiteral("ycr")).toDouble());

            // Try to parse dash array (new format)
            QVector<double> dashArray;

            QDomNode eeSubNode = ee.firstChild();
            while (eeSubNode.isElement()) {
                QDomElement eee = eeSubNode.toElement();
                eeSubNode = eeSubNode.nextSibling();

                if (eee.tagName() != QLatin1String("dashsegm")) {
                    continue;
                }

                dashArray.append(eee.attribute(QStringLiteral("len")).toDouble());
            }

            // If no segments were found use marks/spaces (old format)
            if (dashArray.empty()) {
                dashArray.append(ee.attribute(QStringLiteral("marks")).toDouble());
                dashArray.append(ee.attribute(QStringLiteral("spaces")).toDouble());
            }

            s.setDashArray(dashArray);
        }
        // parse effectStyle if not default
        else if (ee.tagName() == QLatin1String("penEffect")) {
            s.setLineEffect((LineEffect)ee.attribute(QStringLiteral("effect")).toInt());
            s.setEffectIntensity(ee.attribute(QStringLiteral("intensity")).toDouble());
        }
        // parse window if present
        else if (ee.tagName() == QLatin1String("window")) {
            QRectF geom;
            geom.setX(ee.attribute(QStringLiteral("top")).toDouble());
            geom.setY(ee.attribute(QStringLiteral("left")).toDouble());

            if (ee.hasAttribute(QStringLiteral("widthDouble"))) {
                geom.setWidth(ee.attribute(QStringLiteral("widthDouble")).toDouble());
            } else {
                geom.setWidth(ee.attribute(QStringLiteral("width")).toDouble());
            }

            if (ee.hasAttribute(QStringLiteral("widthDouble"))) {
                geom.setHeight(ee.attribute(QStringLiteral("heightDouble")).toDouble());
            } else {
                geom.setHeight(ee.attribute(QStringLiteral("height")).toDouble());
            }

            w.setGeometry(geom);

            w.setFlags(ee.attribute(QStringLiteral("flags")).toInt());
            w.setTitle(ee.attribute(QStringLiteral("title")));
            w.setSummary(ee.attribute(QStringLiteral("summary")));
            // parse window subnodes
            QDomNode winNode = ee.firstChild();
            for (; winNode.isElement(); winNode = winNode.nextSibling()) {
                QDomElement winElement = winNode.toElement();
                if (winElement.tagName() == QLatin1String("text")) {
                    w.setText(winElement.firstChild().toCDATASection().data());
                }
            }
        }
    }

    setStyle(s); // assign parsed style
    setPopup(w); // assign parsed window

    // get the [revisions] element of the annotation node
    QDomNode revNode = annNode.firstChild();
    for (; revNode.isElement(); revNode = revNode.nextSibling()) {
        QDomElement revElement = revNode.toElement();
        if (revElement.tagName() != QLatin1String("revision")) {
            continue;
        }

        Annotation *reply = AnnotationUtils::createAnnotation(revElement);

        if (reply) // if annotation is valid, add as a revision of this annotation
        {
            RevScope scope = (RevScope)revElement.attribute(QStringLiteral("revScope")).toInt();
            RevType type = (RevType)revElement.attribute(QStringLiteral("revType")).toInt();
            d->addRevision(reply, scope, type);
            delete reply;
        }
    }
}

void Annotation::storeBaseAnnotationProperties(QDomNode &annNode, QDomDocument &document) const
{
    // create [base] element of the annotation node
    QDomElement e = document.createElement(QStringLiteral("base"));
    annNode.appendChild(e);

    const Style s = style();
    const Popup w = popup();

    // store -contents- attributes
    if (!author().isEmpty()) {
        e.setAttribute(QStringLiteral("author"), author());
    }
    if (!contents().isEmpty()) {
        e.setAttribute(QStringLiteral("contents"), contents());
    }
    if (!uniqueName().isEmpty()) {
        e.setAttribute(QStringLiteral("uniqueName"), uniqueName());
    }
    if (modificationDate().isValid()) {
        e.setAttribute(QStringLiteral("modifyDate"), modificationDate().toString());
    }
    if (creationDate().isValid()) {
        e.setAttribute(QStringLiteral("creationDate"), creationDate().toString());
    }

    // store -other- attributes
    if (flags()) {
        e.setAttribute(QStringLiteral("flags"), flags());
    }
    if (s.color().isValid()) {
        e.setAttribute(QStringLiteral("color"), s.color().name());
    }
    if (s.opacity() != 1.0) {
        e.setAttribute(QStringLiteral("opacity"), QString::number(s.opacity()));
    }

    // Sub-Node-1 - boundary
    const QRectF brect = boundary();
    QDomElement bE = document.createElement(QStringLiteral("boundary"));
    e.appendChild(bE);
    bE.setAttribute(QStringLiteral("l"), QString::number((double)brect.left()));
    bE.setAttribute(QStringLiteral("t"), QString::number((double)brect.top()));
    bE.setAttribute(QStringLiteral("r"), QString::number((double)brect.right()));
    bE.setAttribute(QStringLiteral("b"), QString::number((double)brect.bottom()));

    // Sub-Node-2 - penStyle
    const QVector<double> &dashArray = s.dashArray();
    if (s.width() != 1 || s.lineStyle() != Solid || s.xCorners() != 0 || s.yCorners() != 0.0 || dashArray.size() != 1 || dashArray[0] != 3) {
        QDomElement psE = document.createElement(QStringLiteral("penStyle"));
        e.appendChild(psE);
        psE.setAttribute(QStringLiteral("width"), QString::number(s.width()));
        psE.setAttribute(QStringLiteral("style"), (int)s.lineStyle());
        psE.setAttribute(QStringLiteral("xcr"), QString::number(s.xCorners()));
        psE.setAttribute(QStringLiteral("ycr"), QString::number(s.yCorners()));

        int marks = 3, spaces = 0; // Do not break code relying on marks/spaces
        if (!dashArray.empty()) {
            marks = (int)dashArray[0];
        }
        if (dashArray.size() > 1) {
            spaces = (int)dashArray[1];
        }

        psE.setAttribute(QStringLiteral("marks"), marks);
        psE.setAttribute(QStringLiteral("spaces"), spaces);

        Q_FOREACH (double segm, dashArray) {
            QDomElement pattE = document.createElement(QStringLiteral("dashsegm"));
            pattE.setAttribute(QStringLiteral("len"), QString::number(segm));
            psE.appendChild(pattE);
        }
    }

    // Sub-Node-3 - penEffect
    if (s.lineEffect() != NoEffect || s.effectIntensity() != 1.0) {
        QDomElement peE = document.createElement(QStringLiteral("penEffect"));
        e.appendChild(peE);
        peE.setAttribute(QStringLiteral("effect"), (int)s.lineEffect());
        peE.setAttribute(QStringLiteral("intensity"), QString::number(s.effectIntensity()));
    }

    // Sub-Node-4 - window
    if (w.flags() != -1 || !w.title().isEmpty() || !w.summary().isEmpty() || !w.text().isEmpty()) {
        QDomElement wE = document.createElement(QStringLiteral("window"));
        const QRectF geom = w.geometry();
        e.appendChild(wE);
        wE.setAttribute(QStringLiteral("flags"), w.flags());
        wE.setAttribute(QStringLiteral("top"), QString::number(geom.x()));
        wE.setAttribute(QStringLiteral("left"), QString::number(geom.y()));
        wE.setAttribute(QStringLiteral("width"), (int)geom.width());
        wE.setAttribute(QStringLiteral("height"), (int)geom.height());
        wE.setAttribute(QStringLiteral("widthDouble"), QString::number(geom.width()));
        wE.setAttribute(QStringLiteral("heightDouble"), QString::number(geom.height()));
        wE.setAttribute(QStringLiteral("title"), w.title());
        wE.setAttribute(QStringLiteral("summary"), w.summary());
        // store window.text as a subnode, because we need escaped data
        if (!w.text().isEmpty()) {
            QDomElement escapedText = document.createElement(QStringLiteral("text"));
            wE.appendChild(escapedText);
            QDomCDATASection textCData = document.createCDATASection(w.text());
            escapedText.appendChild(textCData);
        }
    }

    const QList<Annotation *> revs = revisions();

    // create [revision] element of the annotation node (if any)
    if (revs.isEmpty()) {
        return;
    }

    // add all revisions as children of revisions element
    Q_FOREACH (const Annotation *rev, revs) {
        QDomElement r = document.createElement(QStringLiteral("revision"));
        annNode.appendChild(r);
        // set element attributes
        r.setAttribute(QStringLiteral("revScope"), (int)rev->revisionScope());
        r.setAttribute(QStringLiteral("revType"), (int)rev->revisionType());
        // use revision as the annotation element, so fill it up
        AnnotationUtils::storeAnnotation(rev, r, document);
        delete rev;
    }
}

QString Annotation::author() const
{
    Q_D(const Annotation);

    if (!d->pdfAnnot) {
        return d->author;
    }

    const AnnotMarkup *markupann = dynamic_cast<const AnnotMarkup *>(d->pdfAnnot.get());
    return markupann ? UnicodeParsedString(markupann->getLabel()) : QString();
}

void Annotation::setAuthor(const QString &author)
{
    Q_D(Annotation);

    if (!d->pdfAnnot) {
        d->author = author;
        return;
    }

    AnnotMarkup *markupann = dynamic_cast<AnnotMarkup *>(d->pdfAnnot.get());
    if (markupann) {
        markupann->setLabel(std::unique_ptr<GooString>(QStringToUnicodeGooString(author)));
    }
}

QString Annotation::contents() const
{
    Q_D(const Annotation);

    if (!d->pdfAnnot) {
        return d->contents;
    }

    return UnicodeParsedString(d->pdfAnnot->getContents());
}

void Annotation::setContents(const QString &contents)
{
    Q_D(Annotation);

    if (!d->pdfAnnot) {
        d->contents = contents;
        return;
    }

    d->pdfAnnot->setContents(std::unique_ptr<GooString>(QStringToUnicodeGooString(contents)));

    TextAnnotationPrivate *textAnnotD = dynamic_cast<TextAnnotationPrivate *>(d);
    if (textAnnotD) {
        textAnnotD->setDefaultAppearanceToNative();
    }
}

QString Annotation::uniqueName() const
{
    Q_D(const Annotation);

    if (!d->pdfAnnot) {
        return d->uniqueName;
    }

    return UnicodeParsedString(d->pdfAnnot->getName());
}

void Annotation::setUniqueName(const QString &uniqueName)
{
    Q_D(Annotation);

    if (!d->pdfAnnot) {
        d->uniqueName = uniqueName;
        return;
    }

    QByteArray ascii = uniqueName.toLatin1();
    GooString s(ascii.constData());
    d->pdfAnnot->setName(&s);
}

QDateTime Annotation::modificationDate() const
{
    Q_D(const Annotation);

    if (!d->pdfAnnot) {
        return d->modDate;
    }

    if (d->pdfAnnot->getModified()) {
        return convertDate(d->pdfAnnot->getModified()->c_str());
    } else {
        return QDateTime();
    }
}

void Annotation::setModificationDate(const QDateTime &date)
{
    Q_D(Annotation);

    if (!d->pdfAnnot) {
        d->modDate = date;
        return;
    }

    if (d->pdfAnnot) {
        if (date.isValid()) {
            const time_t t = date.toSecsSinceEpoch();
            std::unique_ptr<GooString> s = timeToDateString(&t);
            d->pdfAnnot->setModified(std::move(s));
        } else {
            d->pdfAnnot->setModified(nullptr);
        }
    }
}

QDateTime Annotation::creationDate() const
{
    Q_D(const Annotation);

    if (!d->pdfAnnot) {
        return d->creationDate;
    }

    const AnnotMarkup *markupann = dynamic_cast<const AnnotMarkup *>(d->pdfAnnot.get());

    if (markupann && markupann->getDate()) {
        return convertDate(markupann->getDate()->c_str());
    }

    return modificationDate();
}

void Annotation::setCreationDate(const QDateTime &date)
{
    Q_D(Annotation);

    if (!d->pdfAnnot) {
        d->creationDate = date;
        return;
    }

    AnnotMarkup *markupann = dynamic_cast<AnnotMarkup *>(d->pdfAnnot.get());
    if (markupann) {
        if (date.isValid()) {
            const time_t t = date.toSecsSinceEpoch();
            std::unique_ptr<GooString> s = timeToDateString(&t);
            markupann->setDate(std::move(s));
        } else {
            markupann->setDate(nullptr);
        }
    }
}

static int fromPdfFlags(int flags)
{
    int qtflags = 0;

    if (flags & Annot::flagHidden) {
        qtflags |= Annotation::Hidden;
    }
    if (flags & Annot::flagNoZoom) {
        qtflags |= Annotation::FixedSize;
    }
    if (flags & Annot::flagNoRotate) {
        qtflags |= Annotation::FixedRotation;
    }
    if (!(flags & Annot::flagPrint)) {
        qtflags |= Annotation::DenyPrint;
    }
    if (flags & Annot::flagReadOnly) {
        qtflags |= (Annotation::DenyWrite | Annotation::DenyDelete);
    }
    if (flags & Annot::flagLocked) {
        qtflags |= Annotation::DenyDelete;
    }
    if (flags & Annot::flagToggleNoView) {
        qtflags |= Annotation::ToggleHidingOnMouse;
    }

    return qtflags;
}

static int toPdfFlags(int qtflags)
{
    int flags = 0;

    if (qtflags & Annotation::Hidden) {
        flags |= Annot::flagHidden;
    }
    if (qtflags & Annotation::FixedSize) {
        flags |= Annot::flagNoZoom;
    }
    if (qtflags & Annotation::FixedRotation) {
        flags |= Annot::flagNoRotate;
    }
    if (!(qtflags & Annotation::DenyPrint)) {
        flags |= Annot::flagPrint;
    }
    if (qtflags & Annotation::DenyWrite) {
        flags |= Annot::flagReadOnly;
    }
    if (qtflags & Annotation::DenyDelete) {
        flags |= Annot::flagLocked;
    }
    if (qtflags & Annotation::ToggleHidingOnMouse) {
        flags |= Annot::flagToggleNoView;
    }

    return flags;
}

int Annotation::flags() const
{
    Q_D(const Annotation);

    if (!d->pdfAnnot) {
        return d->flags;
    }

    return fromPdfFlags(d->pdfAnnot->getFlags());
}

void Annotation::setFlags(int flags)
{
    Q_D(Annotation);

    if (!d->pdfAnnot) {
        d->flags = flags;
        return;
    }

    d->pdfAnnot->setFlags(toPdfFlags(flags));
}

QRectF Annotation::boundary() const
{
    Q_D(const Annotation);

    if (!d->pdfAnnot) {
        return d->boundary;
    }

    const PDFRectangle &rect = d->pdfAnnot->getRect();
    return d->fromPdfRectangle(rect);
}

void Annotation::setBoundary(const QRectF &boundary)
{
    Q_D(Annotation);

    if (!d->pdfAnnot) {
        d->boundary = boundary;
        return;
    }

    const PDFRectangle rect = d->boundaryToPdfRectangle(boundary, flags());
    if (rect == d->pdfAnnot->getRect()) {
        return;
    }
    d->pdfAnnot->setRect(rect);
}

Annotation::Style Annotation::style() const
{
    Q_D(const Annotation);

    if (!d->pdfAnnot) {
        return d->style;
    }

    Style s;
    s.setColor(convertAnnotColor(d->pdfAnnot->getColor()));

    const AnnotMarkup *markupann = dynamic_cast<const AnnotMarkup *>(d->pdfAnnot.get());
    if (markupann) {
        s.setOpacity(markupann->getOpacity());
    }

    const AnnotBorder *border = d->pdfAnnot->getBorder();
    if (border) {
        if (border->getType() == AnnotBorder::typeArray) {
            const AnnotBorderArray *border_array = static_cast<const AnnotBorderArray *>(border);
            s.setXCorners(border_array->getHorizontalCorner());
            s.setYCorners(border_array->getVerticalCorner());
        }

        s.setWidth(border->getWidth());
        s.setLineStyle((Annotation::LineStyle)(1 << border->getStyle()));

        const std::vector<double> &dashArray = border->getDash();
        s.setDashArray(QVector<double>(dashArray.begin(), dashArray.end()));
    }

    AnnotBorderEffect *border_effect;
    switch (d->pdfAnnot->getType()) {
    case Annot::typeFreeText:
        border_effect = static_cast<AnnotFreeText *>(d->pdfAnnot.get())->getBorderEffect();
        break;
    case Annot::typeSquare:
    case Annot::typeCircle:
        border_effect = static_cast<AnnotGeometry *>(d->pdfAnnot.get())->getBorderEffect();
        break;
    default:
        border_effect = nullptr;
    }
    if (border_effect) {
        s.setLineEffect((Annotation::LineEffect)border_effect->getEffectType());
        s.setEffectIntensity(border_effect->getIntensity());
    }

    return s;
}

void Annotation::setStyle(const Annotation::Style &style)
{
    Q_D(Annotation);

    if (!d->pdfAnnot) {
        d->style = style;
        return;
    }

    d->pdfAnnot->setColor(convertQColor(style.color()));

    AnnotMarkup *markupann = dynamic_cast<AnnotMarkup *>(d->pdfAnnot.get());
    if (markupann) {
        markupann->setOpacity(style.opacity());
    }

    auto border = std::make_unique<AnnotBorderArray>();
    border->setWidth(style.width());
    border->setHorizontalCorner(style.xCorners());
    border->setVerticalCorner(style.yCorners());
    d->pdfAnnot->setBorder(std::move(border));
}

Annotation::Popup Annotation::popup() const
{
    Q_D(const Annotation);

    if (!d->pdfAnnot) {
        return d->popup;
    }

    Popup w;
    AnnotPopup *popup = nullptr;
    int flags = -1; // Not initialized

    const AnnotMarkup *markupann = dynamic_cast<const AnnotMarkup *>(d->pdfAnnot.get());
    if (markupann) {
        popup = markupann->getPopup().get();
        w.setSummary(UnicodeParsedString(markupann->getSubject()));
    }

    if (popup) {
        flags = fromPdfFlags(popup->getFlags()) & (Annotation::Hidden | Annotation::FixedSize | Annotation::FixedRotation);

        if (!popup->getOpen()) {
            flags |= Annotation::Hidden;
        }

        const PDFRectangle &rect = popup->getRect();
        w.setGeometry(d->fromPdfRectangle(rect));
    }

    if (d->pdfAnnot->getType() == Annot::typeText) {
        const AnnotText *textann = static_cast<const AnnotText *>(d->pdfAnnot.get());

        // Text annotations default to same rect as annotation
        if (flags == -1) {
            flags = 0;
            w.setGeometry(boundary());
        }

        // If text is not 'opened', force window hiding. if the window
        // was parsed from popup, the flag should already be set
        if (!textann->getOpen() && flags != -1) {
            flags |= Annotation::Hidden;
        }
    }

    w.setFlags(flags);

    return w;
}

void Annotation::setPopup(const Annotation::Popup &popup)
{
    Q_D(Annotation);

    if (!d->pdfAnnot) {
        d->popup = popup;
        return;
    }

#if 0 /* TODO: Remove old popup and add AnnotPopup to page */
    AnnotMarkup *markupann = dynamic_cast<AnnotMarkup*>(d->pdfAnnot);
    if (!markupann)
        return;

    // Create a new AnnotPopup and assign it to pdfAnnot
    PDFRectangle rect = d->toPdfRectangle( popup.geometry() );
    AnnotPopup * p = new AnnotPopup( d->pdfPage->getDoc(), &rect );
    p->setOpen( !(popup.flags() & Annotation::Hidden) );
    if (!popup.summary().isEmpty())
    {
        GooString *s = QStringToUnicodeGooString(popup.summary());
        markupann->setLabel(s);
        delete s;
    }
    markupann->setPopup(p);
#endif
}

Annotation::RevScope Annotation::revisionScope() const
{
    Q_D(const Annotation);

    if (!d->pdfAnnot) {
        return d->revisionScope;
    }

    const AnnotMarkup *markupann = dynamic_cast<const AnnotMarkup *>(d->pdfAnnot.get());

    if (markupann && markupann->isInReplyTo()) {
        switch (markupann->getReplyTo()) {
        case AnnotMarkup::replyTypeR:
            return Annotation::Reply;
        case AnnotMarkup::replyTypeGroup:
            return Annotation::Group;
        }
    }

    return Annotation::Root; // It's not a revision
}

Annotation::RevType Annotation::revisionType() const
{
    Q_D(const Annotation);

    if (!d->pdfAnnot) {
        return d->revisionType;
    }

    const AnnotText *textann = dynamic_cast<const AnnotText *>(d->pdfAnnot.get());

    if (textann && textann->isInReplyTo()) {
        switch (textann->getState()) {
        case AnnotText::stateMarked:
            return Annotation::Marked;
        case AnnotText::stateUnmarked:
            return Annotation::Unmarked;
        case AnnotText::stateAccepted:
            return Annotation::Accepted;
        case AnnotText::stateRejected:
            return Annotation::Rejected;
        case AnnotText::stateCancelled:
            return Annotation::Cancelled;
        case AnnotText::stateCompleted:
            return Annotation::Completed;
        default:
            break;
        }
    }

    return Annotation::None;
}

QList<Annotation *> Annotation::revisions() const
{
    Q_D(const Annotation);

    if (!d->pdfAnnot) {
        /* Return aliases, whose ownership goes to the caller */
        QList<Annotation *> res;
        for (const std::unique_ptr<Annotation> &rev : d->revisions) {
            res.append(rev->d_ptr->makeAlias().release());
        }
        return res;
    }

    /* If the annotation doesn't live in a object on its own (eg bug51361), it
     * has no ref, therefore it can't have revisions */
    if (!d->pdfAnnot->getHasRef()) {
        return QList<Annotation *>();
    }

    return AnnotationPrivate::findAnnotations(d->pdfPage, d->parentDoc, QSet<Annotation::SubType>(), d->pdfAnnot->getId());
}

std::unique_ptr<AnnotationAppearance> Annotation::annotationAppearance() const
{
    Q_D(const Annotation);

    return std::make_unique<AnnotationAppearance>(new AnnotationAppearancePrivate(d->pdfAnnot.get()));
}

void Annotation::setAnnotationAppearance(const AnnotationAppearance &annotationAppearance)
{
    Q_D(Annotation);

    if (!d->pdfAnnot) {
        d->annotationAppearance = annotationAppearance.d->appearance.copy();
        return;
    }

    // Moving the appearance object using std::move would result
    // in the object being completed moved from the AnnotationAppearancePrivate
    // class. So, we'll not be able to retrieve the stamp's original AP stream
    d->pdfAnnot->setNewAppearance(annotationAppearance.d->appearance.copy());
}

// END Annotation implementation

/** TextAnnotation [Annotation] */

TextAnnotationPrivate::TextAnnotationPrivate() : textType(TextAnnotation::Linked), textIcon(QStringLiteral("Note")), inplaceAlign(0), inplaceIntent(TextAnnotation::Unknown) { }

std::unique_ptr<Annotation> TextAnnotationPrivate::makeAlias()
{
    return std::unique_ptr<Annotation>(new TextAnnotation(*this));
}

std::shared_ptr<Annot> TextAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    // Setters are defined in the public clas
    std::unique_ptr<TextAnnotation> q = static_pointer_cast<TextAnnotation>(makeAlias());

    // Set page and contents
    pdfPage = destPage;
    parentDoc = doc;

    // Set pdfAnnot
    PDFRectangle rect = boundaryToPdfRectangle(boundary, flags);
    if (textType == TextAnnotation::Linked) {
        pdfAnnot = std::make_shared<AnnotText>(destPage->getDoc(), &rect);
    } else {
        const double pointSize = textFont ? textFont->pointSizeF() : AnnotFreeText::undefinedFontPtSize;
        if (pointSize < 0) {
            qWarning() << "TextAnnotationPrivate::createNativeAnnot: font pointSize < 0";
        }
        pdfAnnot = std::make_shared<AnnotFreeText>(destPage->getDoc(), &rect);
    }

    // Set properties
    flushBaseAnnotationProperties();
    q->setTextIcon(textIcon);
    q->setInplaceAlign(inplaceAlign);
    q->setCalloutPoints(inplaceCallout);
    q->setInplaceIntent(inplaceIntent);

    inplaceCallout.clear(); // Free up memory

    setDefaultAppearanceToNative();

    return pdfAnnot;
}

void TextAnnotationPrivate::setDefaultAppearanceToNative()
{
    if (pdfAnnot && pdfAnnot->getType() == Annot::typeFreeText) {
        AnnotFreeText *ftextann = static_cast<AnnotFreeText *>(pdfAnnot.get());
        const double pointSize = textFont ? textFont->pointSizeF() : AnnotFreeText::undefinedFontPtSize;
        if (pointSize < 0) {
            qWarning() << "TextAnnotationPrivate::createNativeAnnot: font pointSize < 0";
        }
        std::string fontName = "Invalid_font";
        if (textFont) {
            Form *form = pdfPage->getDoc()->getCatalog()->getCreateForm();
            if (form) {
                fontName = form->findFontInDefaultResources(textFont->family().toStdString(), textFont->styleName().toStdString());
                if (fontName.empty()) {
                    fontName = form->addFontToDefaultResources(textFont->family().toStdString(), textFont->styleName().toStdString()).fontName;
                }

                if (!fontName.empty()) {
                    form->ensureFontsForAllCharacters(pdfAnnot->getContents(), fontName);
                } else {
                    fontName = "Invalid_font";
                }
            }
        }
        DefaultAppearance da { { objName, fontName.c_str() }, pointSize, convertQColor(textColor) };
        ftextann->setDefaultAppearance(da);
    }
}

std::unique_ptr<DefaultAppearance> TextAnnotationPrivate::getDefaultAppearanceFromNative() const
{
    if (pdfAnnot && pdfAnnot->getType() == Annot::typeFreeText) {
        AnnotFreeText *ftextann = static_cast<AnnotFreeText *>(pdfAnnot.get());
        return ftextann->getDefaultAppearance();
    } else {
        return {};
    }
}

TextAnnotation::TextAnnotation(TextAnnotation::TextType type) : Annotation(*new TextAnnotationPrivate())
{
    setTextType(type);
}

TextAnnotation::TextAnnotation(TextAnnotationPrivate &dd) : Annotation(dd) { }

TextAnnotation::TextAnnotation(const QDomNode &node) : Annotation(*new TextAnnotationPrivate, node)
{
    // loop through the whole children looking for a 'text' element
    QDomNode subNode = node.firstChild();
    while (subNode.isElement()) {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if (e.tagName() != QLatin1String("text")) {
            continue;
        }

        // parse the attributes
        if (e.hasAttribute(QStringLiteral("type"))) {
            setTextType((TextAnnotation::TextType)e.attribute(QStringLiteral("type")).toInt());
        }
        if (e.hasAttribute(QStringLiteral("icon"))) {
            setTextIcon(e.attribute(QStringLiteral("icon")));
        }
        if (e.hasAttribute(QStringLiteral("font"))) {
            QFont font;
            font.fromString(e.attribute(QStringLiteral("font")));
            setTextFont(font);
            if (e.hasAttribute(QStringLiteral("fontColor"))) {
                const QColor color = QColor(e.attribute(QStringLiteral("fontColor")));
                setTextColor(color);
            }
        }
        if (e.hasAttribute(QStringLiteral("align"))) {
            setInplaceAlign(e.attribute(QStringLiteral("align")).toInt());
        }
        if (e.hasAttribute(QStringLiteral("intent"))) {
            setInplaceIntent((TextAnnotation::InplaceIntent)e.attribute(QStringLiteral("intent")).toInt());
        }

        // parse the subnodes
        QDomNode eSubNode = e.firstChild();
        while (eSubNode.isElement()) {
            QDomElement ee = eSubNode.toElement();
            eSubNode = eSubNode.nextSibling();

            if (ee.tagName() == QLatin1String("escapedText")) {
                setContents(ee.firstChild().toCDATASection().data());
            } else if (ee.tagName() == QLatin1String("callout")) {
                QVector<QPointF> points(3);
                points[0] = QPointF(ee.attribute(QStringLiteral("ax")).toDouble(), ee.attribute(QStringLiteral("ay")).toDouble());
                points[1] = QPointF(ee.attribute(QStringLiteral("bx")).toDouble(), ee.attribute(QStringLiteral("by")).toDouble());
                points[2] = QPointF(ee.attribute(QStringLiteral("cx")).toDouble(), ee.attribute(QStringLiteral("cy")).toDouble());
                setCalloutPoints(points);
            }
        }

        // loading complete
        break;
    }
}

TextAnnotation::~TextAnnotation() = default;

void TextAnnotation::store(QDomNode &node, QDomDocument &document) const
{
    // store base annotation properties
    storeBaseAnnotationProperties(node, document);

    // create [text] element
    QDomElement textElement = document.createElement(QStringLiteral("text"));
    node.appendChild(textElement);

    // store the optional attributes
    if (textType() != Linked) {
        textElement.setAttribute(QStringLiteral("type"), (int)textType());
    }
    if (textIcon() != QLatin1String("Note")) {
        textElement.setAttribute(QStringLiteral("icon"), textIcon());
    }
    if (inplaceAlign()) {
        textElement.setAttribute(QStringLiteral("align"), inplaceAlign());
    }
    if (inplaceIntent() != Unknown) {
        textElement.setAttribute(QStringLiteral("intent"), (int)inplaceIntent());
    }

    textElement.setAttribute(QStringLiteral("font"), textFont().toString());
    textElement.setAttribute(QStringLiteral("fontColor"), textColor().name());

    // Sub-Node-1 - escapedText
    if (!contents().isEmpty()) {
        QDomElement escapedText = document.createElement(QStringLiteral("escapedText"));
        textElement.appendChild(escapedText);
        QDomCDATASection textCData = document.createCDATASection(contents());
        escapedText.appendChild(textCData);
    }

    // Sub-Node-2 - callout
    if (calloutPoint(0).x() != 0.0) {
        QDomElement calloutElement = document.createElement(QStringLiteral("callout"));
        textElement.appendChild(calloutElement);
        calloutElement.setAttribute(QStringLiteral("ax"), QString::number(calloutPoint(0).x()));
        calloutElement.setAttribute(QStringLiteral("ay"), QString::number(calloutPoint(0).y()));
        calloutElement.setAttribute(QStringLiteral("bx"), QString::number(calloutPoint(1).x()));
        calloutElement.setAttribute(QStringLiteral("by"), QString::number(calloutPoint(1).y()));
        calloutElement.setAttribute(QStringLiteral("cx"), QString::number(calloutPoint(2).x()));
        calloutElement.setAttribute(QStringLiteral("cy"), QString::number(calloutPoint(2).y()));
    }
}

Annotation::SubType TextAnnotation::subType() const
{
    return AText;
}

TextAnnotation::TextType TextAnnotation::textType() const
{
    Q_D(const TextAnnotation);

    if (!d->pdfAnnot) {
        return d->textType;
    }

    return d->pdfAnnot->getType() == Annot::typeText ? TextAnnotation::Linked : TextAnnotation::InPlace;
}

void TextAnnotation::setTextType(TextAnnotation::TextType type)
{
    Q_D(TextAnnotation);

    if (!d->pdfAnnot) {
        d->textType = type;
        return;
    }

    // Type cannot be changed if annotation is already tied
    qWarning() << "You can't change the type of a TextAnnotation that is already in a page";
}

QString TextAnnotation::textIcon() const
{
    Q_D(const TextAnnotation);

    if (!d->pdfAnnot) {
        return d->textIcon;
    }

    if (d->pdfAnnot->getType() == Annot::typeText) {
        const AnnotText *textann = static_cast<const AnnotText *>(d->pdfAnnot.get());
        return QString::fromStdString(textann->getIcon());
    }

    return QString();
}

void TextAnnotation::setTextIcon(const QString &icon)
{
    Q_D(TextAnnotation);

    if (!d->pdfAnnot) {
        d->textIcon = icon;
        return;
    }

    if (d->pdfAnnot->getType() == Annot::typeText) {
        AnnotText *textann = static_cast<AnnotText *>(d->pdfAnnot.get());
        textann->setIcon(icon.toStdString());
    }
}

QFont TextAnnotation::textFont() const
{
    Q_D(const TextAnnotation);

    if (d->textFont) {
        return *d->textFont;
    }

    double fontSize { AnnotFreeText::undefinedFontPtSize };
    if (d->pdfAnnot->getType() == Annot::typeFreeText) {
        std::unique_ptr<DefaultAppearance> da { d->getDefaultAppearanceFromNative() };
        if (da && da->getFontPtSize() > 0) {
            fontSize = da->getFontPtSize();
        }
    }

    QFont font;
    font.setPointSizeF(fontSize);
    return font;
}

void TextAnnotation::setTextFont(const QFont &font)
{
    Q_D(TextAnnotation);
    if (font == d->textFont) {
        return;
    }
    d->textFont = font;

    d->setDefaultAppearanceToNative();
}

QColor TextAnnotation::textColor() const
{
    Q_D(const TextAnnotation);

    if (!d->pdfAnnot) {
        return d->textColor;
    }

    if (std::unique_ptr<DefaultAppearance> da { d->getDefaultAppearanceFromNative() }) {
        return convertAnnotColor(da->getFontColor());
    }

    return {};
}

void TextAnnotation::setTextColor(const QColor &color)
{
    Q_D(TextAnnotation);
    if (color == d->textColor) {
        return;
    }
    d->textColor = color;

    d->setDefaultAppearanceToNative();
}

int TextAnnotation::inplaceAlign() const
{
    Q_D(const TextAnnotation);

    if (!d->pdfAnnot) {
        return d->inplaceAlign;
    }

    if (d->pdfAnnot->getType() == Annot::typeFreeText) {
        const AnnotFreeText *ftextann = static_cast<const AnnotFreeText *>(d->pdfAnnot.get());
        return static_cast<int>(ftextann->getQuadding());
    }

    return 0;
}

void TextAnnotation::setInplaceAlign(int align)
{
    Q_D(TextAnnotation);

    if (!d->pdfAnnot) {
        d->inplaceAlign = align;
        return;
    }

    if (d->pdfAnnot->getType() == Annot::typeFreeText) {
        AnnotFreeText *ftextann = static_cast<AnnotFreeText *>(d->pdfAnnot.get());
        ftextann->setQuadding((VariableTextQuadding)align);
    }
}

QPointF TextAnnotation::calloutPoint(int id) const
{
    const QVector<QPointF> points = calloutPoints();
    if (id < 0 || id >= points.size()) {
        return QPointF();
    } else {
        return points[id];
    }
}

QVector<QPointF> TextAnnotation::calloutPoints() const
{
    Q_D(const TextAnnotation);

    if (!d->pdfAnnot) {
        return d->inplaceCallout;
    }

    if (d->pdfAnnot->getType() == Annot::typeText) {
        return QVector<QPointF>();
    }

    const AnnotFreeText *ftextann = static_cast<const AnnotFreeText *>(d->pdfAnnot.get());
    const AnnotCalloutLine *callout = ftextann->getCalloutLine();

    if (!callout) {
        return QVector<QPointF>();
    }

    double MTX[6];
    d->fillTransformationMTX(MTX);

    const AnnotCalloutMultiLine *callout_v6 = dynamic_cast<const AnnotCalloutMultiLine *>(callout);
    QVector<QPointF> res(callout_v6 ? 3 : 2);
    XPDFReader::transform(MTX, callout->getX1(), callout->getY1(), res[0]);
    XPDFReader::transform(MTX, callout->getX2(), callout->getY2(), res[1]);
    if (callout_v6) {
        XPDFReader::transform(MTX, callout_v6->getX3(), callout_v6->getY3(), res[2]);
    }
    return res;
}

void TextAnnotation::setCalloutPoints(const QVector<QPointF> &points)
{
    Q_D(TextAnnotation);
    if (!d->pdfAnnot) {
        d->inplaceCallout = points;
        return;
    }

    if (d->pdfAnnot->getType() != Annot::typeFreeText) {
        return;
    }

    AnnotFreeText *ftextann = static_cast<AnnotFreeText *>(d->pdfAnnot.get());
    const int count = points.size();

    if (count == 0) {
        ftextann->setCalloutLine(nullptr);
        return;
    }

    if (count != 2 && count != 3) {
        error(errSyntaxError, -1, "Expected zero, two or three points for callout");
        return;
    }

    std::unique_ptr<AnnotCalloutLine> callout;
    double x1, y1, x2, y2;
    double MTX[6];
    d->fillTransformationMTX(MTX);

    XPDFReader::invTransform(MTX, points[0], x1, y1);
    XPDFReader::invTransform(MTX, points[1], x2, y2);
    if (count == 3) {
        double x3, y3;
        XPDFReader::invTransform(MTX, points[2], x3, y3);
        callout = std::make_unique<AnnotCalloutMultiLine>(x1, y1, x2, y2, x3, y3);
    } else {
        callout = std::make_unique<AnnotCalloutLine>(x1, y1, x2, y2);
    }

    ftextann->setCalloutLine(std::move(callout));
}

TextAnnotation::InplaceIntent TextAnnotation::inplaceIntent() const
{
    Q_D(const TextAnnotation);

    if (!d->pdfAnnot) {
        return d->inplaceIntent;
    }

    if (d->pdfAnnot->getType() == Annot::typeFreeText) {
        const AnnotFreeText *ftextann = static_cast<const AnnotFreeText *>(d->pdfAnnot.get());
        return (TextAnnotation::InplaceIntent)ftextann->getIntent();
    }

    return TextAnnotation::Unknown;
}

void TextAnnotation::setInplaceIntent(TextAnnotation::InplaceIntent intent)
{
    Q_D(TextAnnotation);

    if (!d->pdfAnnot) {
        d->inplaceIntent = intent;
        return;
    }

    if (d->pdfAnnot->getType() == Annot::typeFreeText) {
        AnnotFreeText *ftextann = static_cast<AnnotFreeText *>(d->pdfAnnot.get());
        ftextann->setIntent((AnnotFreeText::AnnotFreeTextIntent)intent);
    }
}

/** LineAnnotation [Annotation] */
class LineAnnotationPrivate : public AnnotationPrivate
{
public:
    LineAnnotationPrivate();
    std::unique_ptr<Annotation> makeAlias() override;
    std::shared_ptr<Annot> createNativeAnnot(::Page *destPage, DocumentData *doc) override;

    // data fields (note uses border for rendering style)
    QLinkedList<QPointF> linePoints;
    LineAnnotation::TermStyle lineStartStyle;
    LineAnnotation::TermStyle lineEndStyle;
    bool lineClosed : 1; // (if true draw close shape)
    bool lineShowCaption : 1;
    LineAnnotation::LineType lineType;
    QColor lineInnerColor;
    double lineLeadingFwdPt;
    double lineLeadingBackPt;
    LineAnnotation::LineIntent lineIntent;
};

LineAnnotationPrivate::LineAnnotationPrivate()
    : lineStartStyle(LineAnnotation::None), lineEndStyle(LineAnnotation::None), lineClosed(false), lineShowCaption(false), lineLeadingFwdPt(0), lineLeadingBackPt(0), lineIntent(LineAnnotation::Unknown)
{
}

std::unique_ptr<Annotation> LineAnnotationPrivate::makeAlias()
{
    return std::unique_ptr<LineAnnotation>(new LineAnnotation(*this));
}

std::shared_ptr<Annot> LineAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    // Setters are defined in the public class
    std::unique_ptr<LineAnnotation> q = static_pointer_cast<LineAnnotation>(makeAlias());

    // Set page and document
    pdfPage = destPage;
    parentDoc = doc;

    // Set pdfAnnot
    PDFRectangle rect = boundaryToPdfRectangle(boundary, flags);
    if (lineType == LineAnnotation::StraightLine) {
        pdfAnnot = std::make_shared<AnnotLine>(doc->doc, &rect);
    } else {
        pdfAnnot = std::make_shared<AnnotPolygon>(doc->doc, &rect, lineClosed ? Annot::typePolygon : Annot::typePolyLine);
    }

    // Set properties
    flushBaseAnnotationProperties();
    q->setLinePoints(linePoints);
    q->setLineStartStyle(lineStartStyle);
    q->setLineEndStyle(lineEndStyle);
    q->setLineInnerColor(lineInnerColor);
    q->setLineLeadingForwardPoint(lineLeadingFwdPt);
    q->setLineLeadingBackPoint(lineLeadingBackPt);
    q->setLineShowCaption(lineShowCaption);
    q->setLineIntent(lineIntent);

    linePoints.clear(); // Free up memory

    return pdfAnnot;
}

LineAnnotation::LineAnnotation(LineAnnotation::LineType type) : Annotation(*new LineAnnotationPrivate())
{
    setLineType(type);
}

LineAnnotation::LineAnnotation(LineAnnotationPrivate &dd) : Annotation(dd) { }

LineAnnotation::LineAnnotation(const QDomNode &node) : Annotation(*new LineAnnotationPrivate(), node)
{
    // loop through the whole children looking for a 'line' element
    QDomNode subNode = node.firstChild();
    while (subNode.isElement()) {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if (e.tagName() != QLatin1String("line")) {
            continue;
        }

        // parse the attributes
        if (e.hasAttribute(QStringLiteral("startStyle"))) {
            setLineStartStyle((LineAnnotation::TermStyle)e.attribute(QStringLiteral("startStyle")).toInt());
        }
        if (e.hasAttribute(QStringLiteral("endStyle"))) {
            setLineEndStyle((LineAnnotation::TermStyle)e.attribute(QStringLiteral("endStyle")).toInt());
        }
        if (e.hasAttribute(QStringLiteral("closed"))) {
            setLineClosed(e.attribute(QStringLiteral("closed")).toInt());
        }
        if (e.hasAttribute(QStringLiteral("innerColor"))) {
            setLineInnerColor(QColor(e.attribute(QStringLiteral("innerColor"))));
        }
        if (e.hasAttribute(QStringLiteral("leadFwd"))) {
            setLineLeadingForwardPoint(e.attribute(QStringLiteral("leadFwd")).toDouble());
        }
        if (e.hasAttribute(QStringLiteral("leadBack"))) {
            setLineLeadingBackPoint(e.attribute(QStringLiteral("leadBack")).toDouble());
        }
        if (e.hasAttribute(QStringLiteral("showCaption"))) {
            setLineShowCaption(e.attribute(QStringLiteral("showCaption")).toInt());
        }
        if (e.hasAttribute(QStringLiteral("intent"))) {
            setLineIntent((LineAnnotation::LineIntent)e.attribute(QStringLiteral("intent")).toInt());
        }

        // parse all 'point' subnodes
        QLinkedList<QPointF> points;
        QDomNode pointNode = e.firstChild();
        while (pointNode.isElement()) {
            QDomElement pe = pointNode.toElement();
            pointNode = pointNode.nextSibling();

            if (pe.tagName() != QLatin1String("point")) {
                continue;
            }

            QPointF p(pe.attribute(QStringLiteral("x"), QStringLiteral("0.0")).toDouble(), pe.attribute(QStringLiteral("y"), QStringLiteral("0.0")).toDouble());
            points.append(p);
        }
        setLinePoints(points);
        setLineType(points.size() == 2 ? StraightLine : Polyline);

        // loading complete
        break;
    }
}

LineAnnotation::~LineAnnotation() = default;

void LineAnnotation::store(QDomNode &node, QDomDocument &document) const
{
    // store base annotation properties
    storeBaseAnnotationProperties(node, document);

    // create [line] element
    QDomElement lineElement = document.createElement(QStringLiteral("line"));
    node.appendChild(lineElement);

    // store the attributes
    if (lineStartStyle() != None) {
        lineElement.setAttribute(QStringLiteral("startStyle"), (int)lineStartStyle());
    }
    if (lineEndStyle() != None) {
        lineElement.setAttribute(QStringLiteral("endStyle"), (int)lineEndStyle());
    }
    if (isLineClosed()) {
        lineElement.setAttribute(QStringLiteral("closed"), isLineClosed());
    }
    if (lineInnerColor().isValid()) {
        lineElement.setAttribute(QStringLiteral("innerColor"), lineInnerColor().name());
    }
    if (lineLeadingForwardPoint() != 0.0) {
        lineElement.setAttribute(QStringLiteral("leadFwd"), QString::number(lineLeadingForwardPoint()));
    }
    if (lineLeadingBackPoint() != 0.0) {
        lineElement.setAttribute(QStringLiteral("leadBack"), QString::number(lineLeadingBackPoint()));
    }
    if (lineShowCaption()) {
        lineElement.setAttribute(QStringLiteral("showCaption"), lineShowCaption());
    }
    if (lineIntent() != Unknown) {
        lineElement.setAttribute(QStringLiteral("intent"), lineIntent());
    }

    // append the list of points
    const QLinkedList<QPointF> points = linePoints();
    if (points.count() > 1) {
        QLinkedList<QPointF>::const_iterator it = points.begin(), end = points.end();
        while (it != end) {
            const QPointF &p = *it;
            QDomElement pElement = document.createElement(QStringLiteral("point"));
            lineElement.appendChild(pElement);
            pElement.setAttribute(QStringLiteral("x"), QString::number(p.x()));
            pElement.setAttribute(QStringLiteral("y"), QString::number(p.y()));
            ++it;
        }
    }
}

Annotation::SubType LineAnnotation::subType() const
{
    return ALine;
}

LineAnnotation::LineType LineAnnotation::lineType() const
{
    Q_D(const LineAnnotation);

    if (!d->pdfAnnot) {
        return d->lineType;
    }

    return (d->pdfAnnot->getType() == Annot::typeLine) ? LineAnnotation::StraightLine : LineAnnotation::Polyline;
}

void LineAnnotation::setLineType(LineAnnotation::LineType type)
{
    Q_D(LineAnnotation);

    if (!d->pdfAnnot) {
        d->lineType = type;
        return;
    }

    // Type cannot be changed if annotation is already tied
    qWarning() << "You can't change the type of a LineAnnotation that is already in a page";
}

QLinkedList<QPointF> LineAnnotation::linePoints() const
{
    Q_D(const LineAnnotation);

    if (!d->pdfAnnot) {
        return d->linePoints;
    }

    double MTX[6];
    d->fillTransformationMTX(MTX);

    QLinkedList<QPointF> res;
    if (d->pdfAnnot->getType() == Annot::typeLine) {
        const AnnotLine *lineann = static_cast<const AnnotLine *>(d->pdfAnnot.get());
        QPointF p;
        XPDFReader::transform(MTX, lineann->getX1(), lineann->getY1(), p);
        res.append(p);
        XPDFReader::transform(MTX, lineann->getX2(), lineann->getY2(), p);
        res.append(p);
    } else {
        const AnnotPolygon *polyann = static_cast<const AnnotPolygon *>(d->pdfAnnot.get());
        const AnnotPath *vertices = polyann->getVertices();

        for (int i = 0; i < vertices->getCoordsLength(); ++i) {
            QPointF p;
            XPDFReader::transform(MTX, vertices->getX(i), vertices->getY(i), p);
            res.append(p);
        }
    }

    return res;
}

void LineAnnotation::setLinePoints(const QLinkedList<QPointF> &points)
{
    Q_D(LineAnnotation);

    if (!d->pdfAnnot) {
        d->linePoints = points;
        return;
    }

    if (d->pdfAnnot->getType() == Annot::typeLine) {
        AnnotLine *lineann = static_cast<AnnotLine *>(d->pdfAnnot.get());
        if (points.size() != 2) {
            error(errSyntaxError, -1, "Expected two points for a straight line");
            return;
        }
        double x1, y1, x2, y2;
        double MTX[6];
        d->fillTransformationMTX(MTX);
        XPDFReader::invTransform(MTX, points.first(), x1, y1);
        XPDFReader::invTransform(MTX, points.last(), x2, y2);
        lineann->setVertices(x1, y1, x2, y2);
    } else {
        AnnotPolygon *polyann = static_cast<AnnotPolygon *>(d->pdfAnnot.get());
        const std::unique_ptr<AnnotPath> p = d->toAnnotPath(points);
        polyann->setVertices(*p);
    }
}

LineAnnotation::TermStyle LineAnnotation::lineStartStyle() const
{
    Q_D(const LineAnnotation);

    if (!d->pdfAnnot) {
        return d->lineStartStyle;
    }

    if (d->pdfAnnot->getType() == Annot::typeLine) {
        const AnnotLine *lineann = static_cast<const AnnotLine *>(d->pdfAnnot.get());
        return (LineAnnotation::TermStyle)lineann->getStartStyle();
    } else {
        const AnnotPolygon *polyann = static_cast<const AnnotPolygon *>(d->pdfAnnot.get());
        return (LineAnnotation::TermStyle)polyann->getStartStyle();
    }
}

void LineAnnotation::setLineStartStyle(LineAnnotation::TermStyle style)
{
    Q_D(LineAnnotation);

    if (!d->pdfAnnot) {
        d->lineStartStyle = style;
        return;
    }

    if (d->pdfAnnot->getType() == Annot::typeLine) {
        AnnotLine *lineann = static_cast<AnnotLine *>(d->pdfAnnot.get());
        lineann->setStartEndStyle((AnnotLineEndingStyle)style, lineann->getEndStyle());
    } else {
        AnnotPolygon *polyann = static_cast<AnnotPolygon *>(d->pdfAnnot.get());
        polyann->setStartEndStyle((AnnotLineEndingStyle)style, polyann->getEndStyle());
    }
}

LineAnnotation::TermStyle LineAnnotation::lineEndStyle() const
{
    Q_D(const LineAnnotation);

    if (!d->pdfAnnot) {
        return d->lineEndStyle;
    }

    if (d->pdfAnnot->getType() == Annot::typeLine) {
        const AnnotLine *lineann = static_cast<const AnnotLine *>(d->pdfAnnot.get());
        return (LineAnnotation::TermStyle)lineann->getEndStyle();
    } else {
        const AnnotPolygon *polyann = static_cast<const AnnotPolygon *>(d->pdfAnnot.get());
        return (LineAnnotation::TermStyle)polyann->getEndStyle();
    }
}

void LineAnnotation::setLineEndStyle(LineAnnotation::TermStyle style)
{
    Q_D(LineAnnotation);

    if (!d->pdfAnnot) {
        d->lineEndStyle = style;
        return;
    }

    if (d->pdfAnnot->getType() == Annot::typeLine) {
        AnnotLine *lineann = static_cast<AnnotLine *>(d->pdfAnnot.get());
        lineann->setStartEndStyle(lineann->getStartStyle(), (AnnotLineEndingStyle)style);
    } else {
        AnnotPolygon *polyann = static_cast<AnnotPolygon *>(d->pdfAnnot.get());
        polyann->setStartEndStyle(polyann->getStartStyle(), (AnnotLineEndingStyle)style);
    }
}

bool LineAnnotation::isLineClosed() const
{
    Q_D(const LineAnnotation);

    if (!d->pdfAnnot) {
        return d->lineClosed;
    }

    return d->pdfAnnot->getType() == Annot::typePolygon;
}

void LineAnnotation::setLineClosed(bool closed)
{
    Q_D(LineAnnotation);

    if (!d->pdfAnnot) {
        d->lineClosed = closed;
        return;
    }

    if (d->pdfAnnot->getType() != Annot::typeLine) {
        AnnotPolygon *polyann = static_cast<AnnotPolygon *>(d->pdfAnnot.get());

        // Set new subtype and switch intent if necessary
        if (closed) {
            polyann->setType(Annot::typePolygon);
            if (polyann->getIntent() == AnnotPolygon::polylineDimension) {
                polyann->setIntent(AnnotPolygon::polygonDimension);
            }
        } else {
            polyann->setType(Annot::typePolyLine);
            if (polyann->getIntent() == AnnotPolygon::polygonDimension) {
                polyann->setIntent(AnnotPolygon::polylineDimension);
            }
        }
    }
}

QColor LineAnnotation::lineInnerColor() const
{
    Q_D(const LineAnnotation);

    if (!d->pdfAnnot) {
        return d->lineInnerColor;
    }

    AnnotColor *c;

    if (d->pdfAnnot->getType() == Annot::typeLine) {
        const AnnotLine *lineann = static_cast<const AnnotLine *>(d->pdfAnnot.get());
        c = lineann->getInteriorColor();
    } else {
        const AnnotPolygon *polyann = static_cast<const AnnotPolygon *>(d->pdfAnnot.get());
        c = polyann->getInteriorColor();
    }

    return convertAnnotColor(c);
}

void LineAnnotation::setLineInnerColor(const QColor &color)
{
    Q_D(LineAnnotation);

    if (!d->pdfAnnot) {
        d->lineInnerColor = color;
        return;
    }

    auto c = convertQColor(color);

    if (d->pdfAnnot->getType() == Annot::typeLine) {
        AnnotLine *lineann = static_cast<AnnotLine *>(d->pdfAnnot.get());
        lineann->setInteriorColor(std::move(c));
    } else {
        AnnotPolygon *polyann = static_cast<AnnotPolygon *>(d->pdfAnnot.get());
        polyann->setInteriorColor(std::move(c));
    }
}

double LineAnnotation::lineLeadingForwardPoint() const
{
    Q_D(const LineAnnotation);

    if (!d->pdfAnnot) {
        return d->lineLeadingFwdPt;
    }

    if (d->pdfAnnot->getType() == Annot::typeLine) {
        const AnnotLine *lineann = static_cast<const AnnotLine *>(d->pdfAnnot.get());
        return lineann->getLeaderLineLength();
    }

    return 0;
}

void LineAnnotation::setLineLeadingForwardPoint(double point)
{
    Q_D(LineAnnotation);

    if (!d->pdfAnnot) {
        d->lineLeadingFwdPt = point;
        return;
    }

    if (d->pdfAnnot->getType() == Annot::typeLine) {
        AnnotLine *lineann = static_cast<AnnotLine *>(d->pdfAnnot.get());
        lineann->setLeaderLineLength(point);
    }
}

double LineAnnotation::lineLeadingBackPoint() const
{
    Q_D(const LineAnnotation);

    if (!d->pdfAnnot) {
        return d->lineLeadingBackPt;
    }

    if (d->pdfAnnot->getType() == Annot::typeLine) {
        const AnnotLine *lineann = static_cast<const AnnotLine *>(d->pdfAnnot.get());
        return lineann->getLeaderLineExtension();
    }

    return 0;
}

void LineAnnotation::setLineLeadingBackPoint(double point)
{
    Q_D(LineAnnotation);

    if (!d->pdfAnnot) {
        d->lineLeadingBackPt = point;
        return;
    }

    if (d->pdfAnnot->getType() == Annot::typeLine) {
        AnnotLine *lineann = static_cast<AnnotLine *>(d->pdfAnnot.get());
        lineann->setLeaderLineExtension(point);
    }
}

bool LineAnnotation::lineShowCaption() const
{
    Q_D(const LineAnnotation);

    if (!d->pdfAnnot) {
        return d->lineShowCaption;
    }

    if (d->pdfAnnot->getType() == Annot::typeLine) {
        const AnnotLine *lineann = static_cast<const AnnotLine *>(d->pdfAnnot.get());
        return lineann->getCaption();
    }

    return false;
}

void LineAnnotation::setLineShowCaption(bool show)
{
    Q_D(LineAnnotation);

    if (!d->pdfAnnot) {
        d->lineShowCaption = show;
        return;
    }

    if (d->pdfAnnot->getType() == Annot::typeLine) {
        AnnotLine *lineann = static_cast<AnnotLine *>(d->pdfAnnot.get());
        lineann->setCaption(show);
    }
}

LineAnnotation::LineIntent LineAnnotation::lineIntent() const
{
    Q_D(const LineAnnotation);

    if (!d->pdfAnnot) {
        return d->lineIntent;
    }

    if (d->pdfAnnot->getType() == Annot::typeLine) {
        const AnnotLine *lineann = static_cast<const AnnotLine *>(d->pdfAnnot.get());
        return (LineAnnotation::LineIntent)(lineann->getIntent() + 1);
    } else {
        const AnnotPolygon *polyann = static_cast<const AnnotPolygon *>(d->pdfAnnot.get());
        if (polyann->getIntent() == AnnotPolygon::polygonCloud) {
            return LineAnnotation::PolygonCloud;
        } else { // AnnotPolygon::polylineDimension, AnnotPolygon::polygonDimension
            return LineAnnotation::Dimension;
        }
    }
}

void LineAnnotation::setLineIntent(LineAnnotation::LineIntent intent)
{
    Q_D(LineAnnotation);

    if (!d->pdfAnnot) {
        d->lineIntent = intent;
        return;
    }

    if (intent == LineAnnotation::Unknown) {
        return; // Do not set (actually, it should clear the property)
    }

    if (d->pdfAnnot->getType() == Annot::typeLine) {
        AnnotLine *lineann = static_cast<AnnotLine *>(d->pdfAnnot.get());
        lineann->setIntent((AnnotLine::AnnotLineIntent)(intent - 1));
    } else {
        AnnotPolygon *polyann = static_cast<AnnotPolygon *>(d->pdfAnnot.get());
        if (intent == LineAnnotation::PolygonCloud) {
            polyann->setIntent(AnnotPolygon::polygonCloud);
        } else // LineAnnotation::Dimension
        {
            if (d->pdfAnnot->getType() == Annot::typePolygon) {
                polyann->setIntent(AnnotPolygon::polygonDimension);
            } else { // Annot::typePolyLine
                polyann->setIntent(AnnotPolygon::polylineDimension);
            }
        }
    }
}

/** GeomAnnotation [Annotation] */
class GeomAnnotationPrivate : public AnnotationPrivate
{
public:
    GeomAnnotationPrivate();
    std::unique_ptr<Annotation> makeAlias() override;
    std::shared_ptr<Annot> createNativeAnnot(::Page *destPage, DocumentData *doc) override;

    // data fields (note uses border for rendering style)
    GeomAnnotation::GeomType geomType;
    QColor geomInnerColor;
};

GeomAnnotationPrivate::GeomAnnotationPrivate() : geomType(GeomAnnotation::InscribedSquare) { }

std::unique_ptr<Annotation> GeomAnnotationPrivate::makeAlias()
{
    return std::unique_ptr<GeomAnnotation>(new GeomAnnotation(*this));
}

std::shared_ptr<Annot> GeomAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    // Setters are defined in the public class
    std::unique_ptr<GeomAnnotation> q = static_pointer_cast<GeomAnnotation>(makeAlias());

    // Set page and document
    pdfPage = destPage;
    parentDoc = doc;

    Annot::AnnotSubtype type;
    if (geomType == GeomAnnotation::InscribedSquare) {
        type = Annot::typeSquare;
    } else { // GeomAnnotation::InscribedCircle
        type = Annot::typeCircle;
    }

    // Set pdfAnnot
    PDFRectangle rect = boundaryToPdfRectangle(boundary, flags);
    pdfAnnot = std::make_shared<AnnotGeometry>(destPage->getDoc(), &rect, type);

    // Set properties
    flushBaseAnnotationProperties();
    q->setGeomInnerColor(geomInnerColor);

    return pdfAnnot;
}

GeomAnnotation::GeomAnnotation() : Annotation(*new GeomAnnotationPrivate()) { }

GeomAnnotation::GeomAnnotation(GeomAnnotationPrivate &dd) : Annotation(dd) { }

GeomAnnotation::GeomAnnotation(const QDomNode &node) : Annotation(*new GeomAnnotationPrivate(), node)
{
    // loop through the whole children looking for a 'geom' element
    QDomNode subNode = node.firstChild();
    while (subNode.isElement()) {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if (e.tagName() != QLatin1String("geom")) {
            continue;
        }

        // parse the attributes
        if (e.hasAttribute(QStringLiteral("type"))) {
            setGeomType((GeomAnnotation::GeomType)e.attribute(QStringLiteral("type")).toInt());
        }
        if (e.hasAttribute(QStringLiteral("color"))) {
            setGeomInnerColor(QColor(e.attribute(QStringLiteral("color"))));
        }

        // loading complete
        break;
    }
}

GeomAnnotation::~GeomAnnotation() = default;

void GeomAnnotation::store(QDomNode &node, QDomDocument &document) const
{
    // store base annotation properties
    storeBaseAnnotationProperties(node, document);

    // create [geom] element
    QDomElement geomElement = document.createElement(QStringLiteral("geom"));
    node.appendChild(geomElement);

    // append the optional attributes
    if (geomType() != InscribedSquare) {
        geomElement.setAttribute(QStringLiteral("type"), (int)geomType());
    }
    if (geomInnerColor().isValid()) {
        geomElement.setAttribute(QStringLiteral("color"), geomInnerColor().name());
    }
}

Annotation::SubType GeomAnnotation::subType() const
{
    return AGeom;
}

GeomAnnotation::GeomType GeomAnnotation::geomType() const
{
    Q_D(const GeomAnnotation);

    if (!d->pdfAnnot) {
        return d->geomType;
    }

    if (d->pdfAnnot->getType() == Annot::typeSquare) {
        return GeomAnnotation::InscribedSquare;
    } else { // Annot::typeCircle
        return GeomAnnotation::InscribedCircle;
    }
}

void GeomAnnotation::setGeomType(GeomAnnotation::GeomType type)
{
    Q_D(GeomAnnotation);

    if (!d->pdfAnnot) {
        d->geomType = type;
        return;
    }

    AnnotGeometry *geomann = static_cast<AnnotGeometry *>(d->pdfAnnot.get());
    if (type == GeomAnnotation::InscribedSquare) {
        geomann->setType(Annot::typeSquare);
    } else { // GeomAnnotation::InscribedCircle
        geomann->setType(Annot::typeCircle);
    }
}

QColor GeomAnnotation::geomInnerColor() const
{
    Q_D(const GeomAnnotation);

    if (!d->pdfAnnot) {
        return d->geomInnerColor;
    }

    const AnnotGeometry *geomann = static_cast<const AnnotGeometry *>(d->pdfAnnot.get());
    return convertAnnotColor(geomann->getInteriorColor());
}

void GeomAnnotation::setGeomInnerColor(const QColor &color)
{
    Q_D(GeomAnnotation);

    if (!d->pdfAnnot) {
        d->geomInnerColor = color;
        return;
    }

    AnnotGeometry *geomann = static_cast<AnnotGeometry *>(d->pdfAnnot.get());
    geomann->setInteriorColor(convertQColor(color));
}

/** HighlightAnnotation [Annotation] */
class HighlightAnnotationPrivate : public AnnotationPrivate
{
public:
    HighlightAnnotationPrivate();
    std::unique_ptr<Annotation> makeAlias() override;
    std::shared_ptr<Annot> createNativeAnnot(::Page *destPage, DocumentData *doc) override;

    // data fields
    HighlightAnnotation::HighlightType highlightType;
    QList<HighlightAnnotation::Quad> highlightQuads; // not empty

    // helpers
    static Annot::AnnotSubtype toAnnotSubType(HighlightAnnotation::HighlightType type);
    QList<HighlightAnnotation::Quad> fromQuadrilaterals(AnnotQuadrilaterals *quads) const;
    AnnotQuadrilaterals *toQuadrilaterals(const QList<HighlightAnnotation::Quad> &quads) const;
};

HighlightAnnotationPrivate::HighlightAnnotationPrivate() : highlightType(HighlightAnnotation::Highlight) { }

std::unique_ptr<Annotation> HighlightAnnotationPrivate::makeAlias()
{
    return std::unique_ptr<HighlightAnnotation>(new HighlightAnnotation(*this));
}

Annot::AnnotSubtype HighlightAnnotationPrivate::toAnnotSubType(HighlightAnnotation::HighlightType type)
{
    switch (type) {
    default: // HighlightAnnotation::Highlight:
        return Annot::typeHighlight;
    case HighlightAnnotation::Underline:
        return Annot::typeUnderline;
    case HighlightAnnotation::Squiggly:
        return Annot::typeSquiggly;
    case HighlightAnnotation::StrikeOut:
        return Annot::typeStrikeOut;
    }
}

QList<HighlightAnnotation::Quad> HighlightAnnotationPrivate::fromQuadrilaterals(AnnotQuadrilaterals *hlquads) const
{
    QList<HighlightAnnotation::Quad> quads;

    if (!hlquads || !hlquads->getQuadrilateralsLength()) {
        return quads;
    }
    const int quadsCount = hlquads->getQuadrilateralsLength();

    double MTX[6];
    fillTransformationMTX(MTX);

    quads.reserve(quadsCount);
    for (int q = 0; q < quadsCount; ++q) {
        HighlightAnnotation::Quad quad;
        XPDFReader::transform(MTX, hlquads->getX1(q), hlquads->getY1(q), quad.points[0]);
        XPDFReader::transform(MTX, hlquads->getX2(q), hlquads->getY2(q), quad.points[1]);
        XPDFReader::transform(MTX, hlquads->getX3(q), hlquads->getY3(q), quad.points[2]);
        XPDFReader::transform(MTX, hlquads->getX4(q), hlquads->getY4(q), quad.points[3]);
        // ### PDF1.6 specs says that point are in ccw order, but in fact
        // points 3 and 4 are swapped in every PDF around!
        QPointF tmpPoint = quad.points[2];
        quad.points[2] = quad.points[3];
        quad.points[3] = tmpPoint;
        // initialize other properties and append quad
        quad.capStart = true; // unlinked quads are always capped
        quad.capEnd = true; // unlinked quads are always capped
        quad.feather = 0.1; // default feather
        quads.append(quad);
    }

    return quads;
}

AnnotQuadrilaterals *HighlightAnnotationPrivate::toQuadrilaterals(const QList<HighlightAnnotation::Quad> &quads) const
{
    const int count = quads.size();
    auto ac = std::make_unique<AnnotQuadrilaterals::AnnotQuadrilateral[]>(count);

    double MTX[6];
    fillTransformationMTX(MTX);

    int pos = 0;
    Q_FOREACH (const HighlightAnnotation::Quad &q, quads) {
        double x1, y1, x2, y2, x3, y3, x4, y4;
        XPDFReader::invTransform(MTX, q.points[0], x1, y1);
        XPDFReader::invTransform(MTX, q.points[1], x2, y2);
        // Swap points 3 and 4 (see HighlightAnnotationPrivate::fromQuadrilaterals)
        XPDFReader::invTransform(MTX, q.points[3], x3, y3);
        XPDFReader::invTransform(MTX, q.points[2], x4, y4);
        ac[pos++] = AnnotQuadrilaterals::AnnotQuadrilateral(x1, y1, x2, y2, x3, y3, x4, y4);
    }

    return new AnnotQuadrilaterals(std::move(ac), count);
}

std::shared_ptr<Annot> HighlightAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    // Setters are defined in the public class
    std::unique_ptr<HighlightAnnotation> q = static_pointer_cast<HighlightAnnotation>(makeAlias());

    // Set page and document
    pdfPage = destPage;
    parentDoc = doc;

    // Set pdfAnnot
    PDFRectangle rect = boundaryToPdfRectangle(boundary, flags);
    pdfAnnot = std::make_shared<AnnotTextMarkup>(destPage->getDoc(), &rect, toAnnotSubType(highlightType));

    // Set properties
    flushBaseAnnotationProperties();
    q->setHighlightQuads(highlightQuads);

    highlightQuads.clear(); // Free up memory

    return pdfAnnot;
}

HighlightAnnotation::HighlightAnnotation() : Annotation(*new HighlightAnnotationPrivate()) { }

HighlightAnnotation::HighlightAnnotation(HighlightAnnotationPrivate &dd) : Annotation(dd) { }

HighlightAnnotation::HighlightAnnotation(const QDomNode &node) : Annotation(*new HighlightAnnotationPrivate(), node)
{
    // loop through the whole children looking for a 'hl' element
    QDomNode subNode = node.firstChild();
    while (subNode.isElement()) {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if (e.tagName() != QLatin1String("hl")) {
            continue;
        }

        // parse the attributes
        if (e.hasAttribute(QStringLiteral("type"))) {
            setHighlightType((HighlightAnnotation::HighlightType)e.attribute(QStringLiteral("type")).toInt());
        }

        // parse all 'quad' subnodes
        QList<HighlightAnnotation::Quad> quads;
        QDomNode quadNode = e.firstChild();
        for (; quadNode.isElement(); quadNode = quadNode.nextSibling()) {
            QDomElement qe = quadNode.toElement();
            if (qe.tagName() != QLatin1String("quad")) {
                continue;
            }

            Quad q;
            q.points[0].setX(qe.attribute(QStringLiteral("ax"), QStringLiteral("0.0")).toDouble());
            q.points[0].setY(qe.attribute(QStringLiteral("ay"), QStringLiteral("0.0")).toDouble());
            q.points[1].setX(qe.attribute(QStringLiteral("bx"), QStringLiteral("0.0")).toDouble());
            q.points[1].setY(qe.attribute(QStringLiteral("by"), QStringLiteral("0.0")).toDouble());
            q.points[2].setX(qe.attribute(QStringLiteral("cx"), QStringLiteral("0.0")).toDouble());
            q.points[2].setY(qe.attribute(QStringLiteral("cy"), QStringLiteral("0.0")).toDouble());
            q.points[3].setX(qe.attribute(QStringLiteral("dx"), QStringLiteral("0.0")).toDouble());
            q.points[3].setY(qe.attribute(QStringLiteral("dy"), QStringLiteral("0.0")).toDouble());
            q.capStart = qe.hasAttribute(QStringLiteral("start"));
            q.capEnd = qe.hasAttribute(QStringLiteral("end"));
            q.feather = qe.attribute(QStringLiteral("feather"), QStringLiteral("0.1")).toDouble();
            quads.append(q);
        }
        setHighlightQuads(quads);

        // loading complete
        break;
    }
}

HighlightAnnotation::~HighlightAnnotation() = default;

void HighlightAnnotation::store(QDomNode &node, QDomDocument &document) const
{
    // store base annotation properties
    storeBaseAnnotationProperties(node, document);

    // create [hl] element
    QDomElement hlElement = document.createElement(QStringLiteral("hl"));
    node.appendChild(hlElement);

    // append the optional attributes
    if (highlightType() != Highlight) {
        hlElement.setAttribute(QStringLiteral("type"), (int)highlightType());
    }

    const QList<HighlightAnnotation::Quad> quads = highlightQuads();
    if (quads.count() < 1) {
        return;
    }
    // append highlight quads, all children describe quads
    QList<HighlightAnnotation::Quad>::const_iterator it = quads.begin(), end = quads.end();
    for (; it != end; ++it) {
        QDomElement quadElement = document.createElement(QStringLiteral("quad"));
        hlElement.appendChild(quadElement);
        const Quad &q = *it;
        quadElement.setAttribute(QStringLiteral("ax"), QString::number(q.points[0].x()));
        quadElement.setAttribute(QStringLiteral("ay"), QString::number(q.points[0].y()));
        quadElement.setAttribute(QStringLiteral("bx"), QString::number(q.points[1].x()));
        quadElement.setAttribute(QStringLiteral("by"), QString::number(q.points[1].y()));
        quadElement.setAttribute(QStringLiteral("cx"), QString::number(q.points[2].x()));
        quadElement.setAttribute(QStringLiteral("cy"), QString::number(q.points[2].y()));
        quadElement.setAttribute(QStringLiteral("dx"), QString::number(q.points[3].x()));
        quadElement.setAttribute(QStringLiteral("dy"), QString::number(q.points[3].y()));
        if (q.capStart) {
            quadElement.setAttribute(QStringLiteral("start"), 1);
        }
        if (q.capEnd) {
            quadElement.setAttribute(QStringLiteral("end"), 1);
        }
        quadElement.setAttribute(QStringLiteral("feather"), QString::number(q.feather));
    }
}

Annotation::SubType HighlightAnnotation::subType() const
{
    return AHighlight;
}

HighlightAnnotation::HighlightType HighlightAnnotation::highlightType() const
{
    Q_D(const HighlightAnnotation);

    if (!d->pdfAnnot) {
        return d->highlightType;
    }

    Annot::AnnotSubtype subType = d->pdfAnnot->getType();

    if (subType == Annot::typeHighlight) {
        return HighlightAnnotation::Highlight;
    } else if (subType == Annot::typeUnderline) {
        return HighlightAnnotation::Underline;
    } else if (subType == Annot::typeSquiggly) {
        return HighlightAnnotation::Squiggly;
    } else { // Annot::typeStrikeOut
        return HighlightAnnotation::StrikeOut;
    }
}

void HighlightAnnotation::setHighlightType(HighlightAnnotation::HighlightType type)
{
    Q_D(HighlightAnnotation);

    if (!d->pdfAnnot) {
        d->highlightType = type;
        return;
    }

    AnnotTextMarkup *hlann = static_cast<AnnotTextMarkup *>(d->pdfAnnot.get());
    hlann->setType(HighlightAnnotationPrivate::toAnnotSubType(type));
}

QList<HighlightAnnotation::Quad> HighlightAnnotation::highlightQuads() const
{
    Q_D(const HighlightAnnotation);

    if (!d->pdfAnnot) {
        return d->highlightQuads;
    }

    const AnnotTextMarkup *hlann = static_cast<AnnotTextMarkup *>(d->pdfAnnot.get());
    return d->fromQuadrilaterals(hlann->getQuadrilaterals());
}

void HighlightAnnotation::setHighlightQuads(const QList<HighlightAnnotation::Quad> &quads)
{
    Q_D(HighlightAnnotation);

    if (!d->pdfAnnot) {
        d->highlightQuads = quads;
        return;
    }

    AnnotTextMarkup *hlann = static_cast<AnnotTextMarkup *>(d->pdfAnnot.get());
    AnnotQuadrilaterals *quadrilaterals = d->toQuadrilaterals(quads);
    hlann->setQuadrilaterals(*quadrilaterals);
    delete quadrilaterals;
}

/** StampAnnotation [Annotation] */
class StampAnnotationPrivate : public AnnotationPrivate
{
public:
    StampAnnotationPrivate();
    std::unique_ptr<Annotation> makeAlias() override;
    std::shared_ptr<Annot> createNativeAnnot(::Page *destPage, DocumentData *doc) override;

    std::unique_ptr<AnnotStampImageHelper> convertQImageToAnnotStampImageHelper(const QImage &qimg);

    // data fields
    QString stampIconName;
    QImage stampCustomImage;
};

StampAnnotationPrivate::StampAnnotationPrivate() : stampIconName(QStringLiteral("Draft")) { }

std::unique_ptr<Annotation> StampAnnotationPrivate::makeAlias()
{
    return std::unique_ptr<StampAnnotation>(new StampAnnotation(*this));
}

std::shared_ptr<Annot> StampAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    std::unique_ptr<StampAnnotation> q = static_pointer_cast<StampAnnotation>(makeAlias());

    // Set page and document
    pdfPage = destPage;
    parentDoc = doc;

    // Set pdfAnnot
    PDFRectangle rect = boundaryToPdfRectangle(boundary, flags);
    pdfAnnot = std::make_shared<AnnotStamp>(destPage->getDoc(), &rect);

    // Set properties
    flushBaseAnnotationProperties();
    q->setStampIconName(stampIconName);
    q->setStampCustomImage(stampCustomImage);

    stampIconName.clear(); // Free up memory

    return pdfAnnot;
}

std::unique_ptr<AnnotStampImageHelper> StampAnnotationPrivate::convertQImageToAnnotStampImageHelper(const QImage &qimg)
{
    QImage convertedQImage = qimg;

    QByteArray data;
    QByteArray sMaskData;
    const int width = convertedQImage.width();
    const int height = convertedQImage.height();
    int bitsPerComponent = 1;
    ColorSpace colorSpace = ColorSpace::DeviceGray;

    switch (convertedQImage.format()) {
    case QImage::Format_MonoLSB:
        if (!convertedQImage.allGray()) {
            convertedQImage = convertedQImage.convertToFormat(QImage::Format_RGB888);

            colorSpace = ColorSpace::DeviceRGB;
            bitsPerComponent = 8;
        } else {
            convertedQImage = convertedQImage.convertToFormat(QImage::Format_Mono);
        }
        break;
    case QImage::Format_Mono:
        if (!convertedQImage.allGray()) {
            convertedQImage = convertedQImage.convertToFormat(QImage::Format_RGB888);

            colorSpace = ColorSpace::DeviceRGB;
            bitsPerComponent = 8;
        }
        break;
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_ARGB8565_Premultiplied:
    case QImage::Format_ARGB6666_Premultiplied:
    case QImage::Format_ARGB8555_Premultiplied:
    case QImage::Format_ARGB4444_Premultiplied:
    case QImage::Format_Alpha8:
        convertedQImage = convertedQImage.convertToFormat(QImage::Format_ARGB32);
        colorSpace = ColorSpace::DeviceRGB;
        bitsPerComponent = 8;
        break;
    case QImage::Format_RGBA8888:
    case QImage::Format_RGBA8888_Premultiplied:
    case QImage::Format_RGBX8888:
    case QImage::Format_ARGB32:
        colorSpace = ColorSpace::DeviceRGB;
        bitsPerComponent = 8;
        break;
    case QImage::Format_Grayscale8:
        bitsPerComponent = 8;
        break;
    case QImage::Format_Grayscale16:
        convertedQImage = convertedQImage.convertToFormat(QImage::Format_Grayscale8);

        colorSpace = ColorSpace::DeviceGray;
        bitsPerComponent = 8;
        break;
    case QImage::Format_RGB16:
    case QImage::Format_RGB666:
    case QImage::Format_RGB555:
    case QImage::Format_RGB444:
        convertedQImage = convertedQImage.convertToFormat(QImage::Format_RGB888);
        colorSpace = ColorSpace::DeviceRGB;
        bitsPerComponent = 8;
        break;
    case QImage::Format_RGB888:
        colorSpace = ColorSpace::DeviceRGB;
        bitsPerComponent = 8;
        break;
    default:
        convertedQImage = convertedQImage.convertToFormat(QImage::Format_ARGB32);

        colorSpace = ColorSpace::DeviceRGB;
        bitsPerComponent = 8;
        break;
    }

    getRawDataFromQImage(convertedQImage, convertedQImage.depth(), &data, &sMaskData);

    std::unique_ptr<AnnotStampImageHelper> annotImg;

    if (sMaskData.count() > 0) {
        AnnotStampImageHelper sMask(parentDoc->doc, width, height, ColorSpace::DeviceGray, 8, sMaskData.data(), sMaskData.count());
        annotImg = std::make_unique<AnnotStampImageHelper>(parentDoc->doc, width, height, colorSpace, bitsPerComponent, data.data(), data.count(), sMask.getRef());
    } else {
        annotImg = std::make_unique<AnnotStampImageHelper>(parentDoc->doc, width, height, colorSpace, bitsPerComponent, data.data(), data.count());
    }

    return annotImg;
}

StampAnnotation::StampAnnotation() : Annotation(*new StampAnnotationPrivate()) { }

StampAnnotation::StampAnnotation(StampAnnotationPrivate &dd) : Annotation(dd) { }

StampAnnotation::StampAnnotation(const QDomNode &node) : Annotation(*new StampAnnotationPrivate(), node)
{
    // loop through the whole children looking for a 'stamp' element
    QDomNode subNode = node.firstChild();
    while (subNode.isElement()) {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if (e.tagName() != QLatin1String("stamp")) {
            continue;
        }

        // parse the attributes
        if (e.hasAttribute(QStringLiteral("icon"))) {
            setStampIconName(e.attribute(QStringLiteral("icon")));
        }

        // loading complete
        break;
    }
}

StampAnnotation::~StampAnnotation() = default;

void StampAnnotation::store(QDomNode &node, QDomDocument &document) const
{
    // store base annotation properties
    storeBaseAnnotationProperties(node, document);

    // create [stamp] element
    QDomElement stampElement = document.createElement(QStringLiteral("stamp"));
    node.appendChild(stampElement);

    // append the optional attributes
    if (stampIconName() != QLatin1String("Draft")) {
        stampElement.setAttribute(QStringLiteral("icon"), stampIconName());
    }
}

Annotation::SubType StampAnnotation::subType() const
{
    return AStamp;
}

QString StampAnnotation::stampIconName() const
{
    Q_D(const StampAnnotation);

    if (!d->pdfAnnot) {
        return d->stampIconName;
    }

    const AnnotStamp *stampann = static_cast<const AnnotStamp *>(d->pdfAnnot.get());
    return QString::fromStdString(stampann->getIcon());
}

void StampAnnotation::setStampIconName(const QString &name)
{
    Q_D(StampAnnotation);

    if (!d->pdfAnnot) {
        d->stampIconName = name;
        return;
    }

    AnnotStamp *stampann = static_cast<AnnotStamp *>(d->pdfAnnot.get());
    stampann->setIcon(name.toStdString());
}

void StampAnnotation::setStampCustomImage(const QImage &image)
{
    if (image.isNull()) {
        return;
    }

    Q_D(StampAnnotation);

    if (!d->pdfAnnot) {
        d->stampCustomImage = QImage(image);
        return;
    }

    AnnotStamp *stampann = static_cast<AnnotStamp *>(d->pdfAnnot.get());
    std::unique_ptr<AnnotStampImageHelper> annotCustomImage = d->convertQImageToAnnotStampImageHelper(image);
    stampann->setCustomImage(std::move(annotCustomImage));
}

/** InkAnnotation [Annotation] */
class InkAnnotationPrivate : public AnnotationPrivate
{
public:
    InkAnnotationPrivate();
    std::unique_ptr<Annotation> makeAlias() override;
    std::shared_ptr<Annot> createNativeAnnot(::Page *destPage, DocumentData *doc) override;

    // data fields
    QList<QLinkedList<QPointF>> inkPaths;

    // helper
    std::vector<std::unique_ptr<AnnotPath>> toAnnotPaths(const QList<QLinkedList<QPointF>> &paths);
};

InkAnnotationPrivate::InkAnnotationPrivate() = default;

std::unique_ptr<Annotation> InkAnnotationPrivate::makeAlias()
{
    return std::unique_ptr<InkAnnotation>(new InkAnnotation(*this));
}

// Note: Caller is required to delete array elements and the array itself after use
std::vector<std::unique_ptr<AnnotPath>> InkAnnotationPrivate::toAnnotPaths(const QList<QLinkedList<QPointF>> &paths)
{
    std::vector<std::unique_ptr<AnnotPath>> res;
    res.reserve(paths.size());
    for (const auto &path : paths) {
        res.push_back(toAnnotPath(path));
    }
    return res;
}

std::shared_ptr<Annot> InkAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    // Setters are defined in the public class
    std::unique_ptr<InkAnnotation> q = static_pointer_cast<InkAnnotation>(makeAlias());

    // Set page and document
    pdfPage = destPage;
    parentDoc = doc;

    // Set pdfAnnot
    PDFRectangle rect = boundaryToPdfRectangle(boundary, flags);
    pdfAnnot = std::make_shared<AnnotInk>(destPage->getDoc(), &rect);

    // Set properties
    flushBaseAnnotationProperties();
    q->setInkPaths(inkPaths);

    inkPaths.clear(); // Free up memory

    return pdfAnnot;
}

InkAnnotation::InkAnnotation() : Annotation(*new InkAnnotationPrivate()) { }

InkAnnotation::InkAnnotation(InkAnnotationPrivate &dd) : Annotation(dd) { }

InkAnnotation::InkAnnotation(const QDomNode &node) : Annotation(*new InkAnnotationPrivate(), node)
{
    // loop through the whole children looking for a 'ink' element
    QDomNode subNode = node.firstChild();
    while (subNode.isElement()) {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if (e.tagName() != QLatin1String("ink")) {
            continue;
        }

        // parse the 'path' subnodes
        QList<QLinkedList<QPointF>> paths;
        QDomNode pathNode = e.firstChild();
        while (pathNode.isElement()) {
            QDomElement pathElement = pathNode.toElement();
            pathNode = pathNode.nextSibling();

            if (pathElement.tagName() != QLatin1String("path")) {
                continue;
            }

            // build each path parsing 'point' subnodes
            QLinkedList<QPointF> path;
            QDomNode pointNode = pathElement.firstChild();
            while (pointNode.isElement()) {
                QDomElement pointElement = pointNode.toElement();
                pointNode = pointNode.nextSibling();

                if (pointElement.tagName() != QLatin1String("point")) {
                    continue;
                }

                QPointF p(pointElement.attribute(QStringLiteral("x"), QStringLiteral("0.0")).toDouble(), pointElement.attribute(QStringLiteral("y"), QStringLiteral("0.0")).toDouble());
                path.append(p);
            }

            // add the path to the path list if it contains at least 2 nodes
            if (path.count() >= 2) {
                paths.append(path);
            }
        }
        setInkPaths(paths);

        // loading complete
        break;
    }
}

InkAnnotation::~InkAnnotation() = default;

void InkAnnotation::store(QDomNode &node, QDomDocument &document) const
{
    // store base annotation properties
    storeBaseAnnotationProperties(node, document);

    // create [ink] element
    QDomElement inkElement = document.createElement(QStringLiteral("ink"));
    node.appendChild(inkElement);

    // append the optional attributes
    const QList<QLinkedList<QPointF>> paths = inkPaths();
    if (paths.count() < 1) {
        return;
    }
    QList<QLinkedList<QPointF>>::const_iterator pIt = paths.begin(), pEnd = paths.end();
    for (; pIt != pEnd; ++pIt) {
        QDomElement pathElement = document.createElement(QStringLiteral("path"));
        inkElement.appendChild(pathElement);
        const QLinkedList<QPointF> &path = *pIt;
        QLinkedList<QPointF>::const_iterator iIt = path.begin(), iEnd = path.end();
        for (; iIt != iEnd; ++iIt) {
            const QPointF &point = *iIt;
            QDomElement pointElement = document.createElement(QStringLiteral("point"));
            pathElement.appendChild(pointElement);
            pointElement.setAttribute(QStringLiteral("x"), QString::number(point.x()));
            pointElement.setAttribute(QStringLiteral("y"), QString::number(point.y()));
        }
    }
}

Annotation::SubType InkAnnotation::subType() const
{
    return AInk;
}

QList<QLinkedList<QPointF>> InkAnnotation::inkPaths() const
{
    Q_D(const InkAnnotation);

    if (!d->pdfAnnot) {
        return d->inkPaths;
    }

    const AnnotInk *inkann = static_cast<const AnnotInk *>(d->pdfAnnot.get());

    const std::vector<std::unique_ptr<AnnotPath>> &paths = inkann->getInkList();
    if (paths.empty()) {
        return QList<QLinkedList<QPointF>>();
    }

    double MTX[6];
    d->fillTransformationMTX(MTX);

    QList<QLinkedList<QPointF>> inkPaths;
    inkPaths.reserve(paths.size());
    for (const auto &path : paths) {
        // transform each path in a list of normalized points ..
        QLinkedList<QPointF> localList;
        const int pointsNumber = path ? path->getCoordsLength() : 0;
        for (int n = 0; n < pointsNumber; ++n) {
            QPointF point;
            XPDFReader::transform(MTX, path->getX(n), path->getY(n), point);
            localList.append(point);
        }
        // ..and add it to the annotation
        inkPaths.append(localList);
    }
    return inkPaths;
}

void InkAnnotation::setInkPaths(const QList<QLinkedList<QPointF>> &paths)
{
    Q_D(InkAnnotation);

    if (!d->pdfAnnot) {
        d->inkPaths = paths;
        return;
    }

    AnnotInk *inkann = static_cast<AnnotInk *>(d->pdfAnnot.get());
    const std::vector<std::unique_ptr<AnnotPath>> annotpaths = d->toAnnotPaths(paths);
    inkann->setInkList(annotpaths);
}

/** LinkAnnotation [Annotation] */
class LinkAnnotationPrivate : public AnnotationPrivate
{
public:
    LinkAnnotationPrivate();
    ~LinkAnnotationPrivate() override;
    std::unique_ptr<Annotation> makeAlias() override;
    std::shared_ptr<Annot> createNativeAnnot(::Page *destPage, DocumentData *doc) override;

    // data fields
    Link *linkDestination;
    LinkAnnotation::HighlightMode linkHLMode;
    QPointF linkRegion[4];
};

LinkAnnotationPrivate::LinkAnnotationPrivate() : linkDestination(nullptr), linkHLMode(LinkAnnotation::Invert) { }

LinkAnnotationPrivate::~LinkAnnotationPrivate()
{
    delete linkDestination;
}

std::unique_ptr<Annotation> LinkAnnotationPrivate::makeAlias()
{
    return std::unique_ptr<LinkAnnotation>(new LinkAnnotation(*this));
}

std::shared_ptr<Annot> LinkAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    return nullptr; // Not implemented
}

LinkAnnotation::LinkAnnotation() : Annotation(*new LinkAnnotationPrivate()) { }

LinkAnnotation::LinkAnnotation(LinkAnnotationPrivate &dd) : Annotation(dd) { }

LinkAnnotation::LinkAnnotation(const QDomNode &node) : Annotation(*new LinkAnnotationPrivate(), node)
{
    // loop through the whole children looking for a 'link' element
    QDomNode subNode = node.firstChild();
    while (subNode.isElement()) {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if (e.tagName() != QLatin1String("link")) {
            continue;
        }

        // parse the attributes
        if (e.hasAttribute(QStringLiteral("hlmode"))) {
            setLinkHighlightMode((LinkAnnotation::HighlightMode)e.attribute(QStringLiteral("hlmode")).toInt());
        }

        // parse all 'quad' subnodes
        QDomNode quadNode = e.firstChild();
        for (; quadNode.isElement(); quadNode = quadNode.nextSibling()) {
            QDomElement qe = quadNode.toElement();
            if (qe.tagName() == QLatin1String("quad")) {
                setLinkRegionPoint(0, QPointF(qe.attribute(QStringLiteral("ax"), QStringLiteral("0.0")).toDouble(), qe.attribute(QStringLiteral("ay"), QStringLiteral("0.0")).toDouble()));
                setLinkRegionPoint(1, QPointF(qe.attribute(QStringLiteral("bx"), QStringLiteral("0.0")).toDouble(), qe.attribute(QStringLiteral("by"), QStringLiteral("0.0")).toDouble()));
                setLinkRegionPoint(2, QPointF(qe.attribute(QStringLiteral("cx"), QStringLiteral("0.0")).toDouble(), qe.attribute(QStringLiteral("cy"), QStringLiteral("0.0")).toDouble()));
                setLinkRegionPoint(3, QPointF(qe.attribute(QStringLiteral("dx"), QStringLiteral("0.0")).toDouble(), qe.attribute(QStringLiteral("dy"), QStringLiteral("0.0")).toDouble()));
            } else if (qe.tagName() == QLatin1String("link")) {
                QString type = qe.attribute(QStringLiteral("type"));
                if (type == QLatin1String("GoTo")) {
                    Poppler::LinkGoto *go = new Poppler::LinkGoto(QRect(), qe.attribute(QStringLiteral("filename")), LinkDestination(qe.attribute(QStringLiteral("destination"))));
                    setLinkDestination(go);
                } else if (type == QLatin1String("Exec")) {
                    Poppler::LinkExecute *exec = new Poppler::LinkExecute(QRect(), qe.attribute(QStringLiteral("filename")), qe.attribute(QStringLiteral("parameters")));
                    setLinkDestination(exec);
                } else if (type == QLatin1String("Browse")) {
                    Poppler::LinkBrowse *browse = new Poppler::LinkBrowse(QRect(), qe.attribute(QStringLiteral("url")));
                    setLinkDestination(browse);
                } else if (type == QLatin1String("Action")) {
                    Poppler::LinkAction::ActionType act;
                    QString actString = qe.attribute(QStringLiteral("action"));
                    bool found = true;
                    if (actString == QLatin1String("PageFirst")) {
                        act = Poppler::LinkAction::PageFirst;
                    } else if (actString == QLatin1String("PagePrev")) {
                        act = Poppler::LinkAction::PagePrev;
                    } else if (actString == QLatin1String("PageNext")) {
                        act = Poppler::LinkAction::PageNext;
                    } else if (actString == QLatin1String("PageLast")) {
                        act = Poppler::LinkAction::PageLast;
                    } else if (actString == QLatin1String("HistoryBack")) {
                        act = Poppler::LinkAction::HistoryBack;
                    } else if (actString == QLatin1String("HistoryForward")) {
                        act = Poppler::LinkAction::HistoryForward;
                    } else if (actString == QLatin1String("Quit")) {
                        act = Poppler::LinkAction::Quit;
                    } else if (actString == QLatin1String("Presentation")) {
                        act = Poppler::LinkAction::Presentation;
                    } else if (actString == QLatin1String("EndPresentation")) {
                        act = Poppler::LinkAction::EndPresentation;
                    } else if (actString == QLatin1String("Find")) {
                        act = Poppler::LinkAction::Find;
                    } else if (actString == QLatin1String("GoToPage")) {
                        act = Poppler::LinkAction::GoToPage;
                    } else if (actString == QLatin1String("Close")) {
                        act = Poppler::LinkAction::Close;
                    } else if (actString == QLatin1String("Print")) {
                        act = Poppler::LinkAction::Print;
                    } else {
                        found = false;
                    }
                    if (found) {
                        Poppler::LinkAction *action = new Poppler::LinkAction(QRect(), act);
                        setLinkDestination(action);
                    }
                } else {
                    qWarning("Loading annotations of type %s from DOM nodes is not yet implemented.", type.toLocal8Bit().constData());
                }
            }
        }

        // loading complete
        break;
    }
}

LinkAnnotation::~LinkAnnotation() = default;

void LinkAnnotation::store(QDomNode &node, QDomDocument &document) const
{
    // store base annotation properties
    storeBaseAnnotationProperties(node, document);

    // create [hl] element
    QDomElement linkElement = document.createElement(QStringLiteral("link"));
    node.appendChild(linkElement);

    // append the optional attributes
    if (linkHighlightMode() != Invert) {
        linkElement.setAttribute(QStringLiteral("hlmode"), (int)linkHighlightMode());
    }

    // saving region
    QDomElement quadElement = document.createElement(QStringLiteral("quad"));
    linkElement.appendChild(quadElement);
    quadElement.setAttribute(QStringLiteral("ax"), QString::number(linkRegionPoint(0).x()));
    quadElement.setAttribute(QStringLiteral("ay"), QString::number(linkRegionPoint(0).y()));
    quadElement.setAttribute(QStringLiteral("bx"), QString::number(linkRegionPoint(1).x()));
    quadElement.setAttribute(QStringLiteral("by"), QString::number(linkRegionPoint(1).y()));
    quadElement.setAttribute(QStringLiteral("cx"), QString::number(linkRegionPoint(2).x()));
    quadElement.setAttribute(QStringLiteral("cy"), QString::number(linkRegionPoint(2).y()));
    quadElement.setAttribute(QStringLiteral("dx"), QString::number(linkRegionPoint(3).x()));
    quadElement.setAttribute(QStringLiteral("dy"), QString::number(linkRegionPoint(3).y()));

    // saving link
    QDomElement hyperlinkElement = document.createElement(QStringLiteral("link"));
    linkElement.appendChild(hyperlinkElement);
    if (linkDestination()) {
        switch (linkDestination()->linkType()) {
        case Poppler::Link::Goto: {
            Poppler::LinkGoto *go = static_cast<Poppler::LinkGoto *>(linkDestination());
            hyperlinkElement.setAttribute(QStringLiteral("type"), QStringLiteral("GoTo"));
            hyperlinkElement.setAttribute(QStringLiteral("filename"), go->fileName());
            hyperlinkElement.setAttribute(QStringLiteral("destionation"), go->destination().toString()); // TODO remove for poppler 0.28
            hyperlinkElement.setAttribute(QStringLiteral("destination"), go->destination().toString());
            break;
        }
        case Poppler::Link::Execute: {
            Poppler::LinkExecute *exec = static_cast<Poppler::LinkExecute *>(linkDestination());
            hyperlinkElement.setAttribute(QStringLiteral("type"), QStringLiteral("Exec"));
            hyperlinkElement.setAttribute(QStringLiteral("filename"), exec->fileName());
            hyperlinkElement.setAttribute(QStringLiteral("parameters"), exec->parameters());
            break;
        }
        case Poppler::Link::Browse: {
            Poppler::LinkBrowse *browse = static_cast<Poppler::LinkBrowse *>(linkDestination());
            hyperlinkElement.setAttribute(QStringLiteral("type"), QStringLiteral("Browse"));
            hyperlinkElement.setAttribute(QStringLiteral("url"), browse->url());
            break;
        }
        case Poppler::Link::Action: {
            Poppler::LinkAction *action = static_cast<Poppler::LinkAction *>(linkDestination());
            hyperlinkElement.setAttribute(QStringLiteral("type"), QStringLiteral("Action"));
            switch (action->actionType()) {
            case Poppler::LinkAction::PageFirst:
                hyperlinkElement.setAttribute(QStringLiteral("action"), QStringLiteral("PageFirst"));
                break;
            case Poppler::LinkAction::PagePrev:
                hyperlinkElement.setAttribute(QStringLiteral("action"), QStringLiteral("PagePrev"));
                break;
            case Poppler::LinkAction::PageNext:
                hyperlinkElement.setAttribute(QStringLiteral("action"), QStringLiteral("PageNext"));
                break;
            case Poppler::LinkAction::PageLast:
                hyperlinkElement.setAttribute(QStringLiteral("action"), QStringLiteral("PageLast"));
                break;
            case Poppler::LinkAction::HistoryBack:
                hyperlinkElement.setAttribute(QStringLiteral("action"), QStringLiteral("HistoryBack"));
                break;
            case Poppler::LinkAction::HistoryForward:
                hyperlinkElement.setAttribute(QStringLiteral("action"), QStringLiteral("HistoryForward"));
                break;
            case Poppler::LinkAction::Quit:
                hyperlinkElement.setAttribute(QStringLiteral("action"), QStringLiteral("Quit"));
                break;
            case Poppler::LinkAction::Presentation:
                hyperlinkElement.setAttribute(QStringLiteral("action"), QStringLiteral("Presentation"));
                break;
            case Poppler::LinkAction::EndPresentation:
                hyperlinkElement.setAttribute(QStringLiteral("action"), QStringLiteral("EndPresentation"));
                break;
            case Poppler::LinkAction::Find:
                hyperlinkElement.setAttribute(QStringLiteral("action"), QStringLiteral("Find"));
                break;
            case Poppler::LinkAction::GoToPage:
                hyperlinkElement.setAttribute(QStringLiteral("action"), QStringLiteral("GoToPage"));
                break;
            case Poppler::LinkAction::Close:
                hyperlinkElement.setAttribute(QStringLiteral("action"), QStringLiteral("Close"));
                break;
            case Poppler::LinkAction::Print:
                hyperlinkElement.setAttribute(QStringLiteral("action"), QStringLiteral("Print"));
                break;
            case Poppler::LinkAction::SaveAs:
                hyperlinkElement.setAttribute(QStringLiteral("action"), QStringLiteral("SaveAs"));
                break;
            }
            break;
        }
        case Poppler::Link::Movie: {
            hyperlinkElement.setAttribute(QStringLiteral("type"), QStringLiteral("Movie"));
            break;
        }
        case Poppler::Link::Rendition: {
            hyperlinkElement.setAttribute(QStringLiteral("type"), QStringLiteral("Rendition"));
            break;
        }
        case Poppler::Link::Sound: {
            hyperlinkElement.setAttribute(QStringLiteral("type"), QStringLiteral("Sound"));
            break;
        }
        case Poppler::Link::JavaScript: {
            hyperlinkElement.setAttribute(QStringLiteral("type"), QStringLiteral("JavaScript"));
            break;
        }
        case Poppler::Link::OCGState: {
            hyperlinkElement.setAttribute(QStringLiteral("type"), QStringLiteral("OCGState"));
            break;
        }
        case Poppler::Link::Hide: {
            hyperlinkElement.setAttribute(QStringLiteral("type"), QStringLiteral("Hide"));
            break;
        }
        case Poppler::Link::ResetForm: {
            hyperlinkElement.setAttribute(QStringLiteral("type"), QStringLiteral("ResetForm"));
            break;
        }
        case Poppler::Link::SubmitForm: {
            hyperlinkElement.setAttribute(QStringLiteral("type"), QStringLiteral("SubmitForm"));
        }
        case Poppler::Link::None:
            break;
        }
    }
}

Annotation::SubType LinkAnnotation::subType() const
{
    return ALink;
}

Link *LinkAnnotation::linkDestination() const
{
    Q_D(const LinkAnnotation);
    return d->linkDestination;
}

void LinkAnnotation::setLinkDestination(Link *link)
{
    Q_D(LinkAnnotation);
    delete d->linkDestination;
    d->linkDestination = link;
}

LinkAnnotation::HighlightMode LinkAnnotation::linkHighlightMode() const
{
    Q_D(const LinkAnnotation);
    return d->linkHLMode;
}

void LinkAnnotation::setLinkHighlightMode(LinkAnnotation::HighlightMode mode)
{
    Q_D(LinkAnnotation);
    d->linkHLMode = mode;
}

QPointF LinkAnnotation::linkRegionPoint(int id) const
{
    if (id < 0 || id >= 4) {
        return QPointF();
    }

    Q_D(const LinkAnnotation);
    return d->linkRegion[id];
}

void LinkAnnotation::setLinkRegionPoint(int id, const QPointF &point) // clazy:exclude=function-args-by-value
{
    if (id < 0 || id >= 4) {
        return;
    }

    Q_D(LinkAnnotation);
    d->linkRegion[id] = point;
}

/** CaretAnnotation [Annotation] */
class CaretAnnotationPrivate : public AnnotationPrivate
{
public:
    CaretAnnotationPrivate();
    std::unique_ptr<Annotation> makeAlias() override;
    std::shared_ptr<Annot> createNativeAnnot(::Page *destPage, DocumentData *doc) override;

    // data fields
    CaretAnnotation::CaretSymbol symbol;
};

static QString caretSymbolToString(CaretAnnotation::CaretSymbol symbol)
{
    switch (symbol) {
    case CaretAnnotation::None:
        return QStringLiteral("None");
    case CaretAnnotation::P:
        return QStringLiteral("P");
    }
    return QString();
}

static CaretAnnotation::CaretSymbol caretSymbolFromString(const QString &symbol)
{
    if (symbol == QLatin1String("None")) {
        return CaretAnnotation::None;
    } else if (symbol == QLatin1String("P")) {
        return CaretAnnotation::P;
    }
    return CaretAnnotation::None;
}

CaretAnnotationPrivate::CaretAnnotationPrivate() : symbol(CaretAnnotation::None) { }

std::unique_ptr<Annotation> CaretAnnotationPrivate::makeAlias()
{
    return std::unique_ptr<CaretAnnotation>(new CaretAnnotation(*this));
}

std::shared_ptr<Annot> CaretAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    // Setters are defined in the public class
    std::unique_ptr<CaretAnnotation> q = static_pointer_cast<CaretAnnotation>(makeAlias());

    // Set page and document
    pdfPage = destPage;
    parentDoc = doc;

    // Set pdfAnnot
    PDFRectangle rect = boundaryToPdfRectangle(boundary, flags);
    pdfAnnot = std::make_shared<AnnotCaret>(destPage->getDoc(), &rect);

    // Set properties
    flushBaseAnnotationProperties();
    q->setCaretSymbol(symbol);

    return pdfAnnot;
}

CaretAnnotation::CaretAnnotation() : Annotation(*new CaretAnnotationPrivate()) { }

CaretAnnotation::CaretAnnotation(CaretAnnotationPrivate &dd) : Annotation(dd) { }

CaretAnnotation::CaretAnnotation(const QDomNode &node) : Annotation(*new CaretAnnotationPrivate(), node)
{
    // loop through the whole children looking for a 'caret' element
    QDomNode subNode = node.firstChild();
    while (subNode.isElement()) {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if (e.tagName() != QLatin1String("caret")) {
            continue;
        }

        // parse the attributes
        if (e.hasAttribute(QStringLiteral("symbol"))) {
            setCaretSymbol(caretSymbolFromString(e.attribute(QStringLiteral("symbol"))));
        }

        // loading complete
        break;
    }
}

CaretAnnotation::~CaretAnnotation() = default;

void CaretAnnotation::store(QDomNode &node, QDomDocument &document) const
{
    // store base annotation properties
    storeBaseAnnotationProperties(node, document);

    // create [caret] element
    QDomElement caretElement = document.createElement(QStringLiteral("caret"));
    node.appendChild(caretElement);

    // append the optional attributes
    if (caretSymbol() != CaretAnnotation::None) {
        caretElement.setAttribute(QStringLiteral("symbol"), caretSymbolToString(caretSymbol()));
    }
}

Annotation::SubType CaretAnnotation::subType() const
{
    return ACaret;
}

CaretAnnotation::CaretSymbol CaretAnnotation::caretSymbol() const
{
    Q_D(const CaretAnnotation);

    if (!d->pdfAnnot) {
        return d->symbol;
    }

    const AnnotCaret *caretann = static_cast<const AnnotCaret *>(d->pdfAnnot.get());
    return (CaretAnnotation::CaretSymbol)caretann->getSymbol();
}

void CaretAnnotation::setCaretSymbol(CaretAnnotation::CaretSymbol symbol)
{
    Q_D(CaretAnnotation);

    if (!d->pdfAnnot) {
        d->symbol = symbol;
        return;
    }

    AnnotCaret *caretann = static_cast<AnnotCaret *>(d->pdfAnnot.get());
    caretann->setSymbol((AnnotCaret::AnnotCaretSymbol)symbol);
}

/** FileAttachmentAnnotation [Annotation] */
class FileAttachmentAnnotationPrivate : public AnnotationPrivate
{
public:
    FileAttachmentAnnotationPrivate();
    ~FileAttachmentAnnotationPrivate() override;
    std::unique_ptr<Annotation> makeAlias() override;
    std::shared_ptr<Annot> createNativeAnnot(::Page *destPage, DocumentData *doc) override;

    // data fields
    QString icon;
    EmbeddedFile *embfile;
};

FileAttachmentAnnotationPrivate::FileAttachmentAnnotationPrivate() : icon(QStringLiteral("PushPin")), embfile(nullptr) { }

FileAttachmentAnnotationPrivate::~FileAttachmentAnnotationPrivate()
{
    delete embfile;
}

std::unique_ptr<Annotation> FileAttachmentAnnotationPrivate::makeAlias()
{
    return std::unique_ptr<FileAttachmentAnnotation>(new FileAttachmentAnnotation(*this));
}

std::shared_ptr<Annot> FileAttachmentAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    return nullptr; // Not implemented
}

FileAttachmentAnnotation::FileAttachmentAnnotation() : Annotation(*new FileAttachmentAnnotationPrivate()) { }

FileAttachmentAnnotation::FileAttachmentAnnotation(FileAttachmentAnnotationPrivate &dd) : Annotation(dd) { }

FileAttachmentAnnotation::FileAttachmentAnnotation(const QDomNode &node) : Annotation(*new FileAttachmentAnnotationPrivate(), node)
{
    // loop through the whole children looking for a 'fileattachment' element
    QDomNode subNode = node.firstChild();
    while (subNode.isElement()) {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if (e.tagName() != QLatin1String("fileattachment")) {
            continue;
        }

        // loading complete
        break;
    }
}

FileAttachmentAnnotation::~FileAttachmentAnnotation() = default;

void FileAttachmentAnnotation::store(QDomNode &node, QDomDocument &document) const
{
    // store base annotation properties
    storeBaseAnnotationProperties(node, document);

    // create [fileattachment] element
    QDomElement fileAttachmentElement = document.createElement(QStringLiteral("fileattachment"));
    node.appendChild(fileAttachmentElement);
}

Annotation::SubType FileAttachmentAnnotation::subType() const
{
    return AFileAttachment;
}

QString FileAttachmentAnnotation::fileIconName() const
{
    Q_D(const FileAttachmentAnnotation);
    return d->icon;
}

void FileAttachmentAnnotation::setFileIconName(const QString &icon)
{
    Q_D(FileAttachmentAnnotation);
    d->icon = icon;
}

EmbeddedFile *FileAttachmentAnnotation::embeddedFile() const
{
    Q_D(const FileAttachmentAnnotation);
    return d->embfile;
}

void FileAttachmentAnnotation::setEmbeddedFile(EmbeddedFile *ef)
{
    Q_D(FileAttachmentAnnotation);
    d->embfile = ef;
}

/** SoundAnnotation [Annotation] */
class SoundAnnotationPrivate : public AnnotationPrivate
{
public:
    SoundAnnotationPrivate();
    ~SoundAnnotationPrivate() override;
    std::unique_ptr<Annotation> makeAlias() override;
    std::shared_ptr<Annot> createNativeAnnot(::Page *destPage, DocumentData *doc) override;

    // data fields
    QString icon;
    SoundObject *sound;
};

SoundAnnotationPrivate::SoundAnnotationPrivate() : icon(QStringLiteral("Speaker")), sound(nullptr) { }

SoundAnnotationPrivate::~SoundAnnotationPrivate()
{
    delete sound;
}

std::unique_ptr<Annotation> SoundAnnotationPrivate::makeAlias()
{
    return std::unique_ptr<SoundAnnotation>(new SoundAnnotation(*this));
}

std::shared_ptr<Annot> SoundAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    return nullptr; // Not implemented
}

SoundAnnotation::SoundAnnotation() : Annotation(*new SoundAnnotationPrivate()) { }

SoundAnnotation::SoundAnnotation(SoundAnnotationPrivate &dd) : Annotation(dd) { }

SoundAnnotation::SoundAnnotation(const QDomNode &node) : Annotation(*new SoundAnnotationPrivate(), node)
{
    // loop through the whole children looking for a 'sound' element
    QDomNode subNode = node.firstChild();
    while (subNode.isElement()) {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if (e.tagName() != QLatin1String("sound")) {
            continue;
        }

        // loading complete
        break;
    }
}

SoundAnnotation::~SoundAnnotation() = default;

void SoundAnnotation::store(QDomNode &node, QDomDocument &document) const
{
    // store base annotation properties
    storeBaseAnnotationProperties(node, document);

    // create [sound] element
    QDomElement soundElement = document.createElement(QStringLiteral("sound"));
    node.appendChild(soundElement);
}

Annotation::SubType SoundAnnotation::subType() const
{
    return ASound;
}

QString SoundAnnotation::soundIconName() const
{
    Q_D(const SoundAnnotation);
    return d->icon;
}

void SoundAnnotation::setSoundIconName(const QString &icon)
{
    Q_D(SoundAnnotation);
    d->icon = icon;
}

SoundObject *SoundAnnotation::sound() const
{
    Q_D(const SoundAnnotation);
    return d->sound;
}

void SoundAnnotation::setSound(SoundObject *s)
{
    Q_D(SoundAnnotation);
    d->sound = s;
}

/** MovieAnnotation [Annotation] */
class MovieAnnotationPrivate : public AnnotationPrivate
{
public:
    MovieAnnotationPrivate();
    ~MovieAnnotationPrivate() override;
    std::unique_ptr<Annotation> makeAlias() override;
    std::shared_ptr<Annot> createNativeAnnot(::Page *destPage, DocumentData *doc) override;

    // data fields
    MovieObject *movie;
    QString title;
};

MovieAnnotationPrivate::MovieAnnotationPrivate() : movie(nullptr) { }

MovieAnnotationPrivate::~MovieAnnotationPrivate()
{
    delete movie;
}

std::unique_ptr<Annotation> MovieAnnotationPrivate::makeAlias()
{
    return std::unique_ptr<Annotation>(new MovieAnnotation(*this));
}

std::shared_ptr<Annot> MovieAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    return nullptr; // Not implemented
}

MovieAnnotation::MovieAnnotation() : Annotation(*new MovieAnnotationPrivate()) { }

MovieAnnotation::MovieAnnotation(MovieAnnotationPrivate &dd) : Annotation(dd) { }

MovieAnnotation::MovieAnnotation(const QDomNode &node) : Annotation(*new MovieAnnotationPrivate(), node)
{
    // loop through the whole children looking for a 'movie' element
    QDomNode subNode = node.firstChild();
    while (subNode.isElement()) {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if (e.tagName() != QLatin1String("movie")) {
            continue;
        }

        // loading complete
        break;
    }
}

MovieAnnotation::~MovieAnnotation() = default;

void MovieAnnotation::store(QDomNode &node, QDomDocument &document) const
{
    // store base annotation properties
    storeBaseAnnotationProperties(node, document);

    // create [movie] element
    QDomElement movieElement = document.createElement(QStringLiteral("movie"));
    node.appendChild(movieElement);
}

Annotation::SubType MovieAnnotation::subType() const
{
    return AMovie;
}

MovieObject *MovieAnnotation::movie() const
{
    Q_D(const MovieAnnotation);
    return d->movie;
}

void MovieAnnotation::setMovie(MovieObject *movie)
{
    Q_D(MovieAnnotation);
    d->movie = movie;
}

QString MovieAnnotation::movieTitle() const
{
    Q_D(const MovieAnnotation);
    return d->title;
}

void MovieAnnotation::setMovieTitle(const QString &title)
{
    Q_D(MovieAnnotation);
    d->title = title;
}

/** ScreenAnnotation [Annotation] */
class ScreenAnnotationPrivate : public AnnotationPrivate
{
public:
    ScreenAnnotationPrivate();
    ~ScreenAnnotationPrivate() override;
    std::unique_ptr<Annotation> makeAlias() override;
    std::shared_ptr<Annot> createNativeAnnot(::Page *destPage, DocumentData *doc) override;

    // data fields
    LinkRendition *action;
    QString title;
};

ScreenAnnotationPrivate::ScreenAnnotationPrivate() : action(nullptr) { }

ScreenAnnotationPrivate::~ScreenAnnotationPrivate()
{
    delete action;
}

ScreenAnnotation::ScreenAnnotation(ScreenAnnotationPrivate &dd) : Annotation(dd) { }

std::unique_ptr<Annotation> ScreenAnnotationPrivate::makeAlias()
{
    return std::unique_ptr<ScreenAnnotation>(new ScreenAnnotation(*this));
}

std::shared_ptr<Annot> ScreenAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    return nullptr; // Not implemented
}

ScreenAnnotation::ScreenAnnotation() : Annotation(*new ScreenAnnotationPrivate()) { }

ScreenAnnotation::~ScreenAnnotation() = default;

void ScreenAnnotation::store(QDomNode &node, QDomDocument &document) const
{
    // store base annotation properties
    storeBaseAnnotationProperties(node, document);

    // create [screen] element
    QDomElement screenElement = document.createElement(QStringLiteral("screen"));
    node.appendChild(screenElement);
}

Annotation::SubType ScreenAnnotation::subType() const
{
    return AScreen;
}

LinkRendition *ScreenAnnotation::action() const
{
    Q_D(const ScreenAnnotation);
    return d->action;
}

void ScreenAnnotation::setAction(LinkRendition *action)
{
    Q_D(ScreenAnnotation);
    d->action = action;
}

QString ScreenAnnotation::screenTitle() const
{
    Q_D(const ScreenAnnotation);
    return d->title;
}

void ScreenAnnotation::setScreenTitle(const QString &title)
{
    Q_D(ScreenAnnotation);
    d->title = title;
}

Link *ScreenAnnotation::additionalAction(AdditionalActionType type) const
{
    Q_D(const ScreenAnnotation);
    return d->additionalAction(type);
}

/** WidgetAnnotation [Annotation] */
class WidgetAnnotationPrivate : public AnnotationPrivate
{
public:
    std::unique_ptr<Annotation> makeAlias() override;
    std::shared_ptr<Annot> createNativeAnnot(::Page *destPage, DocumentData *doc) override;
};

std::unique_ptr<Annotation> WidgetAnnotationPrivate::makeAlias()
{
    return std::unique_ptr<WidgetAnnotation>(new WidgetAnnotation(*this));
}

std::shared_ptr<Annot> WidgetAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    return nullptr; // Not implemented
}

WidgetAnnotation::WidgetAnnotation(WidgetAnnotationPrivate &dd) : Annotation(dd) { }

WidgetAnnotation::WidgetAnnotation() : Annotation(*new WidgetAnnotationPrivate()) { }

WidgetAnnotation::~WidgetAnnotation() = default;

void WidgetAnnotation::store(QDomNode &node, QDomDocument &document) const
{
    // store base annotation properties
    storeBaseAnnotationProperties(node, document);

    // create [widget] element
    QDomElement widgetElement = document.createElement(QStringLiteral("widget"));
    node.appendChild(widgetElement);
}

Annotation::SubType WidgetAnnotation::subType() const
{
    return AWidget;
}

Link *WidgetAnnotation::additionalAction(AdditionalActionType type) const
{
    Q_D(const WidgetAnnotation);
    return d->additionalAction(type);
}

/** RichMediaAnnotation [Annotation] */
class RichMediaAnnotation::Params::Private
{
public:
    Private() = default;

    QString flashVars;
};

RichMediaAnnotation::Params::Params() : d(new Private) { }

RichMediaAnnotation::Params::~Params() = default;

void RichMediaAnnotation::Params::setFlashVars(const QString &flashVars)
{
    d->flashVars = flashVars;
}

QString RichMediaAnnotation::Params::flashVars() const
{
    return d->flashVars;
}

class RichMediaAnnotation::Instance::Private
{
public:
    Private() : params(nullptr) { }

    ~Private() { delete params; }

    Private(const Private &) = delete;
    Private &operator=(const Private &) = delete;

    RichMediaAnnotation::Instance::Type type;
    RichMediaAnnotation::Params *params;
};

RichMediaAnnotation::Instance::Instance() : d(new Private) { }

RichMediaAnnotation::Instance::~Instance() = default;

void RichMediaAnnotation::Instance::setType(Type type)
{
    d->type = type;
}

RichMediaAnnotation::Instance::Type RichMediaAnnotation::Instance::type() const
{
    return d->type;
}

void RichMediaAnnotation::Instance::setParams(RichMediaAnnotation::Params *params)
{
    delete d->params;
    d->params = params;
}

RichMediaAnnotation::Params *RichMediaAnnotation::Instance::params() const
{
    return d->params;
}

class RichMediaAnnotation::Configuration::Private
{
public:
    Private() = default;
    ~Private()
    {
        qDeleteAll(instances);
        instances.clear();
    }

    Private(const Private &) = delete;
    Private &operator=(const Private &) = delete;

    RichMediaAnnotation::Configuration::Type type;
    QString name;
    QList<RichMediaAnnotation::Instance *> instances;
};

RichMediaAnnotation::Configuration::Configuration() : d(new Private) { }

RichMediaAnnotation::Configuration::~Configuration() = default;

void RichMediaAnnotation::Configuration::setType(Type type)
{
    d->type = type;
}

RichMediaAnnotation::Configuration::Type RichMediaAnnotation::Configuration::type() const
{
    return d->type;
}

void RichMediaAnnotation::Configuration::setName(const QString &name)
{
    d->name = name;
}

QString RichMediaAnnotation::Configuration::name() const
{
    return d->name;
}

void RichMediaAnnotation::Configuration::setInstances(const QList<RichMediaAnnotation::Instance *> &instances)
{
    qDeleteAll(d->instances);
    d->instances.clear();

    d->instances = instances;
}

QList<RichMediaAnnotation::Instance *> RichMediaAnnotation::Configuration::instances() const
{
    return d->instances;
}

class RichMediaAnnotation::Asset::Private
{
public:
    Private() : embeddedFile(nullptr) { }

    ~Private() { delete embeddedFile; }

    Private(const Private &) = delete;
    Private &operator=(const Private &) = delete;

    QString name;
    EmbeddedFile *embeddedFile;
};

RichMediaAnnotation::Asset::Asset() : d(new Private) { }

RichMediaAnnotation::Asset::~Asset() = default;

void RichMediaAnnotation::Asset::setName(const QString &name)
{
    d->name = name;
}

QString RichMediaAnnotation::Asset::name() const
{
    return d->name;
}

void RichMediaAnnotation::Asset::setEmbeddedFile(EmbeddedFile *embeddedFile)
{
    delete d->embeddedFile;
    d->embeddedFile = embeddedFile;
}

EmbeddedFile *RichMediaAnnotation::Asset::embeddedFile() const
{
    return d->embeddedFile;
}

class RichMediaAnnotation::Content::Private
{
public:
    Private() = default;
    ~Private()
    {
        qDeleteAll(configurations);
        configurations.clear();

        qDeleteAll(assets);
        assets.clear();
    }

    Private(const Private &) = delete;
    Private &operator=(const Private &) = delete;

    QList<RichMediaAnnotation::Configuration *> configurations;
    QList<RichMediaAnnotation::Asset *> assets;
};

RichMediaAnnotation::Content::Content() : d(new Private) { }

RichMediaAnnotation::Content::~Content() = default;

void RichMediaAnnotation::Content::setConfigurations(const QList<RichMediaAnnotation::Configuration *> &configurations)
{
    qDeleteAll(d->configurations);
    d->configurations.clear();

    d->configurations = configurations;
}

QList<RichMediaAnnotation::Configuration *> RichMediaAnnotation::Content::configurations() const
{
    return d->configurations;
}

void RichMediaAnnotation::Content::setAssets(const QList<RichMediaAnnotation::Asset *> &assets)
{
    qDeleteAll(d->assets);
    d->assets.clear();

    d->assets = assets;
}

QList<RichMediaAnnotation::Asset *> RichMediaAnnotation::Content::assets() const
{
    return d->assets;
}

class RichMediaAnnotation::Activation::Private
{
public:
    Private() : condition(RichMediaAnnotation::Activation::UserAction) { }

    RichMediaAnnotation::Activation::Condition condition;
};

RichMediaAnnotation::Activation::Activation() : d(new Private) { }

RichMediaAnnotation::Activation::~Activation() = default;

void RichMediaAnnotation::Activation::setCondition(Condition condition)
{
    d->condition = condition;
}

RichMediaAnnotation::Activation::Condition RichMediaAnnotation::Activation::condition() const
{
    return d->condition;
}

class RichMediaAnnotation::Deactivation::Private : public QSharedData
{
public:
    Private() : condition(RichMediaAnnotation::Deactivation::UserAction) { }

    RichMediaAnnotation::Deactivation::Condition condition;
};

RichMediaAnnotation::Deactivation::Deactivation() : d(new Private) { }

RichMediaAnnotation::Deactivation::~Deactivation() = default;

void RichMediaAnnotation::Deactivation::setCondition(Condition condition)
{
    d->condition = condition;
}

RichMediaAnnotation::Deactivation::Condition RichMediaAnnotation::Deactivation::condition() const
{
    return d->condition;
}

class RichMediaAnnotation::Settings::Private : public QSharedData
{
public:
    Private() : activation(nullptr), deactivation(nullptr) { }

    RichMediaAnnotation::Activation *activation;
    RichMediaAnnotation::Deactivation *deactivation;
};

RichMediaAnnotation::Settings::Settings() : d(new Private) { }

RichMediaAnnotation::Settings::~Settings() = default;

void RichMediaAnnotation::Settings::setActivation(RichMediaAnnotation::Activation *activation)
{
    delete d->activation;
    d->activation = activation;
}

RichMediaAnnotation::Activation *RichMediaAnnotation::Settings::activation() const
{
    return d->activation;
}

void RichMediaAnnotation::Settings::setDeactivation(RichMediaAnnotation::Deactivation *deactivation)
{
    delete d->deactivation;
    d->deactivation = deactivation;
}

RichMediaAnnotation::Deactivation *RichMediaAnnotation::Settings::deactivation() const
{
    return d->deactivation;
}

class RichMediaAnnotationPrivate : public AnnotationPrivate
{
public:
    RichMediaAnnotationPrivate() : settings(nullptr), content(nullptr) { }

    ~RichMediaAnnotationPrivate() override;

    std::unique_ptr<Annotation> makeAlias() override { return std::unique_ptr<RichMediaAnnotation>(new RichMediaAnnotation(*this)); }

    std::shared_ptr<Annot> createNativeAnnot(::Page *destPage, DocumentData *doc) override
    {
        Q_UNUSED(destPage);
        Q_UNUSED(doc);

        return nullptr;
    }

    RichMediaAnnotation::Settings *settings;
    RichMediaAnnotation::Content *content;
};

RichMediaAnnotationPrivate::~RichMediaAnnotationPrivate()
{
    delete settings;
    delete content;
}

RichMediaAnnotation::RichMediaAnnotation() : Annotation(*new RichMediaAnnotationPrivate()) { }

RichMediaAnnotation::RichMediaAnnotation(RichMediaAnnotationPrivate &dd) : Annotation(dd) { }

RichMediaAnnotation::RichMediaAnnotation(const QDomNode &node) : Annotation(*new RichMediaAnnotationPrivate(), node)
{
    // loop through the whole children looking for a 'richMedia' element
    QDomNode subNode = node.firstChild();
    while (subNode.isElement()) {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if (e.tagName() != QLatin1String("richMedia")) {
            continue;
        }

        // loading complete
        break;
    }
}

RichMediaAnnotation::~RichMediaAnnotation() = default;

void RichMediaAnnotation::store(QDomNode &node, QDomDocument &document) const
{
    // store base annotation properties
    storeBaseAnnotationProperties(node, document);

    // create [richMedia] element
    QDomElement richMediaElement = document.createElement(QStringLiteral("richMedia"));
    node.appendChild(richMediaElement);
}

Annotation::SubType RichMediaAnnotation::subType() const
{
    return ARichMedia;
}

void RichMediaAnnotation::setSettings(RichMediaAnnotation::Settings *settings)
{
    Q_D(RichMediaAnnotation);

    delete d->settings;
    d->settings = settings;
}

RichMediaAnnotation::Settings *RichMediaAnnotation::settings() const
{
    Q_D(const RichMediaAnnotation);

    return d->settings;
}

void RichMediaAnnotation::setContent(RichMediaAnnotation::Content *content)
{
    Q_D(RichMediaAnnotation);

    delete d->content;
    d->content = content;
}

RichMediaAnnotation::Content *RichMediaAnnotation::content() const
{
    Q_D(const RichMediaAnnotation);

    return d->content;
}

// BEGIN utility annotation functions
QColor convertAnnotColor(const AnnotColor *color)
{
    if (!color) {
        return QColor();
    }

    QColor newcolor;
    const double *color_data = color->getValues();
    switch (color->getSpace()) {
    case AnnotColor::colorTransparent: // = 0,
        newcolor = Qt::transparent;
        break;
    case AnnotColor::colorGray: // = 1,
        newcolor.setRgbF(color_data[0], color_data[0], color_data[0]);
        break;
    case AnnotColor::colorRGB: // = 3,
        newcolor.setRgbF(color_data[0], color_data[1], color_data[2]);
        break;
    case AnnotColor::colorCMYK: // = 4
        newcolor.setCmykF(color_data[0], color_data[1], color_data[2], color_data[3]);
        break;
    }
    return newcolor;
}

std::unique_ptr<AnnotColor> convertQColor(const QColor &c)
{
    if (c.alpha() == 0) {
        return {}; // Transparent
    }

    switch (c.spec()) {
    case QColor::Rgb:
    case QColor::Hsl:
    case QColor::Hsv:
        return std::make_unique<AnnotColor>(c.redF(), c.greenF(), c.blueF());
    case QColor::Cmyk:
        return std::make_unique<AnnotColor>(c.cyanF(), c.magentaF(), c.yellowF(), c.blackF());
    case QColor::Invalid:
    default:
        return {};
    }
}
// END utility annotation functions

}
