/* poppler-annotation.cc: qt interface to poppler
 * Copyright (C) 2006, 2009 Albert Astals Cid <aacid@kde.org>
 * Copyright (C) 2006, 2008, 2010 Pino Toscano <pino@kde.org>
 * Copyright (C) 2012, Guillermo A. Amaral B. <gamaral@kde.org>
 * Copyright (C) 2012, Fabio D'Urso <fabiodurso@hotmail.it>
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
#include <QtXml/QDomElement>
#include <QtGui/QColor>

// local includes
#include "poppler-annotation.h"
#include "poppler-link.h"
#include "poppler-qt4.h"
#include "poppler-annotation-private.h"

// poppler includes
#include <Page.h>
#include <Annot.h>

namespace Poppler {

//BEGIN AnnotationUtils implementation
Annotation * AnnotationUtils::createAnnotation( const QDomElement & annElement )
{
    // safety check on annotation element
    if ( !annElement.hasAttribute( "type" ) )
        return 0;

    // build annotation of given type
    Annotation * annotation = 0;
    int typeNumber = annElement.attribute( "type" ).toInt();
    switch ( typeNumber )
    {
        // Some annot types are commented because their creation is temporarly disabled
        case Annotation::AText:
            //annotation = new TextAnnotation( annElement );
            break;
        case Annotation::ALine:
            //annotation = new LineAnnotation( annElement );
            break;
        case Annotation::AGeom:
            //annotation = new GeomAnnotation( annElement );
            break;
        case Annotation::AHighlight:
            //annotation = new HighlightAnnotation( annElement );
            break;
        case Annotation::AStamp:
            //annotation = new StampAnnotation( annElement );
            break;
        case Annotation::AInk:
            //annotation = new InkAnnotation( annElement );
            break;
        case Annotation::ACaret:
            //annotation = new CaretAnnotation( annElement );
            break;
    }

    // return created annotation
    return annotation;
}

void AnnotationUtils::storeAnnotation( const Annotation * ann, QDomElement & annElement,
    QDomDocument & document )
{
    // save annotation's type as element's attribute
    annElement.setAttribute( "type", (uint)ann->subType() );

    // append all annotation data as children of this node
    ann->store( annElement, document );
}

QDomElement AnnotationUtils::findChildElement( const QDomNode & parentNode,
    const QString & name )
{
    // loop through the whole children and return a 'name' named element
    QDomNode subNode = parentNode.firstChild();
    while( subNode.isElement() )
    {
        QDomElement element = subNode.toElement();
        if ( element.tagName() == name )
            return element;
        subNode = subNode.nextSibling();
    }
    // if the name can't be found, return a dummy null element
    return QDomElement();
}
//END AnnotationUtils implementation


//BEGIN Annotation implementation
AnnotationPrivate::AnnotationPrivate()
    : flags( 0 ), revisionScope ( Annotation::Root ), revisionType ( Annotation::None )
{
    pdfObjectReference.num = pdfObjectReference.gen = -1;
}

AnnotationPrivate::~AnnotationPrivate()
{
    // Delete all children revisions
    qDeleteAll( revisions );
}

class Annotation::Style::Private : public QSharedData
{
  public:
    Private()
        : opacity( 1.0 ), width( 1.0 ), lineStyle( Solid ), xCorners( 0.0 ),
        yCorners( 0.0 ), lineEffect( NoEffect ), effectIntensity( 1.0 )
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

Annotation::Style::Style()
    : d ( new Private )
{
}

Annotation::Style::Style( const Style &other )
    : d( other.d )
{
}

Annotation::Style& Annotation::Style::operator=( const Style &other )
{
    if ( this != &other )
        d = other.d;

    return *this;
}

Annotation::Style::~Style()
{
}

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

const QVector<double>& Annotation::Style::dashArray() const
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
    Private()
        : flags( -1 ) {}

    int flags;
    QRectF geometry;
    QString title;
    QString summary;
    QString text;
};

Annotation::Popup::Popup()
    : d ( new Private )
{
}

Annotation::Popup::Popup( const Popup &other )
    : d( other.d )
{
}

Annotation::Popup& Annotation::Popup::operator=( const Popup &other )
{
    if ( this != &other )
        d = other.d;

    return *this;
}

Annotation::Popup::~Popup()
{
}

int Annotation::Popup::flags() const
{
    return d->flags;
}

void Annotation::Popup::setFlags( int flags )
{
    d->flags = flags;
}

QRectF Annotation::Popup::geometry() const
{
    return d->geometry;
}

void Annotation::Popup::setGeometry( const QRectF &geom )
{
    d->geometry = geom;
}

QString Annotation::Popup::title() const
{
    return d->title;
}

void Annotation::Popup::setTitle( const QString &title )
{
    d->title = title;
}

QString Annotation::Popup::summary() const
{
    return d->summary;
}

void Annotation::Popup::setSummary( const QString &summary )
{
    d->summary = summary;
}

QString Annotation::Popup::text() const
{
    return d->text;
}

void Annotation::Popup::setText( const QString &text )
{
    d->text = text;
}

Annotation::Annotation( AnnotationPrivate &dd )
    : d_ptr( &dd )
{
    window.width = window.height = 0;
}

Annotation::~Annotation()
{
}

Annotation::Annotation( AnnotationPrivate &dd, const QDomNode &annNode )
    : d_ptr( &dd )
{
    // get the [base] element of the annotation node
    QDomElement e = AnnotationUtils::findChildElement( annNode, "base" );
    if ( e.isNull() )
        return;

    Style s;
    Popup w;

    // parse -contents- attributes
    if ( e.hasAttribute( "author" ) )
        setAuthor(e.attribute( "author" ));
    if ( e.hasAttribute( "contents" ) )
        setContents(e.attribute( "contents" ));
    if ( e.hasAttribute( "uniqueName" ) )
        setUniqueName(e.attribute( "uniqueName" ));
    if ( e.hasAttribute( "modifyDate" ) )
        setModificationDate(QDateTime::fromString( e.attribute( "modifyDate" ) ));
    if ( e.hasAttribute( "creationDate" ) )
        setCreationDate(QDateTime::fromString( e.attribute( "creationDate" ) ));

    // parse -other- attributes
    if ( e.hasAttribute( "flags" ) )
        setFlags(e.attribute( "flags" ).toInt());
    if ( e.hasAttribute( "color" ) )
        s.setColor(QColor( e.attribute( "color" ) ));
    if ( e.hasAttribute( "opacity" ) )
        s.setOpacity(e.attribute( "opacity" ).toDouble());

    // parse -the-subnodes- (describing Style, Window, Revision(s) structures)
    // Note: all subnodes if present must be 'attributes complete'
    QDomNode eSubNode = e.firstChild();
    while ( eSubNode.isElement() )
    {
        QDomElement ee = eSubNode.toElement();
        eSubNode = eSubNode.nextSibling();

        // parse boundary
        if ( ee.tagName() == "boundary" )
        {
            QRectF brect;
            brect.setLeft(ee.attribute( "l" ).toDouble());
            brect.setTop(ee.attribute( "t" ).toDouble());
            brect.setRight(ee.attribute( "r" ).toDouble());
            brect.setBottom(ee.attribute( "b" ).toDouble());
            setBoundary(brect);
        }
        // parse penStyle if not default
        else if ( ee.tagName() == "penStyle" )
        {
            s.setWidth(ee.attribute( "width" ).toDouble());
            s.setLineStyle((LineStyle)ee.attribute( "style" ).toInt());
            s.setXCorners(ee.attribute( "xcr" ).toDouble());
            s.setYCorners(ee.attribute( "ycr" ).toDouble());

            // Try to parse dash array (new format)
            QVector<double> dashArray;

            QDomNode eeSubNode = ee.firstChild();
            while ( eeSubNode.isElement() )
            {
                QDomElement eee = eeSubNode.toElement();
                eeSubNode = eeSubNode.nextSibling();

                if ( eee.tagName() != "dashsegm" )
                    continue;

                dashArray.append(eee.attribute( "len" ).toDouble());
            }

            // If no segments were found use marks/spaces (old format)
            if ( dashArray.size() == 0 )
            {
                dashArray.append(ee.attribute( "marks" ).toDouble());
                dashArray.append(ee.attribute( "spaces" ).toDouble());
            }

            s.setDashArray(dashArray);
        }
        // parse effectStyle if not default
        else if ( ee.tagName() == "penEffect" )
        {
            s.setLineEffect((LineEffect)ee.attribute( "effect" ).toInt());
            s.setEffectIntensity(ee.attribute( "intensity" ).toDouble());
        }
        // parse window if present
        else if ( ee.tagName() == "window" )
        {
            QRectF geom;
            geom.setX(ee.attribute( "top" ).toDouble());
            geom.setY(ee.attribute( "left" ).toDouble());

            if (ee.hasAttribute("widthDouble"))
                geom.setWidth(ee.attribute( "widthDouble" ).toDouble());
            else
                geom.setWidth(ee.attribute( "width" ).toDouble());

            if (ee.hasAttribute("widthDouble"))
                geom.setHeight(ee.attribute( "heightDouble" ).toDouble());
            else
                geom.setHeight(ee.attribute( "height" ).toDouble());

            w.setGeometry(geom);

            w.setFlags(ee.attribute( "flags" ).toInt());
            w.setTitle(ee.attribute( "title" ));
            w.setSummary(ee.attribute( "summary" ));
            // parse window subnodes
            QDomNode winNode = ee.firstChild();
            for ( ; winNode.isElement(); winNode = winNode.nextSibling() )
            {
                QDomElement winElement = winNode.toElement();
                if ( winElement.tagName() == "text" )
                    w.setText(winElement.firstChild().toCDATASection().data());
            }
        }
    }

    setStyle(s);  // assign parsed style
    setPopup(w); // assign parsed window

    // get the [revisions] element of the annotation node
    QDomNode revNode = annNode.firstChild();
    for ( ; revNode.isElement(); revNode = revNode.nextSibling() )
    {
        QDomElement revElement = revNode.toElement();
        if ( revElement.tagName() != "revision" )
            continue;

        Annotation *reply = AnnotationUtils::createAnnotation( revElement );

        if (reply) // if annotation is valid, add as a revision of this annotation
        {
            RevScope scope = (RevScope)revElement.attribute( "revScope" ).toInt();
            RevType type = (RevType)revElement.attribute( "revType" ).toInt();
            addRevision(reply, scope, type);
            delete reply;
        }
    }
}

void Annotation::storeBaseAnnotationProperties( QDomNode & annNode, QDomDocument & document ) const
{
    // create [base] element of the annotation node
    QDomElement e = document.createElement( "base" );
    annNode.appendChild( e );

    const Style s = style();
    const Popup w = popup();

    // store -contents- attributes
    if ( !author().isEmpty() )
        e.setAttribute( "author", author() );
    if ( !contents().isEmpty() )
        e.setAttribute( "contents", contents() );
    if ( !uniqueName().isEmpty() )
        e.setAttribute( "uniqueName", uniqueName() );
    if ( modificationDate().isValid() )
        e.setAttribute( "modifyDate", modificationDate().toString() );
    if ( creationDate().isValid() )
        e.setAttribute( "creationDate", creationDate().toString() );

    // store -other- attributes
    if ( flags() )
        e.setAttribute( "flags", flags() );
    if ( s.color().isValid() && s.color() != Qt::black )
        e.setAttribute( "color", s.color().name() );
    if ( s.opacity() != 1.0 )
        e.setAttribute( "opacity", QString::number( s.opacity() ) );

    // Sub-Node-1 - boundary
    const QRectF brect = boundary();
    QDomElement bE = document.createElement( "boundary" );
    e.appendChild( bE );
    bE.setAttribute( "l", QString::number( (double)brect.left() ) );
    bE.setAttribute( "t", QString::number( (double)brect.top() ) );
    bE.setAttribute( "r", QString::number( (double)brect.right() ) );
    bE.setAttribute( "b", QString::number( (double)brect.bottom() ) );

    // Sub-Node-2 - penStyle
    const QVector<double> dashArray = s.dashArray();
    if ( s.width() != 1 || s.lineStyle() != Solid || s.xCorners() != 0 ||
         s.yCorners() != 0.0 || dashArray.size() != 1 || dashArray[0] != 3 )
    {
        QDomElement psE = document.createElement( "penStyle" );
        e.appendChild( psE );
        psE.setAttribute( "width", QString::number( s.width() ) );
        psE.setAttribute( "style", (int)s.lineStyle() );
        psE.setAttribute( "xcr", QString::number( s.xCorners() ) );
        psE.setAttribute( "ycr", QString::number( s.yCorners() ) );

        int marks = 3, spaces = 0; // Do not break code relying on marks/spaces
        if (dashArray.size() != 0)
            marks = (int)dashArray[0];
        if (dashArray.size() > 1)
            spaces = (int)dashArray[1];

        psE.setAttribute( "marks", marks );
        psE.setAttribute( "spaces", spaces );

        foreach (double segm, dashArray)
        {
            QDomElement pattE = document.createElement( "dashsegm" );
            pattE.setAttribute( "len", QString::number( segm ) );
            psE.appendChild(pattE);
        }
    }

    // Sub-Node-3 - penEffect
    if ( s.lineEffect() != NoEffect || s.effectIntensity() != 1.0 )
    {
        QDomElement peE = document.createElement( "penEffect" );
        e.appendChild( peE );
        peE.setAttribute( "effect", (int)s.lineEffect() );
        peE.setAttribute( "intensity", QString::number( s.effectIntensity() ) );
    }

    // Sub-Node-4 - window
    if ( w.flags() != -1 || !w.title().isEmpty() || !w.summary().isEmpty() ||
         !w.text().isEmpty() )
    {
        QDomElement wE = document.createElement( "window" );
        const QRectF geom = w.geometry();
        e.appendChild( wE );
        wE.setAttribute( "flags", w.flags() );
        wE.setAttribute( "top", QString::number( geom.x() ) );
        wE.setAttribute( "left", QString::number( geom.y() ) );
        wE.setAttribute( "width", (int)geom.width() );
        wE.setAttribute( "height", (int)geom.height() );
        wE.setAttribute( "widthDouble", QString::number( geom.width() ) );
        wE.setAttribute( "heightDouble", QString::number( geom.height() ) );
        wE.setAttribute( "title", w.title() );
        wE.setAttribute( "summary", w.summary() );
        // store window.text as a subnode, because we need escaped data
        if ( !w.text().isEmpty() )
        {
            QDomElement escapedText = document.createElement( "text" );
            wE.appendChild( escapedText );
            QDomCDATASection textCData = document.createCDATASection( w.text() );
            escapedText.appendChild( textCData );
        }
    }

    const QList<Annotation*> revs = revisions();

    // create [revision] element of the annotation node (if any)
    if ( revs.isEmpty() )
        return;

    // add all revisions as children of revisions element
    foreach (const Annotation *rev, revs)
    {
        QDomElement r = document.createElement( "revision" );
        annNode.appendChild( r );
        // set element attributes
        r.setAttribute( "revScope", (int)rev->revisionScope() );
        r.setAttribute( "revType", (int)rev->revisionType() );
        // use revision as the annotation element, so fill it up
        AnnotationUtils::storeAnnotation( rev, r, document );
        delete rev;
    }
}

QString Annotation::author() const
{
    Q_D( const Annotation );
    return d->author;
}

void Annotation::setAuthor( const QString &author )
{
    Q_D( Annotation );
    d->author = author;
}

QString Annotation::contents() const
{
    Q_D( const Annotation );
    return d->contents;
}

void Annotation::setContents( const QString &contents )
{
    Q_D( Annotation );
    d->contents = contents;
}

QString Annotation::uniqueName() const
{
    Q_D( const Annotation );
    return d->uniqueName;
}

void Annotation::setUniqueName( const QString &uniqueName )
{
    Q_D( Annotation );
    d->uniqueName = uniqueName;
}

QDateTime Annotation::modificationDate() const
{
    Q_D( const Annotation );
    return d->modDate;
}

void Annotation::setModificationDate( const QDateTime &date )
{
    Q_D( Annotation );
    d->modDate = date;
}

QDateTime Annotation::creationDate() const
{
    Q_D( const Annotation );
    return d->creationDate;
}

void Annotation::setCreationDate( const QDateTime &date )
{
    Q_D( Annotation );
    d->creationDate = date;
}

int Annotation::flags() const
{
    Q_D( const Annotation );
    return d->flags;
}

void Annotation::setFlags( int flags )
{
    Q_D( Annotation );
    d->flags = flags;
}

QRectF Annotation::boundary() const
{
    Q_D( const Annotation );
    return d->boundary;
}

void Annotation::setBoundary( const QRectF &boundary )
{
    Q_D( Annotation );
    d->boundary = boundary;
}

Annotation::Style Annotation::style() const
{
    Q_D( const Annotation );
    return d->style;
}

void Annotation::setStyle( const Annotation::Style& style )
{
    Q_D( Annotation );
    d->style = style;
}

Annotation::Popup Annotation::popup() const
{
    Q_D( const Annotation );
    return d->popup;
}

void Annotation::setPopup( const Annotation::Popup& popup )
{
    Q_D( Annotation );
    d->popup = popup;
}

Annotation::RevScope Annotation::revisionScope() const
{
    Q_D( const Annotation );
    return d->revisionScope;
}

void Annotation::setRevisionScope( Annotation::RevScope scope )
{
    Q_D( Annotation );
    d->revisionScope = scope;
}

Annotation::RevType Annotation::revisionType() const
{
    Q_D( const Annotation );
    return d->revisionType;
}

void Annotation::setRevisionType( Annotation::RevType type )
{
    Q_D( Annotation );
    d->revisionType = type;
}

QList<Annotation*> Annotation::revisions() const
{
    Q_D( const Annotation );
    QList<Annotation*> res;

    /* Return aliases, whose ownership goes to the caller */
    foreach (Annotation *rev, d->revisions)
        res.append( rev->d_ptr->makeAlias() );

    return res;
}

