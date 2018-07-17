//========================================================================
//
// CertificateInfo.h
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>
//
//========================================================================

#ifndef CERTIFICATEINFO_H
#define CERTIFICATEINFO_H

#include <time.h>
#include "goo/GooString.h"

enum CertificateKeyUsageExtension
{
  KU_DIGITAL_SIGNATURE = 0x80,
  KU_NON_REPUDIATION   = 0x40,
  KU_KEY_ENCIPHERMENT  = 0x20,
  KU_DATA_ENCIPHERMENT = 0x10,
  KU_KEY_AGREEMENT     = 0x08,
  KU_KEY_CERT_SIGN     = 0x04,
  KU_CRL_SIGN          = 0x02,
  KU_ENCIPHER_ONLY     = 0x01,
  KU_NONE              = 0x00
};

enum PublicKeyType
{
  RSAKEY,
  DSAKEY,
  ECKEY,
  OTHERKEY
};

class X509CertificateInfo {
public:
  X509CertificateInfo();
  ~X509CertificateInfo();

  struct PublicKeyInfo {
    GooString *publicKey;
    PublicKeyType publicKeyType;
    unsigned int publicKeyStrength; // in bits
  };

  struct EntityInfo {
    char *commonName;
    char *distinguishedName;
    char *email;
    char *organization;
  };

   struct Validity {
    time_t notBefore;
    time_t notAfter;
  };

  /* GETTERS */
  int getVersion() const;
  GooString *getSerialNumber() const;
  EntityInfo getIssuerInfo() const;
  Validity getValidity() const;
  EntityInfo getSubjectInfo() const;
  PublicKeyInfo getPublicKeyInfo() const;
  unsigned int getKeyUsageExtensions() const;
  GooString *getCertificateDER() const;
  bool getIsSelfSigned() const;

  /* SETTERS */
  void setVersion(int);
  void setSerialNumber(GooString *);
  void setIssuerInfo(EntityInfo);
  void setValidity(Validity);
  void setSubjectInfo(EntityInfo);
  void setPublicKeyInfo(PublicKeyInfo);
  void setKeyUsageExtensions(unsigned int);
  void setCertificateDER(GooString *);
  void setIsSelfSigned(bool);

private:
  X509CertificateInfo(const X509CertificateInfo &);
  X509CertificateInfo& operator=(const X509CertificateInfo &);

  EntityInfo issuer_info;
  EntityInfo subject_info;
  PublicKeyInfo public_key_info;
  Validity cert_validity;
  GooString *cert_serial;
  GooString *cert_der;
  unsigned int ku_extensions;
  int cert_version;
  bool is_self_signed;
};

#endif
