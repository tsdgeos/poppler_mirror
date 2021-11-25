//========================================================================
//
// SignatureInfo.cc
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2015 André Guerreiro <aguerreiro1985@gmail.com>
// Copyright 2015 André Esser <bepandre@hotmail.com>
// Copyright 2017 Hans-Ulrich Jüttner <huj@froreich-bioscientia.de>
// Copyright 2017-2020 Albert Astals Cid <aacid@kde.org>
// Copyright 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@protonmail.com>
// Copyright 2018 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright 2021 Georgiy Sgibnev <georgiy@sgibnev.com>. Work sponsored by lab50.net.
// Copyright 2021 André Guerreiro <aguerreiro1985@gmail.com>
// Copyright 2021 Marek Kasik <mkasik@redhat.com>
//
//========================================================================

#include <config.h>

#include "SignatureInfo.h"
#include "CertificateInfo.h"
#include "goo/gmem.h"
#include <cstdlib>
#include <cstring>

#ifdef ENABLE_NSS3
#    include <hasht.h>
#else
static const int HASH_AlgNULL = -1;
#endif

/* Constructor & Destructor */

SignatureInfo::SignatureInfo()
{
    sig_status = SIGNATURE_NOT_VERIFIED;
    cert_status = CERTIFICATE_NOT_VERIFIED;
    cert_info = nullptr;
    signer_name = nullptr;
    subject_dn = nullptr;
    hash_type = HASH_AlgNULL;
    signing_time = 0;
    sig_subfilter_supported = false;
}

SignatureInfo::SignatureInfo(SignatureValidationStatus sig_val_status, CertificateValidationStatus cert_val_status)
{
    sig_status = sig_val_status;
    cert_status = cert_val_status;
    cert_info = nullptr;
    signer_name = nullptr;
    subject_dn = nullptr;
    hash_type = HASH_AlgNULL;
    signing_time = 0;
    sig_subfilter_supported = false;
}

SignatureInfo::~SignatureInfo()
{
    free(signer_name);
    free(subject_dn);
}

/* GETTERS */

SignatureValidationStatus SignatureInfo::getSignatureValStatus() const
{
    return sig_status;
}

CertificateValidationStatus SignatureInfo::getCertificateValStatus() const
{
    return cert_status;
}

const char *SignatureInfo::getSignerName() const
{
    return signer_name;
}

const char *SignatureInfo::getSubjectDN() const
{
    return subject_dn;
}

const GooString &SignatureInfo::getLocation() const
{
    return location;
}

const GooString &SignatureInfo::getReason() const
{
    return reason;
}

int SignatureInfo::getHashAlgorithm() const
{
    return hash_type;
}

time_t SignatureInfo::getSigningTime() const
{
    return signing_time;
}

const X509CertificateInfo *SignatureInfo::getCertificateInfo() const
{
    return cert_info.get();
}

/* SETTERS */

void SignatureInfo::setSignatureValStatus(enum SignatureValidationStatus sig_val_status)
{
    sig_status = sig_val_status;
}

void SignatureInfo::setCertificateValStatus(enum CertificateValidationStatus cert_val_status)
{
    cert_status = cert_val_status;
}

void SignatureInfo::setSignerName(const char *signerName)
{
    free(signer_name);
    signer_name = signerName ? strdup(signerName) : nullptr;
}

void SignatureInfo::setSubjectDN(const char *subjectDN)
{
    free(subject_dn);
    subject_dn = subjectDN ? strdup(subjectDN) : nullptr;
}

void SignatureInfo::setLocation(const GooString *loc)
{
    location = GooString(loc->toStr());
}

void SignatureInfo::setReason(const GooString *signingReason)
{
    reason = GooString(signingReason->toStr());
}

void SignatureInfo::setHashAlgorithm(int type)
{
    hash_type = type;
}

void SignatureInfo::setSigningTime(time_t signingTime)
{
    signing_time = signingTime;
}

void SignatureInfo::setCertificateInfo(std::unique_ptr<X509CertificateInfo> certInfo)
{
    cert_info = std::move(certInfo);
}