void Annotation::addRevision( Annotation *ann, RevScope scope, RevType type )
{
    Q_D( Annotation );

    /* Since ownership stays with the caller, create an alias of ann */
    d->revisions.append( ann->d_ptr->makeAlias() );

    /* Set revision properties */
    ann->setRevisionScope(scope);
    ann->setRevisionType(type);
}

//END AnnotationUtils implementation


/** TextAnnotation [Annotation] */
class TextAnnotationPrivate : public AnnotationPrivate
{
    public:
        TextAnnotationPrivate();
        Annotation * makeAlias();

        // data fields
        TextAnnotation::TextType textType;
        QString textIcon;
        QFont textFont;
        int inplaceAlign; // 0:left, 1:center, 2:right
        QString inplaceText;  // overrides contents
        QPointF inplaceCallout[3];
        TextAnnotation::InplaceIntent inplaceIntent;
};

TextAnnotationPrivate::TextAnnotationPrivate()
    : AnnotationPrivate(), textType( TextAnnotation::Linked ),
    textIcon( "Note" ), inplaceAlign( 0 ),
    inplaceIntent( TextAnnotation::Unknown )
{
}

Annotation * TextAnnotationPrivate::makeAlias()
{
    return new TextAnnotation(*this);
}

TextAnnotation::TextAnnotation()
    : Annotation( *new TextAnnotationPrivate() )
{}

