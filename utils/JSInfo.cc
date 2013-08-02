//========================================================================
//
// JSInfo.cc
//
// This file is licensed under the GPLv2 or later
//
// Copyright (C) 2013 Adrian Johnson <ajohnson@redneon.com>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================


#include "config.h"
#include "Object.h"
#include "Dict.h"
#include "Annot.h"
#include "PDFDoc.h"
#include "JSInfo.h"
#include "Link.h"
#include "Form.h"

JSInfo::JSInfo(PDFDoc *docA, int firstPage) {
  doc = docA;
  currentPage = firstPage + 1;
}

JSInfo::~JSInfo() {
}


void JSInfo::scanLinkAction(LinkAction *link) {
  if (!link)
    return;

  if (link->getKind() == actionJavaScript) {
    hasJS = gTrue;
  }

  if (link->getKind() == actionRendition) {
    LinkRendition *linkr = static_cast<LinkRendition *>(link);
    if (linkr->getScript())
      hasJS = gTrue;
  }
}

void JSInfo::scanJS(int nPages) {
  Page *page;
  Annots *annots;
  Object obj1, obj2;
  int lastPage;

  hasJS = gFalse;

  // Names
  if (doc->getCatalog()->numJS() > 0) {
    hasJS = gTrue;
  }

  // document actions
  scanLinkAction(doc->getCatalog()->getAdditionalAction(Catalog::actionCloseDocument));
  scanLinkAction(doc->getCatalog()->getAdditionalAction(Catalog::actionSaveDocumentStart));
  scanLinkAction(doc->getCatalog()->getAdditionalAction(Catalog::actionSaveDocumentFinish));
  scanLinkAction(doc->getCatalog()->getAdditionalAction(Catalog::actionPrintDocumentStart));
  scanLinkAction(doc->getCatalog()->getAdditionalAction(Catalog::actionPrintDocumentFinish));

  // form field actions
  if (doc->getCatalog()->getFormType() == Catalog::AcroForm) {
    Form *form = doc->getCatalog()->getForm();
    for (int i = 0; i < form->getNumFields(); i++) {
      FormField *field = form->getRootField(i);
      for (int j = 0; j < field->getNumWidgets(); j++) {
	FormWidget *widget = field->getWidget(j);
	scanLinkAction(widget->getActivationAction());
	scanLinkAction(widget->getAdditionalAction(Annot::actionFieldModified));
	scanLinkAction(widget->getAdditionalAction(Annot::actionFormatField));
	scanLinkAction(widget->getAdditionalAction(Annot::actionValidateField));
	scanLinkAction(widget->getAdditionalAction(Annot::actionCalculateField));
      }
    }
  }

  // scan pages

  if (currentPage > doc->getNumPages()) {
    return;
  }

  lastPage = currentPage + nPages;
  if (lastPage > doc->getNumPages() + 1) {
    lastPage = doc->getNumPages() + 1;
  }

  for (int pg = currentPage; pg < lastPage; ++pg) {
    page = doc->getPage(pg);
    if (!page) continue;

    // page actions (open, close)
    scanLinkAction(page->getAdditionalAction(Page::actionOpenPage));
    scanLinkAction(page->getAdditionalAction(Page::actionClosePage));

    // annotation actions (links, screen, widget)
    annots = page->getAnnots();
    for (int i = 0; i < annots->getNumAnnots(); ++i) {
      if (annots->getAnnot(i)->getType() == Annot::typeLink) {
	AnnotLink *annot = static_cast<AnnotLink *>(annots->getAnnot(i));
	scanLinkAction(annot->getAction());
      } else if (annots->getAnnot(i)->getType() == Annot::typeScreen) {
	AnnotScreen *annot = static_cast<AnnotScreen *>(annots->getAnnot(i));
	scanLinkAction(annot->getAction());
	scanLinkAction(annot->getAdditionalAction(Annot::actionCursorEntering));
	scanLinkAction(annot->getAdditionalAction(Annot::actionCursorLeaving));
	scanLinkAction(annot->getAdditionalAction(Annot::actionMousePressed));
	scanLinkAction(annot->getAdditionalAction(Annot::actionMouseReleased));
	scanLinkAction(annot->getAdditionalAction(Annot::actionFocusIn));
	scanLinkAction(annot->getAdditionalAction(Annot::actionFocusOut));
	scanLinkAction(annot->getAdditionalAction(Annot::actionPageOpening));
	scanLinkAction(annot->getAdditionalAction(Annot::actionPageClosing));
	scanLinkAction(annot->getAdditionalAction(Annot::actionPageVisible));
	scanLinkAction(annot->getAdditionalAction(Annot::actionPageVisible));

      } else if (annots->getAnnot(i)->getType() == Annot::typeWidget) {
	AnnotWidget *annot = static_cast<AnnotWidget *>(annots->getAnnot(i));
	scanLinkAction(annot->getAction());
	scanLinkAction(annot->getAdditionalAction(Annot::actionCursorEntering));
	scanLinkAction(annot->getAdditionalAction(Annot::actionCursorLeaving));
	scanLinkAction(annot->getAdditionalAction(Annot::actionMousePressed));
	scanLinkAction(annot->getAdditionalAction(Annot::actionMouseReleased));
	scanLinkAction(annot->getAdditionalAction(Annot::actionFocusIn));
	scanLinkAction(annot->getAdditionalAction(Annot::actionFocusOut));
	scanLinkAction(annot->getAdditionalAction(Annot::actionPageOpening));
	scanLinkAction(annot->getAdditionalAction(Annot::actionPageClosing));
	scanLinkAction(annot->getAdditionalAction(Annot::actionPageVisible));
	scanLinkAction(annot->getAdditionalAction(Annot::actionPageVisible));
      }
    }
  }

  currentPage = lastPage;
}

GBool JSInfo::containsJS() {
  return hasJS;
};
