//========================================================================
//
// pdf-fullrewrite.cc
//
// Copyright 2007 Julien Rebetez
// Copyright 2012 Fabio D'Urso
// Copyright 2022 Albert Astals Cid <aacid@kde.org>
//
//========================================================================

#include "GlobalParams.h"
#include "Error.h"
#include "Object.h"
#include "PDFDoc.h"
#include "XRef.h"
#include "goo/GooString.h"
#include "utils/parseargs.h"

static bool compareDocuments(PDFDoc *origDoc, PDFDoc *newDoc);
static bool compareObjects(const Object *objA, const Object *objB);

static char ownerPassword[33] = "\001";
static char userPassword[33] = "\001";
static bool forceIncremental = false;
static bool checkOutput = false;
static bool printHelp = false;

static const ArgDesc argDesc[] = { { "-opw", argString, ownerPassword, sizeof(ownerPassword), "owner password (for encrypted files)" },
                                   { "-upw", argString, userPassword, sizeof(userPassword), "user password (for encrypted files)" },
                                   { "-i", argFlag, &forceIncremental, 0, "incremental update mode" },
                                   { "-check", argFlag, &checkOutput, 0, "verify the generated document" },
                                   { "-h", argFlag, &printHelp, 0, "print usage information" },
                                   { "-help", argFlag, &printHelp, 0, "print usage information" },
                                   { "--help", argFlag, &printHelp, 0, "print usage information" },
                                   { "-?", argFlag, &printHelp, 0, "print usage information" },
                                   {} };

int main(int argc, char *argv[])
{
    PDFDoc *doc = nullptr;
    PDFDoc *docOut = nullptr;
    std::optional<GooString> ownerPW;
    std::optional<GooString> userPW;
    int res = 0;

    // parse args
    bool ok = parseArgs(argDesc, &argc, argv);
    if (!ok || (argc < 3) || printHelp) {
        printUsage(argv[0], "INPUT-FILE OUTPUT-FILE", argDesc);
        if (!printHelp) {
            res = 1;
        }
        goto done;
    }

    if (ownerPassword[0] != '\001') {
        ownerPW = GooString(ownerPassword);
    }
    if (userPassword[0] != '\001') {
        userPW = GooString(userPassword);
    }

    // load input document
    globalParams = std::make_unique<GlobalParams>();
    doc = new PDFDoc(std::make_unique<GooString>(argv[1]), ownerPW, userPW);
    if (!doc->isOk()) {
        fprintf(stderr, "Error loading input document\n");
        res = 1;
        goto done;
    }

    // save it back (in rewrite or incremental update mode)
    if (doc->saveAs(*doc->getFileName(), forceIncremental ? writeForceIncremental : writeForceRewrite) != 0) {
        fprintf(stderr, "Error saving document\n");
        res = 1;
        goto done;
    }

    if (checkOutput) {
        // open the generated document to verify it
        docOut = new PDFDoc(std::make_unique<GooString>(argv[2]), ownerPW, userPW);
        if (!docOut->isOk()) {
            fprintf(stderr, "Error loading generated document\n");
            res = 1;
        } else if (!compareDocuments(doc, docOut)) {
            fprintf(stderr, "Verification failed\n");
            res = 1;
        }
    }

done:
    delete docOut;
    delete doc;
    return res;
}

static bool compareDictionaries(Dict *dictA, Dict *dictB)
{
    const int length = dictA->getLength();
    if (dictB->getLength() != length) {
        return false;
    }

    /* Check that every key in dictA is contained in dictB.
     * Since keys are unique and we've already checked that dictA and dictB
     * contain the same number of entries, we don't need to check that every key
     * in dictB is also contained in dictA */
    for (int i = 0; i < length; ++i) {
        const char *key = dictA->getKey(i);
        const Object &valA = dictA->getValNF(i);
        const Object &valB = dictB->lookupNF(key);
        if (!compareObjects(&valA, &valB)) {
            return false;
        }
    }

    return true;
}