TextAnnotation::TextAnnotation(TextAnnotationPrivate &dd)
    : Annotation( dd )
{}

TextAnnotation::TextAnnotation( const QDomNode & node )
    : Annotation( *new TextAnnotationPrivate, node )
{
    // loop through the whole children looking for a 'text' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "text" )
            continue;

        // parse the attributes
        if ( e.hasAttribute( "type" ) )
            setTextType((TextAnnotation::TextType)e.attribute( "type" ).toInt());
        if ( e.hasAttribute( "icon" ) )
            setTextIcon(e.attribute( "icon" ));
        if ( e.hasAttribute( "font" ) )
        {
            QFont font;
            font.fromString( e.attribute( "font" ) );
            setTextFont(font);
        }
        if ( e.hasAttribute( "align" ) )
            setInplaceAlign(e.attribute( "align" ).toInt());
        if ( e.hasAttribute( "intent" ) )
            setInplaceIntent((TextAnnotation::InplaceIntent)e.attribute( "intent" ).toInt());

        // parse the subnodes
        QDomNode eSubNode = e.firstChild();
        while ( eSubNode.isElement() )
        {
            QDomElement ee = eSubNode.toElement();
            eSubNode = eSubNode.nextSibling();

            if ( ee.tagName() == "escapedText" )
            {
                setInplaceText(ee.firstChild().toCDATASection().data());
            }
            else if ( ee.tagName() == "callout" )
            {
                setCalloutPoint(0, QPointF(ee.attribute( "ax" ).toDouble(),
                                           ee.attribute( "ay" ).toDouble()));
                setCalloutPoint(1, QPointF(ee.attribute( "bx" ).toDouble(),
                                           ee.attribute( "by" ).toDouble()));
                setCalloutPoint(2, QPointF(ee.attribute( "cx" ).toDouble(),
                                           ee.attribute( "cy" ).toDouble()));
            }
        }

        // loading complete
        break;
    }
}

TextAnnotation::~TextAnnotation()
{
}

void TextAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // store base annotation properties
    storeBaseAnnotationProperties( node, document );

    // create [text] element
    QDomElement textElement = document.createElement( "text" );
    node.appendChild( textElement );

    // store the optional attributes
    if ( textType() != Linked )
        textElement.setAttribute( "type", (int)textType() );
    if ( textIcon() != "Comment" )
        textElement.setAttribute( "icon", textIcon() );
    if ( inplaceAlign() )
        textElement.setAttribute( "align", inplaceAlign() );
    if ( inplaceIntent() != Unknown )
        textElement.setAttribute( "intent", (int)inplaceIntent() );

    textElement.setAttribute( "font", textFont().toString() );

    // Sub-Node-1 - escapedText
    if ( !inplaceText().isEmpty() )
    {
        QDomElement escapedText = document.createElement( "escapedText" );
        textElement.appendChild( escapedText );
        QDomCDATASection textCData = document.createCDATASection( inplaceText() );
        escapedText.appendChild( textCData );
    }

    // Sub-Node-2 - callout
    if ( calloutPoint(0).x() != 0.0 )
    {
        QDomElement calloutElement = document.createElement( "callout" );
        textElement.appendChild( calloutElement );
        calloutElement.setAttribute( "ax", QString::number( calloutPoint(0).x() ) );
        calloutElement.setAttribute( "ay", QString::number( calloutPoint(0).y() ) );
        calloutElement.setAttribute( "bx", QString::number( calloutPoint(1).x() ) );
        calloutElement.setAttribute( "by", QString::number( calloutPoint(1).y() ) );
        calloutElement.setAttribute( "cx", QString::number( calloutPoint(2).x() ) );
        calloutElement.setAttribute( "cy", QString::number( calloutPoint(2).y() ) );
    }
}

Annotation::SubType TextAnnotation::subType() const
{
    return AText;
}

TextAnnotation::TextType TextAnnotation::textType() const
{
    Q_D( const TextAnnotation );
    return d->textType;
}

void TextAnnotation::setTextType( TextAnnotation::TextType type )
{
    Q_D( TextAnnotation );
    d->textType = type;
}

QString TextAnnotation::textIcon() const
{
    Q_D( const TextAnnotation );
    return d->textIcon;
}

void TextAnnotation::setTextIcon( const QString &icon )
{
    Q_D( TextAnnotation );
    d->textIcon = icon;
}

QFont TextAnnotation::textFont() const
{
    Q_D( const TextAnnotation );
    return d->textFont;
}

void TextAnnotation::setTextFont( const QFont &font )
{
    Q_D( TextAnnotation );
    d->textFont = font;
}

int TextAnnotation::inplaceAlign() const
{
    Q_D( const TextAnnotation );
    return d->inplaceAlign;
}

void TextAnnotation::setInplaceAlign( int align )
{
    Q_D( TextAnnotation );
    d->inplaceAlign = align;
}

QString TextAnnotation::inplaceText() const
{
    Q_D( const TextAnnotation );
    return d->inplaceText;
}

void TextAnnotation::setInplaceText( const QString &text )
{
    Q_D( TextAnnotation );
    d->inplaceText = text;
}

QPointF TextAnnotation::calloutPoint( int id ) const
{
    if ( id < 0 || id >= 3 )
        return QPointF();

    Q_D( const TextAnnotation );
    return d->inplaceCallout[id];
}

void TextAnnotation::setCalloutPoint( int id, const QPointF &point )
{
    if ( id < 0 || id >= 3 )
        return;

    Q_D( TextAnnotation );
    d->inplaceCallout[id] = point;
}

TextAnnotation::InplaceIntent TextAnnotation::inplaceIntent() const
{
    Q_D( const TextAnnotation );
    return d->inplaceIntent;
}

void TextAnnotation::setInplaceIntent( TextAnnotation::InplaceIntent intent )
{
    Q_D( TextAnnotation );
    d->inplaceIntent = intent;
}


/** LineAnnotation [Annotation] */
class LineAnnotationPrivate : public AnnotationPrivate
{
    public:
        LineAnnotationPrivate();
        Annotation * makeAlias();

        // data fields (note uses border for rendering style)
        QLinkedList<QPointF> linePoints;
        LineAnnotation::TermStyle lineStartStyle;
        LineAnnotation::TermStyle lineEndStyle;
        bool lineClosed : 1;  // (if true draw close shape)
        bool lineShowCaption : 1;
        QColor lineInnerColor;
        double lineLeadingFwdPt;
        double lineLeadingBackPt;
        LineAnnotation::LineIntent lineIntent;
};

