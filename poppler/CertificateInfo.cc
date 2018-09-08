//========================================================================
//
// CertificateInfo.cc
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>
// Copyright 2018 Albert Astals Cid <aacid@kde.org>
//
//========================================================================

#include "CertificateInfo.h"

#include <string.h>
#include <stdlib.h>

X509CertificateInfo::PublicKeyInfo::PublicKeyInfo() :
  publicKey(nullptr),
  publicKeyType(OTHERKEY),
  publicKeyStrength(0)
{
}

X509CertificateInfo::PublicKeyInfo::~PublicKeyInfo()
{
  delete publicKey;
}

X509CertificateInfo::PublicKeyInfo::PublicKeyInfo(X509CertificateInfo::PublicKeyInfo &&other)
{
  publicKey = other.publicKey;
  publicKeyType = other.publicKeyType;
  publicKeyStrength = other.publicKeyStrength;
  other.publicKey = nullptr;
}

X509CertificateInfo::PublicKeyInfo &X509CertificateInfo::PublicKeyInfo::operator=(X509CertificateInfo::PublicKeyInfo &&other)
{
  delete publicKey;
  publicKey = other.publicKey;
  publicKeyType = other.publicKeyType;
  publicKeyStrength = other.publicKeyStrength;
  other.publicKey = nullptr;
  return *this;
}

X509CertificateInfo::EntityInfo::EntityInfo() :
  commonName(nullptr),
  distinguishedName(nullptr),
  email(nullptr),
  organization(nullptr)
{
}

X509CertificateInfo::EntityInfo::~EntityInfo()
{
  free(commonName);
  free(distinguishedName);
  free(email);
  free(organization);
}

X509CertificateInfo::EntityInfo::EntityInfo(X509CertificateInfo::EntityInfo &&other)
{
  commonName = other.commonName;
  distinguishedName = other.distinguishedName;
  email = other.email;
  organization = other.organization;
  other.commonName = nullptr;
  other.distinguishedName = nullptr;
  other.email = nullptr;
  other.organization = nullptr;
}

X509CertificateInfo::EntityInfo &X509CertificateInfo::EntityInfo::operator=(X509CertificateInfo::EntityInfo &&other)
{
  free(commonName);
  free(distinguishedName);
  free(email);
  free(organization);
  commonName = other.commonName;
  distinguishedName = other.distinguishedName;
  email = other.email;
  organization = other.organization;
  other.commonName = nullptr;
  other.distinguishedName = nullptr;
  other.email = nullptr;
  other.organization = nullptr;
  return *this;
}

X509CertificateInfo::X509CertificateInfo() :
  cert_serial(nullptr),
  cert_der(nullptr),
  ku_extensions(KU_NONE),
  cert_version(-1),
  is_self_signed(false)
{
}

X509CertificateInfo::~X509CertificateInfo()
{
  delete cert_serial;
  delete cert_der;
}

int X509CertificateInfo::getVersion() const
{
  return cert_version;
}

const GooString &X509CertificateInfo::getSerialNumber() const
{
  return *cert_serial;
}

const X509CertificateInfo::EntityInfo &X509CertificateInfo::getIssuerInfo() const
{
  return issuer_info;
}

const X509CertificateInfo::Validity &X509CertificateInfo::getValidity() const
{
  return cert_validity;
}

const X509CertificateInfo::EntityInfo &X509CertificateInfo::getSubjectInfo() const
{
  return subject_info;
}

const X509CertificateInfo::PublicKeyInfo &X509CertificateInfo::getPublicKeyInfo() const
{
  return public_key_info;
}

unsigned int X509CertificateInfo::getKeyUsageExtensions() const
{
  return ku_extensions;
}

const GooString &X509CertificateInfo::getCertificateDER() const
{
  return *cert_der;
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
  delete cert_serial;
  cert_serial = serialNumber;
}

void X509CertificateInfo::setIssuerInfo(EntityInfo &&issuerInfo)
{
  issuer_info = std::move(issuerInfo);
}

void X509CertificateInfo::setValidity(const Validity &validity)
{
  cert_validity = validity;
}

void X509CertificateInfo::setSubjectInfo(EntityInfo &&subjectInfo)
{
  subject_info = std::move(subjectInfo);
}

void X509CertificateInfo::setPublicKeyInfo(PublicKeyInfo &&pkInfo)
{
  public_key_info = std::move(pkInfo);
}

void X509CertificateInfo::setKeyUsageExtensions(unsigned int keyUsages)
{
  ku_extensions = keyUsages;
}

void X509CertificateInfo::setCertificateDER(GooString *certDer)
{
  delete cert_der;
  cert_der = certDer;
}

void X509CertificateInfo::setIsSelfSigned(bool isSelfSigned)
{
  is_self_signed = isSelfSigned;
}
