//========================================================================
//
// StructTreeRoot.cc
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2013, 2014 Igalia S.L.
// Copyright 2014 Fabio D'Urso <fabiodurso@hotmail.it>
// Copyright 2017 Jan-Erik S <janerik234678@gmail.com>
// Copyright 2017-2019, 2023, 2025, 2026 Albert Astals Cid <aacid@kde.org>
// Copyright 2017, 2018 Adrian Johnson <ajohnson@redneon.com>
// Copyright 2018, Adam Reichold <adam.reichold@t-online.de>
// Copyright 2025, Kevin Backhouse <kevinbackhouse@github.com>
//
//========================================================================

#include "StructTreeRoot.h"
#include "StructElement.h"
#include "PDFDoc.h"
#include "Object.h"
#include "Dict.h"
#include <cassert>

StructTreeRoot::StructTreeRoot(PDFDoc *docA, const Dict &structTreeRootDict) : doc(docA)
{
    assert(doc);
    parse(structTreeRootDict);
}

StructTreeRoot::~StructTreeRoot()
{
    for (StructElement *element : elements) {
        delete element;
    }
}

void StructTreeRoot::parse(const Dict &root)
{
    // The RoleMap/ClassMap dictionaries are needed by all the parsing
    // functions, which will resolve the custom names to canonical
    // standard names.
    roleMap = root.lookup("RoleMap");
    classMap = root.lookup("ClassMap");

    // ParentTree (optional). If present, it must be a number tree,
    // otherwise it is not possible to map stream objects to their
    // corresponding structure element. Here only the references are
    // loaded into the array, the pointers to the StructElements will
    // be filled-in later when parsing them.
    const Object parentTreeObj = root.lookup("ParentTree");
    if (parentTreeObj.isDict()) {
        RefRecursionChecker usedParents;
        parseNumberTreeNode(*parentTreeObj.getDict(), usedParents);
    }

    RefRecursionChecker seenElements;

    // Parse the children StructElements
    const bool marked = doc->getCatalog()->getMarkInfo() & Catalog::markInfoMarked;
    Object kids = root.lookup("K");
    if (kids.isArray()) {
        if (marked && kids.arrayGetLength() > 1) {
            error(errSyntaxWarning, -1, "K in StructTreeRoot has more than one children in a tagged PDF");
        }
        for (int i = 0; i < kids.arrayGetLength(); i++) {
            const Object &ref = kids.arrayGetNF(i);
            if (ref.isRef()) {
                seenElements.insert(ref.getRef());
            }
            Object obj = kids.arrayGet(i);
            if (obj.isDict()) {
                auto *child = new StructElement(obj.getDict(), this, nullptr, seenElements);
                if (child->isOk()) {
                    if (marked && child->getType() != StructElement::Document && child->getType() != StructElement::Part && child->getType() != StructElement::Art && child->getType() != StructElement::Div) {
                        error(errSyntaxWarning, -1, "StructTreeRoot element of tagged PDF is wrong type ({0:s})", child->getTypeName());
                    }
                    appendChild(child);
                    if (ref.isRef()) {
                        parentTreeAdd(ref.getRef(), child);
                    }
                } else {
                    error(errSyntaxWarning, -1, "StructTreeRoot element could not be parsed");
                    delete child;
                }
            } else {
                error(errSyntaxWarning, -1, "K has a child of wrong type ({0:s})", obj.getTypeName());
            }
        }
    } else if (kids.isDict()) {
        auto *child = new StructElement(kids.getDict(), this, nullptr, seenElements);
        if (child->isOk()) {
            appendChild(child);
            const Object &ref = root.lookupNF("K");
            if (ref.isRef()) {
                parentTreeAdd(ref.getRef(), child);
            }
        } else {
            error(errSyntaxWarning, -1, "StructTreeRoot element could not be parsed");
            delete child;
        }
    } else if (!kids.isNull()) {
        error(errSyntaxWarning, -1, "K in StructTreeRoot is wrong type ({0:s})", kids.getTypeName());
    }

    // refToParentMap is only used during parsing. Ensure all memory used by it is freed.
    std::multimap<Ref, Parent *>().swap(refToParentMap);
}

void StructTreeRoot::parseNumberTreeNode(const Dict &node, RefRecursionChecker &usedParents)
{
    Object kids = node.lookup("Kids");
    if (kids.isArray()) {
        for (int i = 0; i < kids.arrayGetLength(); i++) {
            Ref ref;
            const Object obj = kids.getArray()->get(i, &ref);
            if (!usedParents.insert(ref)) {
                return;
            }
            const RefRecursionCheckerRemover remover(usedParents, ref);
            if (obj.isDict()) {
                parseNumberTreeNode(*obj.getDict(), usedParents);
            } else {
                error(errSyntaxError, -1, "Kids item at position {0:d} is wrong type ({1:s})", i, obj.getTypeName());
            }
        }
        return;
    }
    if (!kids.isNull()) {
        error(errSyntaxError, -1, "Kids object is wrong type ({0:s})", kids.getTypeName());
    }

    Object nums = node.lookup("Nums");
    if (nums.isArray()) {
        if (nums.arrayGetLength() % 2 == 0) {
            // keys in even positions, references in odd ones
            for (int i = 0; i < nums.arrayGetLength(); i += 2) {
                Object key = nums.arrayGet(i);

                if (!key.isInt()) {
                    error(errSyntaxError, -1, "Nums item at position {0:d} is wrong type ({1:s})", i, key.getTypeName());
                    continue;
                }
                int keyVal = key.getInt();
                std::vector<Parent> &vec = parentTree[keyVal];
                if (!vec.empty()) {
                    error(errSyntaxError, -1, "Nums item at position {0:d} is a duplicate entry for key {1:d}", i, keyVal);
                    continue;
                }

                Object valueArray = nums.arrayGet(i + 1);
                if (valueArray.isArray()) {
                    vec.resize(valueArray.arrayGetLength());
                    for (int j = 0; j < valueArray.arrayGetLength(); j++) {
                        const Object &itemvalue = valueArray.arrayGetNF(j);
                        if (itemvalue.isRef()) {
                            Ref ref = itemvalue.getRef();
                            vec[j].ref = ref;
                            refToParentMap.insert(std::pair<Ref, Parent *>(ref, &vec[j]));
                        } else if (!itemvalue.isNull()) {
                            error(errSyntaxError, -1, "Nums array item at position {0:d}/{1:d} is invalid type ({2:s})", i, j, itemvalue.getTypeName());
                        }
                    }
                } else {
                    const Object &valueRef = nums.arrayGetNF(i + 1);
                    if (valueRef.isRef()) {
                        Ref ref = valueRef.getRef();
                        vec.resize(1);
                        vec[0].ref = ref;
                        refToParentMap.insert(std::pair<Ref, Parent *>(ref, vec.data()));
                    } else {
                        error(errSyntaxError, -1, "Nums item at position {0:d} is wrong type ({1:s})", i + 1, valueRef.getTypeName());
                    }
                }
            }
        } else {
            error(errSyntaxError, -1, "Nums array length is not a even ({0:d})", nums.arrayGetLength());
        }
    } else {
        error(errSyntaxError, -1, "Nums object is wrong type ({0:s})", nums.getTypeName());
    }
}

void StructTreeRoot::parentTreeAdd(const Ref objectRef, StructElement *element)
{
    auto range = refToParentMap.equal_range(objectRef);
    for (auto it = range.first; it != range.second; ++it) {
        it->second->element = element;
    }
}
