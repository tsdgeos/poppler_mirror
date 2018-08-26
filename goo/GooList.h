//========================================================================
//
// GooList.h
//
// Copyright 2001-2003 Glyph & Cog, LLC
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2012, 2018 Albert Astals Cid <aacid@kde.org>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#pragma once

#include <algorithm>
#include <vector>

//------------------------------------------------------------------------
// GooList
//------------------------------------------------------------------------

struct GooList : public std::vector<void *> {

  // Create an empty list.
  GooList() = default;

  // Create an empty list with space for <size> elements.
  explicit GooList(int size) : std::vector<void *>(size) {}

  // Movable but not copyable
  GooList(GooList &&other) = default;
  GooList& operator=(GooList &&other) = default;

  GooList(const GooList &other) = delete;
  GooList& operator=(const GooList &other) = delete;

  explicit GooList(const std::vector<void *>& vec) : std::vector<void *>(vec) {}
  explicit GooList(std::vector<void *>&& vec) : std::vector<void *>(std::move(vec)) {}

  //----- general

  // Get the number of elements.
  int getLength() const { return size(); }

  // Returns a (shallow) copy of this list.
  GooList *copy() const { return new GooList(static_cast<const std::vector<void *>&>(*this)); }

  //----- ordered list support

  // Return the <i>th element.
  // Assumes 0 <= i < length.
  void *get(int i) const { return (*this)[i]; }

  // Replace the <i>th element.
  // Assumes 0 <= i < length.
  void put(int i, void *p) { (*this)[i] = p; }

  // Append an element to the end of the list.
  void append(void *p) {
    push_back(p);
  }

  // Append another list to the end of this one.
  void append(GooList *list) {
    reserve(size() + list->size());
    static_cast<std::vector<void *>&>(*this).insert(end(), list->begin(), list->end());
  }

  // Insert an element at index <i>.
  // Assumes 0 <= i <= length.
  void insert(int i, void *p) {
    static_cast<std::vector<void *>&>(*this).insert(begin() + i, p);
  }
};

#define deleteGooList(list, T)                        \
  do {                                              \
    GooList *_list = (list);                          \
    {                                               \
      int _i;                                       \
      for (_i = 0; _i < _list->getLength(); ++_i) { \
        delete (T*)_list->get(_i);                  \
      }                                             \
      delete _list;                                 \
    }                                               \
  } while (0)
