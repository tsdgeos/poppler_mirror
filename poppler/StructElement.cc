//========================================================================
//
// StructElement.cc
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2013 Igalia S.L.
//
//========================================================================

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "StructElement.h"
#include "StructTreeRoot.h"
#include "PDFDoc.h"
#include "Dict.h"

#include <assert.h>

class GfxState;


static const char *typeToName(StructElement::Type type)
{
  if (type == StructElement::MCID)
    return "MarkedContent";
  if (type == StructElement::OBJR)
    return "ObjectReference";

  return "Unknown";
}


//------------------------------------------------------------------------
// StructElement
//------------------------------------------------------------------------

StructElement::StructData::StructData():
  altText(0),
  actualText(0),
  id(0),
  title(0),
  expandedAbbr(0),
  language(0),
  revision(0)
{
}

StructElement::StructData::~StructData()
{
  delete altText;
  delete actualText;
  delete id;
  delete title;
  delete language;
  parentRef.free();
  for (ElemPtrArray::iterator i = elements.begin(); i != elements.end(); ++i) delete *i;
}


StructElement::StructElement(Dict *element,
                             StructTreeRoot *treeRootA,
                             StructElement *parentA,
                             std::set<int> &seen):
  type(Unknown),
  treeRoot(treeRootA),
  parent(parentA),
  s(new StructData())
{
  assert(treeRoot);
  assert(element);

  parse(element);
  parseChildren(element, seen);
}

StructElement::StructElement(int mcid, StructTreeRoot *treeRootA, StructElement *parentA):
  type(MCID),
  treeRoot(treeRootA),
  parent(parentA),
  c(new ContentData(mcid))
{
  assert(treeRoot);
  assert(parent);
}

StructElement::StructElement(const Ref& ref, StructTreeRoot *treeRootA, StructElement *parentA):
  type(OBJR),
  treeRoot(treeRootA),
  parent(parentA),
  c(new ContentData(ref))
{
  assert(treeRoot);
  assert(parent);
}

StructElement::~StructElement()
{
  if (isContent())
    delete c;
  else
    delete s;
  pageRef.free();
}

GBool StructElement::hasPageRef() const
{
  return pageRef.isRef() || (parent && parent->hasPageRef());
}

bool StructElement::getPageRef(Ref& ref) const
{
  if (pageRef.isRef()) {
    ref = pageRef.getRef();
    return gTrue;
  }

  if (parent)
    return parent->getPageRef(ref);

  return gFalse;
}

const char* StructElement::getTypeName() const
{
  return typeToName(type);
}

static StructElement::Type roleMapResolve(Dict *roleMap, const char *name, const char *curName, Object *resolved)
{
  // TODO Replace this dummy implementation
  return StructElement::Unknown;
}

void StructElement::parse(Dict *element)
{
  Object obj;

  // Type is optional, but if present must be StructElem
  if (!element->lookup("Type", &obj)->isNull() && !obj.isName("StructElem")) {
    error(errSyntaxError, -1, "Type of StructElem object is wrong");
    obj.free();
    return;
  }
  obj.free();

  // Parent object reference (required).
  if (!element->lookupNF("P", &s->parentRef)->isRef()) {
    error(errSyntaxError, -1, "P object is wrong type ({0:s})", obj.getTypeName());
    return;
  }

  // Check whether the S-type is valid for the top level
  // element and create a node of the appropriate type.
  if (!element->lookup("S", &obj)->isName()) {
    error(errSyntaxError, -1, "S object is wrong type ({0:s})", obj.getTypeName());
    obj.free();
    return;
  }

  // Type name may not be standard, resolve through RoleMap first.
  if (treeRoot->getRoleMap()) {
    Object resolvedName;
    type = roleMapResolve(treeRoot->getRoleMap(), obj.getName(), NULL, &resolvedName);
  }

  obj.free();

  // Object ID (optional), to be looked at the IDTree in the tree root.
  if (element->lookup("ID", &obj)->isString()) {
    s->id = obj.takeString();
  }
  obj.free();

  // Page reference (optional) in which at least one of the child items
  // is to be rendered in. Note: each element stores only the /Pg value
  // contained by it, and StructElement::getPageRef() may look in parent
  // elements to find the page where an element belongs.
  element->lookupNF("Pg", &pageRef);

  // Revision number (optional).
  if (element->lookup("R", &obj)->isInt()) {
    s->revision = obj.getInt();
  }
  obj.free();

  // Element title (optional).
  if (element->lookup("T", &obj)->isString()) {
    s->title = obj.takeString();
  }
  obj.free();

  // Language (optional).
  if (element->lookup("Lang", &obj)->isString()) {
    s->language = obj.takeString();
  }
  obj.free();

  // Alternative text (optional).
  if (element->lookup("Alt", &obj)->isString()) {
    s->altText = obj.takeString();
  }
  obj.free();

  // Expanded form of an abbreviation (optional).
  if (element->lookup("E", &obj)->isString()) {
    s->expandedAbbr = obj.takeString();
  }
  obj.free();

  // Actual text (optional).
  if (element->lookup("ActualText", &obj)->isString()) {
    s->actualText = obj.takeString();
  }
  obj.free();

  // TODO: Attributes directly attached to the element (optional).
  // TODO: Attributes referenced indirectly through the ClassMap (optional).
}