LineAnnotationPrivate::LineAnnotationPrivate()
    : AnnotationPrivate(), lineStartStyle( LineAnnotation::None ),
    lineEndStyle( LineAnnotation::None ), lineClosed( false ),
    lineShowCaption( false ), lineLeadingFwdPt( 0 ), lineLeadingBackPt( 0 ),
    lineIntent( LineAnnotation::Unknown )
{
}

Annotation * LineAnnotationPrivate::makeAlias()
{
    return new LineAnnotation(*this);
}

LineAnnotation::LineAnnotation()
    : Annotation( *new LineAnnotationPrivate() )
{}

LineAnnotation::LineAnnotation(LineAnnotationPrivate &dd)
    : Annotation( dd )
{}

LineAnnotation::LineAnnotation( const QDomNode & node )
    : Annotation( *new LineAnnotationPrivate(), node )
{
    // loop through the whole children looking for a 'line' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "line" )
            continue;

        // parse the attributes
        if ( e.hasAttribute( "startStyle" ) )
            setLineStartStyle((LineAnnotation::TermStyle)e.attribute( "startStyle" ).toInt());
        if ( e.hasAttribute( "endStyle" ) )
            setLineEndStyle((LineAnnotation::TermStyle)e.attribute( "endStyle" ).toInt());
        if ( e.hasAttribute( "closed" ) )
            setLineClosed(e.attribute( "closed" ).toInt());
        if ( e.hasAttribute( "innerColor" ) )
            setLineInnerColor(QColor( e.attribute( "innerColor" ) ));
        if ( e.hasAttribute( "leadFwd" ) )
            setLineLeadingForwardPoint(e.attribute( "leadFwd" ).toDouble());
        if ( e.hasAttribute( "leadBack" ) )
            setLineLeadingBackPoint(e.attribute( "leadBack" ).toDouble());
        if ( e.hasAttribute( "showCaption" ) )
            setLineShowCaption(e.attribute( "showCaption" ).toInt());
        if ( e.hasAttribute( "intent" ) )
            setLineIntent((LineAnnotation::LineIntent)e.attribute( "intent" ).toInt());

        // parse all 'point' subnodes
        QLinkedList<QPointF> points;
        QDomNode pointNode = e.firstChild();
        while ( pointNode.isElement() )
        {
            QDomElement pe = pointNode.toElement();
            pointNode = pointNode.nextSibling();

            if ( pe.tagName() != "point" )
                continue;

            QPointF p(pe.attribute( "x", "0.0" ).toDouble(), pe.attribute( "y", "0.0" ).toDouble());
            points.append( p );
        }
        setLinePoints(points);

        // loading complete
        break;
    }
}

LineAnnotation::~LineAnnotation()
{
}

void LineAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // store base annotation properties
    storeBaseAnnotationProperties( node, document );

    // create [line] element
    QDomElement lineElement = document.createElement( "line" );
    node.appendChild( lineElement );

    // store the attributes
    if ( lineStartStyle() != None )
        lineElement.setAttribute( "startStyle", (int)lineStartStyle() );
    if ( lineEndStyle() != None )
        lineElement.setAttribute( "endStyle", (int)lineEndStyle() );
    if ( isLineClosed() )
        lineElement.setAttribute( "closed", isLineClosed() );
    if ( lineInnerColor().isValid() )
        lineElement.setAttribute( "innerColor", lineInnerColor().name() );
    if ( lineLeadingForwardPoint() != 0.0 )
        lineElement.setAttribute( "leadFwd", QString::number( lineLeadingForwardPoint() ) );
    if ( lineLeadingBackPoint() != 0.0 )
        lineElement.setAttribute( "leadBack", QString::number( lineLeadingBackPoint() ) );
    if ( lineShowCaption() )
        lineElement.setAttribute( "showCaption", lineShowCaption() );
    if ( lineIntent() != Unknown )
        lineElement.setAttribute( "intent", lineIntent() );

    // append the list of points
    const QLinkedList<QPointF> points = linePoints();
    if ( points.count() > 1 )
    {
        QLinkedList<QPointF>::const_iterator it = points.begin(), end = points.end();
        while ( it != end )
        {
            const QPointF & p = *it;
            QDomElement pElement = document.createElement( "point" );
            lineElement.appendChild( pElement );
            pElement.setAttribute( "x", QString::number( p.x() ) );
            pElement.setAttribute( "y", QString::number( p.y() ) );
            ++it;
        }
    }
}

Annotation::SubType LineAnnotation::subType() const
{
    return ALine;
}

QLinkedList<QPointF> LineAnnotation::linePoints() const
{
    Q_D( const LineAnnotation );
    return d->linePoints;
}

void LineAnnotation::setLinePoints( const QLinkedList<QPointF> &points )
{
    Q_D( LineAnnotation );
    d->linePoints = points;
}

LineAnnotation::TermStyle LineAnnotation::lineStartStyle() const
{
    Q_D( const LineAnnotation );
    return d->lineStartStyle;
}

void LineAnnotation::setLineStartStyle( LineAnnotation::TermStyle style )
{
    Q_D( LineAnnotation );
    d->lineStartStyle = style;
}

LineAnnotation::TermStyle LineAnnotation::lineEndStyle() const
{
    Q_D( const LineAnnotation );
    return d->lineEndStyle;
}

void LineAnnotation::setLineEndStyle( LineAnnotation::TermStyle style )
{
    Q_D( LineAnnotation );
    d->lineEndStyle = style;
}

bool LineAnnotation::isLineClosed() const
{
    Q_D( const LineAnnotation );
    return d->lineClosed;
}

void LineAnnotation::setLineClosed( bool closed )
{
    Q_D( LineAnnotation );
    d->lineClosed = closed;
}

QColor LineAnnotation::lineInnerColor() const
{
    Q_D( const LineAnnotation );
    return d->lineInnerColor;
}

void LineAnnotation::setLineInnerColor( const QColor &color )
{
    Q_D( LineAnnotation );
    d->lineInnerColor = color;
}

double LineAnnotation::lineLeadingForwardPoint() const
{
    Q_D( const LineAnnotation );
    return d->lineLeadingFwdPt;
}

void LineAnnotation::setLineLeadingForwardPoint( double point )
{
    Q_D( LineAnnotation );
    d->lineLeadingFwdPt = point;
}

double LineAnnotation::lineLeadingBackPoint() const
{
    Q_D( const LineAnnotation );
    return d->lineLeadingBackPt;
}

void LineAnnotation::setLineLeadingBackPoint( double point )
{
    Q_D( LineAnnotation );
    d->lineLeadingBackPt = point;
}

bool LineAnnotation::lineShowCaption() const
{
    Q_D( const LineAnnotation );
    return d->lineShowCaption;
}

void LineAnnotation::setLineShowCaption( bool show )
{
    Q_D( LineAnnotation );
    d->lineShowCaption = show;
}

LineAnnotation::LineIntent LineAnnotation::lineIntent() const
{
    Q_D( const LineAnnotation );
    return d->lineIntent;
}

void LineAnnotation::setLineIntent( LineAnnotation::LineIntent intent )
{
    Q_D( LineAnnotation );
    d->lineIntent = intent;
}


/** GeomAnnotation [Annotation] */
class GeomAnnotationPrivate : public AnnotationPrivate
{
    public:
        GeomAnnotationPrivate();
        Annotation * makeAlias();

        // data fields (note uses border for rendering style)
        GeomAnnotation::GeomType geomType;
        QColor geomInnerColor;
};

GeomAnnotationPrivate::GeomAnnotationPrivate()
    : AnnotationPrivate(), geomType( GeomAnnotation::InscribedSquare )
{
}

Annotation * GeomAnnotationPrivate::makeAlias()
{
    return new GeomAnnotation(*this);
}

GeomAnnotation::GeomAnnotation()
    : Annotation( *new GeomAnnotationPrivate() )
{}

GeomAnnotation::GeomAnnotation(GeomAnnotationPrivate &dd)
    : Annotation( dd )
{}

GeomAnnotation::GeomAnnotation( const QDomNode & node )
    : Annotation( *new GeomAnnotationPrivate(), node )
{
    // loop through the whole children looking for a 'geom' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "geom" )
            continue;

        // parse the attributes
        if ( e.hasAttribute( "type" ) )
            setGeomType((GeomAnnotation::GeomType)e.attribute( "type" ).toInt());
        if ( e.hasAttribute( "color" ) )
            setGeomInnerColor(QColor( e.attribute( "color" ) ));

        // loading complete
        break;
    }
}

