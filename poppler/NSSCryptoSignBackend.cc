//========================================================================
//
// SignatureHandler.cc
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2015, 2016 André Guerreiro <aguerreiro1985@gmail.com>
// Copyright 2015 André Esser <bepandre@hotmail.com>
// Copyright 2015, 2016, 2018, 2019, 2021-2023, 2025 Albert Astals Cid <aacid@kde.org>
// Copyright 2015 Markus Kilås <digital@markuspage.com>
// Copyright 2017 Sebastian Rasmussen <sebras@gmail.com>
// Copyright 2017 Hans-Ulrich Jüttner <huj@froreich-bioscientia.de>
// Copyright 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@protonmail.com>
// Copyright 2018 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright 2020 Thorsten Behrens <Thorsten.Behrens@CIB.de>
// Copyright 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by Technische Universität Dresden
// Copyright 2021 Theofilos Intzoglou <int.teo@gmail.com>
// Copyright 2021, 2025 Marek Kasik <mkasik@redhat.com>
// Copyright 2022 Erich E. Hoover <erich.e.hoover@gmail.com>
// Copyright 2023 Tobias Deiminger <tobias.deiminger@posteo.de>
// Copyright 2023-2025 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
// Copyright 2023 Ingo Klöcker <kloecker@kde.org>
// Copyright 2025, 2026 Juraj Šarinay <juraj@sarinay.com>
//
//========================================================================

#include <config.h>

#include "CryptoSignBackend.h"
#include "NSSCryptoSignBackend.h"
#include "goo/gmem.h"

#include <algorithm>
#include <array>
#include <optional>
#include <span>
#include <vector>
#include <filesystem>

#include <Error.h>

/* NSS headers */
#include <secmod.h>
#include <secoid.h>
#include <keyhi.h>
#include <secder.h>
#include <pk11pub.h>
#include <secpkcs7.h>

#include <cert.h>
#include <hasht.h>
#include <secerr.h>
#include <sechash.h>
#include <cms.h>
#include <cmst.h>

/**
 * General name, defined by RFC 3280.
 */
struct GeneralName
{
    CERTName name;
};

/**
 * List of general names (only one for now), defined by RFC 3280.
 */
struct GeneralNames
{
    GeneralName names;
};

/**
 * Supplies different fields to identify a certificate, defined by RFC 5035.
 */
struct IssuerSerial
{
    GeneralNames issuer;
    SECItem serialNumber;
};

/**
 * Supplies different fields that are used to identify certificates, defined by
 * RFC 5035.
 */
struct ESSCertIDv2
{
    SECAlgorithmID hashAlgorithm;
    SECItem certHash;
    IssuerSerial issuerSerial;
};

/**
 * This attribute uses the ESSCertIDv2 structure, defined by RFC 5035.
 */
struct SigningCertificateV2
{
    ESSCertIDv2 **certs = nullptr;
};

/**
 * GeneralName ::= CHOICE {
 *      otherName                       [0]     OtherName,
 *      rfc822Name                      [1]     IA5String,
 *      dNSName                         [2]     IA5String,
 *      x400Address                     [3]     ORAddress,
 *      directoryName                   [4]     Name,
 *      ediPartyName                    [5]     EDIPartyName,
 *      uniformResourceIdentifier       [6]     IA5String,
 *      iPAddress                       [7]     OCTET STRING,
 *      registeredID                    [8]     OBJECT IDENTIFIER
 * }
 */
const SEC_ASN1Template GeneralNameTemplate[] = { { SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(GeneralName) }, { SEC_ASN1_INLINE, offsetof(GeneralName, name), SEC_ASN1_GET(CERT_NameTemplate), 0 }, { 0, 0, nullptr, 0 } };

/**
 * GeneralNames ::= SEQUENCE SIZE (1..MAX) OF GeneralName
 */
const SEC_ASN1Template GeneralNamesTemplate[] = { { SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(GeneralNames) }, { SEC_ASN1_INLINE | SEC_ASN1_CONTEXT_SPECIFIC | 4, offsetof(GeneralNames, names), GeneralNameTemplate, 0 }, { 0, 0, nullptr, 0 } };

/**
 * IssuerSerial ::= SEQUENCE {
 *     issuer GeneralNames,
 *     serialNumber CertificateSerialNumber
 * }
 */
const SEC_ASN1Template IssuerSerialTemplate[] = {
    { SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(IssuerSerial) }, { SEC_ASN1_INLINE, offsetof(IssuerSerial, issuer), GeneralNamesTemplate, 0 }, { SEC_ASN1_INTEGER, offsetof(IssuerSerial, serialNumber), nullptr, 0 }, { 0, 0, nullptr, 0 }
};

/**
 * Hash ::= OCTET STRING
 *
 * ESSCertIDv2 ::= SEQUENCE {
 *     hashAlgorithm AlgorithmIdentifier DEFAULT {algorithm id-sha256},
 *     certHash Hash,
 *     issuerSerial IssuerSerial OPTIONAL
 * }
 */

const SEC_ASN1Template ESSCertIDv2Template[] = { { SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(ESSCertIDv2) },
                                                 { SEC_ASN1_INLINE, offsetof(ESSCertIDv2, hashAlgorithm), SEC_ASN1_GET(SECOID_AlgorithmIDTemplate), 0 },
                                                 { SEC_ASN1_OCTET_STRING, offsetof(ESSCertIDv2, certHash), nullptr, 0 },
                                                 { SEC_ASN1_INLINE, offsetof(ESSCertIDv2, issuerSerial), IssuerSerialTemplate, 0 },
                                                 { 0, 0, nullptr, 0 } };

/**
 * SigningCertificateV2 ::= SEQUENCE {
 * }
 */
const SEC_ASN1Template SigningCertificateV2Template[] = { { SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(SigningCertificateV2) }, { SEC_ASN1_SEQUENCE_OF, offsetof(SigningCertificateV2, certs), ESSCertIDv2Template, 0 }, { 0, 0, nullptr, 0 } };

// SEC_ASN1_INLINE | SEC_ASN1_OPTIONAL and SEC_ASN1EncodeItem() do not work well together within NSS.
// An additional template is necessary to accept attributes without the two optional fields.
const SEC_ASN1Template ESSCertIDv2DecodingTemplate[] = { { SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(ESSCertIDv2) },
                                                         { SEC_ASN1_INLINE | SEC_ASN1_OPTIONAL, offsetof(ESSCertIDv2, hashAlgorithm), SEC_ASN1_GET(SECOID_AlgorithmIDTemplate), 0 },
                                                         { SEC_ASN1_OCTET_STRING, offsetof(ESSCertIDv2, certHash), nullptr, 0 },
                                                         { SEC_ASN1_INLINE | SEC_ASN1_OPTIONAL | SEC_ASN1_SKIP, offsetof(ESSCertIDv2, issuerSerial), IssuerSerialTemplate, 0 },
                                                         { 0, 0, nullptr, 0 } };

