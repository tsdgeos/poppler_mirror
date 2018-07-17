//========================================================================
//
// CertificateInfo.cc
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>
//
//========================================================================

#include "CertificateInfo.h"

#include <string.h>
#include <stdlib.h>

X509CertificateInfo::X509CertificateInfo()
{
  memset(&issuer_info, 0, sizeof issuer_info);
  memset(&subject_info, 0, sizeof subject_info);
  cert_serial = nullptr;
  cert_der = nullptr;
  ku_extensions = KU_NONE;
  cert_version = -1;
  is_self_signed = false;
}

X509CertificateInfo::~X509CertificateInfo()
{
  free(issuer_info.commonName);
  free(issuer_info.distinguishedName);
  free(issuer_info.email);
  free(issuer_info.organization);
  free(subject_info.commonName);
  free(subject_info.distinguishedName);
  free(subject_info.email);
  free(subject_info.organization);
}

int X509CertificateInfo::getVersion() const
{
  return cert_version;
}

GooString *X509CertificateInfo::getSerialNumber() const
{
  return cert_serial;
}

X509CertificateInfo::EntityInfo X509CertificateInfo::getIssuerInfo() const
{
  return issuer_info;
}

X509CertificateInfo::Validity X509CertificateInfo::getValidity() const
{
  return cert_validity;
}

X509CertificateInfo::EntityInfo X509CertificateInfo::getSubjectInfo() const
{
  return subject_info;
}

X509CertificateInfo::PublicKeyInfo X509CertificateInfo::getPublicKeyInfo() const
{
  return public_key_info;
}

unsigned int X509CertificateInfo::getKeyUsageExtensions() const
{
  return ku_extensions;
}

GooString *X509CertificateInfo::getCertificateDER() const
{
  return cert_der;
}

bool X509CertificateInfo::getIsSelfSigned() const
{
  return is_self_signed;
}

void X509CertificateInfo::setVersion(int version)
{
  cert_version = version;
}

void X509CertificateInfo::setSerialNumber(GooString *serialNumber)
{
  cert_serial = serialNumber;
}

void X509CertificateInfo::setIssuerInfo(EntityInfo issuerInfo)
{
  issuer_info = issuerInfo;
}

void X509CertificateInfo::setValidity(Validity validity)
{
  cert_validity = validity;
}

void X509CertificateInfo::setSubjectInfo(EntityInfo subjectInfo)
{
  subject_info = subjectInfo;
}

void X509CertificateInfo::setPublicKeyInfo(PublicKeyInfo pkInfo)
{
  public_key_info = pkInfo;
}

void X509CertificateInfo::setKeyUsageExtensions(unsigned int keyUsages)
{
  ku_extensions = keyUsages;
}

void X509CertificateInfo::setCertificateDER(GooString *certDer)
{
  cert_der = certDer;
}

void X509CertificateInfo::setIsSelfSigned(bool isSelfSigned)
{
  is_self_signed = isSelfSigned;
}
