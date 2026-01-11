/* poppler-annotation.cc: qt interface to poppler
 * Copyright (C) 2006, 2009, 2012-2015, 2018-2022, 2024-2026 Albert Astals Cid <aacid@kde.org>
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
 * Copyright (C) 2024-2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
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
#include "CryptoSignBackend.h"
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

template<typename T, typename U>
static std::unique_ptr<T> static_pointer_cast(std::unique_ptr<U> &&in)
{
    return std::unique_ptr<T>(static_cast<std::add_pointer_t<T>>(in.release()));
}

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
AnnotationPrivate::AnnotationPrivate() : pdfAnnot(nullptr) { }

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
    auto *gfxState = new GfxState(72.0, 72.0, pdfPage->getCropBox(), pageRotation, true);
    const std::array<double, 6> &gfxCTM = gfxState->getCTM();

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
    }
    if (rotationFixUp == 90) {
        return PDFRectangle(tl_x, tl_y - width, tl_x + height, tl_y);
    }
    if (rotationFixUp == 180) {
        return PDFRectangle(br_x, tl_y - height, br_x + width, tl_y);
    }
    // rotationFixUp == 270
    return PDFRectangle(br_x, br_y - width, br_x + height, br_y);
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

    Q_FOREACH (const QPointF &p, list) {
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
    for (const std::shared_ptr<Annot> &ann : annots->getAnnots()) {
        if (!ann) {
            error(errInternal, -1, "Annot is null");
            continue;
        }

        // Check parent annotation
        auto *markupann = dynamic_cast<AnnotMarkup *>(ann.get());
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
            auto *linkann = static_cast<AnnotLink *>(ann.get());
            auto *l = new LinkAnnotation();

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
            auto *attachann = static_cast<AnnotFileAttachment *>(ann.get());
            auto *f = new FileAttachmentAnnotation();
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
            auto *soundann = static_cast<AnnotSound *>(ann.get());
            auto *s = new SoundAnnotation();

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
            auto *movieann = static_cast<AnnotMovie *>(ann.get());
            auto *m = new MovieAnnotation();

            // -> movie
            auto *movie = new MovieObject(movieann);
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
            auto *screenann = static_cast<AnnotScreen *>(ann.get());
            // TODO Support other link types than Link::Rendition in ScreenAnnotation
            if (!screenann->getAction() || screenann->getAction()->getKind() != actionRendition) {
                continue;
            }
            auto *s = new ScreenAnnotation();

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
            const AnnotRichMedia *annotRichMedia = static_cast<AnnotRichMedia *>(ann.get());

            auto *richMediaAnnotation = new RichMediaAnnotation;

            const AnnotRichMedia::Settings *annotSettings = annotRichMedia->getSettings();
            if (annotSettings) {
                auto *settings = new RichMediaAnnotation::Settings;

                if (annotSettings->getActivation()) {
                    auto *activation = new RichMediaAnnotation::Activation;

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
                    auto *deactivation = new RichMediaAnnotation::Deactivation;

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
                auto *content = new RichMediaAnnotation::Content;

                const std::vector<std::unique_ptr<AnnotRichMedia::Configuration>> &annotConfigurations = annotContent->getConfigurations();
                QList<RichMediaAnnotation::Configuration *> configurations;

                for (const std::unique_ptr<AnnotRichMedia::Configuration> &annotConfiguration : annotConfigurations) {
                    if (!annotConfiguration) {
                        continue;
                    }

                    auto *configuration = new RichMediaAnnotation::Configuration;

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

                    const std::vector<std::unique_ptr<AnnotRichMedia::Instance>> &annotInstances = annotConfiguration->getInstances();
                    QList<RichMediaAnnotation::Instance *> instances;

                    for (const std::unique_ptr<AnnotRichMedia::Instance> &annotInstance : annotInstances) {
                        if (!annotInstance) {
                            continue;
                        }

                        auto *instance = new RichMediaAnnotation::Instance;

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
                            auto *params = new RichMediaAnnotation::Params;

                            if (annotParams->getFlashVars()) {
                                params->setFlashVars(UnicodeParsedString(annotParams->getFlashVars()));
                            }

                            instance->setParams(params);
                        }

                        instances.append(instance);
                    }

                    configuration->setInstances(instances);

                    configurations.append(configuration);
                }

                content->setConfigurations(configurations);

                const std::vector<std::unique_ptr<AnnotRichMedia::Asset>> &annotAssets = annotContent->getAssets();
                QList<RichMediaAnnotation::Asset *> assets;

                for (const std::unique_ptr<AnnotRichMedia::Asset> &annotAsset : annotAssets) {
                    if (!annotAsset) {
                        continue;
                    }

                    auto *asset = new RichMediaAnnotation::Asset;

                    if (annotAsset->getName()) {
                        asset->setName(UnicodeParsedString(annotAsset->getName()));
                    }

                    auto fileSpec = std::make_unique<FileSpec>(annotAsset->getFileSpec());
                    asset->setEmbeddedFile(new EmbeddedFile(*new EmbeddedFileData(std::move(fileSpec))));

                    assets.append(asset);
                }

                content->setAssets(assets);

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
        linkAction = static_cast<AnnotScreen *>(pdfAnnot.get())->getAdditionalAction(actionType);
    } else {
        linkAction = static_cast<AnnotWidget *>(pdfAnnot.get())->getAdditionalAction(actionType);
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
    TextAnnotation::TextType textType = TextAnnotation::Linked;
    QString textIcon;
    std::optional<QFont> textFont;
    QColor textColor = Qt::black;
    TextAnnotation::InplaceAlignPosition inplaceAlign = TextAnnotation::InplaceAlignLeft;
    QVector<QPointF> inplaceCallout;
    TextAnnotation::InplaceIntent inplaceIntent = TextAnnotation::Unknown;
};

class Annotation::Style::Private : public QSharedData
{
public:
    Private()
    {
        dashArray.resize(1);
        dashArray[0] = 3;
    }

    QColor color;
    double opacity = 1.0;
    double width = 1.0;
    Annotation::LineStyle lineStyle = Solid;
    double xCorners = 0.0;
    double yCorners = 0.0;
    QVector<double> dashArray;
    Annotation::LineEffect lineEffect = NoEffect;
    double effectIntensity = 1.0;
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
    Private() = default;

    int flags = -1;
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

QString Annotation::author() const
{
    Q_D(const Annotation);

    if (!d->pdfAnnot) {
        return d->author;
    }

    const auto *markupann = dynamic_cast<const AnnotMarkup *>(d->pdfAnnot.get());
    return markupann ? UnicodeParsedString(markupann->getLabel()) : QString();
}

void Annotation::setAuthor(const QString &author)
{
    Q_D(Annotation);

    if (!d->pdfAnnot) {
        d->author = author;
        return;
    }

    auto *markupann = dynamic_cast<AnnotMarkup *>(d->pdfAnnot.get());
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

    auto *textAnnotD = dynamic_cast<TextAnnotationPrivate *>(d);
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

    GooString s(uniqueName.toStdString());
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
    }
    return QDateTime();
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

    const auto *markupann = dynamic_cast<const AnnotMarkup *>(d->pdfAnnot.get());

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

    auto *markupann = dynamic_cast<AnnotMarkup *>(d->pdfAnnot.get());
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

    const auto *markupann = dynamic_cast<const AnnotMarkup *>(d->pdfAnnot.get());
    if (markupann) {
        s.setOpacity(markupann->getOpacity());
    }

    const AnnotBorder *border = d->pdfAnnot->getBorder();
    if (border) {
        if (border->getType() == AnnotBorder::typeArray) {
            const auto *border_array = static_cast<const AnnotBorderArray *>(border);
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

    auto *markupann = dynamic_cast<AnnotMarkup *>(d->pdfAnnot.get());
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
    std::shared_ptr<AnnotPopup> popup = nullptr;
    int flags = -1; // Not initialized

    const auto *markupann = dynamic_cast<const AnnotMarkup *>(d->pdfAnnot.get());
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
        const auto *textann = static_cast<const AnnotText *>(d->pdfAnnot.get());

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

    const auto *markupann = dynamic_cast<const AnnotMarkup *>(d->pdfAnnot.get());

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

    const auto *textann = dynamic_cast<const AnnotText *>(d->pdfAnnot.get());

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
        res.reserve(d->revisions.size());
        for (const std::unique_ptr<Annotation> &rev : d->revisions) {
            res.push_back(rev->d_ptr->makeAlias());
        }
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
TextAnnotationPrivate::TextAnnotationPrivate() : textIcon(QStringLiteral("Note")) { }

std::unique_ptr<Annotation> TextAnnotationPrivate::makeAlias()
{
    return std::unique_ptr<Annotation>(new TextAnnotation(*this));
}

std::shared_ptr<Annot> TextAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    // Setters are defined in the public class
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
        auto *ftextann = static_cast<AnnotFreeText *>(pdfAnnot.get());
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
        DefaultAppearance da { fontName, pointSize, convertQColor(textColor) };
        ftextann->setDefaultAppearance(da);
    }
}

std::unique_ptr<DefaultAppearance> TextAnnotationPrivate::getDefaultAppearanceFromNative() const
{
    if (pdfAnnot && pdfAnnot->getType() == Annot::typeFreeText) {
        auto *ftextann = static_cast<AnnotFreeText *>(pdfAnnot.get());
        return ftextann->getDefaultAppearance();
    }
    return {};
}

TextAnnotation::TextAnnotation(TextAnnotation::TextType type) : Annotation(*new TextAnnotationPrivate())
{
    setTextType(type);
}

TextAnnotation::TextAnnotation(TextAnnotationPrivate &dd) : Annotation(dd) { }

TextAnnotation::~TextAnnotation() = default;

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
        const auto *textann = static_cast<const AnnotText *>(d->pdfAnnot.get());
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
        auto *textann = static_cast<AnnotText *>(d->pdfAnnot.get());
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

TextAnnotation::InplaceAlignPosition TextAnnotation::inplaceAlign() const
{
    Q_D(const TextAnnotation);

    if (!d->pdfAnnot) {
        return d->inplaceAlign;
    }

    if (d->pdfAnnot->getType() == Annot::typeFreeText) {
        const auto *ftextann = static_cast<const AnnotFreeText *>(d->pdfAnnot.get());
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
        auto *ftextann = static_cast<AnnotFreeText *>(d->pdfAnnot.get());
        ftextann->setQuadding(alignToQuadding(align));
    }
}

QPointF TextAnnotation::calloutPoint(int id) const
{
    const QVector<QPointF> points = calloutPoints();
    if (id < 0 || id >= points.size()) {
        return QPointF();
    }
    return points[id];
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

    const auto *ftextann = static_cast<const AnnotFreeText *>(d->pdfAnnot.get());
    const AnnotCalloutLine *callout = ftextann->getCalloutLine();

    if (!callout) {
        return QVector<QPointF>();
    }

    double MTX[6];
    d->fillTransformationMTX(MTX);

    const auto *callout_v6 = dynamic_cast<const AnnotCalloutMultiLine *>(callout);
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

    auto *ftextann = static_cast<AnnotFreeText *>(d->pdfAnnot.get());
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
        const auto *ftextann = static_cast<const AnnotFreeText *>(d->pdfAnnot.get());
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
        auto *ftextann = static_cast<AnnotFreeText *>(d->pdfAnnot.get());
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
    QVector<QPointF> linePoints;
    LineAnnotation::TermStyle lineStartStyle = LineAnnotation::None;
    LineAnnotation::TermStyle lineEndStyle = LineAnnotation::None;
    bool lineClosed : 1 = false; // (if true draw close shape)
    bool lineShowCaption : 1 = false;
    LineAnnotation::LineType lineType;
    QColor lineInnerColor;
    double lineLeadingFwdPt = 0;
    double lineLeadingBackPt = 0;
    LineAnnotation::LineIntent lineIntent = LineAnnotation::Unknown;
};

LineAnnotationPrivate::LineAnnotationPrivate() = default;

std::unique_ptr<Annotation> LineAnnotationPrivate::makeAlias()
{
    return std::unique_ptr<Annotation>(new LineAnnotation(*this));
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
        pdfAnnot = std::make_shared<AnnotLine>(doc->doc.get(), &rect);
    } else {
        pdfAnnot = std::make_shared<AnnotPolygon>(doc->doc.get(), &rect, lineClosed ? Annot::typePolygon : Annot::typePolyLine);
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

LineAnnotation::~LineAnnotation() = default;

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
        const auto *lineann = static_cast<const AnnotLine *>(d->pdfAnnot.get());
        QPointF p;
        XPDFReader::transform(MTX, lineann->getX1(), lineann->getY1(), p);
        res.append(p);
        XPDFReader::transform(MTX, lineann->getX2(), lineann->getY2(), p);
        res.append(p);
    } else {
        const auto *polyann = static_cast<const AnnotPolygon *>(d->pdfAnnot.get());
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
        auto *lineann = static_cast<AnnotLine *>(d->pdfAnnot.get());
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
        auto *polyann = static_cast<AnnotPolygon *>(d->pdfAnnot.get());
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
        const auto *lineann = static_cast<const AnnotLine *>(d->pdfAnnot.get());
        return (LineAnnotation::TermStyle)lineann->getStartStyle();
    }
    const auto *polyann = static_cast<const AnnotPolygon *>(d->pdfAnnot.get());
    return (LineAnnotation::TermStyle)polyann->getStartStyle();
}

void LineAnnotation::setLineStartStyle(LineAnnotation::TermStyle style)
{
    Q_D(LineAnnotation);

    if (!d->pdfAnnot) {
        d->lineStartStyle = style;
        return;
    }

    if (d->pdfAnnot->getType() == Annot::typeLine) {
        auto *lineann = static_cast<AnnotLine *>(d->pdfAnnot.get());
        lineann->setStartEndStyle((AnnotLineEndingStyle)style, lineann->getEndStyle());
    } else {
        auto *polyann = static_cast<AnnotPolygon *>(d->pdfAnnot.get());
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
        const auto *lineann = static_cast<const AnnotLine *>(d->pdfAnnot.get());
        return (LineAnnotation::TermStyle)lineann->getEndStyle();
    }
    const auto *polyann = static_cast<const AnnotPolygon *>(d->pdfAnnot.get());
    return (LineAnnotation::TermStyle)polyann->getEndStyle();
}

void LineAnnotation::setLineEndStyle(LineAnnotation::TermStyle style)
{
    Q_D(LineAnnotation);

    if (!d->pdfAnnot) {
        d->lineEndStyle = style;
        return;
    }

    if (d->pdfAnnot->getType() == Annot::typeLine) {
        auto *lineann = static_cast<AnnotLine *>(d->pdfAnnot.get());
        lineann->setStartEndStyle(lineann->getStartStyle(), (AnnotLineEndingStyle)style);
    } else {
        auto *polyann = static_cast<AnnotPolygon *>(d->pdfAnnot.get());
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
        auto *polyann = static_cast<AnnotPolygon *>(d->pdfAnnot.get());

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
        const auto *lineann = static_cast<const AnnotLine *>(d->pdfAnnot.get());
        c = lineann->getInteriorColor();
    } else {
        const auto *polyann = static_cast<const AnnotPolygon *>(d->pdfAnnot.get());
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
        auto *lineann = static_cast<AnnotLine *>(d->pdfAnnot.get());
        lineann->setInteriorColor(std::move(c));
    } else {
        auto *polyann = static_cast<AnnotPolygon *>(d->pdfAnnot.get());
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
        const auto *lineann = static_cast<const AnnotLine *>(d->pdfAnnot.get());
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
        auto *lineann = static_cast<AnnotLine *>(d->pdfAnnot.get());
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
        const auto *lineann = static_cast<const AnnotLine *>(d->pdfAnnot.get());
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
        auto *lineann = static_cast<AnnotLine *>(d->pdfAnnot.get());
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
        const auto *lineann = static_cast<const AnnotLine *>(d->pdfAnnot.get());
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
        auto *lineann = static_cast<AnnotLine *>(d->pdfAnnot.get());
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
        const auto *lineann = static_cast<const AnnotLine *>(d->pdfAnnot.get());
        return (LineAnnotation::LineIntent)(lineann->getIntent() + 1);
    }
    const auto *polyann = static_cast<const AnnotPolygon *>(d->pdfAnnot.get());
    if (polyann->getIntent() == AnnotPolygon::polygonCloud) {
        return LineAnnotation::PolygonCloud;
    }
    // AnnotPolygon::polylineDimension, AnnotPolygon::polygonDimension
    return LineAnnotation::Dimension;
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
        auto *lineann = static_cast<AnnotLine *>(d->pdfAnnot.get());
        lineann->setIntent((AnnotLine::AnnotLineIntent)(intent - 1));
    } else {
        auto *polyann = static_cast<AnnotPolygon *>(d->pdfAnnot.get());
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
    GeomAnnotation::GeomType geomType = GeomAnnotation::InscribedSquare;
    QColor geomInnerColor;
};

GeomAnnotationPrivate::GeomAnnotationPrivate() = default;

std::unique_ptr<Annotation> GeomAnnotationPrivate::makeAlias()
{
    return std::unique_ptr<Annotation>(new GeomAnnotation(*this));
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

GeomAnnotation::~GeomAnnotation() = default;

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
    }
    // Annot::typeCircle
    return GeomAnnotation::InscribedCircle;
}

void GeomAnnotation::setGeomType(GeomAnnotation::GeomType type)
{
    Q_D(GeomAnnotation);

    if (!d->pdfAnnot) {
        d->geomType = type;
        return;
    }

    auto *geomann = static_cast<AnnotGeometry *>(d->pdfAnnot.get());
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

    const auto *geomann = static_cast<const AnnotGeometry *>(d->pdfAnnot.get());
    return convertAnnotColor(geomann->getInteriorColor());
}

void GeomAnnotation::setGeomInnerColor(const QColor &color)
{
    Q_D(GeomAnnotation);

    if (!d->pdfAnnot) {
        d->geomInnerColor = color;
        return;
    }

    auto *geomann = static_cast<AnnotGeometry *>(d->pdfAnnot.get());
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
    HighlightAnnotation::HighlightType highlightType = HighlightAnnotation::Highlight;
    QList<HighlightAnnotation::Quad> highlightQuads; // not empty

    // helpers
    static Annot::AnnotSubtype toAnnotSubType(HighlightAnnotation::HighlightType type);
    QList<HighlightAnnotation::Quad> fromQuadrilaterals(AnnotQuadrilaterals *quads) const;
    AnnotQuadrilaterals *toQuadrilaterals(const QList<HighlightAnnotation::Quad> &quads) const;
};

HighlightAnnotationPrivate::HighlightAnnotationPrivate() = default;

std::unique_ptr<Annotation> HighlightAnnotationPrivate::makeAlias()
{
    return std::unique_ptr<Annotation>(new HighlightAnnotation(*this));
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

HighlightAnnotation::~HighlightAnnotation() = default;

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
    }
    if (subType == Annot::typeUnderline) {
        return HighlightAnnotation::Underline;
    }
    if (subType == Annot::typeSquiggly) {
        return HighlightAnnotation::Squiggly;
    }
    // Annot::typeStrikeOut
    return HighlightAnnotation::StrikeOut;
}

void HighlightAnnotation::setHighlightType(HighlightAnnotation::HighlightType type)
{
    Q_D(HighlightAnnotation);

    if (!d->pdfAnnot) {
        d->highlightType = type;
        return;
    }

    auto *hlann = static_cast<AnnotTextMarkup *>(d->pdfAnnot.get());
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

    auto *hlann = static_cast<AnnotTextMarkup *>(d->pdfAnnot.get());
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
    return std::unique_ptr<Annotation>(new StampAnnotation(*this));
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

    if (sMaskData.size() > 0) {
        AnnotStampImageHelper sMask(parentDoc->doc.get(), width, height, ColorSpace::DeviceGray, 8, sMaskData.data(), sMaskData.size());
        annotImg = std::make_unique<AnnotStampImageHelper>(parentDoc->doc.get(), width, height, colorSpace, bitsPerComponent, data.data(), data.size(), sMask.getRef());
    } else {
        annotImg = std::make_unique<AnnotStampImageHelper>(parentDoc->doc.get(), width, height, colorSpace, bitsPerComponent, data.data(), data.size());
    }

    return annotImg;
}

StampAnnotation::StampAnnotation() : Annotation(*new StampAnnotationPrivate()) { }

StampAnnotation::StampAnnotation(StampAnnotationPrivate &dd) : Annotation(dd) { }

StampAnnotation::~StampAnnotation() = default;

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

    const auto *stampann = static_cast<const AnnotStamp *>(d->pdfAnnot.get());
    return QString::fromStdString(stampann->getIcon());
}

void StampAnnotation::setStampIconName(const QString &name)
{
    Q_D(StampAnnotation);

    if (!d->pdfAnnot) {
        d->stampIconName = name;
        return;
    }

    auto *stampann = static_cast<AnnotStamp *>(d->pdfAnnot.get());
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

    auto *stampann = static_cast<AnnotStamp *>(d->pdfAnnot.get());
    std::unique_ptr<AnnotStampImageHelper> annotCustomImage = d->convertQImageToAnnotStampImageHelper(image);
    stampann->setCustomImage(std::move(annotCustomImage));
}

/** SignatureAnnotation [Annotation] */
class SignatureAnnotationPrivate : public AnnotationPrivate
{
public:
    SignatureAnnotationPrivate();
    std::unique_ptr<Annotation> makeAlias() override;
    std::shared_ptr<Annot> createNativeAnnot(::Page *destPage, DocumentData *doc) override;

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
    ErrorString lastSigningErrorDetails;
    std::unique_ptr<::FormFieldSignature> field = nullptr;
};