static bool compareObjects(const Object *objA, const Object *objB)
{
    switch (objA->getType()) {
    case objBool: {
        if (objB->getType() != objBool) {
            return false;
        } else {
            return (objA->getBool() == objB->getBool());
        }
    }
    case objInt:
    case objInt64:
    case objReal: {
        if (!objB->isNum()) {
            return false;
        } else {
            // Fuzzy comparison
            const double diff = objA->getNum() - objB->getNum();
            return (-0.01 < diff) && (diff < 0.01);
        }
    }
    case objString: {
        if (objB->getType() != objString) {
            return false;
        } else {
            const GooString *strA = objA->getString();
            const GooString *strB = objB->getString();
            return (strA->cmp(strB) == 0);
        }
    }
    case objName: {
        if (objB->getType() != objName) {
            return false;
        } else {
            GooString nameA(objA->getName());
            GooString nameB(objB->getName());
            return (nameA.cmp(&nameB) == 0);
        }
    }
    case objNull: {
        if (objB->getType() != objNull) {
            return false;
        } else {
            return true;
        }
    }
    case objArray: {
        if (objB->getType() != objArray) {
            return false;
        } else {
            Array *arrayA = objA->getArray();
            Array *arrayB = objB->getArray();
            const int length = arrayA->getLength();
            if (arrayB->getLength() != length) {
                return false;
            } else {
                for (int i = 0; i < length; ++i) {
                    const Object &elemA = arrayA->getNF(i);
                    const Object &elemB = arrayB->getNF(i);
                    if (!compareObjects(&elemA, &elemB)) {
                        return false;
                    }
                }
                return true;
            }
        }
    }
    case objDict: {
        if (objB->getType() != objDict) {
            return false;
        } else {
            Dict *dictA = objA->getDict();
            Dict *dictB = objB->getDict();
            return compareDictionaries(dictA, dictB);
        }
    }
    case objStream: {
        if (objB->getType() != objStream) {
            return false;
        } else {
            Stream *streamA = objA->getStream();
            Stream *streamB = objB->getStream();
            if (!compareDictionaries(streamA->getDict(), streamB->getDict())) {
                return false;
            } else {
                int c;
                streamA->reset();
                streamB->reset();
                do {
                    c = streamA->getChar();
                    if (c != streamB->getChar()) {
                        return false;
                    }
                } while (c != EOF);
                return true;
            }
        }
        return true;
    }
    case objRef: {
        if (objB->getType() != objRef) {
            return false;
        } else {
            const Ref refA = objA->getRef();
            const Ref refB = objB->getRef();
            return refA == refB;
        }
    }
    default: {
        fprintf(stderr, "compareObjects failed: unexpected object type %u\n", objA->getType());
        return false;
    }
    }
}

static bool compareDocuments(PDFDoc *origDoc, PDFDoc *newDoc)
{
    bool result = true;
    XRef *origXRef = origDoc->getXRef();
    XRef *newXRef = newDoc->getXRef();

    // Make sure that special flags are set in both documents
    origXRef->scanSpecialFlags();
    newXRef->scanSpecialFlags();

    // Compare XRef tables' size
    const int origNumObjects = origXRef->getNumObjects();
    const int newNumObjects = newXRef->getNumObjects();
    if (forceIncremental && origXRef->isXRefStream()) {
        // In case of incremental update, expect a new entry to be appended to store the new XRef stream
        if (origNumObjects + 1 != newNumObjects) {
            fprintf(stderr, "XRef table: Unexpected number of entries (%d+1 != %d)\n", origNumObjects, newNumObjects);
            result = false;
        }
    } else {
        // In all other cases the number of entries must be the same
        if (origNumObjects != newNumObjects) {
            fprintf(stderr, "XRef table: Different number of entries (%d != %d)\n", origNumObjects, newNumObjects);
            result = false;
        }
    }

    // Compare each XRef entry
    const int numObjects = (origNumObjects < newNumObjects) ? origNumObjects : newNumObjects;
    for (int i = 0; i < numObjects; ++i) {
        XRefEntryType origType = origXRef->getEntry(i)->type;
        XRefEntryType newType = newXRef->getEntry(i)->type;
        const int origGenNum = (origType != xrefEntryCompressed) ? origXRef->getEntry(i)->gen : 0;
        const int newGenNum = (newType != xrefEntryCompressed) ? newXRef->getEntry(i)->gen : 0;

        // Check that DontRewrite entries are freed in full rewrite mode
        if (!forceIncremental && origXRef->getEntry(i)->getFlag(XRefEntry::DontRewrite)) {
            if (newType != xrefEntryFree || origGenNum + 1 != newGenNum) {
                fprintf(stderr, "XRef entry %u: DontRewrite entry was not freed correctly\n", i);
                result = false;
            }
            continue; // There's nothing left to check for this entry
        }

        // Compare generation numbers
        // Object num 0 should always have gen 65535 according to specs, but some
        // documents have it set to 0. We always write 65535 in output
        if (i != 0) {
            if (origGenNum != newGenNum) {
                fprintf(stderr, "XRef entry %u: generation numbers differ (%d != %d)\n", i, origGenNum, newGenNum);
                result = false;
                continue;
            }
        } else {
            if (newGenNum != 65535) {
                fprintf(stderr, "XRef entry %u: generation number was expected to be 65535 (%d != 65535)\n", i, newGenNum);
                result = false;
                continue;
            }
        }

        // Compare object flags. A failure shows that there's some error in XRef::scanSpecialFlags()
        if (origXRef->getEntry(i)->flags != newXRef->getEntry(i)->flags) {
            fprintf(stderr, "XRef entry %u: flags detected by scanSpecialFlags differ (%d != %d)\n", i, origXRef->getEntry(i)->flags, newXRef->getEntry(i)->flags);
            result = false;
        }

        // Check that either both are free or both are in use
        if ((origType == xrefEntryFree) != (newType == xrefEntryFree)) {
            const char *origStatus = (origType == xrefEntryFree) ? "free" : "in use";
            const char *newStatus = (newType == xrefEntryFree) ? "free" : "in use";
            fprintf(stderr, "XRef entry %u: usage status differs (%s != %s)\n", i, origStatus, newStatus);
            result = false;
            continue;
        }

        // Skip free entries
        if (origType == xrefEntryFree) {
            continue;
        }

        // Compare contents
        Object origObj = origXRef->fetch(i, origGenNum);
        Object newObj = newXRef->fetch(i, newGenNum);
        if (!compareObjects(&origObj, &newObj)) {
            fprintf(stderr, "XRef entry %u: contents differ\n", i);
            result = false;
        }
    }

    return result;
}
