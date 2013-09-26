/* poppler-structure.cc: glib interface to poppler
 *
 * Copyright (C) 2013 Igalia S.L.
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

#include "config.h"

#ifndef __GI_SCANNER__
#include <StructTreeRoot.h>
#include <StructElement.h>
#include <GlobalParams.h>
#include <UnicodeMap.h>
#endif /* !__GI_SCANNER__ */

#include "poppler.h"
#include "poppler-private.h"
#include "poppler-structure-element.h"


/**
 * SECTION:poppler-structure-element
 * @short_description: Document structure element.
 * @title: PopplerStructureElement
 * @see_also: #PopplerStructure
 *
 * Instances of #PopplerStructureElement are used to describe the structure
 * of a #PopplerDocument. To access the elements in the structure of the
 * document, first use poppler_document_get_structure() to obtain its
 * #PopplerStructure, and then use poppler_structure_get_n_children()
 * and poppler_structure_get_child() to enumerate the top level elements.
 */

typedef struct _PopplerStructureElementClass
{
  GObjectClass parent_class;
} PopplerStructureElementClass;

G_DEFINE_TYPE (PopplerStructureElement, poppler_structure_element, G_TYPE_OBJECT);

static PopplerStructureElement *
_poppler_structure_element_new (PopplerDocument *document, StructElement *element)
{
  PopplerStructureElement *poppler_structure_element;

  g_assert (POPPLER_IS_DOCUMENT (document));
  g_assert (element);

  poppler_structure_element = (PopplerStructureElement *) g_object_new (POPPLER_TYPE_STRUCTURE_ELEMENT, NULL, NULL);
  poppler_structure_element->document = (PopplerDocument *) g_object_ref (document);
  poppler_structure_element->elem = element;

  return poppler_structure_element;
}


static void
poppler_structure_element_init (PopplerStructureElement *poppler_structure_element)
{
}


static void
poppler_structure_element_finalize (GObject *object)
{
  PopplerStructureElement *poppler_structure_element = POPPLER_STRUCTURE_ELEMENT (object);

  /* poppler_structure_element->elem is owned by the StructTreeRoot */
  g_object_unref (poppler_structure_element->document);

  G_OBJECT_CLASS (poppler_structure_element_parent_class)->finalize (object);
}


static void
poppler_structure_element_class_init (PopplerStructureElementClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = poppler_structure_element_finalize;
}


/**
 * poppler_structure_element_get_kind:
 * @poppler_structure_element: A #PopplerStructureElement
 *
 * Return value: A #PopplerStructureElementKind value.
 *
 * Since: 0.26
 */