SignatureAnnotationPrivate::SignatureAnnotationPrivate() = default;

std::unique_ptr<Annotation> SignatureAnnotationPrivate::makeAlias()
{
    return std::unique_ptr<Annotation>(new SignatureAnnotation(*this));
}

std::shared_ptr<Annot> SignatureAnnotationPrivate::createNativeAnnot(::Page *destPage, DocumentData *doc)
{
    std::unique_ptr<SignatureAnnotation> q = static_pointer_cast<SignatureAnnotation>(makeAlias());

    // Set page and document
    pdfPage = destPage;
    parentDoc = doc;

    // Set pdfAnnot
    PDFRectangle rect = boundaryToPdfRectangle(boundary, flags);

    std::unique_ptr<GooString> gSignatureText = std::unique_ptr<GooString>(QStringToUnicodeGooString(text));
    std::unique_ptr<GooString> gSignatureLeftText = std::unique_ptr<GooString>(QStringToUnicodeGooString(leftText));

    std::variant<PDFDoc::SignatureData, CryptoSign::SigningErrorMessage> result =
            destPage->getDoc()->createSignature(destPage, QStringToGooString(fieldPartialName), rect, *gSignatureText, *gSignatureLeftText, fontSize, leftFontSize, convertQColor(fontColor), borderWidth, convertQColor(borderColor),
                                                convertQColor(backgroundColor), imagePath.toStdString());

    if (std::holds_alternative<CryptoSign::SigningErrorMessage>(result)) {
        error(errInternal, -1, "Failed creating signature data, will crash shortly: {0:s}", std::get<CryptoSign::SigningErrorMessage>(result).message.text.c_str());
        return nullptr;
    }

    auto *sig = std::get_if<PDFDoc::SignatureData>(&result);

    sig->formWidget->updateWidgetAppearance();
    field = std::move(sig->field);

    // Set properties
    flushBaseAnnotationProperties();

    pdfAnnot = sig->annotWidget;

    return pdfAnnot;
}