GeomAnnotation::~GeomAnnotation()
{
}

void GeomAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // store base annotation properties
    storeBaseAnnotationProperties( node, document );

    // create [geom] element
    QDomElement geomElement = document.createElement( "geom" );
    node.appendChild( geomElement );

    // append the optional attributes
    if ( geomType() != InscribedSquare )
        geomElement.setAttribute( "type", (int)geomType() );
    if ( geomInnerColor().isValid() )
        geomElement.setAttribute( "color", geomInnerColor().name() );
}

Annotation::SubType GeomAnnotation::subType() const
{
    return AGeom;
}

GeomAnnotation::GeomType GeomAnnotation::geomType() const
{
    Q_D( const GeomAnnotation );
    return d->geomType;
}

void GeomAnnotation::setGeomType( GeomAnnotation::GeomType type )
{
    Q_D( GeomAnnotation );
    d->geomType = type;
}

QColor GeomAnnotation::geomInnerColor() const
{
    Q_D( const GeomAnnotation );
    return d->geomInnerColor;
}

void GeomAnnotation::setGeomInnerColor( const QColor &color )
{
    Q_D( GeomAnnotation );
    d->geomInnerColor = color;
}


/** HighlightAnnotation [Annotation] */
class HighlightAnnotationPrivate : public AnnotationPrivate
{
    public:
        HighlightAnnotationPrivate();
        Annotation * makeAlias();

        // data fields
        HighlightAnnotation::HighlightType highlightType;
        QList< HighlightAnnotation::Quad > highlightQuads; // not empty
};

HighlightAnnotationPrivate::HighlightAnnotationPrivate()
    : AnnotationPrivate(), highlightType( HighlightAnnotation::Highlight )
{
}

Annotation * HighlightAnnotationPrivate::makeAlias()
{
    return new HighlightAnnotation(*this);
}

HighlightAnnotation::HighlightAnnotation()
    : Annotation( *new HighlightAnnotationPrivate() )
{}

HighlightAnnotation::HighlightAnnotation(HighlightAnnotationPrivate &dd)
    : Annotation( dd )
{}

HighlightAnnotation::HighlightAnnotation( const QDomNode & node )
    : Annotation( *new HighlightAnnotationPrivate(), node )
{
    // loop through the whole children looking for a 'hl' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "hl" )
            continue;

        // parse the attributes
        if ( e.hasAttribute( "type" ) )
            setHighlightType((HighlightAnnotation::HighlightType)e.attribute( "type" ).toInt());

        // parse all 'quad' subnodes
        QList<HighlightAnnotation::Quad> quads;
        QDomNode quadNode = e.firstChild();
        for ( ; quadNode.isElement(); quadNode = quadNode.nextSibling() )
        {
            QDomElement qe = quadNode.toElement();
            if ( qe.tagName() != "quad" )
                continue;

            Quad q;
            q.points[0].setX(qe.attribute( "ax", "0.0" ).toDouble());
            q.points[0].setY(qe.attribute( "ay", "0.0" ).toDouble());
            q.points[1].setX(qe.attribute( "bx", "0.0" ).toDouble());
            q.points[1].setY(qe.attribute( "by", "0.0" ).toDouble());
            q.points[2].setX(qe.attribute( "cx", "0.0" ).toDouble());
            q.points[2].setY(qe.attribute( "cy", "0.0" ).toDouble());
            q.points[3].setX(qe.attribute( "dx", "0.0" ).toDouble());
            q.points[3].setY(qe.attribute( "dy", "0.0" ).toDouble());
            q.capStart = qe.hasAttribute( "start" );
            q.capEnd = qe.hasAttribute( "end" );
            q.feather = qe.attribute( "feather", "0.1" ).toDouble();
            quads.append( q );
        }
        setHighlightQuads(quads);

        // loading complete
        break;
    }
}

HighlightAnnotation::~HighlightAnnotation()
{
}

void HighlightAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // store base annotation properties
    storeBaseAnnotationProperties( node, document );

    // create [hl] element
    QDomElement hlElement = document.createElement( "hl" );
    node.appendChild( hlElement );

    // append the optional attributes
    if ( highlightType() != Highlight )
        hlElement.setAttribute( "type", (int)highlightType() );

    const QList<HighlightAnnotation::Quad> quads = highlightQuads();
    if ( quads.count() < 1 )
        return;
    // append highlight quads, all children describe quads
    QList< HighlightAnnotation::Quad >::const_iterator it = quads.begin(), end = quads.end();
    for ( ; it != end; ++it )
    {
        QDomElement quadElement = document.createElement( "quad" );
        hlElement.appendChild( quadElement );
        const Quad & q = *it;
        quadElement.setAttribute( "ax", QString::number( q.points[0].x() ) );
        quadElement.setAttribute( "ay", QString::number( q.points[0].y() ) );
        quadElement.setAttribute( "bx", QString::number( q.points[1].x() ) );
        quadElement.setAttribute( "by", QString::number( q.points[1].y() ) );
        quadElement.setAttribute( "cx", QString::number( q.points[2].x() ) );
        quadElement.setAttribute( "cy", QString::number( q.points[2].y() ) );
        quadElement.setAttribute( "dx", QString::number( q.points[3].x() ) );
        quadElement.setAttribute( "dy", QString::number( q.points[3].y() ) );
        if ( q.capStart )
            quadElement.setAttribute( "start", 1 );
        if ( q.capEnd )
            quadElement.setAttribute( "end", 1 );
        quadElement.setAttribute( "feather", QString::number( q.feather ) );
    }
}

Annotation::SubType HighlightAnnotation::subType() const
{
    return AHighlight;
}

HighlightAnnotation::HighlightType HighlightAnnotation::highlightType() const
{
    Q_D( const HighlightAnnotation );
    return d->highlightType;
}

void HighlightAnnotation::setHighlightType( HighlightAnnotation::HighlightType type )
{
    Q_D( HighlightAnnotation );
    d->highlightType = type;
}

QList< HighlightAnnotation::Quad > HighlightAnnotation::highlightQuads() const
{
    Q_D( const HighlightAnnotation );
    return d->highlightQuads;
}

void HighlightAnnotation::setHighlightQuads( const QList< HighlightAnnotation::Quad > &quads )
{
    Q_D( HighlightAnnotation );
    d->highlightQuads = quads;
}


/** StampAnnotation [Annotation] */
class StampAnnotationPrivate : public AnnotationPrivate
{
    public:
        StampAnnotationPrivate();
        Annotation * makeAlias();

        // data fields
        QString stampIconName;
};

StampAnnotationPrivate::StampAnnotationPrivate()
    : AnnotationPrivate(), stampIconName( "Draft" )
{
}

Annotation * StampAnnotationPrivate::makeAlias()
{
    return new StampAnnotation(*this);
}

StampAnnotation::StampAnnotation()
    : Annotation( *new StampAnnotationPrivate() )
{}

StampAnnotation::StampAnnotation(StampAnnotationPrivate &dd)
    : Annotation( dd )
{}

StampAnnotation::StampAnnotation( const QDomNode & node )
    : Annotation( *new StampAnnotationPrivate(), node )
{
    // loop through the whole children looking for a 'stamp' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "stamp" )
            continue;

        // parse the attributes
        if ( e.hasAttribute( "icon" ) )
            setStampIconName(e.attribute( "icon" ));

        // loading complete
        break;
    }
}

StampAnnotation::~StampAnnotation()
{
}

void StampAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // store base annotation properties
    storeBaseAnnotationProperties( node, document );

    // create [stamp] element
    QDomElement stampElement = document.createElement( "stamp" );
    node.appendChild( stampElement );

    // append the optional attributes
    if ( stampIconName() != "Draft" )
        stampElement.setAttribute( "icon", stampIconName() );
}

Annotation::SubType StampAnnotation::subType() const
{
    return AStamp;
}

QString StampAnnotation::stampIconName() const
{
    Q_D( const StampAnnotation );
    return d->stampIconName;
}

void StampAnnotation::setStampIconName( const QString &name )
{
    Q_D( StampAnnotation );
    d->stampIconName = name;
}

/** InkAnnotation [Annotation] */
class InkAnnotationPrivate : public AnnotationPrivate
{
    public:
        InkAnnotationPrivate();
        Annotation * makeAlias();

        // data fields
        QList< QLinkedList<QPointF> > inkPaths;
};

InkAnnotationPrivate::InkAnnotationPrivate()
    : AnnotationPrivate()
{
}

Annotation * InkAnnotationPrivate::makeAlias()
{
    return new InkAnnotation(*this);
}

