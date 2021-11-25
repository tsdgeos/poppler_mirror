//========================================================================
//
// SignatureInfo.h
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2015 André Guerreiro <aguerreiro1985@gmail.com>
// Copyright 2015 André Esser <bepandre@hotmail.com>
// Copyright 2015, 2017, 2018, 2020 Albert Astals Cid <aacid@kde.org>
// Copyright 2017 Hans-Ulrich Jüttner <huj@froreich-bioscientia.de>
// Copyright 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@protonmail.com>
// Copyright 2018 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright 2021 Georgiy Sgibnev <georgiy@sgibnev.com>. Work sponsored by lab50.net.
// Copyright 2021 André Guerreiro <aguerreiro1985@gmail.com>
// Copyright 2021 Marek Kasik <mkasik@redhat.com>
//
//========================================================================

#ifndef SIGNATUREINFO_H
#define SIGNATUREINFO_H

#include <memory>
#include <ctime>

#include "poppler_private_export.h"
#include "goo/GooString.h"

enum SignatureValidationStatus
{
    SIGNATURE_VALID,
    SIGNATURE_INVALID,
    SIGNATURE_DIGEST_MISMATCH,
    SIGNATURE_DECODING_ERROR,
    SIGNATURE_GENERIC_ERROR,
    SIGNATURE_NOT_FOUND,
    SIGNATURE_NOT_VERIFIED
};

enum CertificateValidationStatus
{
    CERTIFICATE_TRUSTED,
    CERTIFICATE_UNTRUSTED_ISSUER,
    CERTIFICATE_UNKNOWN_ISSUER,
    CERTIFICATE_REVOKED,
    CERTIFICATE_EXPIRED,
    CERTIFICATE_GENERIC_ERROR,
    CERTIFICATE_NOT_VERIFIED
};

class X509CertificateInfo;

class POPPLER_PRIVATE_EXPORT SignatureInfo
{
public:
    SignatureInfo();
    SignatureInfo(SignatureValidationStatus, CertificateValidationStatus);
    ~SignatureInfo();

    SignatureInfo(const SignatureInfo &) = delete;
    SignatureInfo &operator=(const SignatureInfo &) = delete;

    /* GETTERS */
    SignatureValidationStatus getSignatureValStatus() const;
    CertificateValidationStatus getCertificateValStatus() const;
    const char *getSignerName() const;
    const char *getSubjectDN() const;
    const GooString &getLocation() const;
    const GooString &getReason() const;
    int getHashAlgorithm() const; // Returns a NSS3 HASH_HashType or -1 if compiled without NSS3
    time_t getSigningTime() const;
    bool isSubfilterSupported() const { return sig_subfilter_supported; }
    const X509CertificateInfo *getCertificateInfo() const;

    /* SETTERS */
    void setSignatureValStatus(enum SignatureValidationStatus);
    void setCertificateValStatus(enum CertificateValidationStatus);
    void setSignerName(const char *);
    void setSubjectDN(const char *);
    void setLocation(const GooString *);
    void setReason(const GooString *);
    void setHashAlgorithm(int);
    void setSigningTime(time_t);
    void setSubFilterSupport(bool isSupported) { sig_subfilter_supported = isSupported; }
    void setCertificateInfo(std::unique_ptr<X509CertificateInfo>);

private:
    SignatureValidationStatus sig_status;
    CertificateValidationStatus cert_status;
    std::unique_ptr<X509CertificateInfo> cert_info;
    char *signer_name;
    char *subject_dn;
    GooString location;
    GooString reason;
    int hash_type;
    time_t signing_time;
    bool sig_subfilter_supported;
};

#endif
