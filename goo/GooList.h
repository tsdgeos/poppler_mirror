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
// Copyright (C) 2018 Adam Reichold <adam.reichold@t-online.de>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#ifndef GOO_LIST_H
#define GOO_LIST_H

#include <algorithm>
#include <vector>

//------------------------------------------------------------------------
// GooList
//------------------------------------------------------------------------

class GooList : public std::vector<void *> {
public:

  // Create an empty list.
  GooList() = default;

  // Movable but not copyable
  GooList(GooList &&other) = default;
  GooList& operator=(GooList &&other) = default;

  GooList(const GooList &other) = delete;
  GooList& operator=(const GooList &other) = delete;

  // Zero cost conversion from std::vector
  explicit GooList(const std::vector<void *>& vec) : std::vector<void *>(vec) {}
  explicit GooList(std::vector<void *>&& vec) : std::vector<void *>(std::move(vec)) {}

  // Get the number of elements.
  int getLength() const { return size(); }

  // Return the <i>th element.
  // Assumes 0 <= i < length.
  void *get(int i) const { return (*this)[i]; }
};

template<typename T>
inline void deleteGooList(GooList* list) {
  for (auto ptr : *list) {
    delete static_cast<T *>(ptr);
  }
  delete list;
}

#endif // GOO_LIST_H
