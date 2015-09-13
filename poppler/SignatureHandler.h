//========================================================================
//
// SignatureHandler.h
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2015 André Guerreiro <aguerreiro1985@gmail.com>
// Copyright 2015 André Esser <bepandre@hotmail.com>
// Copyright 2015 Albert Astals Cid <aacid@kde.org>
//
//========================================================================

#ifndef SIGNATURE_HANDLER_H
#define SIGNATURE_HANDLER_H

#include "goo/GooString.h"
#include "SignatureInfo.h"

/* NSPR Headers */
#include <nspr/prprf.h>
#include <nspr/prtypes.h>
#include <nspr/plgetopt.h>
#include <nspr/prio.h>
#include <nspr/prerror.h>

/* NSS headers */
#include <nss/cms.h>
#include <nss/nss.h>
#include <nss/cert.h>
#include <nss/cryptohi.h>
#include <nss/secerr.h>
#include <nss/secoid.h>
#include <nss/secmodt.h>
#include <nss/sechash.h>

class SignatureHandler
{
public:
  SignatureHandler(unsigned char *p7, int p7_length);
  ~SignatureHandler();
  time_t getSigningTime();
  char * getSignerName();
  void setSignature(unsigned char *, int);
  NSSCMSVerificationStatus validateSignature(unsigned char *signed_data, int signed_data_len);
  SECErrorCodes validateCertificate();
  //Translate NSS error codes
  static SignatureValidationStatus NSS_SigTranslate(NSSCMSVerificationStatus nss_code);
  static CertificateValidationStatus NSS_CertTranslate(SECErrorCodes nss_code);

private:
  void init_nss();
  GooString * getDefaultFirefoxCertDB_Linux();
  unsigned int digestLength(SECOidTag digestAlgId);
  NSSCMSMessage *CMS_MessageCreate(SECItem * cms_item);
  NSSCMSSignedData *CMS_SignedDataCreate(NSSCMSMessage * cms_msg);
  NSSCMSSignerInfo *CMS_SignerInfoCreate(NSSCMSSignedData * cms_sig_data);
  void digestFile(unsigned char *digest_buffer, unsigned char *input_data, int input_data_len, SECOidTag hashOIDTag);

  SECItem CMSitem;
  NSSCMSMessage *CMSMessage = NULL;
  NSSCMSSignedData *CMSSignedData = NULL;
  NSSCMSSignerInfo *CMSSignerInfo = NULL;
  CERTCertificate **temp_certs = NULL;
};

#endif
