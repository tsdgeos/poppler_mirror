//========================================================================
//
// This file is under the GPLv2 or later license
//
// Copyright (C) 2005-2006 Kristian Høgsberg <krh@redhat.com>
// Copyright (C) 2005, 2018 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2018 Adam Reichold <adam.reichold@t-online.de>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <string>
#include <vector>

#include "goo/gtypes.h"
#include "Object.h"

class PageLabelInfo {
public:
  PageLabelInfo(Object *tree, int numPages);

  PageLabelInfo(const PageLabelInfo &) = delete;
  PageLabelInfo& operator=(const PageLabelInfo &) = delete;

  bool labelToIndex(GooString *label, int *index) const;
  bool indexToLabel(int index, GooString *label) const;

private:
  void parse(Object *tree);

private:
  struct Interval {
    Interval(Object *dict, int baseA);

    std::string prefix;
    enum NumberStyle {
      None,
      Arabic,
      LowercaseRoman,
      UppercaseRoman,
      UppercaseLatin,
      LowercaseLatin
    } style;
    int first, base, length;
  };

  std::vector<Interval> intervals;
};