const SEC_ASN1Template SigningCertificateV2DecodingTemplate[] = { { SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(SigningCertificateV2) },
                                                                  { SEC_ASN1_SEQUENCE_OF, offsetof(SigningCertificateV2, certs), ESSCertIDv2DecodingTemplate, 0 },
                                                                  { 0, 0, nullptr, 0 } };
// policies omitted on purpose. If present, decoding fails and the attribute is considered invalid, as required by ETSI EN 319 122-1 (CAdES).

const SEC_ASN1Template ESSCertIDDecodingTemplate[] = { { SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(ESSCertIDv2) },
                                                       { SEC_ASN1_OCTET_STRING, offsetof(ESSCertIDv2, certHash), nullptr, 0 },
                                                       { SEC_ASN1_INLINE | SEC_ASN1_OPTIONAL | SEC_ASN1_SKIP, offsetof(ESSCertIDv2, issuerSerial), IssuerSerialTemplate, 0 },
                                                       { 0, 0, nullptr, 0 } };

const SEC_ASN1Template SigningCertificateDecodingTemplate[] = { { SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(SigningCertificateV2) },
                                                                { SEC_ASN1_SEQUENCE_OF, offsetof(SigningCertificateV2, certs), ESSCertIDDecodingTemplate, 0 },
                                                                { 0, 0, nullptr, 0 } };
// policies omitted on purpose. If present, decoding fails and the attribute is considered invalid, as required by ETSI EN 319 122-1 (CAdES).

/*
struct PKIStatusInfo
{
    SECItem status;
    SECItem statusString;
    SECItem failInfo;
};

const SEC_ASN1Template PKIStatusInfo_Template[] = { { SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(PKIStatusInfo) },
                                                    { SEC_ASN1_INTEGER, offsetof(PKIStatusInfo, status), nullptr, 0 },
                                                    { SEC_ASN1_CONSTRUCTED | SEC_ASN1_SEQUENCE | SEC_ASN1_OPTIONAL, offsetof(PKIStatusInfo, statusString), nullptr, 0 },
                                                    { SEC_ASN1_BIT_STRING | SEC_ASN1_OPTIONAL, offsetof(PKIStatusInfo, failInfo), nullptr, 0 },
                                                    { 0, 0, nullptr, 0 } };

const SEC_ASN1Template Any_Template[] = { { SEC_ASN1_ANY, 0, nullptr, sizeof(SECItem) } };

struct TimeStampResp
{
    PKIStatusInfo status;
    SECItem timeStampToken;
};

const SEC_ASN1Template TimeStampResp_Template[] = { { SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(TimeStampResp) },
                                                    { SEC_ASN1_INLINE, offsetof(TimeStampResp, status), PKIStatusInfo_Template, 0 },
                                                    { SEC_ASN1_ANY | SEC_ASN1_OPTIONAL, offsetof(TimeStampResp, timeStampToken), Any_Template, 0 },
                                                    { 0, 0, nullptr, 0 } };

const SEC_ASN1Template MessageImprint_Template[] = { { SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(MessageImprint) },
                                                     { SEC_ASN1_INLINE, offsetof(MessageImprint, hashAlgorithm), SECOID_AlgorithmIDTemplate, 0 },
                                                     { SEC_ASN1_OCTET_STRING, offsetof(MessageImprint, hashedMessage), nullptr, 0 },
                                                     { 0, 0, nullptr, 0 } };

const SEC_ASN1Template Extension_Template[] = { { SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(Extension) },
                                                { SEC_ASN1_OBJECT_ID, offsetof(Extension, extnID), nullptr, 0 },
                                                { SEC_ASN1_BOOLEAN, offsetof(Extension, critical), nullptr, 0 },
                                                { SEC_ASN1_OCTET_STRING, offsetof(Extension, extnValue), nullptr, 0 },
                                                { 0, 0, nullptr, 0 } };

const SEC_ASN1Template Extensions_Template[] = { { SEC_ASN1_SEQUENCE_OF, 0, Extension_Template, 0 } };

const SEC_ASN1Template TimeStampReq_Template[] = { { SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(TimeStampReq) },
                                                   { SEC_ASN1_INTEGER, offsetof(TimeStampReq, version), nullptr, 0 },
                                                   { SEC_ASN1_INLINE, offsetof(TimeStampReq, messageImprint), MessageImprint_Template, 0 },
                                                   { SEC_ASN1_OBJECT_ID | SEC_ASN1_OPTIONAL, offsetof(TimeStampReq, reqPolicy), nullptr, 0 },
                                                   { SEC_ASN1_INTEGER | SEC_ASN1_OPTIONAL, offsetof(TimeStampReq, nonce), nullptr, 0 },
                                                   { SEC_ASN1_BOOLEAN | SEC_ASN1_OPTIONAL, offsetof(TimeStampReq, certReq), nullptr, 0 },
                                                   { SEC_ASN1_OPTIONAL | SEC_ASN1_CONTEXT_SPECIFIC | 0, offsetof(TimeStampReq, extensions), Extensions_Template, 0 },
                                                   { 0, 0, nullptr, 0 } };
*/

static NSSCMSMessage *CMS_MessageCreate(SECItem *cms_item);
static NSSCMSSignedData *CMS_SignedDataCreate(NSSCMSMessage *cms_msg);
static NSSCMSSignerInfo *CMS_SignerInfoCreate(NSSCMSSignedData *cms_sig_data);

// a dummy, actually
static char *passwordCallback(PK11SlotInfo * /*slot*/, PRBool /*retry*/, void *arg)
{
    return PL_strdup(static_cast<char *>(arg));
}

static void shutdownNss()
{
    if (NSS_Shutdown() != SECSuccess) {
        fprintf(stderr, "NSS_Shutdown failed: %s\n", PR_ErrorToString(PORT_GetError(), PR_LANGUAGE_I_DEFAULT));
    }
}

// An ASN.1 object identifier (OID) is typically written as a dot-separated sequence of integers
// and encoded as a sequence of bytes. Because we only ever need to handle BER encoded OIDs, keep
// them encoded from the beginning to avoid conversions at run time.
//
// The mapping from the sequence of integers to an array of bytes follows ITU-T X.690 clause 8.19.
// The first two components are encoded in a single output byte: out[0] = 40 * in[0] + in[1]
//
// EXAMPLE: 1.2 -> 40 * 1 + 2 = 0x2a
//
// From the third component onwards:
//     1. interpret the integer in base 128
//     2. map the 7-bit digits to bytes
//     3. set the most significant bit in all but the least signigicant byte to 1
//     4. output the bytes from left to right
//
// EXAMPLE: 840 = 110 1001000 -> 10000110 01001000 = 0x86 0x48
//          113549 = 110 1110111 0001101 -> 10000110 11110111 00001101 = 0x86 0xf7 0x0d
//
// As a consequence, a component that fits within 7 bits can be output unchanged as a single byte.
// EXAMPLE: .1.9.16.2.47 -> ... 0x01 0x09 0x10 0x02 0x2f
//
// 1.2.840.113549.1.9.16.2.47
constexpr unsigned char OID_SIGNINGCERTIFICATEV2[] { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x09, 0x10, 0x02, 0x2f };