SignatureAnnotation::SignatureAnnotation() : Annotation(*new SignatureAnnotationPrivate()) { }

SignatureAnnotation::SignatureAnnotation(SignatureAnnotationPrivate &dd) : Annotation(dd) { }

SignatureAnnotation::~SignatureAnnotation() = default;

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
    d->lastSigningErrorDetails = formField->lastSigningErrorDetails();

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
    case FormFieldSignature::BadPassphrase:
        return BadPassphrase;
    }
    return GenericSigningError;
}

Poppler::ErrorString SignatureAnnotation::lastSigningErrorDetails() const
{
    Q_D(const SignatureAnnotation);
    return d->lastSigningErrorDetails;
}

/** InkAnnotation [Annotation] */
class InkAnnotationPrivate : public AnnotationPrivate
{
public:
    InkAnnotationPrivate();
    std::unique_ptr<Annotation> makeAlias() override;
    std::shared_ptr<Annot> createNativeAnnot(::Page *destPage, DocumentData *doc) override;

    // data fields
    QList<QVector<QPointF>> inkPaths;

    // helper
    std::vector<std::unique_ptr<AnnotPath>> toAnnotPaths(const QList<QVector<QPointF>> &paths);
};

InkAnnotationPrivate::InkAnnotationPrivate() = default;

