//========================================================================
//
// SignatureHandler.h
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2015 André Guerreiro <aguerreiro1985@gmail.com>
// Copyright 2015 André Esser <bepandre@hotmail.com>
// Copyright 2015, 2017, 2019, 2021 Albert Astals Cid <aacid@kde.org>
// Copyright 2017 Hans-Ulrich Jüttner <huj@froreich-bioscientia.de>
// Copyright 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@protonmail.com>
// Copyright 2018 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright 2020 Thorsten Behrens <Thorsten.Behrens@CIB.de>
// Copyright 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by Technische Universität Dresden
// Copyright 2021 Theofilos Intzoglou <int.teo@gmail.com>
// Copyright 2021 Marek Kasik <mkasik@redhat.com>
//
//========================================================================

#ifndef SIGNATURE_HANDLER_H
#define SIGNATURE_HANDLER_H

#include "goo/GooString.h"
#include "SignatureInfo.h"
#include "CertificateInfo.h"
#include "poppler_private_export.h"

#include <vector>
#include <functional>

/* NSPR Headers */
#include <nspr.h>

/* NSS headers */
#include <cms.h>
#include <nss.h>
#include <cert.h>
#include <cryptohi.h>
#include <secerr.h>
#include <secoid.h>
#include <secmodt.h>
#include <sechash.h>

class POPPLER_PRIVATE_EXPORT SignatureHandler
{
public:
    explicit SignatureHandler();
    SignatureHandler(unsigned char *p7, int p7_length);
    SignatureHandler(const char *certNickname, SECOidTag digestAlgTag);
    ~SignatureHandler();
    time_t getSigningTime();
    std::string getSignerName();
    const char *getSignerSubjectDN();
    HASH_HashType getHashAlgorithm();
    void setSignature(unsigned char *, int);
    void updateHash(unsigned char *data_block, int data_len);
    void restartHash();
    SignatureValidationStatus validateSignature();
    // Use -1 as validation_time for now
    CertificateValidationStatus validateCertificate(time_t validation_time, bool ocspRevocationCheck, bool useAIACertFetch);
    std::unique_ptr<X509CertificateInfo> getCertificateInfo() const;
    static std::vector<std::unique_ptr<X509CertificateInfo>> getAvailableSigningCertificates();
    std::unique_ptr<GooString> signDetached(const char *password) const;

    static SECOidTag getHashOidTag(const char *digestName);

    // Initializes the NSS dir with the custom given directory
    // calling it with an empty string means use the default firefox db, /etc/pki/nssdb, ~/.pki/nssdb
    // If you don't want a custom NSS dir and the default entries are fine for you, not calling this function is fine
    // If wanted, this has to be called before doing signature validation calls
    static void setNSSDir(const GooString &nssDir);

    // Gets the currently in use NSS dir
    static std::string getNSSDir();

    static void setNSSPasswordCallback(const std::function<char *(const char *)> &f);

private:
    typedef struct
    {
        enum
        {
            PW_NONE = 0,
            PW_FROMFILE = 1,
            PW_PLAINTEXT = 2,
            PW_EXTERNAL = 3
        } source;
        const char *data;
    } PWData;

    SignatureHandler(const SignatureHandler &);
    SignatureHandler &operator=(const SignatureHandler &);

    unsigned int digestLength(SECOidTag digestAlgId);
    NSSCMSMessage *CMS_MessageCreate(SECItem *cms_item);
    NSSCMSSignedData *CMS_SignedDataCreate(NSSCMSMessage *cms_msg);
    NSSCMSSignerInfo *CMS_SignerInfoCreate(NSSCMSSignedData *cms_sig_data);
    HASHContext *initHashContext();
    static void outputCallback(void *arg, const char *buf, unsigned long len);

    unsigned int hash_length;
    SECOidTag digest_alg_tag;
    SECItem CMSitem;
    HASHContext *hash_context;
    NSSCMSMessage *CMSMessage;
    NSSCMSSignedData *CMSSignedData;
    NSSCMSSignerInfo *CMSSignerInfo;
    CERTCertificate *signing_cert;
    CERTCertificate **temp_certs;

    static std::string sNssDir;
};

#endif