InkAnnotation::InkAnnotation()
    : Annotation( *new InkAnnotationPrivate() )
{}

InkAnnotation::InkAnnotation(InkAnnotationPrivate &dd)
    : Annotation( dd )
{}

InkAnnotation::InkAnnotation( const QDomNode & node )
    : Annotation( *new InkAnnotationPrivate(), node )
{
    // loop through the whole children looking for a 'ink' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "ink" )
            continue;

        // parse the 'path' subnodes
        QList< QLinkedList<QPointF> > paths;
        QDomNode pathNode = e.firstChild();
        while ( pathNode.isElement() )
        {
            QDomElement pathElement = pathNode.toElement();
            pathNode = pathNode.nextSibling();

            if ( pathElement.tagName() != "path" )
                continue;

            // build each path parsing 'point' subnodes
            QLinkedList<QPointF> path;
            QDomNode pointNode = pathElement.firstChild();
            while ( pointNode.isElement() )
            {
                QDomElement pointElement = pointNode.toElement();
                pointNode = pointNode.nextSibling();

                if ( pointElement.tagName() != "point" )
                    continue;

                QPointF p(pointElement.attribute( "x", "0.0" ).toDouble(), pointElement.attribute( "y", "0.0" ).toDouble());
                path.append( p );
            }

            // add the path to the path list if it contains at least 2 nodes
            if ( path.count() >= 2 )
                paths.append( path );
        }
        setInkPaths(paths);

        // loading complete
        break;
    }
}

InkAnnotation::~InkAnnotation()
{
}

void InkAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // store base annotation properties
    storeBaseAnnotationProperties( node, document );

    // create [ink] element
    QDomElement inkElement = document.createElement( "ink" );
    node.appendChild( inkElement );

    // append the optional attributes
    const QList< QLinkedList<QPointF> > paths = inkPaths();
    if ( paths.count() < 1 )
        return;
    QList< QLinkedList<QPointF> >::const_iterator pIt = paths.begin(), pEnd = paths.end();
    for ( ; pIt != pEnd; ++pIt )
    {
        QDomElement pathElement = document.createElement( "path" );
        inkElement.appendChild( pathElement );
        const QLinkedList<QPointF> & path = *pIt;
        QLinkedList<QPointF>::const_iterator iIt = path.begin(), iEnd = path.end();
        for ( ; iIt != iEnd; ++iIt )
        {
            const QPointF & point = *iIt;
            QDomElement pointElement = document.createElement( "point" );
            pathElement.appendChild( pointElement );
            pointElement.setAttribute( "x", QString::number( point.x() ) );
            pointElement.setAttribute( "y", QString::number( point.y() ) );
        }
    }
}

Annotation::SubType InkAnnotation::subType() const
{
    return AInk;
}

QList< QLinkedList<QPointF> > InkAnnotation::inkPaths() const
{
   Q_D( const InkAnnotation );
   return d->inkPaths;
}

void InkAnnotation::setInkPaths( const QList< QLinkedList<QPointF> > &paths )
{
   Q_D( InkAnnotation );
   d->inkPaths = paths;
}


/** LinkAnnotation [Annotation] */
class LinkAnnotationPrivate : public AnnotationPrivate
{
    public:
        LinkAnnotationPrivate();
        ~LinkAnnotationPrivate();
        Annotation * makeAlias();

        // data fields
        Link * linkDestination;
        LinkAnnotation::HighlightMode linkHLMode;
        QPointF linkRegion[4];
};

LinkAnnotationPrivate::LinkAnnotationPrivate()
    : AnnotationPrivate(), linkDestination( 0 ), linkHLMode( LinkAnnotation::Invert )
{
}

LinkAnnotationPrivate::~LinkAnnotationPrivate()
{
    delete linkDestination;
}

Annotation * LinkAnnotationPrivate::makeAlias()
{
    return new LinkAnnotation(*this);
}

LinkAnnotation::LinkAnnotation()
    : Annotation( *new LinkAnnotationPrivate() )
{}

LinkAnnotation::LinkAnnotation(LinkAnnotationPrivate &dd)
    : Annotation( dd )
{}

LinkAnnotation::LinkAnnotation( const QDomNode & node )
    : Annotation( *new LinkAnnotationPrivate(), node )
{
    // loop through the whole children looking for a 'link' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "link" )
            continue;

        // parse the attributes
        if ( e.hasAttribute( "hlmode" ) )
            setLinkHighlightMode((LinkAnnotation::HighlightMode)e.attribute( "hlmode" ).toInt());

        // parse all 'quad' subnodes
        QDomNode quadNode = e.firstChild();
        for ( ; quadNode.isElement(); quadNode = quadNode.nextSibling() )
        {
            QDomElement qe = quadNode.toElement();
            if ( qe.tagName() == "quad" )
            {
                setLinkRegionPoint(0, QPointF(qe.attribute( "ax", "0.0" ).toDouble(),
                                              qe.attribute( "ay", "0.0" ).toDouble()));
                setLinkRegionPoint(1, QPointF(qe.attribute( "bx", "0.0" ).toDouble(),
                                              qe.attribute( "by", "0.0" ).toDouble()));
                setLinkRegionPoint(2, QPointF(qe.attribute( "cx", "0.0" ).toDouble(),
                                              qe.attribute( "cy", "0.0" ).toDouble()));
                setLinkRegionPoint(3, QPointF(qe.attribute( "dx", "0.0" ).toDouble(),
                                              qe.attribute( "dy", "0.0" ).toDouble()));
            }
            else if ( qe.tagName() == "link" )
            {
                QString type = qe.attribute( "type" );
                if ( type == "GoTo" )
                {
                    Poppler::LinkGoto * go = new Poppler::LinkGoto( QRect(), qe.attribute( "filename" ), LinkDestination( qe.attribute( "destination" ) ) );
                    setLinkDestination(go);
                }
                else if ( type == "Exec" )
                {
                    Poppler::LinkExecute * exec = new Poppler::LinkExecute( QRect(), qe.attribute( "filename" ), qe.attribute( "parameters" ) );
                    setLinkDestination(exec);
                }
                else if ( type == "Browse" )
                {
                    Poppler::LinkBrowse * browse = new Poppler::LinkBrowse( QRect(), qe.attribute( "url" ) );
                    setLinkDestination(browse);
                }
                else if ( type == "Action" )
                {
                    Poppler::LinkAction::ActionType act;
                    QString actString = qe.attribute( "action" );
                    bool found = true;
                    if ( actString == "PageFirst" )
                        act = Poppler::LinkAction::PageFirst;
                    else if ( actString == "PagePrev" )
                        act = Poppler::LinkAction::PagePrev;
                    else if ( actString == "PageNext" )
                        act = Poppler::LinkAction::PageNext;
                    else if ( actString == "PageLast" )
                        act = Poppler::LinkAction::PageLast;
                    else if ( actString == "HistoryBack" )
                        act = Poppler::LinkAction::HistoryBack;
                    else if ( actString == "HistoryForward" )
                        act = Poppler::LinkAction::HistoryForward;
                    else if ( actString == "Quit" )
                        act = Poppler::LinkAction::Quit;
                    else if ( actString == "Presentation" )
                        act = Poppler::LinkAction::Presentation;
                    else if ( actString == "EndPresentation" )
                        act = Poppler::LinkAction::EndPresentation;
                    else if ( actString == "Find" )
                        act = Poppler::LinkAction::Find;
                    else if ( actString == "GoToPage" )
                        act = Poppler::LinkAction::GoToPage;
                    else if ( actString == "Close" )
                        act = Poppler::LinkAction::Close;
                    else if ( actString == "Print" )
                        act = Poppler::LinkAction::Print;
                    else
                        found = false;
                    if (found)
                    {
                        Poppler::LinkAction * action = new Poppler::LinkAction( QRect(), act );
                        setLinkDestination(action);
                    }
                }
#if 0
                else if ( type == "Movie" )
                {
                    Poppler::LinkMovie * movie = new Poppler::LinkMovie( QRect() );
                    setLinkDestination(movie);
                }
#endif
            }
        }

        // loading complete
        break;
    }
}

LinkAnnotation::~LinkAnnotation()
{
}

void LinkAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // store base annotation properties
    storeBaseAnnotationProperties( node, document );

    // create [hl] element
    QDomElement linkElement = document.createElement( "link" );
    node.appendChild( linkElement );

    // append the optional attributes
    if ( linkHighlightMode() != Invert )
        linkElement.setAttribute( "hlmode", (int)linkHighlightMode() );

    // saving region
    QDomElement quadElement = document.createElement( "quad" );
    linkElement.appendChild( quadElement );
    quadElement.setAttribute( "ax", QString::number( linkRegionPoint(0).x() ) );
    quadElement.setAttribute( "ay", QString::number( linkRegionPoint(0).y() ) );
    quadElement.setAttribute( "bx", QString::number( linkRegionPoint(1).x() ) );
    quadElement.setAttribute( "by", QString::number( linkRegionPoint(1).y() ) );
    quadElement.setAttribute( "cx", QString::number( linkRegionPoint(2).x() ) );
    quadElement.setAttribute( "cy", QString::number( linkRegionPoint(2).y() ) );
    quadElement.setAttribute( "dx", QString::number( linkRegionPoint(3).x() ) );
    quadElement.setAttribute( "dy", QString::number( linkRegionPoint(3).y() ) );

    // saving link
    QDomElement hyperlinkElement = document.createElement( "link" );
    linkElement.appendChild( hyperlinkElement );
    if ( linkDestination() )
    {
        switch( linkDestination()->linkType() )
        {
            case Poppler::Link::Goto:
            {
                Poppler::LinkGoto * go = static_cast< Poppler::LinkGoto * >( linkDestination() );
                hyperlinkElement.setAttribute( "type", "GoTo" );
                hyperlinkElement.setAttribute( "filename", go->fileName() );
                hyperlinkElement.setAttribute( "destionation", go->destination().toString() );
                break;
            }
            case Poppler::Link::Execute:
            {
                Poppler::LinkExecute * exec = static_cast< Poppler::LinkExecute * >( linkDestination() );
                hyperlinkElement.setAttribute( "type", "Exec" );
                hyperlinkElement.setAttribute( "filename", exec->fileName() );
                hyperlinkElement.setAttribute( "parameters", exec->parameters() );
                break;
            }
            case Poppler::Link::Browse:
            {
                Poppler::LinkBrowse * browse = static_cast< Poppler::LinkBrowse * >( linkDestination() );
                hyperlinkElement.setAttribute( "type", "Browse" );
                hyperlinkElement.setAttribute( "url", browse->url() );
                break;
            }
            case Poppler::Link::Action:
            {
                Poppler::LinkAction * action = static_cast< Poppler::LinkAction * >( linkDestination() );
                hyperlinkElement.setAttribute( "type", "Action" );
                switch ( action->actionType() )
                {
                    case Poppler::LinkAction::PageFirst:
                        hyperlinkElement.setAttribute( "action", "PageFirst" );
                        break;
                    case Poppler::LinkAction::PagePrev:
                        hyperlinkElement.setAttribute( "action", "PagePrev" );
                        break;
                    case Poppler::LinkAction::PageNext:
                        hyperlinkElement.setAttribute( "action", "PageNext" );
                        break;
                    case Poppler::LinkAction::PageLast:
                        hyperlinkElement.setAttribute( "action", "PageLast" );
                        break;
                    case Poppler::LinkAction::HistoryBack:
                        hyperlinkElement.setAttribute( "action", "HistoryBack" );
                        break;
                    case Poppler::LinkAction::HistoryForward:
                        hyperlinkElement.setAttribute( "action", "HistoryForward" );
                        break;
                    case Poppler::LinkAction::Quit:
                        hyperlinkElement.setAttribute( "action", "Quit" );
                        break;
                    case Poppler::LinkAction::Presentation:
                        hyperlinkElement.setAttribute( "action", "Presentation" );
                        break;
                    case Poppler::LinkAction::EndPresentation:
                        hyperlinkElement.setAttribute( "action", "EndPresentation" );
                        break;
                    case Poppler::LinkAction::Find:
                        hyperlinkElement.setAttribute( "action", "Find" );
                        break;
                    case Poppler::LinkAction::GoToPage:
                        hyperlinkElement.setAttribute( "action", "GoToPage" );
                        break;
                    case Poppler::LinkAction::Close:
                        hyperlinkElement.setAttribute( "action", "Close" );
                        break;
                    case Poppler::LinkAction::Print:
                        hyperlinkElement.setAttribute( "action", "Print" );
                        break;
                }
                break;
            }
            case Poppler::Link::Movie:
            {
                hyperlinkElement.setAttribute( "type", "Movie" );
                break;
            }
            case Poppler::Link::Rendition:
            {
                hyperlinkElement.setAttribute( "type", "Rendition" );
                break;
            }
            case Poppler::Link::Sound:
            {
                // FIXME: implement me
                break;
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

Link* LinkAnnotation::linkDestination() const
{
    Q_D( const LinkAnnotation );
    return d->linkDestination;
}

void LinkAnnotation::setLinkDestination( Link *link )
{
    Q_D( LinkAnnotation );
    delete d->linkDestination;
    d->linkDestination = link;
}

LinkAnnotation::HighlightMode LinkAnnotation::linkHighlightMode() const
{
    Q_D( const LinkAnnotation );
    return d->linkHLMode;
}

void LinkAnnotation::setLinkHighlightMode( LinkAnnotation::HighlightMode mode )
{
    Q_D( LinkAnnotation );
    d->linkHLMode = mode;
}

QPointF LinkAnnotation::linkRegionPoint( int id ) const
{
    if ( id < 0 || id >= 4 )
        return QPointF();

    Q_D( const LinkAnnotation );
    return d->linkRegion[id];
}

void LinkAnnotation::setLinkRegionPoint( int id, const QPointF &point )
{
    if ( id < 0 || id >= 4 )
        return;

    Q_D( LinkAnnotation );
    d->linkRegion[id] = point;
}

/** CaretAnnotation [Annotation] */
class CaretAnnotationPrivate : public AnnotationPrivate
{
    public:
        CaretAnnotationPrivate();
        Annotation * makeAlias();

        // data fields
        CaretAnnotation::CaretSymbol symbol;
};

static QString caretSymbolToString( CaretAnnotation::CaretSymbol symbol )
{
    switch ( symbol )
    {
        case CaretAnnotation::None:
            return QString::fromLatin1( "None" );
        case CaretAnnotation::P:
            return QString::fromLatin1( "P" );
    }
    return QString();
}

static CaretAnnotation::CaretSymbol caretSymbolFromString( const QString &symbol )
{
    if ( symbol == QLatin1String( "None" ) )
        return CaretAnnotation::None;
    else if ( symbol == QLatin1String( "P" ) )
        return CaretAnnotation::P;
    return CaretAnnotation::None;
}

CaretAnnotationPrivate::CaretAnnotationPrivate()
    : AnnotationPrivate(), symbol( CaretAnnotation::None )
{
}

Annotation * CaretAnnotationPrivate::makeAlias()
{
    return new CaretAnnotation(*this);
}

CaretAnnotation::CaretAnnotation()
    : Annotation( *new CaretAnnotationPrivate() )
{
}

CaretAnnotation::CaretAnnotation(CaretAnnotationPrivate &dd)
    : Annotation( dd )
{
}

CaretAnnotation::CaretAnnotation( const QDomNode & node )
    : Annotation( *new CaretAnnotationPrivate(), node )
{
    // loop through the whole children looking for a 'caret' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "caret" )
            continue;

        // parse the attributes
        if ( e.hasAttribute( "symbol" ) )
            setCaretSymbol(caretSymbolFromString( e.attribute( "symbol" ) ));

        // loading complete
        break;
    }
}

CaretAnnotation::~CaretAnnotation()
{
}

void CaretAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // store base annotation properties
    storeBaseAnnotationProperties( node, document );

    // create [caret] element
    QDomElement caretElement = document.createElement( "caret" );
    node.appendChild( caretElement );

    // append the optional attributes
    if ( caretSymbol() != CaretAnnotation::None )
        caretElement.setAttribute( "symbol", caretSymbolToString( caretSymbol() ) );
}

Annotation::SubType CaretAnnotation::subType() const
{
    return ACaret;
}

CaretAnnotation::CaretSymbol CaretAnnotation::caretSymbol() const
{
    Q_D( const CaretAnnotation );
    return d->symbol;
}

void CaretAnnotation::setCaretSymbol( CaretAnnotation::CaretSymbol symbol )
{
    Q_D( CaretAnnotation );
    d->symbol = symbol;
}

/** FileAttachmentAnnotation [Annotation] */
class FileAttachmentAnnotationPrivate : public AnnotationPrivate
{
    public:
        FileAttachmentAnnotationPrivate();
        ~FileAttachmentAnnotationPrivate();
        Annotation * makeAlias();

        // data fields
        QString icon;
        EmbeddedFile *embfile;
};

FileAttachmentAnnotationPrivate::FileAttachmentAnnotationPrivate()
    : AnnotationPrivate(), icon( "PushPin" ), embfile( 0 )
{
}

FileAttachmentAnnotationPrivate::~FileAttachmentAnnotationPrivate()
{
    delete embfile;
}

Annotation * FileAttachmentAnnotationPrivate::makeAlias()
{
    return new FileAttachmentAnnotation(*this);
}

FileAttachmentAnnotation::FileAttachmentAnnotation()
    : Annotation( *new FileAttachmentAnnotationPrivate() )
{
}

FileAttachmentAnnotation::FileAttachmentAnnotation(FileAttachmentAnnotationPrivate &dd)
    : Annotation( dd )
{
}

FileAttachmentAnnotation::FileAttachmentAnnotation( const QDomNode & node )
    : Annotation( *new FileAttachmentAnnotationPrivate(), node )
{
    // loop through the whole children looking for a 'fileattachment' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "fileattachment" )
            continue;

        // loading complete
        break;
    }
}

