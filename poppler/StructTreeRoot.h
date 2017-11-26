//========================================================================
//
// StructTreeRoot.h
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2013, 2014 Igalia S.L.
//
//========================================================================

#ifndef STRUCTTREEROOT_H
#define STRUCTTREEROOT_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "goo/gtypes.h"
#include "Object.h"
#include "StructElement.h"
#include <map>
#include <vector>

class Dict;
class PDFDoc;


class StructTreeRoot
{
public:
  StructTreeRoot(PDFDoc *docA, Dict *rootDict);
  ~StructTreeRoot();

  PDFDoc *getDoc() { return doc; }
  Dict *getRoleMap() { return roleMap.isDict() ? roleMap.getDict() : NULL; }
  Dict *getClassMap() { return classMap.isDict() ? classMap.getDict() : NULL; }
  unsigned getNumChildren() const { return elements.size(); }
  const StructElement *getChild(int i) const { return elements.at(i); }
  StructElement *getChild(int i) { return elements.at(i); }

  void appendChild(StructElement *element) {
    if (element && element->isOk()) {
      elements.push_back(element);
    }
  }

  const StructElement *findParentElement(int key, unsigned mcid = 0) const {
    auto it = parentTree.find(key);
    if (it != parentTree.end()) {
      if (mcid < it->second.size()) {
	return it->second[mcid].element;
      }
    }
    return NULL;
  }

private:
  typedef std::vector<StructElement*> ElemPtrArray;

  // Structure for items in /ParentTree, it keeps a mapping of
  // object references and pointers to StructElement objects.
  struct Parent {
    Ref            ref;
    StructElement *element;

    Parent(): element(NULL) { ref.num = ref.gen = -1; }
    Parent(const Parent &p): element(p.element) {
      ref.num = p.ref.num;
      ref.gen = p.ref.gen;
    }
    ~Parent() {}
  };

  PDFDoc *doc;
  Object roleMap;
  Object classMap;
  ElemPtrArray elements;
  std::map<int, std::vector<Parent> > parentTree;
  std::multimap<Ref, Parent*, RefCompare> refToParentMap;

  void parse(Dict *rootDict);
  void parseNumberTreeNode(Dict *node);
  void parentTreeAdd(const Ref &objectRef, StructElement *element);

  friend class StructElement;
};

#endif