PopplerStructureElementKind
poppler_structure_element_get_kind (PopplerStructureElement *poppler_structure_element)
{
  g_return_val_if_fail (POPPLER_IS_STRUCTURE_ELEMENT (poppler_structure_element), POPPLER_STRUCTURE_ELEMENT_UNKNOWN);
  g_return_val_if_fail (poppler_structure_element->elem != NULL, POPPLER_STRUCTURE_ELEMENT_UNKNOWN);

  switch (poppler_structure_element->elem->getType ())
    {
      case StructElement::Unknown:
        return POPPLER_STRUCTURE_ELEMENT_UNKNOWN;
      case StructElement::MCID:
        return POPPLER_STRUCTURE_ELEMENT_CONTENT;
      case StructElement::OBJR:
        return POPPLER_STRUCTURE_ELEMENT_OBJECT_REFERENCE;
      case StructElement::Document:
        return POPPLER_STRUCTURE_ELEMENT_DOCUMENT;
      case StructElement::Part:
        return POPPLER_STRUCTURE_ELEMENT_PART;
      case StructElement::Sect:
        return POPPLER_STRUCTURE_ELEMENT_SECTION;
      case StructElement::Div:
        return POPPLER_STRUCTURE_ELEMENT_DIV;
      case StructElement::Span:
        return POPPLER_STRUCTURE_ELEMENT_SPAN;
      case StructElement::Quote:
        return POPPLER_STRUCTURE_ELEMENT_QUOTE;
      case StructElement::Note:
        return POPPLER_STRUCTURE_ELEMENT_NOTE;
      case StructElement::Reference:
        return POPPLER_STRUCTURE_ELEMENT_REFERENCE;
      case StructElement::BibEntry:
        return POPPLER_STRUCTURE_ELEMENT_BIBENTRY;
      case StructElement::Code:
        return POPPLER_STRUCTURE_ELEMENT_CODE;
      case StructElement::Link:
        return POPPLER_STRUCTURE_ELEMENT_LINK;
      case StructElement::Annot:
        return POPPLER_STRUCTURE_ELEMENT_ANNOT;
      case StructElement::Ruby:
        return POPPLER_STRUCTURE_ELEMENT_RUBY;
      case StructElement::Warichu:
        return POPPLER_STRUCTURE_ELEMENT_WARICHU;
      case StructElement::BlockQuote:
        return POPPLER_STRUCTURE_ELEMENT_BLOCKQUOTE;
      case StructElement::Caption:
        return POPPLER_STRUCTURE_ELEMENT_CAPTION;
      case StructElement::NonStruct:
        return POPPLER_STRUCTURE_ELEMENT_NONSTRUCT;
      case StructElement::TOC:
        return POPPLER_STRUCTURE_ELEMENT_TOC;
      case StructElement::TOCI:
        return POPPLER_STRUCTURE_ELEMENT_TOC_ITEM;
      case StructElement::Index:
        return POPPLER_STRUCTURE_ELEMENT_INDEX;
      case StructElement::Private:
        return POPPLER_STRUCTURE_ELEMENT_PRIVATE;
      case StructElement::P:
        return POPPLER_STRUCTURE_ELEMENT_PARAGRAPH;
      case StructElement::H:
        return POPPLER_STRUCTURE_ELEMENT_HEADING;
      case StructElement::H1:
        return POPPLER_STRUCTURE_ELEMENT_HEADING_1;
      case StructElement::H2:
        return POPPLER_STRUCTURE_ELEMENT_HEADING_2;
      case StructElement::H3:
        return POPPLER_STRUCTURE_ELEMENT_HEADING_3;
      case StructElement::H4:
        return POPPLER_STRUCTURE_ELEMENT_HEADING_4;
      case StructElement::H5:
        return POPPLER_STRUCTURE_ELEMENT_HEADING_5;
      case StructElement::H6:
        return POPPLER_STRUCTURE_ELEMENT_HEADING_6;
      case StructElement::L:
        return POPPLER_STRUCTURE_ELEMENT_LIST;
      case StructElement::LI:
        return POPPLER_STRUCTURE_ELEMENT_LIST_ITEM;
      case StructElement::Lbl:
        return POPPLER_STRUCTURE_ELEMENT_LIST_LABEL;
      case StructElement::LBody:
        return POPPLER_STRUCTURE_ELEMENT_LIST_BODY;
      case StructElement::Table:
        return POPPLER_STRUCTURE_ELEMENT_TABLE;
      case StructElement::TR:
        return POPPLER_STRUCTURE_ELEMENT_TABLE_ROW;
      case StructElement::TH:
        return POPPLER_STRUCTURE_ELEMENT_TABLE_HEADING;
      case StructElement::TD:
        return POPPLER_STRUCTURE_ELEMENT_TABLE_DATA;
      case StructElement::THead:
        return POPPLER_STRUCTURE_ELEMENT_TABLE_HEADER;
      case StructElement::TFoot:
        return POPPLER_STRUCTURE_ELEMENT_TABLE_FOOTER;
      case StructElement::TBody:
        return POPPLER_STRUCTURE_ELEMENT_TABLE_BODY;
      case StructElement::Figure:
        return POPPLER_STRUCTURE_ELEMENT_FIGURE;
      case StructElement::Formula:
        return POPPLER_STRUCTURE_ELEMENT_FORMULA;
      case StructElement::Form:
        return POPPLER_STRUCTURE_ELEMENT_FORM;
      default:
        g_assert_not_reached ();
    }
}

