/* poppler-link.h: qt interface to poppler
 * Copyright (C) 2006, 2013, 2016, 2018, 2019, 2021, 2022, 2024, Albert Astals Cid <aacid@kde.org>
 * Copyright (C) 2007-2008, 2010, Pino Toscano <pino@kde.org>
 * Copyright (C) 2010, 2012, Guillermo Amaral <gamaral@kdab.com>
 * Copyright (C) 2012, Tobias Koenig <tokoe@kdab.com>
 * Copyright (C) 2013, Anthony Granger <grangeranthony@gmail.com>
 * Copyright (C) 2018 Intevation GmbH <intevation@intevation.de>
 * Copyright (C) 2020 Oliver Sander <oliver.sander@tu-dresden.de>
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

#ifndef _POPPLER_LINK_H_
#define _POPPLER_LINK_H_

#include <QtCore/QString>
#include <QtCore/QRectF>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QVector>
#include "poppler-export.h"

#include <memory>

struct Ref;
class MediaRendition;

namespace Poppler {

class LinkPrivate;
class LinkGotoPrivate;
class LinkExecutePrivate;
class LinkBrowsePrivate;
class LinkActionPrivate;
class LinkSoundPrivate;
class LinkJavaScriptPrivate;
class LinkMoviePrivate;
class LinkDestinationData;
class LinkDestinationPrivate;
class LinkRenditionPrivate;
class LinkOCGStatePrivate;
class LinkHidePrivate;
class LinkResetFormPrivate;
class LinkSubmitFormPrivate;
class MediaRendition;
class MovieAnnotation;
class ScreenAnnotation;
class SoundObject;

/**
 * \short A destination.
 *
 * The LinkDestination class represent a "destination" (in terms of visual
 * viewport to be displayed) for \link Poppler::LinkGoto GoTo\endlink links,
 * and items in the table of contents (TOC) of a document.
 *
 * Coordinates are in 0..1 range
 */
class POPPLER_QT5_EXPORT LinkDestination
{
public:
    /**
     * The possible kind of "viewport destination".
     */
    enum Kind
    {
        /**
         * The new viewport is specified in terms of:
         * - possible new left coordinate (see isChangeLeft() )
         * - possible new top coordinate (see isChangeTop() )
         * - possible new zoom level (see isChangeZoom() )
         */
        destXYZ = 1,
        destFit = 2,
        destFitH = 3,
        destFitV = 4,
        destFitR = 5,
        destFitB = 6,
        destFitBH = 7,
        destFitBV = 8
    };

    /// \cond PRIVATE
    explicit LinkDestination(const LinkDestinationData &data);
    explicit LinkDestination(const QString &description);
    /// \endcond
    /**
     * Copy constructor.
     */
    LinkDestination(const LinkDestination &other);
    /**
     * Destructor.
     */
    ~LinkDestination();

    // Accessors.
    /**
     * The kind of destination.
     */
    Kind kind() const;
    /**
     * Which page is the target of this destination.
     *
     * \note this number is 1-based, so for a 5 pages document the
     *       valid page numbers go from 1 to 5 (both included).
     */
    int pageNumber() const;
    /**
     * The new left for the viewport of the target page, in case
     * it is specified to be changed (see isChangeLeft() )
     */
    double left() const;
    double bottom() const;
    double right() const;
    /**
     * The new top for the viewport of the target page, in case
     * it is specified to be changed (see isChangeTop() )
     */
    double top() const;
    double zoom() const;
    /**
     * Whether the left of the viewport on the target page should
     * be changed.
     *
     * \see left()
     */
    bool isChangeLeft() const;
    /**
     * Whether the top of the viewport on the target page should
     * be changed.
     *
     * \see top()
     */
    bool isChangeTop() const;
    /**
     * Whether the zoom level should be changed.
     *
     * \see zoom()
     */
    bool isChangeZoom() const;

    /**
     * Return a string repesentation of this destination.
     */
    QString toString() const;