// 1.2.840.113549.1.9.16.2.12
constexpr unsigned char OID_SIGNINGCERTIFICATE[] { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x09, 0x10, 0x02, 0x0c };

static NSSCMSAttribute *my_NSS_CMSAttributeArray_FindAttrByOidTag(NSSCMSAttribute **attrs, SECOidTag oidtag, PRBool only)
{
    SECOidData *oid;
    NSSCMSAttribute *attr1, *attr2;

    if (attrs == nullptr) {
        return nullptr;
    }

    oid = SECOID_FindOIDByTag(oidtag);
    if (oid == nullptr) {
        return nullptr;
    }

    while ((attr1 = *attrs++) != nullptr) {
        if (attr1->type.len == oid->oid.len && PORT_Memcmp(attr1->type.data, oid->oid.data, oid->oid.len) == 0) {
            break;
        }
    }

    if (attr1 == nullptr) {
        return nullptr;
    }

    if (!only) {
        return attr1;
    }

    while ((attr2 = *attrs++) != nullptr) {
        if (attr2->type.len == oid->oid.len && PORT_Memcmp(attr2->type.data, oid->oid.data, oid->oid.len) == 0) {
            break;
        }
    }

    if (attr2 != nullptr) {
        return nullptr;
    }

    return attr1;
}

static std::vector<const NSSCMSAttribute *> CMSAttributeArray_FindAttrByOid(NSSCMSAttribute **attrs, std::span<const unsigned char> oid)
{
    if (!attrs) {
        return {};
    }

    std::vector<const NSSCMSAttribute *> res;

    while (auto *attr = *attrs++) {
        if (attr->type.data && std::ranges::equal(oid, std::span { attr->type.data, attr->type.len })) {
            res.push_back(attr);
        }
    }
    return res;
}

static SECStatus my_NSS_CMSArray_Add(PLArenaPool *poolp, void ***array, void *obj)
{
    int n = 0;
    void **dest;

    PORT_Assert(array != NULL);
    if (array == nullptr) {
        return SECFailure;
    }

    if (*array == nullptr) {
        dest = static_cast<void **>(PORT_ArenaAlloc(poolp, 2 * sizeof(void *)));
    } else {
        void **p = *array;
        while (*p++) {
            n++;
        }
        dest = static_cast<void **>(PORT_ArenaGrow(poolp, static_cast<void *>(*array), (n + 1) * sizeof(void *), (n + 2) * sizeof(void *)));
    }

    if (dest == nullptr) {
        return SECFailure;
    }

    dest[n] = obj;
    dest[n + 1] = nullptr;
    *array = dest;
    return SECSuccess;
}

static SECOidTag my_NSS_CMSAttribute_GetType(NSSCMSAttribute *attr)
{
    SECOidData *typetag;

    typetag = SECOID_FindOID(&(attr->type));
    if (typetag == nullptr) {
        return SEC_OID_UNKNOWN;
    }

    return typetag->offset;
}

static SECStatus my_NSS_CMSAttributeArray_AddAttr(PLArenaPool *poolp, NSSCMSAttribute ***attrs, NSSCMSAttribute *attr)
{
    NSSCMSAttribute *oattr;
    void *mark;
    SECOidTag type;

    mark = PORT_ArenaMark(poolp);

    /* find oidtag of attr */
    type = my_NSS_CMSAttribute_GetType(attr);

    /* see if we have one already */
    oattr = my_NSS_CMSAttributeArray_FindAttrByOidTag(*attrs, type, PR_FALSE);
    PORT_Assert(oattr == NULL);
    if (oattr != nullptr) {
        goto loser; /* XXX or would it be better to replace it? */
    }

    /* no, shove it in */
    if (my_NSS_CMSArray_Add(poolp, reinterpret_cast<void ***>(attrs), static_cast<void *>(attr)) != SECSuccess) {
        goto loser;
    }

    PORT_ArenaUnmark(poolp, mark);
    return SECSuccess;

loser:
    PORT_ArenaRelease(poolp, mark);
    return SECFailure;
}

static SECStatus my_NSS_CMSSignerInfo_AddAuthAttr(NSSCMSSignerInfo *signerinfo, NSSCMSAttribute *attr)
{
    return my_NSS_CMSAttributeArray_AddAttr(signerinfo->cmsg->poolp, &(signerinfo->authAttr), attr);
}

static SECOidTag ConvertHashAlgorithmToNss(HashAlgorithm digestAlgId)
{
    switch (digestAlgId) {
    case HashAlgorithm::Md2:
        return SEC_OID_MD2;
    case HashAlgorithm::Md5:
        return SEC_OID_MD5;
    case HashAlgorithm::Sha1:
        return SEC_OID_SHA1;
    case HashAlgorithm::Sha256:
        return SEC_OID_SHA256;
    case HashAlgorithm::Sha384:
        return SEC_OID_SHA384;
    case HashAlgorithm::Sha512:
        return SEC_OID_SHA512;
    case HashAlgorithm::Sha224:
        return SEC_OID_SHA224;
    case HashAlgorithm::Unknown:
        return SEC_OID_UNKNOWN;
    }
    return SEC_OID_UNKNOWN;
}

static HashAlgorithm ConvertHashTypeFromNss(HASH_HashType type)
{
    switch (type) {
    case HASH_AlgMD2:
        return HashAlgorithm::Md2;
    case HASH_AlgMD5:
        return HashAlgorithm::Md5;
    case HASH_AlgSHA1:
        return HashAlgorithm::Sha1;
    case HASH_AlgSHA256:
        return HashAlgorithm::Sha256;
    case HASH_AlgSHA384:
        return HashAlgorithm::Sha384;
    case HASH_AlgSHA512:
        return HashAlgorithm::Sha512;
    case HASH_AlgSHA224:
        return HashAlgorithm::Sha224;
#if NSS_VMAJOR >= 3 && NSS_VMINOR >= 91
    // TODO Expose this in HashAlgorithm if PDF supports them
    case HASH_AlgSHA3_224:
    case HASH_AlgSHA3_256:
    case HASH_AlgSHA3_384:
    case HASH_AlgSHA3_512:
#endif
    case HASH_AlgNULL:
    case HASH_AlgTOTAL:
        return HashAlgorithm::Unknown;
    }
    return HashAlgorithm::Unknown;
}

static unsigned int digestLength(HashAlgorithm digestAlgId)
{
    switch (digestAlgId) {
    case HashAlgorithm::Sha1:
        return 20;
    case HashAlgorithm::Sha256:
        return 32;
    case HashAlgorithm::Sha384:
        return 48;
    case HashAlgorithm::Sha512:
        return 64;
    default:
        printf("ERROR: Unrecognized Hash ID\n");
        return 0;
    }
}

