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
 * Copyright (C) 2020-2022 Oliver Sander <oliver.sander@tu-dresden.de>
 * Copyright (C) 2020 Katarina Behrens <Katarina.Behrens@cib.de>
 * Copyright (C) 2020 Thorsten Behrens <Thorsten.Behrens@CIB.de>
 * Copyright (C) 2020, 2024 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by Technische Universität Dresden
 * Copyright (C) 2021 Mahmoud Ahmed Khalil <mahmoudkhalil11@gmail.com>
 * Copyright (C) 2024 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
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
#include <QtCore/QtAlgorithms>
#include <QtGui/QColor>
#include <QtGui/QTransform>
#include <QImage>

// local includes
#include "poppler-annotation.h"
#include "poppler-link.h"
#include "poppler-qt6.h"
#include "poppler-annotation-helper.h"
#include "poppler-annotation-private.h"
#include "poppler-page-private.h"
#include "poppler-private.h"
#include "poppler-form.h"

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

namespace Poppler {

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
AnnotationPrivate::AnnotationPrivate() : revisionScope(Annotation::Root), revisionType(Annotation::None), pdfAnnot(nullptr), pdfPage(nullptr), parentDoc(nullptr) { }

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
    revisions.append(ann->d_ptr->makeAlias());

    /* Set revision properties */
    revisionScope = scope;
    revisionType = type;
}

AnnotationPrivate::~AnnotationPrivate()
{
    // Delete all children revisions
    qDeleteAll(revisions);

    // Release Annot object
    if (pdfAnnot) {
        pdfAnnot->decRefCnt();
    }
}

void AnnotationPrivate::tieToNativeAnnot(Annot *ann, ::Page *page, Poppler::DocumentData *doc)
{
    if (pdfAnnot) {
        error(errIO, -1, "Annotation is already tied");
        return;
    }

    pdfAnnot = ann;
    pdfPage = page;
    parentDoc = doc;

    pdfAnnot->incRefCnt();
}

/* This method is called when a new annotation is created, after pdfAnnot and
 * pdfPage have been set */