    /**
     * Return the name of this destination.
     *
     * \since 0.12
     */
    QString destinationName() const;

    /**
     * Assignment operator.
     */
    LinkDestination &operator=(const LinkDestination &other);

private:
    QSharedDataPointer<LinkDestinationPrivate> d;
};

/**
 * \short Encapsulates data that describes a link.
 *
 * This is the base class for links. It makes mandatory for inherited
 * kind of links to reimplement the linkType() method and return the type of
 * the link described by the reimplemented class.
 */
class POPPLER_QT5_EXPORT Link
{
public:
    /// \cond PRIVATE
    explicit Link(const QRectF &linkArea);
    /// \endcond

    /**
     * The possible kinds of link.
     *
     * Inherited classes must return an unique identifier
     */
    enum LinkType
    {
        None, ///< Unknown link
        Goto, ///< A "Go To" link
        Execute, ///< A command to be executed
        Browse, ///< An URL to be browsed (eg "http://poppler.freedesktop.org")
        Action, ///< A "standard" action to be executed in the viewer
        Sound, ///< A link representing a sound to be played
        Movie, ///< An action to be executed on a movie
        Rendition, ///< A rendition link \since 0.20
        JavaScript, ///< A JavaScript code to be interpreted \since 0.10
        OCGState, ///< An Optional Content Group state change \since 0.50
        Hide, ///< An action to hide a field \since 0.64
        ResetForm, ///< An action to reset the form \since 24.07
        SubmitForm, ///< An action to submit a form \since 24.10
    };

    /**
     * The type of this link.
     */
    virtual LinkType linkType() const;

    /**
     * Destructor.
     */
    virtual ~Link();

    /**
     * The area of a Page where the link should be active.
     *
     * \note this can be a null rect, in this case the link represents
     * a general action. The area is given in 0..1 range
     */
    QRectF linkArea() const;

    /**
     * Get the next links to be activated / executed after this link.
     *
     * \since 0.64
     */
    QVector<Link *> nextLinks() const;

protected:
    /// \cond PRIVATE
    explicit Link(LinkPrivate &dd);
    Q_DECLARE_PRIVATE(Link)
    LinkPrivate *d_ptr;
    /// \endcond

private:
    Q_DISABLE_COPY(Link)
};

/**
 * \brief Viewport reaching request.
 *
 * With a LinkGoto link, the document requests the specified viewport to be
 * reached (aka, displayed in a viewer). Furthermore, if a file name is specified,
 * then the destination refers to that document (and not to the document the
 * current LinkGoto belongs to).
 */
class POPPLER_QT5_EXPORT LinkGoto : public Link
{
public:
    /**
     * Create a new Goto link.
     *
     * \param linkArea the active area of the link
     * \param extFileName if not empty, the file name to be open
     * \param destination the destination to be reached
     */
    // TODO Next ABI break, make extFileName const &
    LinkGoto(const QRectF &linkArea, QString extFileName, const LinkDestination &destination);
    /**
     * Destructor.
     */
    ~LinkGoto() override;

    /**
     * Whether the destination is in an external document
     * (i.e. not the current document)
     */
    bool isExternal() const;
    // query for goto parameters
    /**
     * The file name of the document the destination() refers to,
     * or an empty string in case it refers to the current document.
     */
    QString fileName() const;
    /**
     * The destination to reach.
     */
    LinkDestination destination() const;
    LinkType linkType() const override;

private:
    Q_DECLARE_PRIVATE(LinkGoto)
    Q_DISABLE_COPY(LinkGoto)
};

/**
 * \brief Generic execution request.
 *
 * The LinkExecute link represent a "file name" execution request. The result
 * depends on the \link fileName() file name\endlink:
 * - if it is a document, then it is requested to be open
 * - otherwise, it represents an executable to be run with the specified parameters
 */
class POPPLER_QT5_EXPORT LinkExecute : public Link
{
public:
    /**
     * The file name to be executed
     */
    QString fileName() const;
    /**
     * The parameters for the command.
     */
    QString parameters() const;

