/* poppler-link.cc: qt interface to poppler
 * Copyright (C) 2006-2007, 2013, 2016-2021, 2024, Albert Astals Cid
 * Copyright (C) 2007-2008, Pino Toscano <pino@kde.org>
 * Copyright (C) 2010 Hib Eris <hib@hiberis.nl>
 * Copyright (C) 2012, Tobias Koenig <tokoe@kdab.com>
 * Copyright (C) 2012, Guillermo A. Amaral B. <gamaral@kde.org>
 * Copyright (C) 2018 Intevation GmbH <intevation@intevation.de>
 * Copyright (C) 2018 Adam Reichold <adam.reichold@t-online.de>
 * Copyright (C) 2020, 2021 Oliver Sander <oliver.sander@tu-dresden.de>
 * Copyright (C) 2024 Pratham Gandhi <ppg.1382@gmail.com>
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

#include <poppler-qt6.h>
#include <poppler-link-private.h>
#include <poppler-private.h>
#include <poppler-media.h>

#include <QtCore/QStringList>
#include <utility>

#include "poppler-annotation-private.h"

#include "Link.h"
#include "Rendition.h"

namespace Poppler {

class LinkDestinationPrivate : public QSharedData
{
public:
    LinkDestinationPrivate();

    LinkDestination::Kind kind; // destination type
    QString name;
    int pageNum; // page number
    double left, bottom; // position
    double right, top;
    double zoom; // zoom factor
    bool changeLeft : 1, changeTop : 1; // for destXYZ links, which position
    bool changeZoom : 1; //   components to change
};

LinkDestinationPrivate::LinkDestinationPrivate()
{
    // sane defaults
    kind = LinkDestination::destXYZ;
    pageNum = 0;
    left = 0;
    bottom = 0;
    right = 0;
    top = 0;
    zoom = 1;
    changeLeft = true;
    changeTop = true;
    changeZoom = false;
}

LinkPrivate::~LinkPrivate() = default;

LinkOCGStatePrivate::~LinkOCGStatePrivate() = default;

LinkHidePrivate::~LinkHidePrivate() = default;

LinkResetFormPrivate::~LinkResetFormPrivate() = default;

LinkSubmitFormPrivate::~LinkSubmitFormPrivate() = default;

class LinkGotoPrivate : public LinkPrivate
{
public:
    LinkGotoPrivate(const QRectF &area, const LinkDestination &dest);
    ~LinkGotoPrivate() override;

    QString extFileName;
    LinkDestination destination;
};

LinkGotoPrivate::LinkGotoPrivate(const QRectF &area, const LinkDestination &dest) : LinkPrivate(area), destination(dest) { }

LinkGotoPrivate::~LinkGotoPrivate() = default;

class LinkExecutePrivate : public LinkPrivate
{
public:
    explicit LinkExecutePrivate(const QRectF &area);
    ~LinkExecutePrivate() override;

    QString fileName;
    QString parameters;
};

LinkExecutePrivate::LinkExecutePrivate(const QRectF &area) : LinkPrivate(area) { }

LinkExecutePrivate::~LinkExecutePrivate() = default;

class LinkBrowsePrivate : public LinkPrivate
{
public:
    explicit LinkBrowsePrivate(const QRectF &area);
    ~LinkBrowsePrivate() override;

    QString url;
};

LinkBrowsePrivate::LinkBrowsePrivate(const QRectF &area) : LinkPrivate(area) { }

LinkBrowsePrivate::~LinkBrowsePrivate() = default;

class LinkActionPrivate : public LinkPrivate
{
public:
    explicit LinkActionPrivate(const QRectF &area);
    ~LinkActionPrivate() override;

    LinkAction::ActionType type;
};

LinkActionPrivate::LinkActionPrivate(const QRectF &area) : LinkPrivate(area) { }

LinkActionPrivate::~LinkActionPrivate() = default;

class LinkSoundPrivate : public LinkPrivate
{
public:
    explicit LinkSoundPrivate(const QRectF &area);
    ~LinkSoundPrivate() override;

    double volume;
    bool sync : 1;
    bool repeat : 1;
    bool mix : 1;
    SoundObject *sound = nullptr;
};

LinkSoundPrivate::LinkSoundPrivate(const QRectF &area) : LinkPrivate(area) { }

LinkSoundPrivate::~LinkSoundPrivate()
{
    delete sound;
}

class LinkRenditionPrivate : public LinkPrivate
{
public:
    LinkRenditionPrivate(const QRectF &area, std::unique_ptr<::MediaRendition> &&r, ::LinkRendition::RenditionOperation operation, QString script, Ref ref);
    ~LinkRenditionPrivate() override;

    std::unique_ptr<MediaRendition> rendition;
    LinkRendition::RenditionAction action = LinkRendition::PlayRendition;
    QString script;
    Ref annotationReference;
};

LinkRenditionPrivate::LinkRenditionPrivate(const QRectF &area, std::unique_ptr<::MediaRendition> &&r, ::LinkRendition::RenditionOperation operation, QString javaScript, const Ref ref)
    : LinkPrivate(area), rendition(r ? new MediaRendition(std::move(r)) : nullptr), script(std::move(javaScript)), annotationReference(ref)
{
    switch (operation) {
    case ::LinkRendition::NoRendition:
        action = LinkRendition::NoRendition;
        break;
    case ::LinkRendition::PlayRendition:
        action = LinkRendition::PlayRendition;
        break;
    case ::LinkRendition::StopRendition:
        action = LinkRendition::StopRendition;
        break;
    case ::LinkRendition::PauseRendition:
        action = LinkRendition::PauseRendition;
        break;
    case ::LinkRendition::ResumeRendition:
        action = LinkRendition::ResumeRendition;
        break;
    }
}

LinkRenditionPrivate::~LinkRenditionPrivate() = default;

class LinkJavaScriptPrivate : public LinkPrivate
{
public:
    explicit LinkJavaScriptPrivate(const QRectF &area);
    ~LinkJavaScriptPrivate() override;

    QString js;
};

LinkJavaScriptPrivate::LinkJavaScriptPrivate(const QRectF &area) : LinkPrivate(area) { }

LinkJavaScriptPrivate::~LinkJavaScriptPrivate() = default;

class LinkMoviePrivate : public LinkPrivate
{
public:
    LinkMoviePrivate(const QRectF &area, LinkMovie::Operation operation, QString title, Ref reference);
    ~LinkMoviePrivate() override;

    LinkMovie::Operation operation;
    QString annotationTitle;
    Ref annotationReference;
};

LinkMoviePrivate::LinkMoviePrivate(const QRectF &area, LinkMovie::Operation _operation, QString title, const Ref reference) : LinkPrivate(area), operation(_operation), annotationTitle(std::move(title)), annotationReference(reference) { }

LinkMoviePrivate::~LinkMoviePrivate() = default;

static void cvtUserToDev(::Page *page, double xu, double yu, int *xd, int *yd)
{
    double ctm[6];

    page->getDefaultCTM(ctm, 72.0, 72.0, 0, false, true);
    *xd = (int)(ctm[0] * xu + ctm[2] * yu + ctm[4] + 0.5);
    *yd = (int)(ctm[1] * xu + ctm[3] * yu + ctm[5] + 0.5);
}

LinkDestination::LinkDestination(const LinkDestinationData &data) : d(new LinkDestinationPrivate)
{
    bool deleteDest = false;
    const LinkDest *ld = data.ld;

    if (data.namedDest && !ld && !data.externalDest) {
        deleteDest = true;
        ld = data.doc->doc->findDest(data.namedDest).release();
    }
    // in case this destination was named one, and it was not resolved
    if (data.namedDest && !ld) {
        d->name = QString::fromLatin1(data.namedDest->c_str());
    }

    if (!ld) {
        return;
    }

    if (ld->getKind() == ::destXYZ) {
        d->kind = destXYZ;
    } else if (ld->getKind() == ::destFit) {
        d->kind = destFit;
    } else if (ld->getKind() == ::destFitH) {
        d->kind = destFitH;
    } else if (ld->getKind() == ::destFitV) {
        d->kind = destFitV;
    } else if (ld->getKind() == ::destFitR) {
        d->kind = destFitR;
    } else if (ld->getKind() == ::destFitB) {
        d->kind = destFitB;
    } else if (ld->getKind() == ::destFitBH) {
        d->kind = destFitBH;
    } else if (ld->getKind() == ::destFitBV) {
        d->kind = destFitBV;
    }

    if (!ld->isPageRef()) {
        d->pageNum = ld->getPageNum();
    } else {
        const Ref ref = ld->getPageRef();
        d->pageNum = data.doc->doc->findPage(ref);
    }
    double left = ld->getLeft();
    double bottom = ld->getBottom();
    double right = ld->getRight();
    double top = ld->getTop();
    d->zoom = ld->getZoom();
    d->changeLeft = ld->getChangeLeft();
    d->changeTop = ld->getChangeTop();
    d->changeZoom = ld->getChangeZoom();

    int leftAux = 0, topAux = 0, rightAux = 0, bottomAux = 0;

    if (!data.externalDest) {
        ::Page *page;
        if (d->pageNum > 0 && d->pageNum <= data.doc->doc->getNumPages() && (page = data.doc->doc->getPage(d->pageNum))) {
            cvtUserToDev(page, left, top, &leftAux, &topAux);
            cvtUserToDev(page, right, bottom, &rightAux, &bottomAux);

            d->left = leftAux / page->getCropWidth();
            d->top = topAux / page->getCropHeight();
            d->right = rightAux / page->getCropWidth();
            d->bottom = bottomAux / page->getCropHeight();
        } else {
            d->pageNum = 0;
        }
    }

    if (deleteDest) {
        delete ld;
    }
}

LinkDestination::LinkDestination(const QString &description) : d(new LinkDestinationPrivate)
{
    const QStringList tokens = description.split(QChar::fromLatin1(';'));
    if (tokens.size() >= 10) {
        d->kind = static_cast<Kind>(tokens.at(0).toInt());
        d->pageNum = tokens.at(1).toInt();
        d->left = tokens.at(2).toDouble();
        d->bottom = tokens.at(3).toDouble();
        d->right = tokens.at(4).toDouble();
        d->top = tokens.at(5).toDouble();
        d->zoom = tokens.at(6).toDouble();
        d->changeLeft = static_cast<bool>(tokens.at(7).toInt());
        d->changeTop = static_cast<bool>(tokens.at(8).toInt());
        d->changeZoom = static_cast<bool>(tokens.at(9).toInt());
    }
}

LinkDestination::LinkDestination(const LinkDestination &other) = default;

LinkDestination::~LinkDestination() = default;

LinkDestination::Kind LinkDestination::kind() const
{
    return d->kind;
}

int LinkDestination::pageNumber() const
{
    return d->pageNum;
}

double LinkDestination::left() const
{
    return d->left;
}

double LinkDestination::bottom() const
{
    return d->bottom;
}

double LinkDestination::right() const
{
    return d->right;
}

double LinkDestination::top() const
{
    return d->top;
}

double LinkDestination::zoom() const
{
    return d->zoom;
}

bool LinkDestination::isChangeLeft() const
{
    return d->changeLeft;
}

bool LinkDestination::isChangeTop() const
{
    return d->changeTop;
}

bool LinkDestination::isChangeZoom() const
{
    return d->changeZoom;
}

QString LinkDestination::toString() const
{
    const QChar semicolon = QChar::fromLatin1(';');
    QString s = QString::number((qint8)d->kind);
    s += semicolon + QString::number(d->pageNum);
    s += semicolon + QString::number(d->left);
    s += semicolon + QString::number(d->bottom);
    s += semicolon + QString::number(d->right);
    s += semicolon + QString::number(d->top);
    s += semicolon + QString::number(d->zoom);
    s += semicolon + QString::number((qint8)d->changeLeft);
    s += semicolon + QString::number((qint8)d->changeTop);
    s += semicolon + QString::number((qint8)d->changeZoom);
    return s;
}

QString LinkDestination::destinationName() const
{
    return d->name;
}

LinkDestination &LinkDestination::operator=(const LinkDestination &other)
{
    if (this == &other) {
        return *this;
    }

    d = other.d;
    return *this;
}

// Link
Link::~Link()
{
    delete d_ptr;
}

Link::Link(const QRectF &linkArea) : d_ptr(new LinkPrivate(linkArea)) { }

Link::Link(LinkPrivate &dd) : d_ptr(&dd) { }

Link::LinkType Link::linkType() const
{
    return None;
}

QRectF Link::linkArea() const
{
    Q_D(const Link);
    return d->linkArea;
}

QVector<Link *> Link::nextLinks() const
{
    QVector<Link *> links(d_ptr->nextLinks.size());
    for (qsizetype i = 0; i < links.size(); i++) {
        links[i] = d_ptr->nextLinks[i].get();
    }

    return links;
}

// LinkGoto
LinkGoto::LinkGoto(const QRectF &linkArea, const QString &extFileName, const LinkDestination &destination) : Link(*new LinkGotoPrivate(linkArea, destination))
{
    Q_D(LinkGoto);
    d->extFileName = extFileName;
}

LinkGoto::~LinkGoto() = default;

bool LinkGoto::isExternal() const
{
    Q_D(const LinkGoto);
    return !d->extFileName.isEmpty();
}

QString LinkGoto::fileName() const
{
    Q_D(const LinkGoto);
    return d->extFileName;
}

LinkDestination LinkGoto::destination() const
{
    Q_D(const LinkGoto);
    return d->destination;
}

Link::LinkType LinkGoto::linkType() const
{
    return Goto;
}

// LinkExecute
LinkExecute::LinkExecute(const QRectF &linkArea, const QString &file, const QString &params) : Link(*new LinkExecutePrivate(linkArea))
{
    Q_D(LinkExecute);
    d->fileName = file;
    d->parameters = params;
}

LinkExecute::~LinkExecute() = default;

QString LinkExecute::fileName() const
{
    Q_D(const LinkExecute);
    return d->fileName;
}
QString LinkExecute::parameters() const
{
    Q_D(const LinkExecute);
    return d->parameters;
}

Link::LinkType LinkExecute::linkType() const
{
    return Execute;
}

// LinkBrowse
LinkBrowse::LinkBrowse(const QRectF &linkArea, const QString &url) : Link(*new LinkBrowsePrivate(linkArea))
{
    Q_D(LinkBrowse);
    d->url = url;
}

LinkBrowse::~LinkBrowse() = default;

QString LinkBrowse::url() const
{
    Q_D(const LinkBrowse);
    return d->url;
}

Link::LinkType LinkBrowse::linkType() const
{
    return Browse;
}

// LinkAction
LinkAction::LinkAction(const QRectF &linkArea, ActionType actionType) : Link(*new LinkActionPrivate(linkArea))
{
    Q_D(LinkAction);
    d->type = actionType;
}

LinkAction::~LinkAction() = default;

LinkAction::ActionType LinkAction::actionType() const
{
    Q_D(const LinkAction);
    return d->type;
}

Link::LinkType LinkAction::linkType() const
{
    return Action;
}

// LinkSound
LinkSound::LinkSound(const QRectF &linkArea, double volume, bool sync, bool repeat, bool mix, SoundObject *sound) : Link(*new LinkSoundPrivate(linkArea))
{
    Q_D(LinkSound);
    d->volume = volume;
    d->sync = sync;
    d->repeat = repeat;
    d->mix = mix;
    d->sound = sound;
}

LinkSound::~LinkSound() = default;

Link::LinkType LinkSound::linkType() const
{
    return Sound;
}

double LinkSound::volume() const
{
    Q_D(const LinkSound);
    return d->volume;
}

bool LinkSound::synchronous() const
{
    Q_D(const LinkSound);
    return d->sync;
}

bool LinkSound::repeat() const
{
    Q_D(const LinkSound);
    return d->repeat;
}

bool LinkSound::mix() const
{
    Q_D(const LinkSound);
    return d->mix;
}

SoundObject *LinkSound::sound() const
{
    Q_D(const LinkSound);
    return d->sound;
}

// LinkRendition
LinkRendition::LinkRendition(const QRectF &linkArea, ::MediaRendition *rendition, int operation, const QString &script, const Ref annotationReference)
    : LinkRendition(linkArea, std::unique_ptr<::MediaRendition>(rendition), operation, script, annotationReference)
{
}

LinkRendition::LinkRendition(const QRectF &linkArea, std::unique_ptr<::MediaRendition> &&rendition, int operation, const QString &script, const Ref annotationReference)
    : Link(*new LinkRenditionPrivate(linkArea, std::move(rendition), static_cast<enum ::LinkRendition::RenditionOperation>(operation), script, annotationReference))
{
}

LinkRendition::~LinkRendition() = default;

Link::LinkType LinkRendition::linkType() const
{
    return Rendition;
}

MediaRendition *LinkRendition::rendition() const
{
    Q_D(const LinkRendition);
    return d->rendition.get();
}

LinkRendition::RenditionAction LinkRendition::action() const
{
    Q_D(const LinkRendition);
    return d->action;
}

QString LinkRendition::script() const
{
    Q_D(const LinkRendition);
    return d->script;
}

bool LinkRendition::isReferencedAnnotation(const ScreenAnnotation *annotation) const
{
    Q_D(const LinkRendition);
    return d->annotationReference != Ref::INVALID() && d->annotationReference == annotation->d_ptr->pdfObjectReference();
}

// LinkJavaScript
LinkJavaScript::LinkJavaScript(const QRectF &linkArea, const QString &js) : Link(*new LinkJavaScriptPrivate(linkArea))
{
    Q_D(LinkJavaScript);
    d->js = js;
}

LinkJavaScript::~LinkJavaScript() = default;

Link::LinkType LinkJavaScript::linkType() const
{
    return JavaScript;
}

QString LinkJavaScript::script() const
{
    Q_D(const LinkJavaScript);
    return d->js;
}

// LinkMovie
LinkMovie::LinkMovie(const QRectF &linkArea, Operation operation, const QString &annotationTitle, const Ref annotationReference) : Link(*new LinkMoviePrivate(linkArea, operation, annotationTitle, annotationReference)) { }

LinkMovie::~LinkMovie() = default;

Link::LinkType LinkMovie::linkType() const
{
    return Movie;
}

LinkMovie::Operation LinkMovie::operation() const
{
    Q_D(const LinkMovie);
    return d->operation;
}

bool LinkMovie::isReferencedAnnotation(const MovieAnnotation *annotation) const
{
    Q_D(const LinkMovie);
    if (d->annotationReference != Ref::INVALID() && d->annotationReference == annotation->d_ptr->pdfObjectReference()) {
        return true;
    }
    if (!d->annotationTitle.isNull()) {
        return (annotation->movieTitle() == d->annotationTitle);
    }

    return false;
}

LinkOCGState::LinkOCGState(LinkOCGStatePrivate *ocgp) : Link(*ocgp) { }

LinkOCGState::~LinkOCGState() = default;

Link::LinkType LinkOCGState::linkType() const
{
    return OCGState;
}

// LinkHide
LinkHide::LinkHide(LinkHidePrivate *lhidep) : Link(*lhidep) { }

LinkHide::~LinkHide() = default;

Link::LinkType LinkHide::linkType() const
{
    return Hide;
}

QVector<QString> LinkHide::targets() const
{
    Q_D(const LinkHide);
    return QVector<QString>() << d->targetName;
}

bool LinkHide::isShowAction() const
{
    Q_D(const LinkHide);
    return d->isShow;
}

// LinkResetForm
LinkResetForm::LinkResetForm(LinkResetFormPrivate *lrfp) : Link(*lrfp) { }

LinkResetForm::~LinkResetForm() = default;

Link::LinkType LinkResetForm::linkType() const
{
    return ResetForm;
}

// LinkSubmitForm
// static assertions to ensure flags from Link.h match those from poppler-link.h
static_assert(static_cast<int>(LinkSubmitForm::NoOpFlag) == static_cast<int>(::LinkSubmitForm::NoOpFlag), "NoOpFlag does not match");
static_assert(static_cast<int>(LinkSubmitForm::ExcludeFlag) == static_cast<int>(::LinkSubmitForm::ExcludeFlag), "ExcludeFlag does not match");
static_assert(static_cast<int>(LinkSubmitForm::IncludeNoValueFieldsFlag) == static_cast<int>(::LinkSubmitForm::IncludeNoValueFieldsFlag), "IncludeNoValueFieldsFlag does not match");
static_assert(static_cast<int>(LinkSubmitForm::ExportFormatFlag) == static_cast<int>(::LinkSubmitForm::ExportFormatFlag), "ExportFormatFlag does not match");
static_assert(static_cast<int>(LinkSubmitForm::GetMethodFlag) == static_cast<int>(::LinkSubmitForm::GetMethodFlag), "GetMethodFlag does not match");
static_assert(static_cast<int>(LinkSubmitForm::SubmitCoordinatesFlag) == static_cast<int>(::LinkSubmitForm::SubmitCoordinatesFlag), "SubmitCoordinatesFlag does not match");
static_assert(static_cast<int>(LinkSubmitForm::XFDFFlag) == static_cast<int>(::LinkSubmitForm::XFDFFlag), "XFDFFlag does not match");
static_assert(static_cast<int>(LinkSubmitForm::IncludeAppendSavesFlag) == static_cast<int>(::LinkSubmitForm::IncludeAppendSavesFlag), "IncludeAppendSavesFlag does not match");
static_assert(static_cast<int>(LinkSubmitForm::IncludeAnnotationsFlag) == static_cast<int>(::LinkSubmitForm::IncludeAnnotationsFlag), "IncludeAnnotationsFlag does not match");
static_assert(static_cast<int>(LinkSubmitForm::SubmitPDFFlag) == static_cast<int>(::LinkSubmitForm::SubmitPDFFlag), "SubmitPDFFlag does not match");
static_assert(static_cast<int>(LinkSubmitForm::CanonicalFormatFlag) == static_cast<int>(::LinkSubmitForm::CanonicalFormatFlag), "CanonicalFormatFlag does not match");
static_assert(static_cast<int>(LinkSubmitForm::ExclNonUserAnnotsFlag) == static_cast<int>(::LinkSubmitForm::ExclNonUserAnnotsFlag), "ExclNonUserAnnotsFlag does not match");
static_assert(static_cast<int>(LinkSubmitForm::ExclFKeyFlag) == static_cast<int>(::LinkSubmitForm::ExclFKeyFlag), "ExclFKeyFlag does not match");
static_assert(static_cast<int>(LinkSubmitForm::EmbedFormFlag) == static_cast<int>(::LinkSubmitForm::EmbedFormFlag), "EmbedFormFlag does not match");

LinkSubmitForm::LinkSubmitForm(LinkSubmitFormPrivate *lsfp) : Link(*lsfp) { }

LinkSubmitForm::~LinkSubmitForm() = default;

Link::LinkType LinkSubmitForm::linkType() const
{
    return SubmitForm;
}

QVector<int> LinkSubmitForm::getFieldIds() const
{
    Q_D(const LinkSubmitForm);
    return d->m_fieldIds;
}

QString LinkSubmitForm::getUrl() const
{
    Q_D(const LinkSubmitForm);
    return d->m_url;
}

LinkSubmitForm::SubmitFormFlags LinkSubmitForm::getFlags() const
{
    Q_D(const LinkSubmitForm);
    return d->m_flags;
}
}