/**
 * poppler_structure_element_get_page:
 * @poppler_structure_element: A #PopplerStructureElement
 *
 * Obtains the page number in which the element is contained.
 *
 * Return value: Number of the page that contains the element, of
 *    <code>-1</code> if not defined.
 *
 * Since: 0.26
 */
gint
poppler_structure_element_get_page (PopplerStructureElement *poppler_structure_element)
{
  g_return_val_if_fail (POPPLER_IS_STRUCTURE_ELEMENT (poppler_structure_element), -1);
  g_return_val_if_fail (poppler_structure_element->elem != NULL, -1);

  Ref ref;
  if (poppler_structure_element->elem->getPageRef (ref))
    {
      return poppler_structure_element->document->doc->findPage(ref.num, ref.gen) - 1;
    }

  return -1;
}

/**
 * poppler_structure_element_is_content:
 * @poppler_structure_element: A #PopplerStructureElement
 *
 * Checks whether an element is actual document content.
 *
 * Return value: %TRUE if the element is content, or %FALSE otherwise.
 *
 * Since: 0.26
 */
gboolean
poppler_structure_element_is_content (PopplerStructureElement *poppler_structure_element)
{
  g_return_val_if_fail (POPPLER_IS_STRUCTURE_ELEMENT (poppler_structure_element), FALSE);
  g_return_val_if_fail (poppler_structure_element->elem != NULL, FALSE);

  return poppler_structure_element->elem->isContent ();
}

/**
 * poppler_structure_element_is_inline:
 * @poppler_structure_element: A #PopplerStructureElement
 *
 * Checks whether an element is an inline element.
 *
 * Return value: %TRUE if the element is an inline element, or %FALSE otherwise.
 *
 * Since: 0.26
 */
gboolean
poppler_structure_element_is_inline (PopplerStructureElement *poppler_structure_element)
{
  g_return_val_if_fail (POPPLER_IS_STRUCTURE_ELEMENT (poppler_structure_element), FALSE);
  g_return_val_if_fail (poppler_structure_element->elem != NULL, FALSE);

  return poppler_structure_element->elem->isInline ();
}

/**
 * poppler_structure_element_is_block:
 * @poppler_structure_element: A #PopplerStructureElement
 *
 * Checks whether an element is a block element.
 *
 * Return value: %TRUE if  the element is a block element, or %FALSE otherwise.
 *
 * Since: 0.26
 */
gboolean
poppler_structure_element_is_block (PopplerStructureElement *poppler_structure_element)
{
  g_return_val_if_fail (POPPLER_IS_STRUCTURE_ELEMENT (poppler_structure_element), FALSE);
  g_return_val_if_fail (poppler_structure_element->elem != NULL, FALSE);

  return poppler_structure_element->elem->isBlock ();
}

/**
 * poppler_structure_element_get_id:
 * @poppler_structure_element: A #PopplerStructureElement
 *
 * Obtains the identifier of an element.
 *
 * Return value: (transfer full): The identifier of the element (if
 *    defined), or %NULL.
 *
 * Since: 0.26
 */
gchar *
poppler_structure_element_get_id (PopplerStructureElement *poppler_structure_element)
{
  g_return_val_if_fail (POPPLER_IS_STRUCTURE_ELEMENT (poppler_structure_element), NULL);
  g_return_val_if_fail (poppler_structure_element->elem != NULL, NULL);

  GooString *string = poppler_structure_element->elem->getID ();
  return string ? _poppler_goo_string_to_utf8 (string) : NULL;
}

/**
 * poppler_structure_element_get_title:
 * @poppler_structure_element: A #PopplerStructureElement
 *
 * Obtains the title of an element.
 *
 * Return value: (transfer full): The title of the element, or %NULL.
 *
 * Since: 0.26
 */