    /**
     * Create a new Execute link.
     *
     * \param linkArea the active area of the link
     * \param file the file name to be open, or the program to be execute
     * \param params the parameters for the program to execute
     */
    LinkExecute(const QRectF &linkArea, const QString &file, const QString &params);
    /**
     * Destructor.
     */
    ~LinkExecute() override;
    LinkType linkType() const override;

private:
    Q_DECLARE_PRIVATE(LinkExecute)
    Q_DISABLE_COPY(LinkExecute)
};

/**
 * \brief An URL to browse.
 *
 * The LinkBrowse link holds a URL (eg 'http://poppler.freedesktop.org',
 * 'mailto:john@some.org', etc) to be open.
 *
 * The format of the URL is specified by RFC 2396 (http://www.ietf.org/rfc/rfc2396.txt)
 */
class POPPLER_QT5_EXPORT LinkBrowse : public Link
{
public:
    /**
     * The URL to open
     */
    QString url() const;

    /**
     * Create a new browse link.
     *
     * \param linkArea the active area of the link
     * \param url the URL to be open
     */
    LinkBrowse(const QRectF &linkArea, const QString &url);
    /**
     * Destructor.
     */
    ~LinkBrowse() override;
    LinkType linkType() const override;

private:
    Q_DECLARE_PRIVATE(LinkBrowse)
    Q_DISABLE_COPY(LinkBrowse)
};

/**
 * \brief "Standard" action request.
 *
 * The LinkAction class represents a link that request a "standard" action
 * to be performed by the viewer on the displayed document.
 */
class POPPLER_QT5_EXPORT LinkAction : public Link
{
public:
    /**
     * The possible types of actions
     */
    enum ActionType
    {
        PageFirst = 1,
        PagePrev = 2,
        PageNext = 3,
        PageLast = 4,
        HistoryBack = 5,
        HistoryForward = 6,
        Quit = 7,
        Presentation = 8,
        EndPresentation = 9,
        Find = 10,
        GoToPage = 11,
        Close = 12,
        Print = 13, ///< \since 0.16
        SaveAs = 14 ///< \since 22.04
    };

    /**
     * The action of the current LinkAction
     */
    ActionType actionType() const;

    /**
     * Create a new Action link, that executes a specified action
     * on the document.
     *
     * \param linkArea the active area of the link
     * \param actionType which action should be executed
     */
    LinkAction(const QRectF &linkArea, ActionType actionType);
    /**
     * Destructor.
     */
    ~LinkAction() override;
    LinkType linkType() const override;

private:
    Q_DECLARE_PRIVATE(LinkAction)
    Q_DISABLE_COPY(LinkAction)
};

/**
 * Sound: a sound to be played.
 *
 * \since 0.6
 */
class POPPLER_QT5_EXPORT LinkSound : public Link
{
public:
    // create a Link_Sound
    LinkSound(const QRectF &linkArea, double volume, bool sync, bool repeat, bool mix, SoundObject *sound);
    /**
     * Destructor.
     */
    ~LinkSound() override;

    LinkType linkType() const override;

    /**
     * The volume to be used when playing the sound.
     *
     * The volume is in the range [ -1, 1 ], where:
     * - a negative number: no volume (mute)
     * - 1: full volume
     */
    double volume() const;
    /**
     * Whether the playback of the sound should be synchronous
     * (thus blocking, waiting for the end of the sound playback).
     */
    bool synchronous() const;
    /**
     * Whether the sound should be played continuously (that is,
     * started again when it ends)
     */
    bool repeat() const;
    /**
     * Whether the playback of this sound can be mixed with
     * playbacks with other sounds of the same document.
     *
     * \note When false, any other playback must be stopped before
     *       playing the sound.
     */
    bool mix() const;
    /**
     * The sound object to be played
     */
    SoundObject *sound() const;

private:
    Q_DECLARE_PRIVATE(LinkSound)
    Q_DISABLE_COPY(LinkSound)
};

/**
 * Rendition: Rendition link.
 *
 * \since 0.20
 */