std::unique_ptr<Annotation> InkAnnotationPrivate::makeAlias()
{
    return std::unique_ptr<Annotation>(new InkAnnotation(*this));
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

InkAnnotation::~InkAnnotation() = default;

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

    const auto *inkann = static_cast<const AnnotInk *>(d->pdfAnnot.get());

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

    auto *inkann = static_cast<AnnotInk *>(d->pdfAnnot.get());
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
    std::unique_ptr<Link> linkDestination;
    LinkAnnotation::HighlightMode linkHLMode = LinkAnnotation::Invert;
    QPointF linkRegion[4];
};

LinkAnnotationPrivate::LinkAnnotationPrivate() = default;

LinkAnnotationPrivate::~LinkAnnotationPrivate() = default;

std::unique_ptr<Annotation> LinkAnnotationPrivate::makeAlias()
{
    return std::unique_ptr<Annotation>(new LinkAnnotation(*this));
}

std::shared_ptr<Annot> LinkAnnotationPrivate::createNativeAnnot(::Page * /*destPage*/, DocumentData * /*doc*/)
{
    return nullptr; // Not implemented
}

LinkAnnotation::LinkAnnotation() : Annotation(*new LinkAnnotationPrivate()) { }

LinkAnnotation::LinkAnnotation(LinkAnnotationPrivate &dd) : Annotation(dd) { }