void AnnotationPrivate::flushBaseAnnotationProperties()
{
    Q_ASSERT(pdfPage);

    Annotation *q = makeAlias(); // Setters are defined in the public class

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
    foreach (Annotation *r, revisions) {
        // TODO: Flush revision
        delete r; // Object is no longer needed
    }

    delete q;

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

std::unique_ptr<AnnotPath> AnnotationPrivate::toAnnotPath(const QVector<QPointF> &list) const
{
    const int count = list.size();
    std::vector<AnnotCoord> ac;
    ac.reserve(count);

    double MTX[6];
    fillTransformationMTX(MTX);

    foreach (const QPointF &p, list) {
        double x, y;
        XPDFReader::invTransform(MTX, p, x, y);
        ac.emplace_back(x, y);
    }

    return std::make_unique<AnnotPath>(std::move(ac));
}

std::vector<std::unique_ptr<Annotation>> AnnotationPrivate::findAnnotations(::Page *pdfPage, DocumentData *doc, const QSet<Annotation::SubType> &subtypes, int parentID)
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
    std::vector<std::unique_ptr<Annotation>> res;
    for (Annot *ann : annots->getAnnots()) {
        if (!ann) {
            error(errInternal, -1, "Annot is null");
            continue;
        }

        // Check parent annotation
        AnnotMarkup *markupann = dynamic_cast<AnnotMarkup *>(ann);
        if (!markupann) {
            // Assume it's a root annotation, and skip if user didn't request it
            if (parentID != -1) {
                continue;
            }
        } else if (markupann->getInReplyToID() != parentID) {
            continue;
        }

        /* Create Annotation of the right subclass */
        std::unique_ptr<Annotation> annotation;
        Annot::AnnotSubtype subType = ann->getType();

        switch (subType) {
        case Annot::typeText:
            if (!wantTextAnnotations) {
                continue;
            }
            annotation = std::make_unique<TextAnnotation>(TextAnnotation::Linked);
            break;
        case Annot::typeFreeText:
            if (!wantTextAnnotations) {
                continue;
            }
            annotation = std::make_unique<TextAnnotation>(TextAnnotation::InPlace);
            break;
        case Annot::typeLine:
            if (!wantLineAnnotations) {
                continue;
            }
            annotation = std::make_unique<LineAnnotation>(LineAnnotation::StraightLine);
            break;
        case Annot::typePolygon:
        case Annot::typePolyLine:
            if (!wantLineAnnotations) {
                continue;
            }
            annotation = std::make_unique<LineAnnotation>(LineAnnotation::Polyline);
            break;
        case Annot::typeSquare:
        case Annot::typeCircle:
            if (!wantGeomAnnotations) {
                continue;
            }
            annotation = std::make_unique<GeomAnnotation>();
            break;
        case Annot::typeHighlight:
        case Annot::typeUnderline:
        case Annot::typeSquiggly:
        case Annot::typeStrikeOut:
            if (!wantHighlightAnnotations) {
                continue;
            }
            annotation = std::make_unique<HighlightAnnotation>();
            break;
        case Annot::typeStamp:
            if (!wantStampAnnotations) {
                continue;
            }
            annotation = std::make_unique<StampAnnotation>();
            break;
        case Annot::typeInk:
            if (!wantInkAnnotations) {
                continue;
            }
            annotation = std::make_unique<InkAnnotation>();
            break;
        case Annot::typeLink: /* TODO: Move logic to getters */
        {
            if (!wantLinkAnnotations) {
                continue;
            }
            // parse Link params
            AnnotLink *linkann = static_cast<AnnotLink *>(ann);
            LinkAnnotation *l = new LinkAnnotation();

            // -> hlMode
            l->setLinkHighlightMode((LinkAnnotation::HighlightMode)linkann->getLinkEffect());

            // -> link region
            // TODO

            // reading link action
            if (linkann->getAction()) {
                std::unique_ptr<Link> popplerLink = PageData::convertLinkActionToLink(linkann->getAction(), doc, QRectF());
                if (popplerLink) {
                    l->setLinkDestination(std::move(popplerLink));
                }
            }
            annotation.reset(l);
            break;
        }
        case Annot::typeCaret:
            if (!wantCaretAnnotations) {
                continue;
            }
            annotation = std::make_unique<CaretAnnotation>();
            break;
        case Annot::typeFileAttachment: /* TODO: Move logic to getters */
        {
            if (!wantFileAttachmentAnnotations) {
                continue;
            }
            AnnotFileAttachment *attachann = static_cast<AnnotFileAttachment *>(ann);
            FileAttachmentAnnotation *f = new FileAttachmentAnnotation();
            // -> fileIcon
            f->setFileIconName(QString::fromLatin1(attachann->getName()->c_str()));
            // -> embeddedFile
            auto filespec = std::make_unique<FileSpec>(attachann->getFile());
            f->setEmbeddedFile(new EmbeddedFile(*new EmbeddedFileData(std::move(filespec))));
            annotation.reset(f);
            break;
        }
        case Annot::typeSound: /* TODO: Move logic to getters */
        {
            if (!wantSoundAnnotations) {
                continue;
            }
            AnnotSound *soundann = static_cast<AnnotSound *>(ann);
            SoundAnnotation *s = new SoundAnnotation();

            // -> soundIcon
            s->setSoundIconName(QString::fromLatin1(soundann->getName()->c_str()));
            // -> sound
            s->setSound(new SoundObject(soundann->getSound()));
            annotation.reset(s);
            break;
        }
        case Annot::typeMovie: /* TODO: Move logic to getters */
        {
            if (!wantMovieAnnotations) {
                continue;
            }
            AnnotMovie *movieann = static_cast<AnnotMovie *>(ann);
            MovieAnnotation *m = new MovieAnnotation();

            // -> movie
            MovieObject *movie = new MovieObject(movieann);
            m->setMovie(movie);
            // -> movieTitle
            const GooString *movietitle = movieann->getTitle();
            if (movietitle) {
                m->setMovieTitle(QString::fromLatin1(movietitle->c_str()));
            }
            annotation.reset(m);
            break;
        }
        case Annot::typeScreen: {
            if (!wantScreenAnnotations) {
                continue;
            }
            AnnotScreen *screenann = static_cast<AnnotScreen *>(ann);
            // TODO Support other link types than Link::Rendition in ScreenAnnotation
            if (!screenann->getAction() || screenann->getAction()->getKind() != actionRendition) {
                continue;
            }
            ScreenAnnotation *s = new ScreenAnnotation();

            // -> screen
            std::unique_ptr<Link> popplerLink = PageData::convertLinkActionToLink(screenann->getAction(), doc, QRectF());
            s->setAction(static_cast<Poppler::LinkRendition *>(popplerLink.release()));

            // -> screenTitle
            const GooString *screentitle = screenann->getTitle();
            if (screentitle) {
                s->setScreenTitle(UnicodeParsedString(screentitle));
            }
            annotation.reset(s);
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
            annotation.reset(new WidgetAnnotation());
            break;
        case Annot::typeRichMedia: {
            const AnnotRichMedia *annotRichMedia = static_cast<AnnotRichMedia *>(ann);

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

            annotation.reset(richMediaAnnotation);

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
        res.push_back(std::move(annotation));
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

std::unique_ptr<Link> AnnotationPrivate::additionalAction(Annotation::AdditionalActionType type) const
{
    if (pdfAnnot->getType() != Annot::typeScreen && pdfAnnot->getType() != Annot::typeWidget) {
        return {};
    }

    const Annot::AdditionalActionsType actionType = toPopplerAdditionalActionType(type);

    std::unique_ptr<::LinkAction> linkAction;
    if (pdfAnnot->getType() == Annot::typeScreen) {
        linkAction = static_cast<AnnotScreen *>(pdfAnnot)->getAdditionalAction(actionType);
    } else {
        linkAction = static_cast<AnnotWidget *>(pdfAnnot)->getAdditionalAction(actionType);
    }

    if (linkAction) {
        return PageData::convertLinkActionToLink(linkAction.get(), parentDoc, QRectF());
    }

    return {};
}

void AnnotationPrivate::addAnnotationToPage(::Page *pdfPage, DocumentData *doc, const Annotation *ann)
{
    if (ann->d_ptr->pdfAnnot != nullptr) {
        error(errIO, -1, "Annotation is already tied");
        return;
    }

    // Unimplemented annotations can't be created by the user because their ctor
    // is private. Therefore, createNativeAnnot will never return 0
    Annot *nativeAnnot = ann->d_ptr->createNativeAnnot(pdfPage, doc);
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
    Annotation *makeAlias() override;
    Annot *createNativeAnnot(::Page *destPage, DocumentData *doc) override;
    void setDefaultAppearanceToNative();
    std::unique_ptr<DefaultAppearance> getDefaultAppearanceFromNative() const;

    // data fields
    TextAnnotation::TextType textType;
    QString textIcon;
    std::optional<QFont> textFont;
    QColor textColor = Qt::black;
    TextAnnotation::InplaceAlignPosition inplaceAlign;
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

Annotation::Style::Style(const Style &other) : d(other.d) { }

Annotation::Style &Annotation::Style::operator=(const Style &other)
{
    if (this != &other) {
        d = other.d;
    }

    return *this;
}

Annotation::Style::~Style() { }

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

Annotation::Popup::Popup(const Popup &other) : d(other.d) { }

Annotation::Popup &Annotation::Popup::operator=(const Popup &other)
{
    if (this != &other) {
        d = other.d;
    }

    return *this;
}

Annotation::Popup::~Popup() { }

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

Annotation::~Annotation() { }

QString Annotation::author() const
{
    Q_D(const Annotation);

    if (!d->pdfAnnot) {
        return d->author;
    }

    const AnnotMarkup *markupann = dynamic_cast<const AnnotMarkup *>(d->pdfAnnot);
    return markupann ? UnicodeParsedString(markupann->getLabel()) : QString();
}

void Annotation::setAuthor(const QString &author)
{
    Q_D(Annotation);

    if (!d->pdfAnnot) {
        d->author = author;
        return;
    }

    AnnotMarkup *markupann = dynamic_cast<AnnotMarkup *>(d->pdfAnnot);
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
            GooString *s = timeToDateString(&t);
            d->pdfAnnot->setModified(s);
            delete s;
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

    const AnnotMarkup *markupann = dynamic_cast<const AnnotMarkup *>(d->pdfAnnot);

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

    AnnotMarkup *markupann = dynamic_cast<AnnotMarkup *>(d->pdfAnnot);
    if (markupann) {
        if (date.isValid()) {
            const time_t t = date.toSecsSinceEpoch();
            GooString *s = timeToDateString(&t);
            markupann->setDate(s);
            delete s;
        } else {
            markupann->setDate(nullptr);
        }
    }
}

static Annotation::Flags fromPdfFlags(int flags)
{
    Annotation::Flags qtflags;

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
        qtflags |= Annotation::DenyWrite;
        qtflags |= Annotation::DenyDelete;
    }
    if (flags & Annot::flagLocked) {
        qtflags |= Annotation::DenyDelete;
    }
    if (flags & Annot::flagToggleNoView) {
        qtflags |= Annotation::ToggleHidingOnMouse;
    }

    return qtflags;
}

static int toPdfFlags(Annotation::Flags qtflags)
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

Annotation::Flags Annotation::flags() const
{
    Q_D(const Annotation);

    if (!d->pdfAnnot) {
        return d->flags;
    }

    return fromPdfFlags(d->pdfAnnot->getFlags());
}

void Annotation::setFlags(Annotation::Flags flags)
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

    const AnnotMarkup *markupann = dynamic_cast<const AnnotMarkup *>(d->pdfAnnot);
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
        border_effect = static_cast<AnnotFreeText *>(d->pdfAnnot)->getBorderEffect();
        break;
    case Annot::typeSquare:
    case Annot::typeCircle:
        border_effect = static_cast<AnnotGeometry *>(d->pdfAnnot)->getBorderEffect();
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

    AnnotMarkup *markupann = dynamic_cast<AnnotMarkup *>(d->pdfAnnot);
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

    const AnnotMarkup *markupann = dynamic_cast<const AnnotMarkup *>(d->pdfAnnot);
    if (markupann) {
        popup = markupann->getPopup();
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
        const AnnotText *textann = static_cast<const AnnotText *>(d->pdfAnnot);

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

    const AnnotMarkup *markupann = dynamic_cast<const AnnotMarkup *>(d->pdfAnnot);

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

    const AnnotText *textann = dynamic_cast<const AnnotText *>(d->pdfAnnot);

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

std::vector<std::unique_ptr<Annotation>> Annotation::revisions() const
{
    Q_D(const Annotation);

    if (!d->pdfAnnot) {
        /* Return aliases, whose ownership goes to the caller */
        std::vector<std::unique_ptr<Annotation>> res;
        foreach (Annotation *rev, d->revisions)
            res.push_back(std::unique_ptr<Annotation>(rev->d_ptr->makeAlias()));
        return res;
    }

    /* If the annotation doesn't live in a object on its own (eg bug51361), it
     * has no ref, therefore it can't have revisions */
    if (!d->pdfAnnot->getHasRef()) {
        return std::vector<std::unique_ptr<Annotation>>();
    }

    return AnnotationPrivate::findAnnotations(d->pdfPage, d->parentDoc, QSet<Annotation::SubType>(), d->pdfAnnot->getId());
}

std::unique_ptr<AnnotationAppearance> Annotation::annotationAppearance() const
{
    Q_D(const Annotation);

    return std::make_unique<AnnotationAppearance>(new AnnotationAppearancePrivate(d->pdfAnnot));
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
TextAnnotationPrivate::TextAnnotationPrivate() : AnnotationPrivate(), textType(TextAnnotation::Linked), textIcon(QStringLiteral("Note")), inplaceAlign(TextAnnotation::InplaceAlignLeft), inplaceIntent(TextAnnotation::Unknown) { }

Annotation *TextAnnotationPrivate::makeAlias()
{
    return new TextAnnotation(*this);
}

Annot *TextAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    // Setters are defined in the public class
    TextAnnotation *q = static_cast<TextAnnotation *>(makeAlias());

    // Set page and contents
    pdfPage = destPage;
    parentDoc = doc;

    // Set pdfAnnot
    PDFRectangle rect = boundaryToPdfRectangle(boundary, flags);
    if (textType == TextAnnotation::Linked) {
        pdfAnnot = new AnnotText { destPage->getDoc(), &rect };
    } else {
        const double pointSize = textFont ? textFont->pointSizeF() : AnnotFreeText::undefinedFontPtSize;
        if (pointSize < 0) {
            qWarning() << "TextAnnotationPrivate::createNativeAnnot: font pointSize < 0";
        }
        pdfAnnot = new AnnotFreeText { destPage->getDoc(), &rect };
    }

    // Set properties
    flushBaseAnnotationProperties();
    q->setTextIcon(textIcon);
    q->setInplaceAlign(inplaceAlign);
    q->setCalloutPoints(inplaceCallout);
    q->setInplaceIntent(inplaceIntent);

    delete q;

    inplaceCallout.clear(); // Free up memory

    setDefaultAppearanceToNative();

    return pdfAnnot;
}

void TextAnnotationPrivate::setDefaultAppearanceToNative()
{
    if (pdfAnnot && pdfAnnot->getType() == Annot::typeFreeText) {
        AnnotFreeText *ftextann = static_cast<AnnotFreeText *>(pdfAnnot);
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
        AnnotFreeText *ftextann = static_cast<AnnotFreeText *>(pdfAnnot);
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

TextAnnotation::~TextAnnotation() { }

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
        const AnnotText *textann = static_cast<const AnnotText *>(d->pdfAnnot);
        return QString::fromLatin1(textann->getIcon()->c_str());
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
        AnnotText *textann = static_cast<AnnotText *>(d->pdfAnnot);
        QByteArray encoded = icon.toLatin1();
        GooString s(encoded.constData());
        textann->setIcon(&s);
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

TextAnnotation::InplaceAlignPosition TextAnnotation::inplaceAlign() const
{
    Q_D(const TextAnnotation);

    if (!d->pdfAnnot) {
        return d->inplaceAlign;
    }

    if (d->pdfAnnot->getType() == Annot::typeFreeText) {
        const AnnotFreeText *ftextann = static_cast<const AnnotFreeText *>(d->pdfAnnot);
        switch (ftextann->getQuadding()) {
        case VariableTextQuadding::leftJustified:
            return InplaceAlignLeft;
        case VariableTextQuadding::centered:
            return InplaceAlignCenter;
        case VariableTextQuadding::rightJustified:
            return InplaceAlignRight;
        }
    }

    return InplaceAlignLeft;
}

static VariableTextQuadding alignToQuadding(TextAnnotation::InplaceAlignPosition align)
{
    switch (align) {
    case TextAnnotation::InplaceAlignLeft:
        return VariableTextQuadding::leftJustified;
    case TextAnnotation::InplaceAlignCenter:
        return VariableTextQuadding::centered;
    case TextAnnotation::InplaceAlignRight:
        return VariableTextQuadding::rightJustified;
    }
    return VariableTextQuadding::leftJustified;
}

void TextAnnotation::setInplaceAlign(InplaceAlignPosition align)
{
    Q_D(TextAnnotation);

    if (!d->pdfAnnot) {
        d->inplaceAlign = align;
        return;
    }

    if (d->pdfAnnot->getType() == Annot::typeFreeText) {
        AnnotFreeText *ftextann = static_cast<AnnotFreeText *>(d->pdfAnnot);
        ftextann->setQuadding(alignToQuadding(align));
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

    const AnnotFreeText *ftextann = static_cast<const AnnotFreeText *>(d->pdfAnnot);
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

    AnnotFreeText *ftextann = static_cast<AnnotFreeText *>(d->pdfAnnot);
    const int count = points.size();

    if (count == 0) {
        ftextann->setCalloutLine(nullptr);
        return;
    }

    if (count != 2 && count != 3) {
        error(errSyntaxError, -1, "Expected zero, two or three points for callout");
        return;
    }

    AnnotCalloutLine *callout;
    double x1, y1, x2, y2;
    double MTX[6];
    d->fillTransformationMTX(MTX);

    XPDFReader::invTransform(MTX, points[0], x1, y1);
    XPDFReader::invTransform(MTX, points[1], x2, y2);
    if (count == 3) {
        double x3, y3;
        XPDFReader::invTransform(MTX, points[2], x3, y3);
        callout = new AnnotCalloutMultiLine(x1, y1, x2, y2, x3, y3);
    } else {
        callout = new AnnotCalloutLine(x1, y1, x2, y2);
    }

    ftextann->setCalloutLine(callout);
    delete callout;
}

TextAnnotation::InplaceIntent TextAnnotation::inplaceIntent() const
{
    Q_D(const TextAnnotation);

    if (!d->pdfAnnot) {
        return d->inplaceIntent;
    }

    if (d->pdfAnnot->getType() == Annot::typeFreeText) {
        const AnnotFreeText *ftextann = static_cast<const AnnotFreeText *>(d->pdfAnnot);
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
        AnnotFreeText *ftextann = static_cast<AnnotFreeText *>(d->pdfAnnot);
        ftextann->setIntent((AnnotFreeText::AnnotFreeTextIntent)intent);
    }
}

/** LineAnnotation [Annotation] */
class LineAnnotationPrivate : public AnnotationPrivate
{
public:
    LineAnnotationPrivate();
    Annotation *makeAlias() override;
    Annot *createNativeAnnot(::Page *destPage, DocumentData *doc) override;

    // data fields (note uses border for rendering style)
    QVector<QPointF> linePoints;
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
    : AnnotationPrivate(), lineStartStyle(LineAnnotation::None), lineEndStyle(LineAnnotation::None), lineClosed(false), lineShowCaption(false), lineLeadingFwdPt(0), lineLeadingBackPt(0), lineIntent(LineAnnotation::Unknown)
{
}

Annotation *LineAnnotationPrivate::makeAlias()
{
    return new LineAnnotation(*this);
}

Annot *LineAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    // Setters are defined in the public class
    LineAnnotation *q = static_cast<LineAnnotation *>(makeAlias());

    // Set page and document
    pdfPage = destPage;
    parentDoc = doc;

    // Set pdfAnnot
    PDFRectangle rect = boundaryToPdfRectangle(boundary, flags);
    if (lineType == LineAnnotation::StraightLine) {
        pdfAnnot = new AnnotLine(doc->doc, &rect);
    } else {
        pdfAnnot = new AnnotPolygon(doc->doc, &rect, lineClosed ? Annot::typePolygon : Annot::typePolyLine);
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

    delete q;

    linePoints.clear(); // Free up memory

    return pdfAnnot;
}

LineAnnotation::LineAnnotation(LineAnnotation::LineType type) : Annotation(*new LineAnnotationPrivate())
{
    setLineType(type);
}

LineAnnotation::LineAnnotation(LineAnnotationPrivate &dd) : Annotation(dd) { }

LineAnnotation::~LineAnnotation() { }

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

QVector<QPointF> LineAnnotation::linePoints() const
{
    Q_D(const LineAnnotation);

    if (!d->pdfAnnot) {
        return d->linePoints;
    }

    double MTX[6];
    d->fillTransformationMTX(MTX);

    QVector<QPointF> res;
    if (d->pdfAnnot->getType() == Annot::typeLine) {
        const AnnotLine *lineann = static_cast<const AnnotLine *>(d->pdfAnnot);
        QPointF p;
        XPDFReader::transform(MTX, lineann->getX1(), lineann->getY1(), p);
        res.append(p);
        XPDFReader::transform(MTX, lineann->getX2(), lineann->getY2(), p);
        res.append(p);
    } else {
        const AnnotPolygon *polyann = static_cast<const AnnotPolygon *>(d->pdfAnnot);
        const AnnotPath *vertices = polyann->getVertices();

        for (int i = 0; i < vertices->getCoordsLength(); ++i) {
            QPointF p;
            XPDFReader::transform(MTX, vertices->getX(i), vertices->getY(i), p);
            res.append(p);
        }
    }

    return res;
}

void LineAnnotation::setLinePoints(const QVector<QPointF> &points)
{
    Q_D(LineAnnotation);

    if (!d->pdfAnnot) {
        d->linePoints = points;
        return;
    }

    if (d->pdfAnnot->getType() == Annot::typeLine) {
        AnnotLine *lineann = static_cast<AnnotLine *>(d->pdfAnnot);
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
        AnnotPolygon *polyann = static_cast<AnnotPolygon *>(d->pdfAnnot);
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
        const AnnotLine *lineann = static_cast<const AnnotLine *>(d->pdfAnnot);
        return (LineAnnotation::TermStyle)lineann->getStartStyle();
    } else {
        const AnnotPolygon *polyann = static_cast<const AnnotPolygon *>(d->pdfAnnot);
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
        AnnotLine *lineann = static_cast<AnnotLine *>(d->pdfAnnot);
        lineann->setStartEndStyle((AnnotLineEndingStyle)style, lineann->getEndStyle());
    } else {
        AnnotPolygon *polyann = static_cast<AnnotPolygon *>(d->pdfAnnot);
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
        const AnnotLine *lineann = static_cast<const AnnotLine *>(d->pdfAnnot);
        return (LineAnnotation::TermStyle)lineann->getEndStyle();
    } else {
        const AnnotPolygon *polyann = static_cast<const AnnotPolygon *>(d->pdfAnnot);
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
        AnnotLine *lineann = static_cast<AnnotLine *>(d->pdfAnnot);
        lineann->setStartEndStyle(lineann->getStartStyle(), (AnnotLineEndingStyle)style);
    } else {
        AnnotPolygon *polyann = static_cast<AnnotPolygon *>(d->pdfAnnot);
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
        AnnotPolygon *polyann = static_cast<AnnotPolygon *>(d->pdfAnnot);

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
        const AnnotLine *lineann = static_cast<const AnnotLine *>(d->pdfAnnot);
        c = lineann->getInteriorColor();
    } else {
        const AnnotPolygon *polyann = static_cast<const AnnotPolygon *>(d->pdfAnnot);
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
        AnnotLine *lineann = static_cast<AnnotLine *>(d->pdfAnnot);
        lineann->setInteriorColor(std::move(c));
    } else {
        AnnotPolygon *polyann = static_cast<AnnotPolygon *>(d->pdfAnnot);
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
        const AnnotLine *lineann = static_cast<const AnnotLine *>(d->pdfAnnot);
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
        AnnotLine *lineann = static_cast<AnnotLine *>(d->pdfAnnot);
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
        const AnnotLine *lineann = static_cast<const AnnotLine *>(d->pdfAnnot);
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
        AnnotLine *lineann = static_cast<AnnotLine *>(d->pdfAnnot);
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
        const AnnotLine *lineann = static_cast<const AnnotLine *>(d->pdfAnnot);
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
        AnnotLine *lineann = static_cast<AnnotLine *>(d->pdfAnnot);
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
        const AnnotLine *lineann = static_cast<const AnnotLine *>(d->pdfAnnot);
        return (LineAnnotation::LineIntent)(lineann->getIntent() + 1);
    } else {
        const AnnotPolygon *polyann = static_cast<const AnnotPolygon *>(d->pdfAnnot);
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
        AnnotLine *lineann = static_cast<AnnotLine *>(d->pdfAnnot);
        lineann->setIntent((AnnotLine::AnnotLineIntent)(intent - 1));
    } else {
        AnnotPolygon *polyann = static_cast<AnnotPolygon *>(d->pdfAnnot);
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
    Annotation *makeAlias() override;
    Annot *createNativeAnnot(::Page *destPage, DocumentData *doc) override;

    // data fields (note uses border for rendering style)
    GeomAnnotation::GeomType geomType;
    QColor geomInnerColor;
};

GeomAnnotationPrivate::GeomAnnotationPrivate() : AnnotationPrivate(), geomType(GeomAnnotation::InscribedSquare) { }

Annotation *GeomAnnotationPrivate::makeAlias()
{
    return new GeomAnnotation(*this);
}

Annot *GeomAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    // Setters are defined in the public class
    GeomAnnotation *q = static_cast<GeomAnnotation *>(makeAlias());

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
    pdfAnnot = new AnnotGeometry(destPage->getDoc(), &rect, type);

    // Set properties
    flushBaseAnnotationProperties();
    q->setGeomInnerColor(geomInnerColor);

    delete q;
    return pdfAnnot;
}

GeomAnnotation::GeomAnnotation() : Annotation(*new GeomAnnotationPrivate()) { }

GeomAnnotation::GeomAnnotation(GeomAnnotationPrivate &dd) : Annotation(dd) { }

GeomAnnotation::~GeomAnnotation() { }

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

    AnnotGeometry *geomann = static_cast<AnnotGeometry *>(d->pdfAnnot);
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

    const AnnotGeometry *geomann = static_cast<const AnnotGeometry *>(d->pdfAnnot);
    return convertAnnotColor(geomann->getInteriorColor());
}

void GeomAnnotation::setGeomInnerColor(const QColor &color)
{
    Q_D(GeomAnnotation);

    if (!d->pdfAnnot) {
        d->geomInnerColor = color;
        return;
    }

    AnnotGeometry *geomann = static_cast<AnnotGeometry *>(d->pdfAnnot);
    geomann->setInteriorColor(convertQColor(color));
}

/** HighlightAnnotation [Annotation] */
class HighlightAnnotationPrivate : public AnnotationPrivate
{
public:
    HighlightAnnotationPrivate();
    Annotation *makeAlias() override;
    Annot *createNativeAnnot(::Page *destPage, DocumentData *doc) override;

    // data fields
    HighlightAnnotation::HighlightType highlightType;
    QList<HighlightAnnotation::Quad> highlightQuads; // not empty

    // helpers
    static Annot::AnnotSubtype toAnnotSubType(HighlightAnnotation::HighlightType type);
    QList<HighlightAnnotation::Quad> fromQuadrilaterals(AnnotQuadrilaterals *quads) const;
    AnnotQuadrilaterals *toQuadrilaterals(const QList<HighlightAnnotation::Quad> &quads) const;
};

HighlightAnnotationPrivate::HighlightAnnotationPrivate() : AnnotationPrivate(), highlightType(HighlightAnnotation::Highlight) { }

Annotation *HighlightAnnotationPrivate::makeAlias()
{
    return new HighlightAnnotation(*this);
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
    foreach (const HighlightAnnotation::Quad &q, quads) {
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

Annot *HighlightAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    // Setters are defined in the public class
    HighlightAnnotation *q = static_cast<HighlightAnnotation *>(makeAlias());

    // Set page and document
    pdfPage = destPage;
    parentDoc = doc;

    // Set pdfAnnot
    PDFRectangle rect = boundaryToPdfRectangle(boundary, flags);
    pdfAnnot = new AnnotTextMarkup(destPage->getDoc(), &rect, toAnnotSubType(highlightType));

    // Set properties
    flushBaseAnnotationProperties();
    q->setHighlightQuads(highlightQuads);

    highlightQuads.clear(); // Free up memory

    delete q;

    return pdfAnnot;
}

HighlightAnnotation::HighlightAnnotation() : Annotation(*new HighlightAnnotationPrivate()) { }

HighlightAnnotation::HighlightAnnotation(HighlightAnnotationPrivate &dd) : Annotation(dd) { }

HighlightAnnotation::~HighlightAnnotation() { }

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

    AnnotTextMarkup *hlann = static_cast<AnnotTextMarkup *>(d->pdfAnnot);
    hlann->setType(HighlightAnnotationPrivate::toAnnotSubType(type));
}

QList<HighlightAnnotation::Quad> HighlightAnnotation::highlightQuads() const
{
    Q_D(const HighlightAnnotation);

    if (!d->pdfAnnot) {
        return d->highlightQuads;
    }

    const AnnotTextMarkup *hlann = static_cast<AnnotTextMarkup *>(d->pdfAnnot);
    return d->fromQuadrilaterals(hlann->getQuadrilaterals());
}

void HighlightAnnotation::setHighlightQuads(const QList<HighlightAnnotation::Quad> &quads)
{
    Q_D(HighlightAnnotation);

    if (!d->pdfAnnot) {
        d->highlightQuads = quads;
        return;
    }

    AnnotTextMarkup *hlann = static_cast<AnnotTextMarkup *>(d->pdfAnnot);
    AnnotQuadrilaterals *quadrilaterals = d->toQuadrilaterals(quads);
    hlann->setQuadrilaterals(*quadrilaterals);
    delete quadrilaterals;
}

/** StampAnnotation [Annotation] */
class StampAnnotationPrivate : public AnnotationPrivate
{
public:
    StampAnnotationPrivate();
    Annotation *makeAlias() override;
    Annot *createNativeAnnot(::Page *destPage, DocumentData *doc) override;

    AnnotStampImageHelper *convertQImageToAnnotStampImageHelper(const QImage &qimg);

    // data fields
    QString stampIconName;
    QImage stampCustomImage;
};

StampAnnotationPrivate::StampAnnotationPrivate() : AnnotationPrivate(), stampIconName(QStringLiteral("Draft")) { }

Annotation *StampAnnotationPrivate::makeAlias()
{
    return new StampAnnotation(*this);
}

Annot *StampAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    StampAnnotation *q = static_cast<StampAnnotation *>(makeAlias());

    // Set page and document
    pdfPage = destPage;
    parentDoc = doc;

    // Set pdfAnnot
    PDFRectangle rect = boundaryToPdfRectangle(boundary, flags);
    pdfAnnot = new AnnotStamp(destPage->getDoc(), &rect);

    // Set properties
    flushBaseAnnotationProperties();
    q->setStampIconName(stampIconName);
    q->setStampCustomImage(stampCustomImage);

    delete q;

    stampIconName.clear(); // Free up memory

    return pdfAnnot;
}

AnnotStampImageHelper *StampAnnotationPrivate::convertQImageToAnnotStampImageHelper(const QImage &qimg)
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

    AnnotStampImageHelper *annotImg;

    if (sMaskData.size() > 0) {
        AnnotStampImageHelper sMask(parentDoc->doc, width, height, ColorSpace::DeviceGray, 8, sMaskData.data(), sMaskData.size());
        annotImg = new AnnotStampImageHelper(parentDoc->doc, width, height, colorSpace, bitsPerComponent, data.data(), data.size(), sMask.getRef());
    } else {
        annotImg = new AnnotStampImageHelper(parentDoc->doc, width, height, colorSpace, bitsPerComponent, data.data(), data.size());
    }

    return annotImg;
}

StampAnnotation::StampAnnotation() : Annotation(*new StampAnnotationPrivate()) { }

StampAnnotation::StampAnnotation(StampAnnotationPrivate &dd) : Annotation(dd) { }

StampAnnotation::~StampAnnotation() { }

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

    const AnnotStamp *stampann = static_cast<const AnnotStamp *>(d->pdfAnnot);
    return QString::fromLatin1(stampann->getIcon()->c_str());
}

void StampAnnotation::setStampIconName(const QString &name)
{
    Q_D(StampAnnotation);

    if (!d->pdfAnnot) {
        d->stampIconName = name;
        return;
    }

    AnnotStamp *stampann = static_cast<AnnotStamp *>(d->pdfAnnot);
    QByteArray encoded = name.toLatin1();
    GooString s(encoded.constData());
    stampann->setIcon(&s);
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

    AnnotStamp *stampann = static_cast<AnnotStamp *>(d->pdfAnnot);
    AnnotStampImageHelper *annotCustomImage = d->convertQImageToAnnotStampImageHelper(image);
    stampann->setCustomImage(annotCustomImage);
}

/** SignatureAnnotation [Annotation] */
class SignatureAnnotationPrivate : public AnnotationPrivate
{
public:
    SignatureAnnotationPrivate();
    Annotation *makeAlias() override;
    Annot *createNativeAnnot(::Page *destPage, DocumentData *doc) override;

    // data fields
    QString text;
    QString leftText;
    double fontSize = 10.0;
    double leftFontSize = 20.0;
    QColor fontColor = Qt::red;
    QColor borderColor = Qt::red;
    double borderWidth = 1.5;
    QColor backgroundColor = QColor(240, 240, 240);
    QString imagePath;
    QString fieldPartialName;
    std::unique_ptr<::FormFieldSignature> field = nullptr;
};

SignatureAnnotationPrivate::SignatureAnnotationPrivate() : AnnotationPrivate() { }

Annotation *SignatureAnnotationPrivate::makeAlias()
{
    return new SignatureAnnotation(*this);
}

Annot *SignatureAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    SignatureAnnotation *q = static_cast<SignatureAnnotation *>(makeAlias());

    // Set page and document
    pdfPage = destPage;
    parentDoc = doc;

    // Set pdfAnnot
    PDFRectangle rect = boundaryToPdfRectangle(boundary, flags);

    std::unique_ptr<GooString> gSignatureText = std::unique_ptr<GooString>(QStringToUnicodeGooString(text));
    std::unique_ptr<GooString> gSignatureLeftText = std::unique_ptr<GooString>(QStringToUnicodeGooString(leftText));

    std::optional<PDFDoc::SignatureData> sig = destPage->getDoc()->createSignature(destPage, QStringToGooString(fieldPartialName), rect, *gSignatureText, *gSignatureLeftText, fontSize, leftFontSize, convertQColor(fontColor), borderWidth,
                                                                                   convertQColor(borderColor), convertQColor(backgroundColor), imagePath.toStdString());

    if (!sig) {
        return nullptr;
    }

    sig->formWidget->updateWidgetAppearance();
    field = std::move(sig->field);

    // Set properties
    flushBaseAnnotationProperties();

    pdfAnnot = sig->annotWidget;

    pdfAnnot->incRefCnt();

    delete q;

    return pdfAnnot;
}

SignatureAnnotation::SignatureAnnotation() : Annotation(*new SignatureAnnotationPrivate()) { }

SignatureAnnotation::SignatureAnnotation(SignatureAnnotationPrivate &dd) : Annotation(dd) { }

SignatureAnnotation::~SignatureAnnotation() { }

Annotation::SubType SignatureAnnotation::subType() const
{
    return AWidget;
}

void SignatureAnnotation::setText(const QString &text)
{
    Q_D(SignatureAnnotation);
    d->text = text;
}

void SignatureAnnotation::setLeftText(const QString &text)
{
    Q_D(SignatureAnnotation);
    d->leftText = text;
}

double SignatureAnnotation::fontSize() const
{
    Q_D(const SignatureAnnotation);
    return d->fontSize;
}

void SignatureAnnotation::setFontSize(double fontSize)
{
    Q_D(SignatureAnnotation);
    d->fontSize = fontSize;
}

double SignatureAnnotation::leftFontSize() const
{
    Q_D(const SignatureAnnotation);
    return d->leftFontSize;
}

void SignatureAnnotation::setLeftFontSize(double fontSize)
{
    Q_D(SignatureAnnotation);
    d->leftFontSize = fontSize;
}

QColor SignatureAnnotation::fontColor() const
{
    Q_D(const SignatureAnnotation);
    return d->fontColor;
}

void SignatureAnnotation::setFontColor(const QColor &color)
{
    Q_D(SignatureAnnotation);
    d->fontColor = color;
}

QColor SignatureAnnotation::borderColor() const
{
    Q_D(const SignatureAnnotation);
    return d->borderColor;
}

void SignatureAnnotation::setBorderColor(const QColor &color)
{
    Q_D(SignatureAnnotation);
    d->borderColor = color;
}

QColor SignatureAnnotation::backgroundColor() const
{
    Q_D(const SignatureAnnotation);
    return d->backgroundColor;
}

double SignatureAnnotation::borderWidth() const
{
    Q_D(const SignatureAnnotation);
    return d->borderWidth;
}

void SignatureAnnotation::setBorderWidth(double width)
{
    Q_D(SignatureAnnotation);
    d->borderWidth = width;
}

void SignatureAnnotation::setBackgroundColor(const QColor &color)
{
    Q_D(SignatureAnnotation);
    d->backgroundColor = color;
}

QString SignatureAnnotation::imagePath() const
{
    Q_D(const SignatureAnnotation);
    return d->imagePath;
}

void SignatureAnnotation::setImagePath(const QString &imagePath)
{
    Q_D(SignatureAnnotation);
    d->imagePath = imagePath;
}

QString SignatureAnnotation::fieldPartialName() const
{
    Q_D(const SignatureAnnotation);
    return d->fieldPartialName;
}

void SignatureAnnotation::setFieldPartialName(const QString &fieldPartialName)
{
    Q_D(SignatureAnnotation);
    d->fieldPartialName = fieldPartialName;
}

SignatureAnnotation::SigningResult SignatureAnnotation::sign(const QString &outputFileName, const PDFConverter::NewSignatureData &data)
{
    Q_D(SignatureAnnotation);
    auto formField = std::make_unique<FormFieldSignature>(d->parentDoc, d->pdfPage, static_cast<::FormWidgetSignature *>(d->field->getCreateWidget()));

    const auto result = formField->sign(outputFileName, data);

    switch (result) {
    case FormFieldSignature::SigningSuccess:
        return SigningSuccess;
    case FormFieldSignature::FieldAlreadySigned:
        return FieldAlreadySigned;
    case FormFieldSignature::GenericSigningError:
        return GenericSigningError;
    case FormFieldSignature::InternalError:
        return InternalError;
    case FormFieldSignature::KeyMissing:
        return KeyMissing;
    case FormFieldSignature::WriteFailed:
        return WriteFailed;
    case FormFieldSignature::UserCancelled:
        return UserCancelled;
    }
    return GenericSigningError;
}

/** InkAnnotation [Annotation] */
class InkAnnotationPrivate : public AnnotationPrivate
{
public:
    InkAnnotationPrivate();
    Annotation *makeAlias() override;
    Annot *createNativeAnnot(::Page *destPage, DocumentData *doc) override;

    // data fields
    QList<QVector<QPointF>> inkPaths;

    // helper
    std::vector<std::unique_ptr<AnnotPath>> toAnnotPaths(const QList<QVector<QPointF>> &paths);
};

InkAnnotationPrivate::InkAnnotationPrivate() : AnnotationPrivate() { }

Annotation *InkAnnotationPrivate::makeAlias()
{
    return new InkAnnotation(*this);
}

// Note: Caller is required to delete array elements and the array itself after use
std::vector<std::unique_ptr<AnnotPath>> InkAnnotationPrivate::toAnnotPaths(const QList<QVector<QPointF>> &paths)
{
    std::vector<std::unique_ptr<AnnotPath>> res;
    res.reserve(paths.size());
    for (const auto &path : paths) {
        res.push_back(toAnnotPath(path));
    }
    return res;
}

Annot *InkAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    // Setters are defined in the public class
    InkAnnotation *q = static_cast<InkAnnotation *>(makeAlias());

    // Set page and document
    pdfPage = destPage;
    parentDoc = doc;

    // Set pdfAnnot
    PDFRectangle rect = boundaryToPdfRectangle(boundary, flags);
    pdfAnnot = new AnnotInk(destPage->getDoc(), &rect);

    // Set properties
    flushBaseAnnotationProperties();
    q->setInkPaths(inkPaths);

    inkPaths.clear(); // Free up memory

    delete q;

    return pdfAnnot;
}

InkAnnotation::InkAnnotation() : Annotation(*new InkAnnotationPrivate()) { }

InkAnnotation::InkAnnotation(InkAnnotationPrivate &dd) : Annotation(dd) { }

InkAnnotation::~InkAnnotation() { }

Annotation::SubType InkAnnotation::subType() const
{
    return AInk;
}

QList<QVector<QPointF>> InkAnnotation::inkPaths() const
{
    Q_D(const InkAnnotation);

    if (!d->pdfAnnot) {
        return d->inkPaths;
    }

    const AnnotInk *inkann = static_cast<const AnnotInk *>(d->pdfAnnot);

    const std::vector<std::unique_ptr<AnnotPath>> &paths = inkann->getInkList();
    if (paths.empty()) {
        return {};
    }

    double MTX[6];
    d->fillTransformationMTX(MTX);

    QList<QVector<QPointF>> inkPaths;
    inkPaths.reserve(paths.size());
    for (const auto &path : paths) {
        // transform each path in a list of normalized points ..
        QVector<QPointF> localList;
        const int pointsNumber = path ? path->getCoordsLength() : 0;
        for (int n = 0; n < pointsNumber; ++n) {
            QPointF point;
            XPDFReader::transform(MTX, path->getX(n), path->getY(n), point);
            localList.append(point);
        }
        // ..and add it to the annotation
        inkPaths.append(localList);
    }
    inkPaths.shrink_to_fit();
    return inkPaths;
}

void InkAnnotation::setInkPaths(const QList<QVector<QPointF>> &paths)
{
    Q_D(InkAnnotation);

    if (!d->pdfAnnot) {
        d->inkPaths = paths;
        return;
    }

    AnnotInk *inkann = static_cast<AnnotInk *>(d->pdfAnnot);
    const std::vector<std::unique_ptr<AnnotPath>> annotpaths = d->toAnnotPaths(paths);
    inkann->setInkList(annotpaths);
}

/** LinkAnnotation [Annotation] */
class LinkAnnotationPrivate : public AnnotationPrivate
{
public:
    LinkAnnotationPrivate();
    ~LinkAnnotationPrivate() override;
    Annotation *makeAlias() override;
    Annot *createNativeAnnot(::Page *destPage, DocumentData *doc) override;

    // data fields
    std::unique_ptr<Link> linkDestination;
    LinkAnnotation::HighlightMode linkHLMode;
    QPointF linkRegion[4];
};

LinkAnnotationPrivate::LinkAnnotationPrivate() : AnnotationPrivate(), linkHLMode(LinkAnnotation::Invert) { }

LinkAnnotationPrivate::~LinkAnnotationPrivate() { }

Annotation *LinkAnnotationPrivate::makeAlias()
{
    return new LinkAnnotation(*this);
}

Annot *LinkAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    return nullptr; // Not implemented
}

LinkAnnotation::LinkAnnotation() : Annotation(*new LinkAnnotationPrivate()) { }

LinkAnnotation::LinkAnnotation(LinkAnnotationPrivate &dd) : Annotation(dd) { }

LinkAnnotation::~LinkAnnotation() { }

Annotation::SubType LinkAnnotation::subType() const
{
    return ALink;
}

Link *LinkAnnotation::linkDestination() const
{
    Q_D(const LinkAnnotation);
    return d->linkDestination.get();
}

void LinkAnnotation::setLinkDestination(std::unique_ptr<Link> &&link)
{
    Q_D(LinkAnnotation);
    d->linkDestination = std::move(link);
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

void LinkAnnotation::setLinkRegionPoint(int id, const QPointF point)
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
    Annotation *makeAlias() override;
    Annot *createNativeAnnot(::Page *destPage, DocumentData *doc) override;

    // data fields
    CaretAnnotation::CaretSymbol symbol;
};

CaretAnnotationPrivate::CaretAnnotationPrivate() : AnnotationPrivate(), symbol(CaretAnnotation::None) { }

Annotation *CaretAnnotationPrivate::makeAlias()
{
    return new CaretAnnotation(*this);
}

Annot *CaretAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    // Setters are defined in the public class
    CaretAnnotation *q = static_cast<CaretAnnotation *>(makeAlias());

    // Set page and document
    pdfPage = destPage;
    parentDoc = doc;

    // Set pdfAnnot
    PDFRectangle rect = boundaryToPdfRectangle(boundary, flags);
    pdfAnnot = new AnnotCaret(destPage->getDoc(), &rect);

    // Set properties
    flushBaseAnnotationProperties();
    q->setCaretSymbol(symbol);

    delete q;
    return pdfAnnot;
}

CaretAnnotation::CaretAnnotation() : Annotation(*new CaretAnnotationPrivate()) { }

CaretAnnotation::CaretAnnotation(CaretAnnotationPrivate &dd) : Annotation(dd) { }

CaretAnnotation::~CaretAnnotation() { }

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

    const AnnotCaret *caretann = static_cast<const AnnotCaret *>(d->pdfAnnot);
    return (CaretAnnotation::CaretSymbol)caretann->getSymbol();
}

void CaretAnnotation::setCaretSymbol(CaretAnnotation::CaretSymbol symbol)
{
    Q_D(CaretAnnotation);

    if (!d->pdfAnnot) {
        d->symbol = symbol;
        return;
    }

    AnnotCaret *caretann = static_cast<AnnotCaret *>(d->pdfAnnot);
    caretann->setSymbol((AnnotCaret::AnnotCaretSymbol)symbol);
}

/** FileAttachmentAnnotation [Annotation] */
class FileAttachmentAnnotationPrivate : public AnnotationPrivate
{
public:
    FileAttachmentAnnotationPrivate();
    ~FileAttachmentAnnotationPrivate() override;
    Annotation *makeAlias() override;
    Annot *createNativeAnnot(::Page *destPage, DocumentData *doc) override;

    // data fields
    QString icon;
    EmbeddedFile *embfile;
};

FileAttachmentAnnotationPrivate::FileAttachmentAnnotationPrivate() : AnnotationPrivate(), icon(QStringLiteral("PushPin")), embfile(nullptr) { }

FileAttachmentAnnotationPrivate::~FileAttachmentAnnotationPrivate()
{
    delete embfile;
}

Annotation *FileAttachmentAnnotationPrivate::makeAlias()
{
    return new FileAttachmentAnnotation(*this);
}

Annot *FileAttachmentAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    return nullptr; // Not implemented
}