class POPPLER_QT5_EXPORT LinkRendition : public Link
{
public:
    /**
     * Describes the possible rendition actions.
     *
     * \since 0.22
     */
    enum RenditionAction
    {
        NoRendition,
        PlayRendition,
        StopRendition,
        PauseRendition,
        ResumeRendition
    };

    /**
     * Create a new rendition link.
     *
     * \param linkArea the active area of the link
     * \param rendition the media rendition object. Ownership is taken
     * \param operation the numeric operation (action) (@see ::LinkRendition::RenditionOperation)
     * \param script the java script code
     * \param annotationReference the object reference of the screen annotation associated with this rendition action
     * \since 0.22
     */
    // TODO Next ABI break, remove & from annotationReference
    [[deprecated]] LinkRendition(const QRectF &linkArea, ::MediaRendition *rendition, int operation, const QString &script, const Ref &annotationReference);

    /**
     * Create a new rendition link.
     *
     * \param linkArea the active area of the link
     * \param rendition the media rendition object.
     * \param operation the numeric operation (action) (@see ::LinkRendition::RenditionOperation)
     * \param script the java script code
     * \param annotationReference the object reference of the screen annotation associated with this rendition action
     */
    LinkRendition(const QRectF &linkArea, std::unique_ptr<::MediaRendition> &&rendition, int operation, const QString &script, const Ref annotationReference);

    /**
     * Destructor.
     */
    ~LinkRendition() override;

    LinkType linkType() const override;

    /**
     * Returns the media rendition object if the redition provides one, @c 0 otherwise
     */
    MediaRendition *rendition() const;

    /**
     * Returns the action that should be executed if a rendition object is provided.
     *
     * \since 0.22
     */
    RenditionAction action() const;

    /**
     * The JS code that shall be executed or an empty string.
     *
     * \since 0.22
     */
    QString script() const;

    /**
     * Returns whether the given @p annotation is the referenced screen annotation for this rendition @p link.
     *
     * \since 0.22
     */
    bool isReferencedAnnotation(const ScreenAnnotation *annotation) const;

private:
    Q_DECLARE_PRIVATE(LinkRendition)
    Q_DISABLE_COPY(LinkRendition)
};

/**
 * JavaScript: a JavaScript code to be interpreted.
 *
 * \since 0.10
 */
class POPPLER_QT5_EXPORT LinkJavaScript : public Link
{
public:
    /**
     * Create a new JavaScript link.
     *
     * \param linkArea the active area of the link
     * \param js the JS code to be interpreted
     */
    LinkJavaScript(const QRectF &linkArea, const QString &js);
    /**
     * Destructor.
     */
    ~LinkJavaScript() override;

    LinkType linkType() const override;

    /**
     * The JS code
     */
    QString script() const;

private:
    Q_DECLARE_PRIVATE(LinkJavaScript)
    Q_DISABLE_COPY(LinkJavaScript)
};

/**
 * Movie: a movie to be played.
 *
 * \since 0.20
 */
class POPPLER_QT5_EXPORT LinkMovie : public Link
{
public:
    /**
     * Describes the operation to be performed on the movie.
     */
    enum Operation
    {
        Play,
        Stop,
        Pause,
        Resume
    };

    /**
     * Create a new Movie link.
     *
     * \param linkArea the active area of the link
     * \param operation the operation to be performed on the movie
     * \param annotationTitle the title of the movie annotation identifying the movie to be played
     * \param annotationReference the object reference of the movie annotation identifying the movie to be played
     *
     * Note: This constructor is supposed to be used by Poppler::Page only.
     */
    // TODO Next ABI break, remove & from annotationReference
    LinkMovie(const QRectF &linkArea, Operation operation, const QString &annotationTitle, const Ref &annotationReference);
    /**
     * Destructor.
     */
    ~LinkMovie() override;
    LinkType linkType() const override;
    /**
     * Returns the operation to be performed on the movie.
     */
    Operation operation() const;
    /**
     * Returns whether the given @p annotation is the referenced movie annotation for this movie @p link.
     */
    bool isReferencedAnnotation(const MovieAnnotation *annotation) const;

private:
    Q_DECLARE_PRIVATE(LinkMovie)
    Q_DISABLE_COPY(LinkMovie)
};