std::string NSSSignatureVerification::getSignerName() const
{
    if (!NSS_IsInitialized()) {
        return {};
    }
    if (!CMSSignerInfo) {
        return {};
    }

    auto *signing_cert = NSS_CMSSignerInfo_GetSigningCertificate(CMSSignerInfo, CERT_GetDefaultCertDB());
    if (!signing_cert) {
        return {};
    }

    char *commonName = CERT_GetCommonName(&signing_cert->subject);
    if (!commonName) {
        return {};
    }
    std::string name(commonName);
    PORT_Free(commonName);

    return name;
}

std::string NSSSignatureVerification::getSignerSubjectDN() const
{
    if (!CMSSignerInfo) {
        return {};
    }
    auto *signing_cert = NSS_CMSSignerInfo_GetSigningCertificate(CMSSignerInfo, CERT_GetDefaultCertDB());
    if (!signing_cert) {
        return {};
    }
    return std::string { signing_cert->subjectName };
}

std::chrono::system_clock::time_point NSSSignatureVerification::getSigningTime() const
{
    if (!CMSSignerInfo) {
        return {};
    }
    PRTime sTime; // time in microseconds since the epoch

    if (NSS_CMSSignerInfo_GetSigningTime(CMSSignerInfo, &sTime) != SECSuccess) {
        return {};
    }

    return std::chrono::system_clock::from_time_t(static_cast<time_t>(sTime / 1000000));
}

static X509CertificateInfo::EntityInfo getEntityInfo(CERTName *entityName)
{
    X509CertificateInfo::EntityInfo info;

    if (!entityName) {
        return info;
    }

    char *dn = CERT_NameToAscii(entityName);
    if (dn) {
        info.distinguishedName = dn;
        PORT_Free(dn);
    }

    char *cn = CERT_GetCommonName(entityName);
    if (cn) {
        info.commonName = cn;
        PORT_Free(cn);
    }

    char *email = CERT_GetCertEmailAddress(entityName);
    if (email) {
        info.email = email;
        PORT_Free(email);
    }

    char *org = CERT_GetOrgName(entityName);
    if (org) {
        info.organization = org;
        PORT_Free(org);
    }

    return info;
}

static GooString SECItemToGooString(const SECItem &secItem)
{
    // TODO do we need to handle secItem.type;
    return GooString((const char *)secItem.data, secItem.len);
}

static std::unique_ptr<X509CertificateInfo> getCertificateInfoFromCERT(CERTCertificate *cert)
{
    auto certInfo = std::make_unique<X509CertificateInfo>();

    certInfo->setVersion(DER_GetInteger(&cert->version) + 1);
    certInfo->setSerialNumber(SECItemToGooString(cert->serialNumber));

    // issuer info
    certInfo->setIssuerInfo(getEntityInfo(&cert->issuer));

    // validity
    PRTime notBefore, notAfter;
    CERT_GetCertTimes(cert, &notBefore, &notAfter);
    X509CertificateInfo::Validity certValidity;
    certValidity.notBefore = static_cast<time_t>(notBefore / 1000000);
    certValidity.notAfter = static_cast<time_t>(notAfter / 1000000);
    certInfo->setValidity(certValidity);

    // subject info
    certInfo->setSubjectInfo(getEntityInfo(&cert->subject));

    // nickname (as a handle to refer to the CERT later)
    certInfo->setNickName(GooString(cert->dbnickname));

    // public key info
    X509CertificateInfo::PublicKeyInfo pkInfo;
    SECKEYPublicKey *pk = CERT_ExtractPublicKey(cert);
    if (pk) {
        switch (pk->keyType) {
        case rsaKey:
            pkInfo.publicKey = SECItemToGooString(pk->u.rsa.modulus);
            pkInfo.publicKeyType = RSAKEY;
            break;
        case dsaKey:
            pkInfo.publicKey = SECItemToGooString(pk->u.dsa.publicValue);
            pkInfo.publicKeyType = DSAKEY;
            break;
        case ecKey:
            pkInfo.publicKey = SECItemToGooString(pk->u.ec.publicValue);
            pkInfo.publicKeyType = ECKEY;
            break;
        default:
            pkInfo.publicKey = SECItemToGooString(cert->subjectPublicKeyInfo.subjectPublicKey);
            pkInfo.publicKeyType = OTHERKEY;
            break;
        }
        pkInfo.publicKeyStrength = SECKEY_PublicKeyStrengthInBits(pk);
        SECKEY_DestroyPublicKey(pk);
    } else {
        pkInfo.publicKeyType = OTHERKEY;
    }
    certInfo->setPublicKeyInfo(std::move(pkInfo));

    certInfo->setKeyUsageExtensions(cert->keyUsage);
    certInfo->setCertificateDER(SECItemToGooString(cert->derCert));
    certInfo->setIsSelfSigned(CERT_CompareName(&cert->subject, &cert->issuer) == SECEqual);

    return certInfo;
}

std::unique_ptr<X509CertificateInfo> NSSSignatureVerification::getCertificateInfo() const
{
    if (!CMSSignerInfo) {
        return nullptr;
    }
    CERTCertificate *cert = NSS_CMSSignerInfo_GetSigningCertificate(CMSSignerInfo, CERT_GetDefaultCertDB());
    if (!cert) {
        return nullptr;
    }
    return getCertificateInfoFromCERT(cert);
}

std::unique_ptr<X509CertificateInfo> NSSSignatureCreation::getCertificateInfo() const
{
    if (!signing_cert) {
        return nullptr;
    }
    return getCertificateInfoFromCERT(signing_cert);
}

static std::optional<std::string> getDefaultFirefoxCertDB()
{
#ifdef _WIN32
    const char *env = getenv("APPDATA");
    if (!env) {
        return {};
    }
    const std::string firefoxPath = std::string(env) + "/Mozilla/Firefox/Profiles/";
#else
    const char *env = getenv("HOME");
    if (!env) {
        return {};
    }
    const std::string firefoxPath = std::string(env) + "/.mozilla/firefox/";
#endif

    std::error_code ec; // ensures directory_iterator doesn't throw exceptions
    std::optional<std::string> latestDir;
    std::filesystem::file_time_type latestWriteTime;
    for (const auto &entry : std::filesystem::directory_iterator { firefoxPath, ec }) {
        if (entry.is_directory() && entry.path().string().find("default") != std::string::npos) {
            const auto certPath = entry.path() / "cert9.db";
            if (std::filesystem::exists(certPath, ec) && std::filesystem::is_regular_file(certPath, ec)) {
                const auto writeTime = std::filesystem::last_write_time(certPath, ec);
                if (!latestDir.has_value() || writeTime > latestWriteTime) {
                    latestWriteTime = writeTime;
                    latestDir = entry.path().string();
                }
            }
        }
    }
    return latestDir;
}

std::string NSSSignatureConfiguration::sNssDir;

/**
 * Initialise NSS
 */