FileAttachmentAnnotation::FileAttachmentAnnotation() : Annotation(*new FileAttachmentAnnotationPrivate()) { }

FileAttachmentAnnotation::FileAttachmentAnnotation(FileAttachmentAnnotationPrivate &dd) : Annotation(dd) { }

FileAttachmentAnnotation::~FileAttachmentAnnotation() { }

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
    Annotation *makeAlias() override;
    Annot *createNativeAnnot(::Page *destPage, DocumentData *doc) override;

    // data fields
    QString icon;
    SoundObject *sound;
};

SoundAnnotationPrivate::SoundAnnotationPrivate() : AnnotationPrivate(), icon(QStringLiteral("Speaker")), sound(nullptr) { }

SoundAnnotationPrivate::~SoundAnnotationPrivate()
{
    delete sound;
}

Annotation *SoundAnnotationPrivate::makeAlias()
{
    return new SoundAnnotation(*this);
}

Annot *SoundAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    return nullptr; // Not implemented
}

SoundAnnotation::SoundAnnotation() : Annotation(*new SoundAnnotationPrivate()) { }

SoundAnnotation::SoundAnnotation(SoundAnnotationPrivate &dd) : Annotation(dd) { }

SoundAnnotation::~SoundAnnotation() { }

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
    Annotation *makeAlias() override;
    Annot *createNativeAnnot(::Page *destPage, DocumentData *doc) override;

    // data fields
    MovieObject *movie;
    QString title;
};