StructElement *StructElement::parseChild(Object *ref,
                                         Object *childObj,
                                         std::set<int> &seen)
{
  assert(childObj);
  assert(ref);

  StructElement *child = NULL;

  if (childObj->isInt()) {
    child = new StructElement(childObj->getInt(), treeRoot, this);
  } else if (childObj->isDict("MCR")) {
    /*
     * TODO: The optional Stm/StwOwn attributes are not handled, so all the
     *      page will be always scanned when calling StructElement::getText().
     */
    Object mcidObj;
    Object pageRefObj;

    if (!childObj->dictLookup("MCID", &mcidObj)->isInt()) {
      error(errSyntaxError, -1, "MCID object is wrong type ({0:s})", mcidObj.getTypeName());
      mcidObj.free();
      return NULL;
    }

    child = new StructElement(mcidObj.getInt(), treeRoot, this);
    mcidObj.free();

    if (childObj->dictLookupNF("Pg", &pageRefObj)->isRef()) {
      child->pageRef = pageRefObj;
    } else {
      pageRefObj.free();
    }
  } else if (childObj->isDict("OBJR")) {
    Object refObj;

    if (childObj->dictLookupNF("Obj", &refObj)->isRef()) {
      Object pageRefObj;

      child = new StructElement(refObj.getRef(), treeRoot, this);

      if (childObj->dictLookupNF("Pg", &pageRefObj)->isRef()) {
        child->pageRef = pageRefObj;
      } else {
        pageRefObj.free();
      }
    } else {
      error(errSyntaxError, -1, "Obj object is wrong type ({0:s})", refObj.getTypeName());
    }
    refObj.free();
  } else if (childObj->isDict()) {
    if (!ref->isRef()) {
      error(errSyntaxError, -1,
            "Structure element dictionary is not an indirect reference ({0:s})",
            ref->getTypeName());
    } else if (seen.find(ref->getRefNum()) == seen.end()) {
      seen.insert(ref->getRefNum());
      child = new StructElement(childObj->getDict(), treeRoot, this, seen);
    } else {
      error(errSyntaxWarning, -1,
            "Loop detected in structure tree, skipping subtree at object {0:i}:{0:i}",
            ref->getRefNum(), ref->getRefGen());
    }
  } else {
    error(errSyntaxWarning, -1, "K has a child of wrong type ({0:s})", childObj->getTypeName());
  }

  if (child) {
    if (child->isOk()) {
      appendElement(child);
      if (ref->isRef())
        treeRoot->parentTreeAdd(ref->getRef(), child);
    } else {
      delete child;
      child = NULL;
    }
  }

  return child;
}

void StructElement::parseChildren(Dict *element, std::set<int> &seen)
{
  Object kids;

  if (element->lookup("K", &kids)->isArray()) {
    for (int i = 0; i < kids.arrayGetLength(); i++) {
      Object obj, ref;
      parseChild(kids.arrayGetNF(i, &ref), kids.arrayGet(i, &obj), seen);
      obj.free();
      ref.free();
    }
  } else if (kids.isDict() || kids.isInt()) {
    Object ref;
    parseChild(element->lookupNF("K", &ref), &kids, seen);
    ref.free();
  }

  kids.free();
}