void NSSSignatureConfiguration::setNSSDir(const GooString &nssDir)
{
    static bool setNssDirCalled = false;

    if (NSS_IsInitialized() && !nssDir.empty()) {
        error(errInternal, 0, "You need to call setNSSDir before signature validation related operations happen");
        return;
    }

    if (setNssDirCalled) {
        return;
    }

    setNssDirCalled = true;

    atexit(shutdownNss);

    bool initSuccess = false;
    if (!nssDir.empty()) {
        initSuccess = (NSS_Init(nssDir.c_str()) == SECSuccess);
        sNssDir = nssDir.toStr();
    } else {
        const std::optional<std::string> certDBPath = getDefaultFirefoxCertDB();
        if (!certDBPath) {
            initSuccess = (NSS_Init("sql:/etc/pki/nssdb") == SECSuccess);
            sNssDir = "sql:/etc/pki/nssdb";
        } else {
            initSuccess = (NSS_Init(certDBPath->c_str()) == SECSuccess);
            sNssDir = *certDBPath;
        }
        if (!initSuccess) {
            GooString homeNssDb(getenv("HOME"));
            homeNssDb.append("/.pki/nssdb");
            initSuccess = (NSS_Init(homeNssDb.c_str()) == SECSuccess);
            sNssDir = homeNssDb.toStr();
        }
    }

    if (initSuccess) {
        // Make sure NSS root certificates module is loaded
        SECMOD_AddNewModule("Root Certs", "libnssckbi.so", 0, 0);
    } else {
        fprintf(stderr, "NSS_Init failed: %s\n", PR_ErrorToString(PORT_GetError(), PR_LANGUAGE_I_DEFAULT));
        NSS_NoDB_Init(nullptr);
    }
}

std::string NSSSignatureConfiguration::getNSSDir()
{
    return sNssDir;
}

static std::function<char *(const char *)> PasswordFunction;

void NSSSignatureConfiguration::setNSSPasswordCallback(const std::function<char *(const char *)> &f)
{
    PasswordFunction = f;
}

NSSSignatureVerification::NSSSignatureVerification(std::vector<unsigned char> &&p7data, CryptoSign::SignatureType subfilter) : p7(std::move(p7data)), type(subfilter)
{
    NSSSignatureConfiguration::setNSSDir({});
    CMSitem.data = p7.data();
    CMSitem.len = p7.size();
    CMSMessage = CMS_MessageCreate(&CMSitem);
    CMSSignedData = CMS_SignedDataCreate(CMSMessage);
    if (CMSSignedData) {
        CMSSignerInfo = CMS_SignerInfoCreate(CMSSignedData);
        SECAlgorithmID **algs = NSS_CMSSignedData_GetDigestAlgs(CMSSignedData);
        while (*algs != nullptr) {
            SECItem usedAlgorithm = (*algs)->algorithm;
            auto hashAlgorithm = SECOID_FindOIDTag(&usedAlgorithm);
            HASH_HashType hashType = HASH_GetHashTypeByOidTag(hashAlgorithm);
            innerHashAlgorithm = ConvertHashTypeFromNss(hashType);
            auto outerHashAlgorithm = (type == CryptoSign::SignatureType::adbe_pkcs7_sha1) ? HashAlgorithm::Sha1 : innerHashAlgorithm;
            hashContext = HashContext::create(outerHashAlgorithm);

            if (hashContext) {
                break;
            }
            ++algs;
        }
    }
}

NSSSignatureCreation::NSSSignatureCreation(const std::string &certNickname, HashAlgorithm digestAlgTag) : hashContext(HashContext::create(digestAlgTag))
{
    NSSSignatureConfiguration::setNSSDir({});
    signing_cert = CERT_FindCertByNickname(CERT_GetDefaultCertDB(), certNickname.c_str());
}

HashAlgorithm NSSSignatureVerification::getHashAlgorithm() const
{
    if (hashContext) {
        return hashContext->getHashAlgorithm();
    }
    return HashAlgorithm::Unknown;
}

void NSSSignatureVerification::addData(unsigned char *data_block, int data_len)
{
    if (hashContext) {
        hashContext->updateHash(data_block, data_len);
    }
}

void NSSSignatureCreation::addData(unsigned char *data_block, int data_len)
{
    hashContext->updateHash(data_block, data_len);
}

NSSSignatureCreation::~NSSSignatureCreation()
{
    if (signing_cert) {
        CERT_DestroyCertificate(signing_cert);
    }
}
NSSSignatureVerification::~NSSSignatureVerification()
{
    if (CMSMessage) {
        // in the CMS_SignedDataCreate, we malloc some memory
        // inside the CMSSignedData structure
        // which is otherwise destructed by NSS_CMSMessage_Destroy
        // but given we did the malloc ourselves
        // we also need to free it ourselves.
        // After we free the surrounding memory but we need
        // a handle to it before.
        CERTCertificate **toFree = nullptr;
        if (CMSSignedData) {
            toFree = CMSSignedData->tempCerts;
        }
        NSS_CMSMessage_Destroy(CMSMessage);
        free(static_cast<void *>(toFree));
    }
}

static NSSCMSMessage *CMS_MessageCreate(SECItem *cms_item)
{
    if (cms_item->data) {
        return NSS_CMSMessage_CreateFromDER(cms_item, nullptr, nullptr /* Content callback */
                                            ,
                                            nullptr, nullptr /*Password callback*/
                                            ,
                                            nullptr, nullptr /*Decrypt callback*/);
    }
    return nullptr;
}

static NSSCMSSignedData *CMS_SignedDataCreate(NSSCMSMessage *cms_msg)
{
    if (!NSS_CMSMessage_IsSigned(cms_msg)) {
        error(errInternal, 0, "Input couldn't be parsed as a CMS signature");
        return nullptr;
    }

    NSSCMSContentInfo *cinfo = NSS_CMSMessage_ContentLevel(cms_msg, 0);
    if (!cinfo) {
        error(errInternal, 0, "Error in NSS_CMSMessage_ContentLevel");
        return nullptr;
    }

    auto *signedData = (NSSCMSSignedData *)NSS_CMSContentInfo_GetContent(cinfo);
    if (!signedData) {
        error(errInternal, 0, "CError in NSS_CMSContentInfo_GetContent()");
        return nullptr;
    }

    if (signedData->rawCerts) {
        size_t i;
        for (i = 0; signedData->rawCerts[i]; ++i) { } // just count the length of the certificate chain

        // tempCerts field needs to be filled for complete memory release by NSSCMSSignedData_Destroy
        signedData->tempCerts = (CERTCertificate **)gmallocn(i + 1, sizeof(CERTCertificate *));
        memset(static_cast<void *>(signedData->tempCerts), 0, (i + 1) * sizeof(CERTCertificate *));
        // store the addresses of these temporary certificates for future release
        for (i = 0; signedData->rawCerts[i]; ++i) {
            signedData->tempCerts[i] = CERT_NewTempCertificate(CERT_GetDefaultCertDB(), signedData->rawCerts[i], nullptr, 0, 0);
        }
        return signedData;
    }
    return nullptr;
}

static NSSCMSSignerInfo *CMS_SignerInfoCreate(NSSCMSSignedData *cms_sig_data)
{
    NSSCMSSignerInfo *signerInfo = NSS_CMSSignedData_GetSignerInfo(cms_sig_data, 0);
    if (!signerInfo) {
        printf("Error in NSS_CMSSignedData_GetSignerInfo()\n");
        return nullptr;
    }
    return signerInfo;
}