MovieAnnotationPrivate::MovieAnnotationPrivate() : AnnotationPrivate(), movie(nullptr) { }

MovieAnnotationPrivate::~MovieAnnotationPrivate()
{
    delete movie;
}

Annotation *MovieAnnotationPrivate::makeAlias()
{
    return new MovieAnnotation(*this);
}

Annot *MovieAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    return nullptr; // Not implemented
}

MovieAnnotation::MovieAnnotation() : Annotation(*new MovieAnnotationPrivate()) { }

MovieAnnotation::MovieAnnotation(MovieAnnotationPrivate &dd) : Annotation(dd) { }

MovieAnnotation::~MovieAnnotation() { }

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
    Annotation *makeAlias() override;
    Annot *createNativeAnnot(::Page *destPage, DocumentData *doc) override;

    // data fields
    LinkRendition *action;
    QString title;
};

ScreenAnnotationPrivate::ScreenAnnotationPrivate() : AnnotationPrivate(), action(nullptr) { }

ScreenAnnotationPrivate::~ScreenAnnotationPrivate()
{
    delete action;
}

ScreenAnnotation::ScreenAnnotation(ScreenAnnotationPrivate &dd) : Annotation(dd) { }

Annotation *ScreenAnnotationPrivate::makeAlias()
{
    return new ScreenAnnotation(*this);
}