LinkAnnotation::~LinkAnnotation() = default;

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
    std::unique_ptr<Annotation> makeAlias() override;
    std::shared_ptr<Annot> createNativeAnnot(::Page *destPage, DocumentData *doc) override;

    // data fields
    CaretAnnotation::CaretSymbol symbol = CaretAnnotation::None;
};

CaretAnnotationPrivate::CaretAnnotationPrivate() = default;

std::unique_ptr<Annotation> CaretAnnotationPrivate::makeAlias()
{
    return std::unique_ptr<Annotation>(new CaretAnnotation(*this));
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

CaretAnnotation::~CaretAnnotation() = default;

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

    const auto *caretann = static_cast<const AnnotCaret *>(d->pdfAnnot.get());
    return (CaretAnnotation::CaretSymbol)caretann->getSymbol();
}

void CaretAnnotation::setCaretSymbol(CaretAnnotation::CaretSymbol symbol)
{
    Q_D(CaretAnnotation);

    if (!d->pdfAnnot) {
        d->symbol = symbol;
        return;
    }

    auto *caretann = static_cast<AnnotCaret *>(d->pdfAnnot.get());
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
    EmbeddedFile *embfile = nullptr;
};

FileAttachmentAnnotationPrivate::FileAttachmentAnnotationPrivate() : icon(QStringLiteral("PushPin")) { }