static SignatureValidationStatus NSS_SigTranslate(NSSCMSVerificationStatus nss_code)
{
    switch (nss_code) {
    case NSSCMSVS_GoodSignature:
        return SIGNATURE_VALID;

    case NSSCMSVS_BadSignature:
        return SIGNATURE_INVALID;

    case NSSCMSVS_DigestMismatch:
        return SIGNATURE_DIGEST_MISMATCH;

    case NSSCMSVS_ProcessingError:
        return SIGNATURE_DECODING_ERROR;

    default:
        return SIGNATURE_GENERIC_ERROR;
    }
}

SignatureValidationStatus NSSSignatureVerification::validateSignature()
{
    if (!CMSSignedData) {
        return SIGNATURE_GENERIC_ERROR;
    }

    if (!NSS_IsInitialized()) {
        return SIGNATURE_GENERIC_ERROR;
    }

    if (!hashContext) {
        return SIGNATURE_GENERIC_ERROR;
    }

    std::vector<unsigned char> digest_buffer = hashContext->endHash();

    SECItem digest;
    digest.data = digest_buffer.data();
    digest.len = digest_buffer.size();

    if (!signingCertificateAvailable()) {
        return SIGNATURE_INVALID;
    }

    SECItem *content_info_data = CMSSignedData->contentInfo.content.data;
    const bool econtent_present = content_info_data && content_info_data->data;
    const bool signature_type_requires_econtent = type == CryptoSign::SignatureType::adbe_pkcs7_sha1;
    if (signature_type_requires_econtent != econtent_present) {
        return SIGNATURE_INVALID;
    }

    if (type == CryptoSign::SignatureType::adbe_pkcs7_sha1) {
        /*
          This means it's not a detached type signature
          so the digest is contained in SignedData->contentInfo
        */
        if (digest.len != content_info_data->len || memcmp(digest.data, content_info_data->data, digest.len) != 0) {
            return SIGNATURE_DIGEST_MISMATCH;
        }

        auto innerHashContext = HashContext::create(innerHashAlgorithm);
        innerHashContext->updateHash(content_info_data->data, content_info_data->len);
        digest_buffer = innerHashContext->endHash();
        digest.data = digest_buffer.data();
        digest.len = digest_buffer.size();
    }

    if (NSS_CMSSignerInfo_Verify(CMSSignerInfo, &digest, nullptr) != SECSuccess) {
        return NSS_SigTranslate(CMSSignerInfo->verificationStatus);
    }
    return SIGNATURE_VALID;
}

void NSSSignatureVerification::validateCertificateAsync(std::chrono::system_clock::time_point validation_time, bool ocspRevocationCheck, bool useAIACertFetch, const std::function<void()> &doneCallback)
{
    cachedValidationStatus.reset();
    CERTCertificate *cert;

    if (!CMSSignerInfo) {
        validationStatus = std::async([doneCallback]() {
            if (doneCallback) {
                doneCallback();
            }
            return CERTIFICATE_GENERIC_ERROR;
        });
        return;
    }

    if ((cert = NSS_CMSSignerInfo_GetSigningCertificate(CMSSignerInfo, CERT_GetDefaultCertDB())) == nullptr) {
        CMSSignerInfo->verificationStatus = NSSCMSVS_SigningCertNotFound;
    }

    PRTime vTime = 0; // time in microseconds since the epoch, special value 0 means now
    if (validation_time > std::chrono::system_clock::time_point {}) {
        vTime = 1000000 * (PRTime)std::chrono::system_clock::to_time_t(validation_time);
    }
    CERTValInParam inParams[4];
    inParams[0].type = cert_pi_revocationFlags;
    if (ocspRevocationCheck) {
        inParams[0].value.pointer.revocation = CERT_GetClassicOCSPEnabledSoftFailurePolicy();
    } else {
        inParams[0].value.pointer.revocation = CERT_GetClassicOCSPDisabledPolicy();
    }
    inParams[1].type = cert_pi_date;
    inParams[1].value.scalar.time = vTime;
    if (useAIACertFetch) {
        inParams[2].type = cert_pi_useAIACertFetch;
        inParams[2].value.scalar.b = PR_TRUE;
        inParams[3].type = cert_pi_end;
    } else {
        inParams[2].type = cert_pi_end;
    }

    int result = 0;
    if (CERT_PKIXVerifyCert(cert, certificateUsageEmailSigner, inParams, nullptr, CMSSignerInfo->cmsg->pwfn_arg) != SECSuccess) {
        result = PORT_GetError();
    }

    // Here we are just faking the asynchronousness. It should
    // somehow be the call to CERT_PXIXVerifyCert that would
    // be put in the thread, but I'm not sure about all of the
    // thread safety of nss.

    validationStatus = std::async([result, doneCallback]() {
        if (doneCallback) {
            doneCallback();
        }

        switch (result) {
        // 0 not defined in SECErrorCodes, it means success for this purpose.
        case 0:
            return CERTIFICATE_TRUSTED;

        case SEC_ERROR_UNKNOWN_ISSUER:
            return CERTIFICATE_UNKNOWN_ISSUER;

        case SEC_ERROR_UNTRUSTED_ISSUER:
            return CERTIFICATE_UNTRUSTED_ISSUER;

        case SEC_ERROR_REVOKED_CERTIFICATE:
            return CERTIFICATE_REVOKED;

        case SEC_ERROR_EXPIRED_CERTIFICATE:
            return CERTIFICATE_EXPIRED;
        }

        return CERTIFICATE_GENERIC_ERROR;
    });
}

CertificateValidationStatus NSSSignatureVerification::validateCertificateResult()
{
    if (cachedValidationStatus) {
        return cachedValidationStatus.value();
    }
    if (!validationStatus.valid()) {
        return CERTIFICATE_NOT_VERIFIED;
    }
    validationStatus.wait();
    cachedValidationStatus = validationStatus.get();
    return cachedValidationStatus.value();
}