gchar *
poppler_structure_element_get_title (PopplerStructureElement *poppler_structure_element)
{
  g_return_val_if_fail (POPPLER_IS_STRUCTURE_ELEMENT (poppler_structure_element), NULL);
  g_return_val_if_fail (poppler_structure_element->elem != NULL, NULL);

  GooString *string = poppler_structure_element->elem->getTitle ();
  return string ? _poppler_goo_string_to_utf8 (string) : NULL;
}

/**
 * popppler_structure_element_get_abbreviation:
 * @poppler_structure_element: A #PopplerStructureElement
 *
 * Acronyms and abbreviations contained in elements of type
 * #POPPLER_STRUCTURE_ELEMENT_SPAN may have an associated expanded
 * text form, which can be retrieved using this function.
 *
 * Return value: (transfer full): Text of the expanded abbreviation if the
 *    element text is an abbreviation or acrony, %NULL if not.
 *
 * Since: 0.26
 */
gchar *
poppler_structure_element_get_abbreviation (PopplerStructureElement *poppler_structure_element)
{
  g_return_val_if_fail (POPPLER_IS_STRUCTURE_ELEMENT (poppler_structure_element), NULL);
  g_return_val_if_fail (poppler_structure_element->elem != NULL, NULL);

  if (poppler_structure_element->elem->getType () != StructElement::Span)
    return NULL;

  GooString *string = poppler_structure_element->elem->getExpandedAbbr ();
  return string ? _poppler_goo_string_to_utf8 (string) : NULL;
}

/**
 * poppler_structure_element_get_language:
 * @poppler_structure_element: A #PopplerStructureElement
 *
 * Obtains the language and country code for the content in an element,
 * in two-letter ISO format, e.g. <code>en_ES</code>, or %NULL if not
 * defined.
 *
 * Return value: (transfer full): language and country code, or %NULL.
 *
 * Since: 0.26
 */
gchar *
poppler_structure_element_get_language (PopplerStructureElement *poppler_structure_element)
{
  g_return_val_if_fail (POPPLER_IS_STRUCTURE_ELEMENT (poppler_structure_element), NULL);
  g_return_val_if_fail (poppler_structure_element->elem != NULL, NULL);

  GooString *string = poppler_structure_element->elem->getLanguage ();
  return string ? _poppler_goo_string_to_utf8 (string) : NULL;
}

/**
 * poppler_structure_element_get_alt_text:
 * @poppler_structure_element: A #PopplerStructureElement
 *
 * Obtains the “alternate” text representation of the element (and its child
 * elements). This is mostly used for non-text elements like images and
 * figures, to specify a textual description of the element.
 *
 * Note that for elements containing proper text, the function
 * poppler_structure_element_get_text() must be used instead.
 *
 * Return value: (transfer full): The alternate text representation for the
 *    element, or %NULL if not defined.
 *
 * Since: 0.26
 */
gchar *
poppler_structure_element_get_alt_text (PopplerStructureElement *poppler_structure_element)
{
  g_return_val_if_fail (POPPLER_IS_STRUCTURE_ELEMENT (poppler_structure_element), NULL);
  g_return_val_if_fail (poppler_structure_element->elem != NULL, NULL);

  GooString *string = poppler_structure_element->elem->getAltText ();
  return string ? _poppler_goo_string_to_utf8 (string) : NULL;
}

/**
 * poppler_structure_element_get_actual_text:
 * @poppler_structure_element: A #PopplerStructureElement
 *
 * Obtains the actual text enclosed by the element (and its child elements).
 * The actual text is mostly used for non-text elements like images and
 * figures which <em>do</em> have the graphical appearance of text, like
 * a logo. For those the actual text is the equivalent text to those
 * graphical elements which look like text when rendered.
 *
 * Note that for elements containing proper text, the function
 * poppler_structure_element_get_text() must be used instead.
 *
 * Return value: (transfer full): The actual text for the element, or %NULL
 *    if not defined.
 *
 * Since: 0.26
 */
gchar *
poppler_structure_element_get_actual_text (PopplerStructureElement *poppler_structure_element)
{
  g_return_val_if_fail (POPPLER_IS_STRUCTURE_ELEMENT (poppler_structure_element), NULL);
  g_return_val_if_fail (poppler_structure_element->elem != NULL, NULL);

  GooString *string = poppler_structure_element->elem->getActualText ();
  return string ? _poppler_goo_string_to_utf8 (string) : NULL;
}