FileAttachmentAnnotationPrivate::~FileAttachmentAnnotationPrivate()
{
    delete embfile;
}

std::unique_ptr<Annotation> FileAttachmentAnnotationPrivate::makeAlias()
{
    return std::unique_ptr<Annotation>(new FileAttachmentAnnotation(*this));
}

std::shared_ptr<Annot> FileAttachmentAnnotationPrivate::createNativeAnnot(::Page * /*destPage*/, DocumentData * /*doc*/)
{
    return nullptr; // Not implemented
}

FileAttachmentAnnotation::FileAttachmentAnnotation() : Annotation(*new FileAttachmentAnnotationPrivate()) { }

FileAttachmentAnnotation::FileAttachmentAnnotation(FileAttachmentAnnotationPrivate &dd) : Annotation(dd) { }

FileAttachmentAnnotation::~FileAttachmentAnnotation() = default;

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
    SoundObject *sound = nullptr;
};

SoundAnnotationPrivate::SoundAnnotationPrivate() : icon(QStringLiteral("Speaker")) { }

SoundAnnotationPrivate::~SoundAnnotationPrivate()
{
    delete sound;
}

std::unique_ptr<Annotation> SoundAnnotationPrivate::makeAlias()
{
    return std::unique_ptr<Annotation>(new SoundAnnotation(*this));
}

