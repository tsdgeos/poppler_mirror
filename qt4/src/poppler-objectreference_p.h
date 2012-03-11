/* poppler-objectreference.h: qt interface to poppler
 *
 * Copyright (C) 2012, Tobias Koenig <tokoe@kdab.com>
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

#ifndef POPPLER_OBJECTREFERENCE_H
#define POPPLER_OBJECTREFERENCE_H

#include "poppler-export.h"

namespace Poppler
{
  class ObjectReferencePrivate;

  /**
   * \brief Reference of a PDF object
   *
   * ObjectReference is a class that encapsulates a reference to a PDF object.
   * It is needed to store (and later on resolve) references between various PDF entities.
   */
  class POPPLER_QT4_EXPORT ObjectReference
  {
    public:
      /**
       * Creates a new, invalid ObjectReference.
       */
      ObjectReference();

      /**
       * Creates a new ObjectReference.
       *
       * @param number The number of the PDF object.
       * @param generation The generation of the PDF object.
       */
      ObjectReference( int number, int generation );

      /**
       * Creates a new ObjectReference from an @p other one.
       */
      ObjectReference( const ObjectReference &other );

      /**
       * Destroys the ObjectReference.
       */
      ~ObjectReference();

      ObjectReference& operator=( const ObjectReference &other );

      /**
       * Returns whether the object reference is valid.
       */
      bool isValid() const;

      bool operator==( const ObjectReference &other ) const;

      bool operator!=( const ObjectReference &other ) const;

    private:
      ObjectReferencePrivate * const d;
  };
}

#endif