/**
 * poppler_structure_element_get_text:
 * @poppler_structure_element: A #PopplerStructureElement
 * @recursive: If %TRUE, the text of child elements is gathered recursively
 *   in logical order and returned as part of the result.
 *
 * Obtains the text enclosed by an element, or the text enclosed by the
 * elements in the subtree (including the element itself).
 *
 * Return value: (transfer full): A string.
 *
 * Since: 0.26
 */
gchar *
poppler_structure_element_get_text (PopplerStructureElement *poppler_structure_element,
                                    gboolean                 recursive)
{
  g_return_val_if_fail (POPPLER_IS_STRUCTURE_ELEMENT (poppler_structure_element), NULL);
  g_return_val_if_fail (poppler_structure_element->elem != NULL, NULL);

  GooString *string = poppler_structure_element->elem->getText (recursive);
  gchar *result = string ? _poppler_goo_string_to_utf8 (string) : NULL;
  delete string;
  return result;
}

struct _PopplerStructureElementIter
{
  PopplerDocument *document;
  union {
    StructElement  *elem;
    StructTreeRoot *root;
  };
  gboolean is_root;
  unsigned index;
};

POPPLER_DEFINE_BOXED_TYPE (PopplerStructureElementIter,
                           poppler_structure_element_iter,
                           poppler_structure_element_iter_copy,
                           poppler_structure_element_iter_free)

/**
 * poppler_structure_element_iter_copy:
 * @iter: a #PopplerStructureElementIter
 *
 * Creates a new #PopplerStructureElementIter as a copy of @iter. The
 * returned value must be freed with poppler_structure_element_iter_free().
 *
 * Return value: (transfer full): a new #PopplerStructureElementIter
 *
 * Since: 0.26
 */
PopplerStructureElementIter *
poppler_structure_element_iter_copy (PopplerStructureElementIter *iter)
{
  PopplerStructureElementIter *new_iter;

  g_return_val_if_fail (iter != NULL, NULL);

  new_iter = g_slice_dup (PopplerStructureElementIter, iter);
  new_iter->document = (PopplerDocument *) g_object_ref (new_iter->document);

  return new_iter;
}

/**
 * poppler_structure_element_iter_free:
 * @iter: a #PopplerStructureElementIter
 *
 * Frees @iter.
 *
 * Since: 0.26
 */
void
poppler_structure_element_iter_free (PopplerStructureElementIter *iter)
{
  if (G_UNLIKELY (iter == NULL))
    return;

  g_object_unref (iter->document);
  g_slice_free (PopplerStructureElementIter, iter);
}

/**
 * poppler_structure_element_iter_new:
 * @poppler_document: a #PopplerDocument.
 *
 * Returns the root #PopplerStructureElementIter for @document, or %NULL. The
 * returned value must be freed with poppler_structure_element_iter_free().
 *
 * Documents may have an associated structure tree &mdashmostly, Tagged-PDF
 * compliant documents&mdash; which can be used to obtain information about
 * the document structure and its contents. Each node in the tree contains
 * a #PopplerStructureElement.
 *
 * Here is a simple example that walks the whole tree:
 *
 * <informalexample><programlisting>
 * static void
 * walk_structure (PopplerStructureElementIter *iter)
 * {
 *   do {
 *     /<!-- -->* Get the element and do something with it *<!-- -->/
 *     PopplerStructureElementIter *child = poppler_structure_element_iter_get_child (iter);
 *     if (child)
 *       walk_structure (child);
 *     poppler_structure_element_iter_free (child);
 *   } while (poppler_structure_element_iter_next (iter));
 * }
 * ...
 * {
 *   iter = poppler_structure_element_iter_new (document);
 *   walk_structure (iter);
 *   poppler_structure_element_iter_free (iter);
 * }
 * </programlisting></informalexample>
 *
 * Return value: (transfer full): a new #PopplerStructureElementIter, or %NULL if document
 *    doesn't have structure tree.
 *
 * Since: 0.26
 */