/**
 * OCGState: an optional content group state change.
 *
 * \since 0.50
 */
class POPPLER_QT5_EXPORT LinkOCGState : public Link
{
    friend class OptContentModel;

public:
    /**
     * Create a new OCGState link. This is only used by Poppler::Page.
     */
    explicit LinkOCGState(LinkOCGStatePrivate *ocgp);
    /**
     * Destructor.
     */
    ~LinkOCGState() override;

    LinkType linkType() const override;

private:
    Q_DECLARE_PRIVATE(LinkOCGState)
    Q_DISABLE_COPY(LinkOCGState)
};

/**
 * Hide: an action to show / hide a field.
 *
 * \since 0.64
 */
class POPPLER_QT5_EXPORT LinkHide : public Link
{
public:
    /**
     * Create a new Hide link. This is only used by Poppler::Page.
     */
    explicit LinkHide(LinkHidePrivate *lhidep);
    /**
     * Destructor.
     */
    ~LinkHide() override;

    LinkType linkType() const override;

    /**
     * The fully qualified target names of the action.
     */
    QVector<QString> targets() const;

    /**
     * Should this action change the visibility of the target to true.
     */
    bool isShowAction() const;

private:
    Q_DECLARE_PRIVATE(LinkHide)
    Q_DISABLE_COPY(LinkHide)
};

/**
 * ResetForm: an action to reset form fields.
 *
 * \since 24.07
 */
class POPPLER_QT5_EXPORT LinkResetForm : public Link
{
    friend class Document;

public:
    /**
     * Creates a new ResetForm link. This is only used by Poppler::Page
     */
    explicit LinkResetForm(LinkResetFormPrivate *lrfp);
    /*
     * Destructor
     */
    ~LinkResetForm() override;

    LinkType linkType() const override;

private:
    Q_DECLARE_PRIVATE(LinkResetForm)
    Q_DISABLE_COPY(LinkResetForm)
};

/**
 * SubmitForm : An action to submit a form.
 *
 * \since 24.10
 */
class POPPLER_QT5_EXPORT LinkSubmitForm : public Link
{
public:
    enum SubmitFormFlag
    {
        NoOpFlag = 0,
        ExcludeFlag = 1,
        IncludeNoValueFieldsFlag = 1 << 1,
        ExportFormatFlag = 1 << 2,
        GetMethodFlag = 1 << 3,
        SubmitCoordinatesFlag = 1 << 4,
        XFDFFlag = 1 << 5,
        IncludeAppendSavesFlag = 1 << 6,
        IncludeAnnotationsFlag = 1 << 7,
        SubmitPDFFlag = 1 << 8,
        CanonicalFormatFlag = 1 << 9,
        ExclNonUserAnnotsFlag = 1 << 10,
        ExclFKeyFlag = 1 << 11,
        // 13th high bit flag is undefined
        EmbedFormFlag = 1 << 13,
    };
    Q_DECLARE_FLAGS(SubmitFormFlags, SubmitFormFlag)

    /**
     * Create a new SubmitFormLink.
     */
    explicit LinkSubmitForm(LinkSubmitFormPrivate *lsfp);
    /**
     * Destructor
     */
    ~LinkSubmitForm() override;

    LinkType linkType() const override;

    /**
     * The ids of fields that are to be submitted.
     */
    QVector<int> getFieldIds() const;

    /**
     * The url to which the form is to be submitted.
     */
    QString getUrl() const;

    /**
     * The flags specifying how the form should be submitted.
     */
    SubmitFormFlags getFlags() const;

private:
    Q_DECLARE_PRIVATE(LinkSubmitForm)
    Q_DISABLE_COPY(LinkSubmitForm)
};
}

#endif