std::variant<std::vector<unsigned char>, CryptoSign::SigningErrorMessage> NSSSignatureCreation::signDetached(const std::string &password)
{
    if (!hashContext) {
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::InternalError, .message = ERROR_IN_CODE_LOCATION };
    }
    std::vector<unsigned char> digest_buffer = hashContext->endHash();
    SECItem digest;
    digest.data = digest_buffer.data();
    digest.len = digest_buffer.size();

    /////////////////////////////////////
    /// Code from LibreOffice under MPLv2
    /////////////////////////////////////
    struct NSSCMSMessageDestroyer
    {
        void operator()(NSSCMSMessage *message) { NSS_CMSMessage_Destroy(message); }
    };
    std::unique_ptr<NSSCMSMessage, NSSCMSMessageDestroyer> cms_msg { NSS_CMSMessage_Create(nullptr) };
    if (!cms_msg) {
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::GenericError, .message = ERROR_IN_CODE_LOCATION };
    }

    NSSCMSSignedData *cms_sd = NSS_CMSSignedData_Create(cms_msg.get());
    if (!cms_sd) {
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::GenericError, .message = ERROR_IN_CODE_LOCATION };
    }

    NSSCMSContentInfo *cms_cinfo = NSS_CMSMessage_GetContentInfo(cms_msg.get());

    if (NSS_CMSContentInfo_SetContent_SignedData(cms_msg.get(), cms_cinfo, cms_sd) != SECSuccess) {
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::GenericError, .message = ERROR_IN_CODE_LOCATION };
    }

    cms_cinfo = NSS_CMSSignedData_GetContentInfo(cms_sd);

    // Attach NULL data as detached data
    if (NSS_CMSContentInfo_SetContent_Data(cms_msg.get(), cms_cinfo, nullptr, PR_TRUE) != SECSuccess) {
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::GenericError, .message = ERROR_IN_CODE_LOCATION };
    }

    // hardcode SHA256 these days...
    NSSCMSSignerInfo *cms_signer = NSS_CMSSignerInfo_Create(cms_msg.get(), signing_cert, SEC_OID_SHA256);
    if (!cms_signer) {
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::GenericError, .message = ERROR_IN_CODE_LOCATION };
    }

    if (NSS_CMSSignerInfo_IncludeCerts(cms_signer, NSSCMSCM_CertChain, certUsageEmailSigner) != SECSuccess) {
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::GenericError, .message = ERROR_IN_CODE_LOCATION };
    }

    if (NSS_CMSSignedData_AddSignerInfo(cms_sd, cms_signer) != SECSuccess) {
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::GenericError, .message = ERROR_IN_CODE_LOCATION };
    }

    if (NSS_CMSSignedData_SetDigestValue(cms_sd, SEC_OID_SHA256, &digest) != SECSuccess) {
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::GenericError, .message = ERROR_IN_CODE_LOCATION };
    }

    struct PLArenaFreeFalse
    {
        void operator()(PLArenaPool *arena) { PORT_FreeArena(arena, PR_FALSE); }
    };
    std::unique_ptr<PLArenaPool, PLArenaFreeFalse> arena { PORT_NewArena(CryptoSign::maxSupportedSignatureSize) };

    // Add the signing certificate as a signed attribute.
    ESSCertIDv2 *aCertIDs[2];
    ESSCertIDv2 aCertID;
    // Write ESSCertIDv2.hashAlgorithm.
    aCertID.hashAlgorithm.algorithm.data = nullptr;
    aCertID.hashAlgorithm.parameters.data = nullptr;
    SECOID_SetAlgorithmID(arena.get(), &aCertID.hashAlgorithm, SEC_OID_SHA256, nullptr);

    // Write ESSCertIDv2.certHash.
    SECItem aCertHashItem;
    unsigned char certhash[32];
    SECStatus rv = PK11_HashBuf(SEC_OID_SHA256, certhash, signing_cert->derCert.data, signing_cert->derCert.len);
    if (rv != SECSuccess) {
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::GenericError, .message = ERROR_IN_CODE_LOCATION };
    }

    aCertHashItem.type = siBuffer;
    aCertHashItem.data = certhash;
    aCertHashItem.len = 32;
    aCertID.certHash = aCertHashItem;

    // Write ESSCertIDv2.issuerSerial.
    IssuerSerial aSerial;
    GeneralName aName;
    aName.name = signing_cert->issuer;
    aSerial.issuer.names = aName;
    aSerial.serialNumber = signing_cert->serialNumber;
    aCertID.issuerSerial = aSerial;
    // Write SigningCertificateV2.certs.
    aCertIDs[0] = &aCertID;
    aCertIDs[1] = nullptr;
    SigningCertificateV2 aCertificate;
    aCertificate.certs = &aCertIDs[0];

    SECItem *pEncodedCertificate = SEC_ASN1EncodeItem(nullptr, nullptr, &aCertificate, SigningCertificateV2Template);
    if (!pEncodedCertificate) {
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::GenericError, .message = ERROR_IN_CODE_LOCATION };
    }

    NSSCMSAttribute aAttribute;
    SECItem aAttributeValues[2];
    SECItem *pAttributeValues[2];
    pAttributeValues[0] = aAttributeValues;
    pAttributeValues[1] = nullptr;
    aAttributeValues[0] = *pEncodedCertificate;
    aAttributeValues[1].type = siBuffer;
    aAttributeValues[1].data = nullptr;
    aAttributeValues[1].len = 0;
    aAttribute.values = pAttributeValues;

    SECOidData aOidData;
    /*
     * id-aa-signingCertificateV2 OBJECT IDENTIFIER ::=
     * { iso(1) member-body(2) us(840) rsadsi(113549) pkcs(1) pkcs9(9)
     *   smime(16) id-aa(2) 47 }
     */
    auto cert_oid_buffer = std::to_array(OID_SIGNINGCERTIFICATEV2);
    aOidData.oid.type = siBuffer;
    aOidData.oid.data = cert_oid_buffer.data();
    aOidData.oid.len = cert_oid_buffer.size();

    aOidData.offset = SEC_OID_UNKNOWN;
    aOidData.desc = "id-aa-signingCertificateV2";
    aOidData.mechanism = CKM_SHA_1;
    aOidData.supportedExtension = UNSUPPORTED_CERT_EXTENSION;
    aAttribute.typeTag = &aOidData;
    aAttribute.type = aOidData.oid;
    aAttribute.encoded = PR_TRUE;

    if (my_NSS_CMSSignerInfo_AddAuthAttr(cms_signer, &aAttribute) != SECSuccess) {
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::GenericError, .message = ERROR_IN_CODE_LOCATION };
    }

    SECItem cms_output;
    cms_output.data = nullptr;
    cms_output.len = 0;

    NSSCMSEncoderContext *cms_ecx = NSS_CMSEncoder_Start(cms_msg.get(), nullptr, nullptr, &cms_output, arena.get(), passwordCallback, password.empty() ? nullptr : const_cast<char *>(password.c_str()), nullptr, nullptr, nullptr, nullptr);
    if (!cms_ecx) {
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::GenericError, .message = ERROR_IN_CODE_LOCATION };
    }

    if (NSS_CMSEncoder_Finish(cms_ecx) != SECSuccess) {
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::GenericError, .message = ERROR_IN_CODE_LOCATION };
    }

    auto signature = std::vector<unsigned char>(cms_output.data, cms_output.data + cms_output.len);

    SECITEM_FreeItem(pEncodedCertificate, PR_TRUE);

    return signature;
}