PopplerStructureElementIter *
poppler_structure_element_iter_new (PopplerDocument *poppler_document)
{
  PopplerStructureElementIter *iter;
  StructTreeRoot *root;

  g_return_val_if_fail (POPPLER_IS_DOCUMENT (poppler_document), NULL);

  root = poppler_document->doc->getStructTreeRoot ();
  if (root == NULL)
    return NULL;

  if (root->getNumElements () == 0)
    return NULL;

  iter = g_slice_new0 (PopplerStructureElementIter);
  iter->document = (PopplerDocument *) g_object_ref (poppler_document);
  iter->is_root = TRUE;
  iter->root = root;

  return iter;
}

/**
 * poppler_structure_element_iter_next:
 * @iter: a #PopplerStructureElementIter
 *
 * Sets @iter to point to the next structure element at the current level
 * of the tree, if valid. See poppler_structure_element_iter_new() for more
 * information.
 *
 * Return value: %TRUE, if @iter was set to the next structure element
 *
 * Since: 0.26
 */
gboolean
poppler_structure_element_iter_next (PopplerStructureElementIter *iter)
{
  unsigned elements;

  g_return_val_if_fail (iter != NULL, FALSE);

  elements = iter->is_root
    ? iter->root->getNumElements ()
    : iter->elem->getNumElements ();

  return ++iter->index < elements;
}

/**
 * poppler_structure_element_iter_get_element:
 * @iter: a #PopplerStructureElementIter
 *
 * Returns the #PopplerStructureElementIter associated with @iter.
 *
 * Return value: (transfer full): a new #PopplerStructureElementIter
 *
 * Since: 0.26
 */
PopplerStructureElement *
poppler_structure_element_iter_get_element (PopplerStructureElementIter *iter)
{
  StructElement *elem;

  g_return_val_if_fail (iter != NULL, NULL);

  elem = iter->is_root
    ? iter->root->getElement (iter->index)
    : iter->elem->getElement (iter->index);

  return _poppler_structure_element_new (iter->document, elem);
}

/**
 * poppler_structure_element_iter_get_child:
 * @parent: a #PopplerStructureElementIter
 *
 * Returns a new iterator to the children elements of the
 * #PopplerStructureElement associated with @iter. The returned value must
 * be freed with poppler_structure_element_iter_free().
 *
 * Return value: a new #PopplerStructureElementIter
 *
 * Since: 0.26
 */
PopplerStructureElementIter *
poppler_structure_element_iter_get_child (PopplerStructureElementIter *parent)
{
  StructElement *elem;

  g_return_val_if_fail (parent != NULL, NULL);

  elem = parent->is_root
    ? parent->root->getElement (parent->index)
    : parent->elem->getElement (parent->index);

  if (elem->getNumElements () > 0)
    {
      PopplerStructureElementIter *child = g_slice_new0 (PopplerStructureElementIter);
      child->document = (PopplerDocument *) g_object_ref (parent->document);
      child->elem = elem;
      return child;
    }

  return NULL;
}


struct _PopplerTextSpan {
  gchar *text;
  gchar *font_name;
  guint  flags;
  PopplerColor color;
};

POPPLER_DEFINE_BOXED_TYPE (PopplerTextSpan,
                           poppler_text_span,
                           poppler_text_span_copy,
                           poppler_text_span_free)

enum {
  POPPLER_TEXT_SPAN_FIXED_WIDTH = (1 << 0),
  POPPLER_TEXT_SPAN_SERIF       = (1 << 1),
  POPPLER_TEXT_SPAN_ITALIC      = (1 << 2),
  POPPLER_TEXT_SPAN_BOLD        = (1 << 3),
};