Annot *ScreenAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    return nullptr; // Not implemented
}

ScreenAnnotation::ScreenAnnotation() : Annotation(*new ScreenAnnotationPrivate()) { }

ScreenAnnotation::~ScreenAnnotation() { }

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

std::unique_ptr<Link> ScreenAnnotation::additionalAction(AdditionalActionType type) const
{
    Q_D(const ScreenAnnotation);
    return d->additionalAction(type);
}

/** WidgetAnnotation [Annotation] */
class WidgetAnnotationPrivate : public AnnotationPrivate
{
public:
    Annotation *makeAlias() override;
    Annot *createNativeAnnot(::Page *destPage, DocumentData *doc) override;
};

Annotation *WidgetAnnotationPrivate::makeAlias()
{
    return new WidgetAnnotation(*this);
}

Annot *WidgetAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    return nullptr; // Not implemented
}

WidgetAnnotation::WidgetAnnotation(WidgetAnnotationPrivate &dd) : Annotation(dd) { }

WidgetAnnotation::WidgetAnnotation() : Annotation(*new WidgetAnnotationPrivate()) { }

WidgetAnnotation::~WidgetAnnotation() { }

Annotation::SubType WidgetAnnotation::subType() const
{
    return AWidget;
}

std::unique_ptr<Link> WidgetAnnotation::additionalAction(AdditionalActionType type) const
{
    Q_D(const WidgetAnnotation);
    return d->additionalAction(type);
}

/** RichMediaAnnotation [Annotation] */
class RichMediaAnnotation::Params::Private
{
public:
    Private() { }

    QString flashVars;
};

RichMediaAnnotation::Params::Params() : d(new Private) { }

RichMediaAnnotation::Params::~Params() { }

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

RichMediaAnnotation::Instance::~Instance() { }

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
    Private() { }
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

RichMediaAnnotation::Configuration::~Configuration() { }

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

RichMediaAnnotation::Asset::~Asset() { }

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
    Private() { }
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

RichMediaAnnotation::Content::~Content() { }

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

RichMediaAnnotation::Activation::~Activation() { }

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

RichMediaAnnotation::Deactivation::~Deactivation() { }

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

RichMediaAnnotation::Settings::~Settings() { }

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

    Annotation *makeAlias() override { return new RichMediaAnnotation(*this); }

    Annot *createNativeAnnot(::Page *destPage, DocumentData *doc) override
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

RichMediaAnnotation::~RichMediaAnnotation() { }

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