std::shared_ptr<Annot> SoundAnnotationPrivate::createNativeAnnot(::Page * /*destPage*/, DocumentData * /*doc*/)
{
    return nullptr; // Not implemented
}

SoundAnnotation::SoundAnnotation() : Annotation(*new SoundAnnotationPrivate()) { }

SoundAnnotation::SoundAnnotation(SoundAnnotationPrivate &dd) : Annotation(dd) { }

SoundAnnotation::~SoundAnnotation() = default;

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
    MovieObject *movie = nullptr;
    QString title;
};

MovieAnnotationPrivate::MovieAnnotationPrivate() = default;

MovieAnnotationPrivate::~MovieAnnotationPrivate()
{
    delete movie;
}

std::unique_ptr<Annotation> MovieAnnotationPrivate::makeAlias()
{
    return std::unique_ptr<Annotation>(new MovieAnnotation(*this));
}

std::shared_ptr<Annot> MovieAnnotationPrivate::createNativeAnnot(::Page * /*destPage*/, DocumentData * /*doc*/)
{
    return nullptr; // Not implemented
}

MovieAnnotation::MovieAnnotation() : Annotation(*new MovieAnnotationPrivate()) { }

MovieAnnotation::MovieAnnotation(MovieAnnotationPrivate &dd) : Annotation(dd) { }