static PopplerTextSpan *
text_span_poppler_text_span (const TextSpan& span)
{
    PopplerTextSpan *new_span = g_slice_new0 (PopplerTextSpan);
    if (GooString *text = span.getText ())
      new_span->text = _poppler_goo_string_to_utf8 (text);

    new_span->color.red = colToDbl (span.getColor ().r) * 65535;
    new_span->color.green = colToDbl (span.getColor ().g) * 65535;
    new_span->color.blue = colToDbl (span.getColor ().b) * 65535;

    if (span.getFont ())
      {
        // GfxFont sometimes does not have a family name but there
        // is always a font name that can be used as fallback.
        GooString *font_name = span.getFont ()->getFamily ();
        if (font_name == NULL)
          font_name = span.getFont ()->getName ();

        new_span->font_name = _poppler_goo_string_to_utf8 (font_name);
        if (span.getFont ()->isFixedWidth ())
          new_span->flags |= POPPLER_TEXT_SPAN_FIXED_WIDTH;
        if (span.getFont ()->isSerif ())
            new_span->flags |= POPPLER_TEXT_SPAN_SERIF;
        if (span.getFont ()->isItalic ())
            new_span->flags |= POPPLER_TEXT_SPAN_ITALIC;
        if (span.getFont ()->isBold ())
            new_span->flags |= POPPLER_TEXT_SPAN_BOLD;

        /* isBold() can return false for some fonts whose weight is heavy */
        switch (span.getFont ()->getWeight ())
          {
          case GfxFont::W500:
          case GfxFont::W600:
          case GfxFont::W700:
          case GfxFont::W800:
          case GfxFont::W900:
            new_span->flags |= POPPLER_TEXT_SPAN_BOLD;
          default:
            break;
          }
      }

    return new_span;
}

/**
 * poppler_text_span_copy:
 * @poppler_text_span: a #PopplerTextSpan
 *
 * Makes a copy of a text span.
 *
 * Return value: (transfer full): A new #PopplerTextSpan
 *
 * Since: 0.26
 */
PopplerTextSpan *
poppler_text_span_copy (PopplerTextSpan *poppler_text_span)
{
  PopplerTextSpan *new_span;

  g_return_val_if_fail (poppler_text_span != NULL, NULL);

  new_span = g_slice_dup (PopplerTextSpan, poppler_text_span);
  new_span->text = g_strdup (poppler_text_span->text);
  if (poppler_text_span->font_name)
    new_span->font_name = g_strdup (poppler_text_span->font_name);
  return new_span;
}

/**
 * poppler_text_span_free:
 * @poppler_text_span: A #PopplerTextSpan
 *
 * Frees a text span.
 *
 * Since: 0.26
 */
void
poppler_text_span_free (PopplerTextSpan *poppler_text_span)
{
  if (G_UNLIKELY (poppler_text_span == NULL))
    return;

  g_free (poppler_text_span->text);
  g_free (poppler_text_span->font_name);
  g_slice_free (PopplerTextSpan, poppler_text_span);
}

/**
 * poppler_text_span_is_fixed_width_font:
 * @poppler_text_span: a #PopplerTextSpan
 *
 * Check wether a text span is meant to be rendered using a fixed-width font.
 *
 * Return value: Whether the span uses a fixed-width font.
 *
 * Since: 0.26
 */
gboolean
poppler_text_span_is_fixed_width_font (PopplerTextSpan *poppler_text_span)
{
  g_return_val_if_fail (poppler_text_span != NULL, FALSE);

  return (poppler_text_span->flags & POPPLER_TEXT_SPAN_FIXED_WIDTH);
}

/**
 * poppler_text_span_is_serif_font:
 * @poppler_text_span: a #PopplerTextSpan
 *
 * Check whether a text span is meant to be rendered using a serif font.
 *
 * Return value: Whether the span uses a serif font.
 *
 * Since: 0.26
 */
gboolean
poppler_text_span_is_serif_font (PopplerTextSpan *poppler_text_span)
{
  g_return_val_if_fail (poppler_text_span != NULL, FALSE);

  return (poppler_text_span->flags & POPPLER_TEXT_SPAN_SERIF);
}