bool NSSSignatureVerification::signingCertificateAvailable()
{
    auto *cert = NSS_CMSSignerInfo_GetSigningCertificate(CMSSignerInfo, CERT_GetDefaultCertDB());

    if (!cert) {
        return false;
    }

    if (type != CryptoSign::SignatureType::ETSI_CAdES_detached) {
        return true;
    }

    const auto ess_signing_cert = CMSAttributeArray_FindAttrByOid(CMSSignerInfo->authAttr, OID_SIGNINGCERTIFICATE);
    const auto ess_signing_cert_v2 = CMSAttributeArray_FindAttrByOid(CMSSignerInfo->authAttr, OID_SIGNINGCERTIFICATEV2);

    const SEC_ASN1Template *decoding_template {};
    HashAlgorithm hash_algorithm = HashAlgorithm::Unknown;
    const NSSCMSAttribute *attr {};
    bool have_attr_v2 = false;

    if (ess_signing_cert.size() == 1 && ess_signing_cert_v2.empty()) {
        attr = ess_signing_cert.front();
        decoding_template = SigningCertificateDecodingTemplate;
        hash_algorithm = HashAlgorithm::Sha1;
    } else if (ess_signing_cert.empty() && ess_signing_cert_v2.size() == 1) {
        attr = ess_signing_cert_v2.front();
        decoding_template = SigningCertificateV2DecodingTemplate;
        hash_algorithm = HashAlgorithm::Sha256;
        have_attr_v2 = true;
    } else {
        return false;
    }

    if (!attr || !attr->values || !*attr->values || *std::next(attr->values)) {
        return false;
    }

    SigningCertificateV2 decoded_attr;
    auto arena_deleter = [](PLArenaPool *arena) { PORT_FreeArena(arena, PR_FALSE); };
    std::unique_ptr<PLArenaPool, decltype(arena_deleter)> arena { PORT_NewArena(DER_DEFAULT_CHUNKSIZE) };

    if (SEC_ASN1DecodeItem(arena.get(), &decoded_attr, decoding_template, *attr->values) != SECSuccess) {
        return false;
    }

    if (!decoded_attr.certs || !*decoded_attr.certs || !decoded_attr.certs[0]->certHash.data) {
        return false;
    }

    if (have_attr_v2) {
        const SECItem *used_algorithm = &decoded_attr.certs[0]->hashAlgorithm.algorithm;
        if (used_algorithm->data) {
            auto hashType = HASH_GetHashTypeByOidTag(SECOID_FindOIDTag(used_algorithm));
            hash_algorithm = ConvertHashTypeFromNss(hashType);
        }
    }

    if (hash_algorithm == HashAlgorithm::Unknown) {
        return false;
    }

    auto hash_context = HashContext::create(hash_algorithm);
    hash_context->updateHash(cert->derCert.data, cert->derCert.len);
    const auto cert_hash = hash_context->endHash();

    return std::ranges::equal(cert_hash, std::span { decoded_attr.certs[0]->certHash.data, decoded_attr.certs[0]->certHash.len });
}

static char *GetPasswordFunction(PK11SlotInfo *slot, PRBool /*retry*/, void * /*arg*/)
{
    const char *name = PK11_GetTokenName(slot);
    if (PasswordFunction) {
        return PasswordFunction(name);
    }
    return nullptr;
}

std::unique_ptr<CryptoSign::VerificationInterface> NSSCryptoSignBackend::createVerificationHandler(std::vector<unsigned char> &&pkcs7, CryptoSign::SignatureType type)
{
    switch (type) {
    case CryptoSign::SignatureType::unknown_signature_type:
    case CryptoSign::SignatureType::unsigned_signature_field:
    case CryptoSign::SignatureType::g10c_pgp_signature_detached:
        return {};
    case CryptoSign::SignatureType::ETSI_CAdES_detached:
    case CryptoSign::SignatureType::adbe_pkcs7_detached:
    case CryptoSign::SignatureType::adbe_pkcs7_sha1:
        return std::make_unique<NSSSignatureVerification>(std::move(pkcs7), type);
    }
    return {};
}

std::unique_ptr<CryptoSign::SigningInterface> NSSCryptoSignBackend::createSigningHandler(const std::string &certID, HashAlgorithm digestAlgTag)
{
    return std::make_unique<NSSSignatureCreation>(certID, digestAlgTag);
}

std::vector<std::unique_ptr<X509CertificateInfo>> NSSCryptoSignBackend::getAvailableSigningCertificates()
{
    // set callback, in case one of the slots has a password set
    PK11_SetPasswordFunc(GetPasswordFunction);
    NSSSignatureConfiguration::setNSSDir({});

    std::vector<std::unique_ptr<X509CertificateInfo>> certsList;
    PK11SlotList *slotList = PK11_GetAllTokens(CKM_INVALID_MECHANISM, PR_FALSE, PR_FALSE, nullptr);
    if (slotList) {
        for (PK11SlotListElement *slotElement = slotList->head; slotElement; slotElement = slotElement->next) {
            PK11SlotInfo *pSlot = slotElement->slot;
            if (PK11_NeedLogin(pSlot)) {
                SECStatus nRet = PK11_Authenticate(pSlot, PR_TRUE, nullptr);
                // PK11_Authenticate may fail in case the a slot has not been initialized.
                // this is the case if the user has a new profile, so that they have never
                // added a personal certificate.
                if (nRet != SECSuccess && PORT_GetError() != SEC_ERROR_IO) {
                    continue;
                }
            }

            SECKEYPrivateKeyList *privKeyList = PK11_ListPrivateKeysInSlot(pSlot);
            if (privKeyList) {
                for (SECKEYPrivateKeyListNode *curPri = PRIVKEY_LIST_HEAD(privKeyList); !PRIVKEY_LIST_END(curPri, privKeyList) && curPri != nullptr; curPri = PRIVKEY_LIST_NEXT(curPri)) {
                    if (curPri->key) {
                        CERTCertificate *cert = PK11_GetCertFromPrivateKey(curPri->key);
                        if (cert) {
                            certsList.push_back(getCertificateInfoFromCERT(cert));
                            CERT_DestroyCertificate(cert);
                        }
                    }
                }
                SECKEY_DestroyPrivateKeyList(privKeyList);
            }
        }
        PK11_FreeSlotList(slotList);
    }

    PK11_SetPasswordFunc(nullptr);

    return certsList;
}

void HashContext::updateHash(unsigned char *data_block, int data_len)
{
    HASH_Update(hash_context.get(), data_block, data_len);
}

std::vector<unsigned char> HashContext::endHash()
{
    auto hash_length = digestLength(digest_alg_tag);
    std::vector<unsigned char> digestBuffer(hash_length);
    unsigned int result_length = 0;
    HASH_End(hash_context.get(), digestBuffer.data(), &result_length, digestBuffer.size());
    digestBuffer.resize(result_length);

    return digestBuffer;
}

HashContext::HashContext(HashAlgorithm algorithm, private_tag /*unused*/) : hash_context { HASH_Create(HASH_GetHashTypeByOidTag(ConvertHashAlgorithmToNss(algorithm))) }, digest_alg_tag(algorithm) { }

std::unique_ptr<HashContext> HashContext::create(HashAlgorithm algorithm)
{
    auto ctx = std::make_unique<HashContext>(algorithm, private_tag {});
    if (ctx->hash_context) {
        return ctx;
    }
    return {};
}

HashAlgorithm HashContext::getHashAlgorithm() const
{
    return digest_alg_tag;
}

NSSCryptoSignBackend::~NSSCryptoSignBackend() = default;
