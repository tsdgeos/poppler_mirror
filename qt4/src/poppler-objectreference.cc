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

#include "poppler-objectreference_p.h"

namespace Poppler {

  class ObjectReferencePrivate
  {
    public:
      ObjectReferencePrivate()
        : m_number( -1 ), m_generation( -1 )
      {
      }

      ObjectReferencePrivate( int number, int generation )
        : m_number( number ), m_generation( generation )
      {
      }

      int m_number;
      int m_generation;
  };

}

using namespace Poppler;

ObjectReference::ObjectReference()
  : d( new ObjectReferencePrivate() )
{
}

ObjectReference::ObjectReference( int number, int generation )
  : d( new ObjectReferencePrivate( number, generation ) )
{
}

ObjectReference::ObjectReference( const ObjectReference &other )
  : d( new ObjectReferencePrivate( other.d->m_number, other.d->m_generation ) )
{
}

ObjectReference::~ObjectReference()
{
  delete d;
}

ObjectReference& ObjectReference::operator=( const ObjectReference &other )
{
  if ( this != &other )
  {
    d->m_number = other.d->m_number;
    d->m_generation = other.d->m_generation;
  }

  return *this;
}

bool ObjectReference::isValid() const
{
  return ( d->m_number != -1 );
}

bool ObjectReference::operator==( const ObjectReference &other ) const
{
  return ( ( d->m_number == other.d->m_number ) && ( d->m_generation == other.d->m_generation ) );
}

bool ObjectReference::operator!=( const ObjectReference &other ) const
{
  return !operator==( other );
}