/**
 * poppler_text_span_is_bold_font:
 * @poppler_text_span: a #PopplerTextSpan
 *
 * Check whether a text span is meant to be rendered using a bold font.
 *
 * Return value: Whether the span uses bold font.
 *
 * Since: 0.26
 */
gboolean
poppler_text_span_is_bold_font (PopplerTextSpan *poppler_text_span)
{
  g_return_val_if_fail (poppler_text_span != NULL, FALSE);

  return (poppler_text_span->flags & POPPLER_TEXT_SPAN_BOLD);
}

/**
 * poppler_text_span_get_color:
 * @poppler_text_span: a #PopplerTextSpan
 * @color: (out): a return location for a #PopplerColor
 *
 * Obtains the color in which the text is to be rendered.
 *
 * Since: 0.26
 */
void
poppler_text_span_get_color (PopplerTextSpan *poppler_text_span,
                             PopplerColor *color)
{
  g_return_if_fail (poppler_text_span != NULL);
  g_return_if_fail (color != NULL);

  *color = poppler_text_span->color;
}

/**
 * poppler_text_span_get_text:
 * @poppler_text_span: a #PopplerTextSpan
 *
 * Obtains the text contained in the span.
 *
 * Return value: (transfer none): A string.
 *
 * Since: 0.26
 */
const gchar *
poppler_text_span_get_text (PopplerTextSpan *poppler_text_span)
{
  g_return_val_if_fail (poppler_text_span != NULL, NULL);

  return poppler_text_span->text;
}

/**
 * poppler_text_span_get_font_name:
 * @poppler_text_span: a #PopplerTextSpan
 *
 * Obtains the name of the font in which the span is to be rendered.
 *
 * Return value: (transfer none): A string containing the font name, or
 *   %NULL if a font is not defined.
 *
 * Since: 0.26
 */
const gchar *
poppler_text_span_get_font_name (PopplerTextSpan *poppler_text_span)
{
  g_return_val_if_fail (poppler_text_span != NULL, NULL);

  return poppler_text_span->font_name;
}


/**
 * poppler_structure_element_get_text_spans:
 * @poppler_structure_element: A #PopplerStructureElement
 * @n_text_spans: (out): A pointer to the location where the number of elements in
 *    the returned array will be stored.
 *
 * Obtains the text enclosed by an element, as an array of #PopplerTextSpan
 * structures. Each item in the list is a piece of text which share the same
 * attributes, plus its attributes. The following example shows how to
 * obtain and free the text spans of an element:
 *
 * <informalexample><programlisting>
 * guint i, n_spans;
 * PopplerTextSpan **text_spans =
 *    poppler_structure_element_get_text_spans (element, &n_spans);
 * /<!-- -->* Use the text spans *<!-- -->/
 * for (i = 0; i < n_spans; i++)
 *    poppler_text_span_free (text_spans[i]);
 * g_free (text_spans);
 * </programlisting></informalexample>
 *
 * Return value: (transfer full) (array length=n_text_spans) (element-type PopplerTextSpan):
 *    An array of #PopplerTextSpan elments.
 *
 * Since: 0.26
 */
PopplerTextSpan **
poppler_structure_element_get_text_spans (PopplerStructureElement *poppler_structure_element,
                                          guint                   *n_text_spans)
{
  g_return_val_if_fail (POPPLER_IS_STRUCTURE_ELEMENT (poppler_structure_element), NULL);
  g_return_val_if_fail (n_text_spans != NULL, NULL);
  g_return_val_if_fail (poppler_structure_element->elem != NULL, NULL);

  if (!poppler_structure_element->elem->isContent ())
    return NULL;

  const TextSpanArray spans(poppler_structure_element->elem->getTextSpans ());
  PopplerTextSpan **text_spans = g_new0 (PopplerTextSpan*, spans.size ());

  size_t i = 0;
  for (TextSpanArray::const_iterator s = spans.begin (); s != spans.end (); ++s)
    text_spans[i++] = text_span_poppler_text_span (*s);

  *n_text_spans = spans.size ();

  return text_spans;
}