MovieAnnotation::~MovieAnnotation() = default;

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
    LinkRendition *action = nullptr;
    QString title;
};

ScreenAnnotationPrivate::ScreenAnnotationPrivate() = default;

ScreenAnnotationPrivate::~ScreenAnnotationPrivate()
{
    delete action;
}

ScreenAnnotation::ScreenAnnotation(ScreenAnnotationPrivate &dd) : Annotation(dd) { }

std::unique_ptr<Annotation> ScreenAnnotationPrivate::makeAlias()
{
    return std::unique_ptr<Annotation>(new ScreenAnnotation(*this));
}

std::shared_ptr<Annot> ScreenAnnotationPrivate::createNativeAnnot(::Page * /*destPage*/, DocumentData * /*doc*/)
{
    return nullptr; // Not implemented
}

ScreenAnnotation::ScreenAnnotation() : Annotation(*new ScreenAnnotationPrivate()) { }

ScreenAnnotation::~ScreenAnnotation() = default;

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
    std::unique_ptr<Annotation> makeAlias() override;
    std::shared_ptr<Annot> createNativeAnnot(::Page *destPage, DocumentData *doc) override;
};

std::unique_ptr<Annotation> WidgetAnnotationPrivate::makeAlias()
{
    return std::unique_ptr<Annotation>(new WidgetAnnotation(*this));
}

std::shared_ptr<Annot> WidgetAnnotationPrivate::createNativeAnnot(::Page * /*destPage*/, DocumentData * /*doc*/)
{
    return nullptr; // Not implemented
}

WidgetAnnotation::WidgetAnnotation(WidgetAnnotationPrivate &dd) : Annotation(dd) { }

WidgetAnnotation::WidgetAnnotation() : Annotation(*new WidgetAnnotationPrivate()) { }

WidgetAnnotation::~WidgetAnnotation() = default;

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
    Private() = default;

    ~Private() { delete params; }

    Private(const Private &) = delete;
    Private &operator=(const Private &) = delete;

    RichMediaAnnotation::Instance::Type type;
    RichMediaAnnotation::Params *params = nullptr;
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
    Private() = default;

    ~Private() { delete embeddedFile; }

    Private(const Private &) = delete;
    Private &operator=(const Private &) = delete;

    QString name;
    EmbeddedFile *embeddedFile = nullptr;
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
    Private() = default;

    RichMediaAnnotation::Activation::Condition condition = RichMediaAnnotation::Activation::UserAction;
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
    Private() = default;

    RichMediaAnnotation::Deactivation::Condition condition = RichMediaAnnotation::Deactivation::UserAction;
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
    Private() = default;

    RichMediaAnnotation::Activation *activation = nullptr;
    RichMediaAnnotation::Deactivation *deactivation = nullptr;
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
    RichMediaAnnotationPrivate() = default;

    ~RichMediaAnnotationPrivate() override;

    std::unique_ptr<Annotation> makeAlias() override { return std::unique_ptr<Annotation>(new RichMediaAnnotation(*this)); }

    std::shared_ptr<Annot> createNativeAnnot(::Page *destPage, DocumentData *doc) override
    {
        Q_UNUSED(destPage);
        Q_UNUSED(doc);

        return nullptr;
    }

    RichMediaAnnotation::Settings *settings = nullptr;
    RichMediaAnnotation::Content *content = nullptr;
};

RichMediaAnnotationPrivate::~RichMediaAnnotationPrivate()
{
    delete settings;
    delete content;
}

RichMediaAnnotation::RichMediaAnnotation() : Annotation(*new RichMediaAnnotationPrivate()) { }

RichMediaAnnotation::RichMediaAnnotation(RichMediaAnnotationPrivate &dd) : Annotation(dd) { }

RichMediaAnnotation::~RichMediaAnnotation() = default;

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
    const std::array<double, 4> &color_data = color->getValues();
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