FileAttachmentAnnotation::~FileAttachmentAnnotation()
{
}

void FileAttachmentAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // store base annotation properties
    storeBaseAnnotationProperties( node, document );

    // create [fileattachment] element
    QDomElement fileAttachmentElement = document.createElement( "fileattachment" );
    node.appendChild( fileAttachmentElement );
}

Annotation::SubType FileAttachmentAnnotation::subType() const
{
    return AFileAttachment;
}

QString FileAttachmentAnnotation::fileIconName() const
{
    Q_D( const FileAttachmentAnnotation );
    return d->icon;
}

void FileAttachmentAnnotation::setFileIconName( const QString &icon )
{
    Q_D( FileAttachmentAnnotation );
    d->icon = icon;
}

EmbeddedFile* FileAttachmentAnnotation::embeddedFile() const
{
    Q_D( const FileAttachmentAnnotation );
    return d->embfile;
}

void FileAttachmentAnnotation::setEmbeddedFile( EmbeddedFile *ef )
{
    Q_D( FileAttachmentAnnotation );
    d->embfile = ef;
}

/** SoundAnnotation [Annotation] */
class SoundAnnotationPrivate : public AnnotationPrivate
{
    public:
        SoundAnnotationPrivate();
        ~SoundAnnotationPrivate();
        Annotation * makeAlias();

        // data fields
        QString icon;
        SoundObject *sound;
};

SoundAnnotationPrivate::SoundAnnotationPrivate()
    : AnnotationPrivate(), icon( "Speaker" ), sound( 0 )
{
}

SoundAnnotationPrivate::~SoundAnnotationPrivate()
{
    delete sound;
}

Annotation * SoundAnnotationPrivate::makeAlias()
{
    return new SoundAnnotation(*this);
}

SoundAnnotation::SoundAnnotation()
    : Annotation( *new SoundAnnotationPrivate() )
{
}

SoundAnnotation::SoundAnnotation(SoundAnnotationPrivate &dd)
    : Annotation( dd )
{
}

SoundAnnotation::SoundAnnotation( const QDomNode & node )
    : Annotation( *new SoundAnnotationPrivate(), node )
{
    // loop through the whole children looking for a 'sound' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "sound" )
            continue;

        // loading complete
        break;
    }
}

SoundAnnotation::~SoundAnnotation()
{
}

void SoundAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // store base annotation properties
    storeBaseAnnotationProperties( node, document );

    // create [sound] element
    QDomElement soundElement = document.createElement( "sound" );
    node.appendChild( soundElement );
}

Annotation::SubType SoundAnnotation::subType() const
{
    return ASound;
}

QString SoundAnnotation::soundIconName() const
{
    Q_D( const SoundAnnotation );
    return d->icon;
}

void SoundAnnotation::setSoundIconName( const QString &icon )
{
    Q_D( SoundAnnotation );
    d->icon = icon;
}

SoundObject* SoundAnnotation::sound() const
{
    Q_D( const SoundAnnotation );
    return d->sound;
}

void SoundAnnotation::setSound( SoundObject *s )
{
    Q_D( SoundAnnotation );
    d->sound = s;
}

/** MovieAnnotation [Annotation] */
class MovieAnnotationPrivate : public AnnotationPrivate
{
    public:
        MovieAnnotationPrivate();
        ~MovieAnnotationPrivate();
        Annotation * makeAlias();

        // data fields
        MovieObject *movie;
        QString title;
};

MovieAnnotationPrivate::MovieAnnotationPrivate()
    : AnnotationPrivate(), movie( 0 )
{
}

MovieAnnotationPrivate::~MovieAnnotationPrivate()
{
    delete movie;
}

Annotation * MovieAnnotationPrivate::makeAlias()
{
    return new MovieAnnotation(*this);
}

MovieAnnotation::MovieAnnotation()
    : Annotation( *new MovieAnnotationPrivate() )
{
}

MovieAnnotation::MovieAnnotation(MovieAnnotationPrivate &dd)
    : Annotation( dd )
{
}

MovieAnnotation::MovieAnnotation( const QDomNode & node )
    : Annotation( *new MovieAnnotationPrivate(), node )
{
    // loop through the whole children looking for a 'movie' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "movie" )
            continue;

        // loading complete
        break;
    }
}

MovieAnnotation::~MovieAnnotation()
{
}

void MovieAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // store base annotation properties
    storeBaseAnnotationProperties( node, document );

    // create [movie] element
    QDomElement movieElement = document.createElement( "movie" );
    node.appendChild( movieElement );
}

Annotation::SubType MovieAnnotation::subType() const
{
    return AMovie;
}

MovieObject* MovieAnnotation::movie() const
{
    Q_D( const MovieAnnotation );
    return d->movie;
}

void MovieAnnotation::setMovie( MovieObject *movie )
{
    Q_D( MovieAnnotation );
    d->movie = movie;
}

QString MovieAnnotation::movieTitle() const
{
    Q_D( const MovieAnnotation );
    return d->title;
}

void MovieAnnotation::setMovieTitle( const QString &title )
{
    Q_D( MovieAnnotation );
    d->title = title;
}

/** ScreenAnnotation [Annotation] */
class ScreenAnnotationPrivate : public AnnotationPrivate
{
    public:
        ScreenAnnotationPrivate();
        ~ScreenAnnotationPrivate();
        Annotation * makeAlias();

        // data fields
        LinkRendition *action;
        QString title;
};

ScreenAnnotationPrivate::ScreenAnnotationPrivate()
    : AnnotationPrivate(), action( 0 )
{
}

ScreenAnnotationPrivate::~ScreenAnnotationPrivate()
{
    delete action;
}

ScreenAnnotation::ScreenAnnotation(ScreenAnnotationPrivate &dd)
    : Annotation( dd )
{}

Annotation * ScreenAnnotationPrivate::makeAlias()
{
    return new ScreenAnnotation(*this);
}

ScreenAnnotation::ScreenAnnotation()
    : Annotation( *new ScreenAnnotationPrivate() )
{
}

ScreenAnnotation::~ScreenAnnotation()
{
}

void ScreenAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // store base annotation properties
    storeBaseAnnotationProperties( node, document );

    // create [screen] element
    QDomElement screenElement = document.createElement( "screen" );
    node.appendChild( screenElement );
}

Annotation::SubType ScreenAnnotation::subType() const
{
    return AScreen;
}

LinkRendition* ScreenAnnotation::action() const
{
    Q_D( const ScreenAnnotation );
    return d->action;
}

void ScreenAnnotation::setAction( LinkRendition *action )
{
    Q_D( ScreenAnnotation );
    d->action = action;
}

QString ScreenAnnotation::screenTitle() const
{
    Q_D( const ScreenAnnotation );
    return d->title;
}

void ScreenAnnotation::setScreenTitle( const QString &title )
{
    Q_D( ScreenAnnotation );
    d->title = title;
}

//BEGIN utility annotation functions
QColor convertAnnotColor( AnnotColor *color )
{
    if ( !color )
        return QColor();

    QColor newcolor;
    const double *color_data = color->getValues();
    switch ( color->getSpace() )
    {
        case AnnotColor::colorTransparent: // = 0,
            newcolor = Qt::transparent;
            break;
        case AnnotColor::colorGray: // = 1,
            newcolor.setRgbF( color_data[0], color_data[0], color_data[0] );
            break;
        case AnnotColor::colorRGB: // = 3,
            newcolor.setRgbF( color_data[0], color_data[1], color_data[2] );
            break;
        case AnnotColor::colorCMYK: // = 4
            newcolor.setCmykF( color_data[0], color_data[1], color_data[2], color_data[3] );
            break;
    }
    return newcolor;
}
//END utility annotation functions

}
