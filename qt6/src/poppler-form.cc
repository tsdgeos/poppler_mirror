/* poppler-form.h: qt interface to poppler
 * Copyright (C) 2007-2008, 2011, Pino Toscano <pino@kde.org>
 * Copyright (C) 2008, 2011, 2012, 2015-2026 Albert Astals Cid <aacid@kde.org>
 * Copyright (C) 2011 Carlos Garcia Campos <carlosgc@gnome.org>
 * Copyright (C) 2012, Adam Reichold <adamreichold@myopera.com>
 * Copyright (C) 2016, Hanno Meyer-Thurow <h.mth@web.de>
 * Copyright (C) 2017, Hans-Ulrich Jüttner <huj@froreich-bioscientia.de>
 * Copyright (C) 2018, Andre Heinecke <aheinecke@intevation.de>
 * Copyright (C) 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by the LiMux project of the city of Munich
 * Copyright (C) 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@protonmail.com>
 * Copyright (C) 2018, 2020, 2021 Oliver Sander <oliver.sander@tu-dresden.de>
 * Copyright (C) 2019 João Netto <joaonetto901@gmail.com>
 * Copyright (C) 2020 David García Garzón <voki@canvoki.net>
 * Copyright (C) 2020 Thorsten Behrens <Thorsten.Behrens@CIB.de>
 * Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by Technische Universität Dresden
 * Copyright (C) 2021 Georgiy Sgibnev <georgiy@sgibnev.com>. Work sponsored by lab50.net.
 * Copyright (C) 2021 Theofilos Intzoglou <int.teo@gmail.com>
 * Copyright (C) 2022 Alexander Sulfrian <asulfrian@zedat.fu-berlin.de>
 * Copyright (C) 2023-2025 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
 * Copyright (C) 2024 Pratham Gandhi <ppg.1382@gmail.com>
 * Copyright (C) 2024 Stefan Brüns <stefan.bruens@rwth-aachen.de>
 * Copyright (C) 2025 Blair Bonnett <blair.bonnett@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "poppler-form.h"
#include "poppler-converter.h"

#include <config.h>

#include <QtCore/QSizeF>
#include <QTimeZone>
#include <QUrl>

#include <Form.h>
#include <Object.h>
#include <Link.h>
#include <SignatureInfo.h>
#include <CertificateInfo.h>
#include <CryptoSignBackend.h>
#if ENABLE_NSS3
#    include <NSSCryptoSignBackend.h>
#endif
#if ENABLE_GPGME
#    include <GPGMECryptoSignBackendConfiguration.h>
#endif

#include "poppler-page-private.h"
#include "poppler-private.h"
#include "poppler-annotation-helper.h"

namespace {

Qt::Alignment formTextAlignment(::FormWidget *fm)
{
    Qt::Alignment qtalign = Qt::AlignLeft;
    switch (fm->getField()->getTextQuadding()) {
    case VariableTextQuadding::centered:
        qtalign = Qt::AlignHCenter;
        break;
    case VariableTextQuadding::rightJustified:
        qtalign = Qt::AlignRight;
        break;
    case VariableTextQuadding::leftJustified:
        qtalign = Qt::AlignLeft;
    }
    return qtalign;
}

}

namespace Poppler {

FormFieldIcon::FormFieldIcon(FormFieldIconData *data) : d_ptr(data) { }

FormFieldIcon::FormFieldIcon(const FormFieldIcon &ffIcon)
{
    d_ptr = new FormFieldIconData;
    d_ptr->icon = ffIcon.d_ptr->icon;
}

FormFieldIcon &FormFieldIcon::operator=(const FormFieldIcon &ffIcon)
{
    if (this != &ffIcon) {
        delete d_ptr;
        d_ptr = nullptr;

        d_ptr = new FormFieldIconData;
        *d_ptr = *ffIcon.d_ptr;
    }

    return *this;
}

FormFieldIcon::~FormFieldIcon()
{
    delete d_ptr;
}

FormField::FormField(std::unique_ptr<FormFieldData> dd) : m_formData(std::move(dd))
{
    if (m_formData->page) {
        const int rotation = m_formData->page->getRotate();
        // reading the coords
        double left, top, right, bottom;
        m_formData->fm->getRect(&left, &bottom, &right, &top);
        // build a normalized transform matrix for this page at 100% scale
        GfxState gfxState(72.0, 72.0, m_formData->page->getCropBox(), rotation, true);
        const std::array<double, 6> &gfxCTM = gfxState.getCTM();
        double MTX[6];
        double pageWidth = m_formData->page->getCropWidth();
        double pageHeight = m_formData->page->getCropHeight();
        // landscape and seascape page rotation: be sure to use the correct (== rotated) page size
        if (((rotation / 90) % 2) == 1) {
            qSwap(pageWidth, pageHeight);
        }
        for (int i = 0; i < 6; i += 2) {
            MTX[i] = gfxCTM[i] / pageWidth;
            MTX[i + 1] = gfxCTM[i + 1] / pageHeight;
        }
        QPointF topLeft;
        XPDFReader::transform(MTX, qMin(left, right), qMax(top, bottom), topLeft);
        QPointF bottomRight;
        XPDFReader::transform(MTX, qMax(left, right), qMin(top, bottom), bottomRight);
        m_formData->box = QRectF(topLeft, QSizeF(bottomRight.x() - topLeft.x(), bottomRight.y() - topLeft.y()));
    }
}

FormField::~FormField() = default;

QRectF FormField::rect() const
{
    return m_formData->box;
}

int FormField::id() const
{
    return m_formData->fm->getID();
}

QString FormField::name() const
{
    QString name;
    if (const GooString *goo = m_formData->fm->getPartialName()) {
        name = UnicodeParsedString(goo);
    }
    return name;
}

void FormField::setName(const QString &name) const
{
    const std::unique_ptr<GooString> goo = QStringToGooString(name);
    m_formData->fm->setPartialName(*goo);
}

QString FormField::fullyQualifiedName() const
{
    QString name;
    if (const GooString *goo = m_formData->fm->getFullyQualifiedName()) {
        name = UnicodeParsedString(goo);
    }
    return name;
}

QString FormField::uiName() const
{
    QString name;
    if (const GooString *goo = m_formData->fm->getAlternateUiName()) {
        name = UnicodeParsedString(goo);
    }
    return name;
}

bool FormField::isReadOnly() const
{
    return m_formData->fm->isReadOnly();
}

void FormField::setReadOnly(bool value)
{
    m_formData->fm->setReadOnly(value);
}

bool FormField::isVisible() const
{
    const unsigned int flags = m_formData->fm->getWidgetAnnotation()->getFlags();
    if (flags & Annot::flagHidden) {
        return false;
    }
    if (flags & Annot::flagNoView) {
        return false;
    }
    return true;
}

void FormField::setVisible(bool value)
{
    unsigned int flags = m_formData->fm->getWidgetAnnotation()->getFlags();
    if (value) {
        flags &= ~Annot::flagHidden;
        flags &= ~Annot::flagNoView;
    } else {
        flags |= Annot::flagHidden;
    }
    m_formData->fm->getWidgetAnnotation()->setFlags(flags);
}

bool FormField::isPrintable() const
{
    return (m_formData->fm->getWidgetAnnotation()->getFlags() & Annot::flagPrint);
}

void FormField::setPrintable(bool value)
{
    unsigned int flags = m_formData->fm->getWidgetAnnotation()->getFlags();
    if (value) {
        flags |= Annot::flagPrint;
    } else {
        flags &= ~Annot::flagPrint;
    }
    m_formData->fm->getWidgetAnnotation()->setFlags(flags);
}

std::unique_ptr<Link> FormField::activationAction() const
{
    if (::LinkAction *act = m_formData->fm->getActivationAction()) {
        return PageData::convertLinkActionToLink(act, m_formData->doc, QRectF());
    }

    return {};
}

std::unique_ptr<Link> FormField::additionalAction(AdditionalActionType type) const
{
    Annot::FormAdditionalActionsType actionType = Annot::actionFieldModified;
    switch (type) {
    case FieldModified:
        actionType = Annot::actionFieldModified;
        break;
    case FormatField:
        actionType = Annot::actionFormatField;
        break;
    case ValidateField:
        actionType = Annot::actionValidateField;
        break;
    case CalculateField:
        actionType = Annot::actionCalculateField;
        break;
    }

    if (std::unique_ptr<::LinkAction> act = m_formData->fm->getAdditionalAction(actionType)) {
        return PageData::convertLinkActionToLink(act.get(), m_formData->doc, QRectF());
    }

    return {};
}

std::unique_ptr<Link> FormField::additionalAction(Annotation::AdditionalActionType type) const
{
    ::AnnotWidget *w = m_formData->fm->getWidgetAnnotation().get();
    if (!w) {
        return {};
    }

    const Annot::AdditionalActionsType actionType = toPopplerAdditionalActionType(type);

    if (std::unique_ptr<::LinkAction> act = w->getAdditionalAction(actionType)) {
        return PageData::convertLinkActionToLink(act.get(), m_formData->doc, QRectF());
    }

    return {};
}

FormFieldButton::FormFieldButton(DocumentData *doc, ::Page *p, ::FormWidgetButton *w) : FormField(std::make_unique<FormFieldData>(doc, p, w)) { }

FormFieldButton::~FormFieldButton() = default;

FormFieldButton::FormType FormFieldButton::type() const
{
    return FormField::FormButton;
}

FormFieldButton::ButtonType FormFieldButton::buttonType() const
{
    auto *fwb = static_cast<FormWidgetButton *>(m_formData->fm);
    switch (fwb->getButtonType()) {
    case formButtonCheck:
        return FormFieldButton::CheckBox;
        break;
    case formButtonPush:
        return FormFieldButton::Push;
        break;
    case formButtonRadio:
        return FormFieldButton::Radio;
        break;
    }
    return FormFieldButton::CheckBox;
}

QString FormFieldButton::caption() const
{
    auto *fwb = static_cast<FormWidgetButton *>(m_formData->fm);
    QString ret;
    if (fwb->getButtonType() == formButtonPush) {
        Dict *dict = m_formData->fm->getObj()->getDict();
        Object obj1 = dict->lookup("MK");
        if (obj1.isDict()) {
            AnnotAppearanceCharacs appearCharacs(obj1.getDict());
            if (appearCharacs.getNormalCaption()) {
                ret = UnicodeParsedString(appearCharacs.getNormalCaption());
            }
        }
    } else {
        if (const char *goo = fwb->getOnStr()) {
            ret = QString::fromUtf8(goo);
        }
    }
    return ret;
}

FormFieldIcon FormFieldButton::icon() const
{
    auto *fwb = static_cast<FormWidgetButton *>(m_formData->fm);
    if (fwb->getButtonType() == formButtonPush) {
        Dict *dict = m_formData->fm->getObj()->getDict();
        auto *data = new FormFieldIconData;
        data->icon = dict;
        return FormFieldIcon(data);
    }
    return FormFieldIcon(nullptr);
}

void FormFieldButton::setIcon(const FormFieldIcon &icon)
{
    if (FormFieldIconData::getData(icon) == nullptr) {
        return;
    }

    auto *fwb = static_cast<FormWidgetButton *>(m_formData->fm);
    if (fwb->getButtonType() == formButtonPush) {
        std::shared_ptr<::AnnotWidget> w = m_formData->fm->getWidgetAnnotation();
        FormFieldIconData *data = FormFieldIconData::getData(icon);
        if (data->icon != nullptr) {
            w->setNewAppearance(data->icon->lookup("AP"));
        }
    }
}

bool FormFieldButton::state() const
{
    auto *fwb = static_cast<FormWidgetButton *>(m_formData->fm);
    return fwb->getState();
}

void FormFieldButton::setState(bool state)
{
    auto *fwb = static_cast<FormWidgetButton *>(m_formData->fm);
    fwb->setState(state);
}

QList<int> FormFieldButton::siblings() const
{
    auto *fwb = static_cast<FormWidgetButton *>(m_formData->fm);
    auto *ffb = static_cast<::FormFieldButton *>(fwb->getField());
    if (fwb->getButtonType() == formButtonPush) {
        return QList<int>();
    }

    QList<int> ret;
    for (int i = 0; i < ffb->getNumSiblings(); ++i) {
        auto *sibling = static_cast<::FormFieldButton *>(ffb->getSibling(i));
        for (int j = 0; j < sibling->getNumWidgets(); ++j) {
            FormWidget *w = sibling->getWidget(j);
            if (w) {
                ret.append(w->getID());
            }
        }
    }

    return ret;
}

FormFieldText::FormFieldText(DocumentData *doc, ::Page *p, ::FormWidgetText *w) : FormField(std::make_unique<FormFieldData>(doc, p, w)) { }

FormFieldText::~FormFieldText() = default;

FormField::FormType FormFieldText::type() const
{
    return FormField::FormText;
}

FormFieldText::TextType FormFieldText::textType() const
{
    auto *fwt = static_cast<FormWidgetText *>(m_formData->fm);
    if (fwt->isFileSelect()) {
        return FormFieldText::FileSelect;
    }
    if (fwt->isMultiline()) {
        return FormFieldText::Multiline;
    }
    return FormFieldText::Normal;
}

QString FormFieldText::text() const
{
    const GooString *goo = static_cast<FormWidgetText *>(m_formData->fm)->getContent();
    return UnicodeParsedString(goo);
}

void FormFieldText::setText(const QString &text)
{
    auto *fwt = static_cast<FormWidgetText *>(m_formData->fm);
    std::unique_ptr<GooString> goo = QStringToUnicodeGooString(text);
    fwt->setContent(std::move(goo));
}

void FormFieldText::setAppearanceText(const QString &text)
{
    auto *fwt = static_cast<FormWidgetText *>(m_formData->fm);
    std::unique_ptr<GooString> goo = QStringToUnicodeGooString(text);
    fwt->setAppearanceContent(std::move(goo));
}

bool FormFieldText::isPassword() const
{
    auto *fwt = static_cast<FormWidgetText *>(m_formData->fm);
    return fwt->isPassword();
}

bool FormFieldText::isRichText() const
{
    auto *fwt = static_cast<FormWidgetText *>(m_formData->fm);
    return fwt->isRichText();
}

int FormFieldText::maximumLength() const
{
    auto *fwt = static_cast<FormWidgetText *>(m_formData->fm);
    const int maxlen = fwt->getMaxLen();
    return maxlen > 0 ? maxlen : -1;
}

Qt::Alignment FormFieldText::textAlignment() const
{
    return formTextAlignment(m_formData->fm);
}

bool FormFieldText::canBeSpellChecked() const
{
    auto *fwt = static_cast<FormWidgetText *>(m_formData->fm);
    return !fwt->noSpellCheck();
}

double FormFieldText::getFontSize() const
{
    auto *fwt = static_cast<FormWidgetText *>(m_formData->fm);
    return fwt->getTextFontSize();
}

void FormFieldText::setFontSize(int fontSize)
{
    auto *fwt = static_cast<FormWidgetText *>(m_formData->fm);
    fwt->setTextFontSize(fontSize);
}

FormFieldChoice::FormFieldChoice(DocumentData *doc, ::Page *p, ::FormWidgetChoice *w) : FormField(std::make_unique<FormFieldData>(doc, p, w)) { }

FormFieldChoice::~FormFieldChoice() = default;

FormFieldChoice::FormType FormFieldChoice::type() const
{
    return FormField::FormChoice;
}

FormFieldChoice::ChoiceType FormFieldChoice::choiceType() const
{
    auto *fwc = static_cast<FormWidgetChoice *>(m_formData->fm);
    if (fwc->isCombo()) {
        return FormFieldChoice::ComboBox;
    }
    return FormFieldChoice::ListBox;
}

QStringList FormFieldChoice::choices() const
{
    auto *fwc = static_cast<FormWidgetChoice *>(m_formData->fm);
    QStringList ret;
    int num = fwc->getNumChoices();
    ret.reserve(num);
    for (int i = 0; i < num; ++i) {
        ret.append(UnicodeParsedString(fwc->getChoice(i)));
    }
    return ret;
}

QVector<QPair<QString, QString>> FormFieldChoice::choicesWithExportValues() const
{
    auto *fwc = static_cast<FormWidgetChoice *>(m_formData->fm);
    QVector<QPair<QString, QString>> ret;
    const int num = fwc->getNumChoices();
    ret.reserve(num);
    for (int i = 0; i < num; ++i) {
        const QString display = UnicodeParsedString(fwc->getChoice(i));
        const GooString *exportValueG = fwc->getExportVal(i);
        const QString exportValue = exportValueG ? UnicodeParsedString(exportValueG) : display;
        ret.append({ display, exportValue });
    }
    return ret;
}

bool FormFieldChoice::isEditable() const
{
    auto *fwc = static_cast<FormWidgetChoice *>(m_formData->fm);
    return fwc->isCombo() ? fwc->hasEdit() : false;
}

bool FormFieldChoice::multiSelect() const
{
    auto *fwc = static_cast<FormWidgetChoice *>(m_formData->fm);
    return !fwc->isCombo() ? fwc->isMultiSelect() : false;
}

QList<int> FormFieldChoice::currentChoices() const
{
    auto *fwc = static_cast<FormWidgetChoice *>(m_formData->fm);
    int num = fwc->getNumChoices();
    QList<int> choices;
    for (int i = 0; i < num; ++i) {
        if (fwc->isSelected(i)) {
            choices.append(i);
        }
    }
    return choices;
}

void FormFieldChoice::setCurrentChoices(const QList<int> &choice)
{
    auto *fwc = static_cast<FormWidgetChoice *>(m_formData->fm);
    fwc->deselectAll();
    for (int i = 0; i < choice.count(); ++i) {
        fwc->select(choice.at(i));
    }
}

QString FormFieldChoice::editChoice() const
{
    auto *fwc = static_cast<FormWidgetChoice *>(m_formData->fm);

    if (fwc->isCombo() && fwc->hasEdit()) {
        return UnicodeParsedString(fwc->getEditChoice());
    }
    return QString();
}

void FormFieldChoice::setEditChoice(const QString &text)
{
    auto *fwc = static_cast<FormWidgetChoice *>(m_formData->fm);

    if (fwc->isCombo() && fwc->hasEdit()) {
        std::unique_ptr<GooString> goo = QStringToUnicodeGooString(text);
        fwc->setEditChoice(std::move(goo));
    }
}

Qt::Alignment FormFieldChoice::textAlignment() const
{
    return formTextAlignment(m_formData->fm);
}

bool FormFieldChoice::canBeSpellChecked() const
{
    auto *fwc = static_cast<FormWidgetChoice *>(m_formData->fm);
    return !fwc->noSpellCheck();
}

void FormFieldChoice::setAppearanceChoiceText(const QString &text)
{
    auto *fwc = static_cast<FormWidgetChoice *>(m_formData->fm);
    std::unique_ptr<GooString> goo = QStringToUnicodeGooString(text);
    fwc->setAppearanceChoiceContent(std::move(goo));
}

class CertificateInfoPrivate
{
public:
    struct EntityInfo
    {
        QString common_name;
        QString email_address;
        QString org_name;
        QString distinguished_name;
    };

    EntityInfo issuer_info;
    EntityInfo subject_info;
    QString nick_name;
    QByteArray certificate_der;
    QByteArray serial_number;
    QByteArray public_key;
    QDateTime validity_start;
    QDateTime validity_end;
    int public_key_type;
    int public_key_strength;
    int ku_extensions;
    int version;
    bool is_self_signed;
    bool is_null;
    bool is_qualified;
    CertificateInfo::CertificateType certificateType;
    CertificateInfo::KeyLocation keyLocation;
};

CertificateInfo::CertificateInfo() : d_ptr(new CertificateInfoPrivate())
{
    d_ptr->is_null = true;
}

CertificateInfo::CertificateInfo(CertificateInfoPrivate *priv) : d_ptr(priv) { }

CertificateInfo::CertificateInfo(const CertificateInfo &other) = default;

CertificateInfo::~CertificateInfo() = default;

CertificateInfo &CertificateInfo::operator=(const CertificateInfo &other)
{
    if (this != &other) {
        d_ptr = other.d_ptr;
    }

    return *this;
}

bool CertificateInfo::isNull() const
{
    Q_D(const CertificateInfo);
    return d->is_null;
}

int CertificateInfo::version() const
{
    Q_D(const CertificateInfo);
    return d->version;
}

QByteArray CertificateInfo::serialNumber() const
{
    Q_D(const CertificateInfo);
    return d->serial_number;
}

bool CertificateInfo::isQualified() const
{
    Q_D(const CertificateInfo);
    return d->is_qualified;
}

CertificateInfo::CertificateType CertificateInfo::certificateType() const
{
    Q_D(const CertificateInfo);
    return d->certificateType;
}

QString CertificateInfo::issuerInfo(EntityInfoKey key) const
{
    Q_D(const CertificateInfo);
    switch (key) {
    case CommonName:
        return d->issuer_info.common_name;
    case DistinguishedName:
        return d->issuer_info.distinguished_name;
    case EmailAddress:
        return d->issuer_info.email_address;
    case Organization:
        return d->issuer_info.org_name;
    default:
        return QString();
    }
}

QString CertificateInfo::subjectInfo(EntityInfoKey key) const
{
    Q_D(const CertificateInfo);
    switch (key) {
    case CommonName:
        return d->subject_info.common_name;
    case DistinguishedName:
        return d->subject_info.distinguished_name;
    case EmailAddress:
        return d->subject_info.email_address;
    case Organization:
        return d->subject_info.org_name;
    default:
        return QString();
    }
}

QString CertificateInfo::nickName() const
{
    Q_D(const CertificateInfo);
    return d->nick_name;
}

QDateTime CertificateInfo::validityStart() const
{
    Q_D(const CertificateInfo);
    return d->validity_start;
}

QDateTime CertificateInfo::validityEnd() const
{
    Q_D(const CertificateInfo);
    return d->validity_end;
}

CertificateInfo::KeyUsageExtensions CertificateInfo::keyUsageExtensions() const
{
    Q_D(const CertificateInfo);

    KeyUsageExtensions kuExtensions = KuNone;
    if (d->ku_extensions & KU_DIGITAL_SIGNATURE) {
        kuExtensions |= KuDigitalSignature;
    }
    if (d->ku_extensions & KU_NON_REPUDIATION) {
        kuExtensions |= KuNonRepudiation;
    }
    if (d->ku_extensions & KU_KEY_ENCIPHERMENT) {
        kuExtensions |= KuKeyEncipherment;
    }
    if (d->ku_extensions & KU_DATA_ENCIPHERMENT) {
        kuExtensions |= KuDataEncipherment;
    }
    if (d->ku_extensions & KU_KEY_AGREEMENT) {
        kuExtensions |= KuKeyAgreement;
    }
    if (d->ku_extensions & KU_KEY_CERT_SIGN) {
        kuExtensions |= KuKeyCertSign;
    }
    if (d->ku_extensions & KU_CRL_SIGN) {
        kuExtensions |= KuClrSign;
    }
    if (d->ku_extensions & KU_ENCIPHER_ONLY) {
        kuExtensions |= KuEncipherOnly;
    }

    return kuExtensions;
}

CertificateInfo::KeyLocation CertificateInfo::keyLocation() const
{
    Q_D(const CertificateInfo);
    return d->keyLocation;
}

QByteArray CertificateInfo::publicKey() const
{
    Q_D(const CertificateInfo);
    return d->public_key;
}

CertificateInfo::PublicKeyType CertificateInfo::publicKeyType() const
{
    Q_D(const CertificateInfo);
    switch (d->public_key_type) {
    case RSAKEY:
        return RsaKey;
    case DSAKEY:
        return DsaKey;
    case ECKEY:
        return EcKey;
    default:
        return OtherKey;
    }
}

int CertificateInfo::publicKeyStrength() const
{
    Q_D(const CertificateInfo);
    return d->public_key_strength;
}

bool CertificateInfo::isSelfSigned() const
{
    Q_D(const CertificateInfo);
    return d->is_self_signed;
}

QByteArray CertificateInfo::certificateData() const
{
    Q_D(const CertificateInfo);
    return d->certificate_der;
}

bool CertificateInfo::checkPassword(const QString &password) const
{
#if ENABLE_SIGNATURES
    auto backend = CryptoSign::Factory::createActive();
    if (!backend) {
        return false;
    }
    Q_D(const CertificateInfo);
    auto sigHandler = backend->createSigningHandler(d->nick_name.toStdString(), HashAlgorithm::Sha256);
    unsigned char buffer[5];
    memcpy(buffer, "test", 5);
    sigHandler->addData(buffer, 5);
    std::variant<std::vector<unsigned char>, CryptoSign::SigningErrorMessage> tmpSignature = sigHandler->signDetached(password.toStdString());
    return std::holds_alternative<std::vector<unsigned char>>(tmpSignature);
#else
    (void)password;
    return false;
#endif
}

class SignatureValidationInfoPrivate
{
public:
    explicit SignatureValidationInfoPrivate(CertificateInfo &&ci) : cert_info(ci) { }

    SignatureValidationInfo::SignatureStatus signature_status;
    SignatureValidationInfo::CertificateStatus certificate_status;
    CertificateInfo cert_info;

    QByteArray signature;
    QString signer_name;
    QString signer_subject_dn;
    QString location;
    QString reason;
    HashAlgorithm hash_algorithm;
    time_t signing_time;
    QList<qint64> range_bounds;
    qint64 docLength;
};

SignatureValidationInfo::SignatureValidationInfo(SignatureValidationInfoPrivate *priv) : d_ptr(priv) { }

SignatureValidationInfo::SignatureValidationInfo(const SignatureValidationInfo &other) = default;

SignatureValidationInfo::~SignatureValidationInfo() = default;

SignatureValidationInfo::SignatureStatus SignatureValidationInfo::signatureStatus() const
{
    Q_D(const SignatureValidationInfo);
    return d->signature_status;
}

SignatureValidationInfo::CertificateStatus SignatureValidationInfo::certificateStatus() const
{
    Q_D(const SignatureValidationInfo);
    return d->certificate_status;
}

QString SignatureValidationInfo::signerName() const
{
    Q_D(const SignatureValidationInfo);
    return d->signer_name;
}

QString SignatureValidationInfo::signerSubjectDN() const
{
    Q_D(const SignatureValidationInfo);
    return d->signer_subject_dn;
}

QString SignatureValidationInfo::location() const
{
    Q_D(const SignatureValidationInfo);
    return d->location;
}

QString SignatureValidationInfo::reason() const
{
    Q_D(const SignatureValidationInfo);
    return d->reason;
}

SignatureValidationInfo::HashAlgorithm SignatureValidationInfo::hashAlgorithm() const
{
#if ENABLE_SIGNATURES
    Q_D(const SignatureValidationInfo);

    switch (d->hash_algorithm) {
    case ::HashAlgorithm::Md2:
        return HashAlgorithmMd2;
    case ::HashAlgorithm::Md5:
        return HashAlgorithmMd5;
    case ::HashAlgorithm::Sha1:
        return HashAlgorithmSha1;
    case ::HashAlgorithm::Sha256:
        return HashAlgorithmSha256;
    case ::HashAlgorithm::Sha384:
        return HashAlgorithmSha384;
    case ::HashAlgorithm::Sha512:
        return HashAlgorithmSha512;
    case ::HashAlgorithm::Sha224:
        return HashAlgorithmSha224;
    case ::HashAlgorithm::Unknown:
        return HashAlgorithmUnknown;
    }
#endif
    return HashAlgorithmUnknown;
}

time_t SignatureValidationInfo::signingTime() const
{
    Q_D(const SignatureValidationInfo);
    return d->signing_time;
}

QByteArray SignatureValidationInfo::signature() const
{
    Q_D(const SignatureValidationInfo);
    return d->signature;
}

QList<qint64> SignatureValidationInfo::signedRangeBounds() const
{
    Q_D(const SignatureValidationInfo);
    return d->range_bounds;
}

bool SignatureValidationInfo::signsTotalDocument() const
{
    Q_D(const SignatureValidationInfo);
    if (d->range_bounds.size() == 4 && d->range_bounds.value(0) == 0 && d->range_bounds.value(1) >= 0 && d->range_bounds.value(2) > d->range_bounds.value(1) && d->range_bounds.value(3) >= d->range_bounds.value(2)) {
        // The range from d->range_bounds.value(1) to d->range_bounds.value(2) is
        // not authenticated by the signature and should only contain the signature
        // itself padded with 0 bytes. This has been checked in readSignature().
        // If it failed, d->signature is empty.
        // A potential range after d->range_bounds.value(3) would be also not
        // authenticated. Therefore d->range_bounds.value(3) should coincide with
        // the end of the document.
        if (d->docLength == d->range_bounds.value(3) && !d->signature.isEmpty()) {
            return true;
        }
    }
    return false;
}

CertificateInfo SignatureValidationInfo::certificateInfo() const
{
    Q_D(const SignatureValidationInfo);
    return d->cert_info;
}

SignatureValidationInfo &SignatureValidationInfo::operator=(const SignatureValidationInfo &other)
{
    if (this != &other) {
        d_ptr = other.d_ptr;
    }

    return *this;
}

FormFieldSignature::FormFieldSignature(DocumentData *doc, ::Page *p, ::FormWidgetSignature *w) : FormField(std::make_unique<FormFieldData>(doc, p, w)) { }

FormFieldSignature::~FormFieldSignature() = default;

FormField::FormType FormFieldSignature::type() const
{
    return FormField::FormSignature;
}

FormFieldSignature::SignatureType FormFieldSignature::signatureType() const
{
    SignatureType sigType = AdbePkcs7detached;
    auto *fws = static_cast<FormWidgetSignature *>(m_formData->fm);
    switch (fws->signatureType()) {
    case CryptoSign::SignatureType::adbe_pkcs7_sha1:
        sigType = AdbePkcs7sha1;
        break;
    case CryptoSign::SignatureType::adbe_pkcs7_detached:
        sigType = AdbePkcs7detached;
        break;
    case CryptoSign::SignatureType::ETSI_CAdES_detached:
        sigType = EtsiCAdESdetached;
        break;
    case CryptoSign::SignatureType::unknown_signature_type:
        sigType = UnknownSignatureType;
        break;
    case CryptoSign::SignatureType::g10c_pgp_signature_detached:
        sigType = G10cPgpSignatureDetached;
        break;
    case CryptoSign::SignatureType::unsigned_signature_field:
        sigType = UnsignedSignature;
        break;
    }
    return sigType;
}

SignatureValidationInfo FormFieldSignature::validate(ValidateOptions opt) const
{
    auto tempResult = validateAsync(opt);
    tempResult.first.d_ptr->certificate_status = validateResult();
    return tempResult.first;
}

static CertificateInfo::CertificateType fromPopplerCore(CertificateType type)
{
    switch (type) {
    case CertificateType::PGP:
        return CertificateInfo::CertificateType::PGP;
    case CertificateType::X509:
        return CertificateInfo::CertificateType::X509;
    }
    return CertificateInfo::CertificateType::X509; // fallback
}

static CertificateInfo::KeyLocation fromPopplerCore(KeyLocation location)
{
    switch (location) {
    case KeyLocation::Computer:
        return CertificateInfo::KeyLocation::Computer;
    case KeyLocation::Other:
        return CertificateInfo::KeyLocation::Other;
    case KeyLocation::Unknown:
        return CertificateInfo::KeyLocation::Unknown;
    case KeyLocation::HardwareToken:
        return CertificateInfo::KeyLocation::HardwareToken;
    }
    return CertificateInfo::KeyLocation::Unknown;
}

static CertificateInfoPrivate *createCertificateInfoPrivate(const X509CertificateInfo *ci)
{
    auto *certPriv = new CertificateInfoPrivate;
    certPriv->is_null = true;
    if (ci) {
        certPriv->version = ci->getVersion();
        certPriv->ku_extensions = ci->getKeyUsageExtensions();
        certPriv->keyLocation = fromPopplerCore(ci->getKeyLocation());
        certPriv->certificateType = fromPopplerCore(ci->getCertificateType());

        const GooString &certSerial = ci->getSerialNumber();
        certPriv->serial_number = QByteArray(certSerial.c_str(), certSerial.size());

        const X509CertificateInfo::EntityInfo &issuerInfo = ci->getIssuerInfo();
        certPriv->issuer_info.common_name = QString::fromStdString(issuerInfo.commonName);
        certPriv->issuer_info.distinguished_name = QString::fromStdString(issuerInfo.distinguishedName);
        certPriv->issuer_info.email_address = QString::fromStdString(issuerInfo.email);
        certPriv->issuer_info.org_name = QString::fromStdString(issuerInfo.organization);

        const X509CertificateInfo::EntityInfo &subjectInfo = ci->getSubjectInfo();
        certPriv->subject_info.common_name = QString::fromStdString(subjectInfo.commonName);
        certPriv->subject_info.distinguished_name = QString::fromStdString(subjectInfo.distinguishedName);
        certPriv->subject_info.email_address = QString::fromStdString(subjectInfo.email);
        certPriv->subject_info.org_name = QString::fromStdString(subjectInfo.organization);

        certPriv->nick_name = QString::fromStdString(ci->getNickName().toStr());

        X509CertificateInfo::Validity certValidity = ci->getValidity();
        certPriv->validity_start = QDateTime::fromSecsSinceEpoch(certValidity.notBefore, QTimeZone::utc());
        certPriv->validity_end = QDateTime::fromSecsSinceEpoch(certValidity.notAfter, QTimeZone::utc());

        const X509CertificateInfo::PublicKeyInfo &pkInfo = ci->getPublicKeyInfo();
        certPriv->public_key = QByteArray(pkInfo.publicKey.c_str(), pkInfo.publicKey.size());
        certPriv->public_key_type = static_cast<int>(pkInfo.publicKeyType);
        certPriv->public_key_strength = pkInfo.publicKeyStrength;

        const GooString &certDer = ci->getCertificateDER();
        certPriv->certificate_der = QByteArray(certDer.c_str(), certDer.size());

        certPriv->is_null = false;
        certPriv->is_qualified = ci->isQualified();
    }

    return certPriv;
}

static SignatureValidationInfo::CertificateStatus fromInternal(CertificateValidationStatus status)
{
    switch (status) {
    case CERTIFICATE_TRUSTED:
        return SignatureValidationInfo::CertificateTrusted;
    case CERTIFICATE_UNTRUSTED_ISSUER:
        return SignatureValidationInfo::CertificateUntrustedIssuer;
    case CERTIFICATE_UNKNOWN_ISSUER:
        return SignatureValidationInfo::CertificateUnknownIssuer;
    case CERTIFICATE_REVOKED:
        return SignatureValidationInfo::CertificateRevoked;
    case CERTIFICATE_EXPIRED:
        return SignatureValidationInfo::CertificateExpired;
    default:
    case CERTIFICATE_GENERIC_ERROR:
        return SignatureValidationInfo::CertificateGenericError;
    case CERTIFICATE_NOT_VERIFIED:
        return SignatureValidationInfo::CertificateNotVerified;
    }
}

static SignatureValidationInfo fromInternal(SignatureInfo *si, FormWidgetSignature *fws)
{
    // get certificate info
    const X509CertificateInfo *ci = si->getCertificateInfo();
    CertificateInfoPrivate *certPriv = createCertificateInfoPrivate(ci);

    auto *priv = new SignatureValidationInfoPrivate(CertificateInfo(certPriv));
    switch (si->getSignatureValStatus()) {
    case SIGNATURE_VALID:
        priv->signature_status = SignatureValidationInfo::SignatureValid;
        break;
    case SIGNATURE_INVALID:
        priv->signature_status = SignatureValidationInfo::SignatureInvalid;
        break;
    case SIGNATURE_DIGEST_MISMATCH:
        priv->signature_status = SignatureValidationInfo::SignatureDigestMismatch;
        break;
    case SIGNATURE_DECODING_ERROR:
        priv->signature_status = SignatureValidationInfo::SignatureDecodingError;
        break;
    default:
    case SIGNATURE_GENERIC_ERROR:
        priv->signature_status = SignatureValidationInfo::SignatureGenericError;
        break;
    case SIGNATURE_NOT_FOUND:
        priv->signature_status = SignatureValidationInfo::SignatureNotFound;
        break;
    case SIGNATURE_NOT_VERIFIED:
        priv->signature_status = SignatureValidationInfo::SignatureNotVerified;
        break;
    }
    priv->certificate_status = SignatureValidationInfo::CertificateVerificationInProgress;
    priv->signer_name = QString::fromStdString(si->getSignerName());
    priv->signer_subject_dn = QString::fromStdString(si->getSubjectDN());
    priv->hash_algorithm = si->getHashAlgorithm();
    priv->location = UnicodeParsedString(si->getLocation().toStr());
    priv->reason = UnicodeParsedString(si->getReason().toStr());

    priv->signing_time = si->getSigningTime();
    const std::vector<Goffset> ranges = fws->getSignedRangeBounds();
    if (!ranges.empty()) {
        for (Goffset bound : ranges) {
            priv->range_bounds.append(bound);
        }
    }
    std::optional<std::vector<unsigned char>> checkedSignature;
    std::tie(checkedSignature, priv->docLength) = fws->getCheckedSignature();
    if (priv->range_bounds.size() == 4 && checkedSignature) {
        priv->signature = QByteArray::fromRawData(reinterpret_cast<const char *>(checkedSignature->data()), checkedSignature->size());
    }

    return SignatureValidationInfo(priv);
}

SignatureValidationInfo FormFieldSignature::validate(int opt, const QDateTime &validationTime) const
{
    auto tempResult = validateAsync(static_cast<ValidateOptions>(opt), validationTime);
    tempResult.first.d_ptr->certificate_status = validateResult();
    return tempResult.first;
}

class AsyncObjectPrivate
{ /*Currently unused. Created for abi future proofing*/
};

AsyncObject::AsyncObject() : QObject(nullptr) { }

AsyncObject::~AsyncObject() = default;

std::pair<SignatureValidationInfo, std::shared_ptr<Poppler::AsyncObject>> FormFieldSignature::validateAsync(ValidateOptions opt, const QDateTime &validationTime) const
{
    auto object = std::make_shared<AsyncObject>();
    auto *fws = static_cast<FormWidgetSignature *>(m_formData->fm);
    const time_t validationTimeT = validationTime.isValid() ? validationTime.toSecsSinceEpoch() : -1;
    SignatureInfo *si = fws->validateSignatureAsync(opt & ValidateVerifyCertificate, opt & ValidateForceRevalidation, validationTimeT, !(opt & ValidateWithoutOCSPRevocationCheck), opt & ValidateUseAIACertFetch,
                                                    [obj = std::weak_ptr<AsyncObject>(object)]() {
                                                        if (auto l = obj.lock()) {
                                                            // We need to roundtrip over the eventloop
                                                            // to ensure callers have a chance of connecting to AsyncObject::done
                                                            QMetaObject::invokeMethod(
                                                                    l.get(),
                                                                    [innerObj = std::weak_ptr<AsyncObject>(l)]() {
                                                                        if (auto innerLocked = innerObj.lock()) {
                                                                            Q_EMIT innerLocked->done();
                                                                        }
                                                                    },
                                                                    Qt::QueuedConnection);
                                                        }
                                                    });

    return { fromInternal(si, fws), object };
}

SignatureValidationInfo::CertificateStatus FormFieldSignature::validateResult() const
{
    return fromInternal(static_cast<FormWidgetSignature *>(m_formData->fm)->validateSignatureResult());
}

FormFieldSignature::SigningResult FormFieldSignature::sign(const QString &outputFileName, const PDFConverter::NewSignatureData &data) const
{
    auto *fws = static_cast<FormWidgetSignature *>(m_formData->fm);
    if (fws->signatureType() != CryptoSign::SignatureType::unsigned_signature_field) {
        return FieldAlreadySigned;
    }

    const auto [sig, file_size] = fws->getCheckedSignature();
    if (sig) {
        // the above unsigned_signature_field check
        // should already catch this, but double check
        return FieldAlreadySigned;
    }
    const auto reason = std::unique_ptr<GooString>(data.reason().isEmpty() ? nullptr : QStringToUnicodeGooString(data.reason()));
    const auto location = std::unique_ptr<GooString>(data.location().isEmpty() ? nullptr : QStringToUnicodeGooString(data.location()));
    const auto ownerPwd = std::optional<GooString>(data.documentOwnerPassword().constData());
    const auto userPwd = std::optional<GooString>(data.documentUserPassword().constData());
    const auto gSignatureText = std::unique_ptr<GooString>(QStringToUnicodeGooString(data.signatureText()));
    const auto gSignatureLeftText = std::unique_ptr<GooString>(QStringToUnicodeGooString(data.signatureLeftText()));

    const auto failure = fws->signDocumentWithAppearance(outputFileName.toStdString(), data.certNickname().toStdString(), data.password().toStdString(), reason.get(), location.get(), ownerPwd, userPwd, *gSignatureText, *gSignatureLeftText,
                                                         data.fontSize(), data.leftFontSize(), convertQColor(data.fontColor()), data.borderWidth(), convertQColor(data.borderColor()), convertQColor(data.backgroundColor()));
    if (failure) {
        m_formData->lastSigningErrorDetails = fromPopplerCore(failure.value().message);
        switch (failure.value().type) {
        case CryptoSign::SigningError::GenericError:
            return GenericSigningError;
        case CryptoSign::SigningError::InternalError:
            return InternalError;
        case CryptoSign::SigningError::KeyMissing:
            return KeyMissing;
        case CryptoSign::SigningError::UserCancelled:
            return UserCancelled;
        case CryptoSign::SigningError::WriteFailed:
            return WriteFailed;
        case CryptoSign::SigningError::BadPassphrase:
            return BadPassphrase;
        }
        return GenericSigningError;
    }
    m_formData->lastSigningErrorDetails = {};
    return SigningSuccess;
}

ErrorString FormFieldSignature::lastSigningErrorDetails() const
{
    return m_formData->lastSigningErrorDetails;
}

bool hasNSSSupport()
{
#if ENABLE_NSS3
    return true;
#else
    return false;
#endif
}

QVector<CertificateInfo> getAvailableSigningCertificates()
{
    auto backend = CryptoSign::Factory::createActive();
    if (!backend) {
        return {};
    }
    QVector<CertificateInfo> vReturnCerts;
    std::vector<std::unique_ptr<X509CertificateInfo>> vCerts = backend->getAvailableSigningCertificates();

    for (auto &cert : vCerts) {
        CertificateInfoPrivate *certPriv = createCertificateInfoPrivate(cert.get());
        vReturnCerts.append(CertificateInfo(certPriv));
    }

    return vReturnCerts;
}

static std::optional<CryptoSignBackend> convertToFrontend(std::optional<CryptoSign::Backend::Type> type)
{
    if (!type) {
        return std::nullopt;
    }
    switch (type.value()) {
    case CryptoSign::Backend::Type::NSS3:
        return CryptoSignBackend::NSS;
    case CryptoSign::Backend::Type::GPGME:
        return CryptoSignBackend::GPG;
    }
    return std::nullopt;
}

static std::optional<CryptoSign::Backend::Type> convertToBackend(std::optional<CryptoSignBackend> backend)
{
    if (!backend) {
        return std::nullopt;
    }

    switch (backend.value()) {
    case CryptoSignBackend::NSS:
        return CryptoSign::Backend::Type::NSS3;
    case CryptoSignBackend::GPG:
        return CryptoSign::Backend::Type::GPGME;
    }
    return std::nullopt;
}

QVector<CryptoSignBackend> availableCryptoSignBackends()
{
    QVector<CryptoSignBackend> backends;
    for (auto &backend : CryptoSign::Factory::getAvailable()) {
        auto converted = convertToFrontend(backend);
        if (converted) {
            backends.push_back(converted.value());
        }
    }
    return backends;
}

std::optional<CryptoSignBackend> activeCryptoSignBackend()
{
    return convertToFrontend(CryptoSign::Factory::getActive());
}

bool setActiveCryptoSignBackend(CryptoSignBackend backend)
{
    auto available = availableCryptoSignBackends();
    if (!available.contains(backend)) {
        return false;
    }
    auto converted = convertToBackend(backend);
    if (!converted) {
        return false;
    }
    CryptoSign::Factory::setPreferredBackend(converted.value());
    return activeCryptoSignBackend() == backend;
}

static bool hasNSSBackendFeature(CryptoSignBackendFeature feature)
{
    switch (feature) {
    case CryptoSignBackendFeature::BackendAsksPassphrase:
        return false;
    }
    return false;
}

static bool hasGPGBackendFeature(CryptoSignBackendFeature feature)
{
    switch (feature) {
    case CryptoSignBackendFeature::BackendAsksPassphrase:
        return true;
    }
    return false;
}

bool hasCryptoSignBackendFeature(CryptoSignBackend backend, CryptoSignBackendFeature feature)
{
    switch (backend) {
    case CryptoSignBackend::NSS:
        return hasNSSBackendFeature(feature);
    case CryptoSignBackend::GPG:
        return hasGPGBackendFeature(feature);
    }
    return false;
}

QString POPPLER_QT6_EXPORT getNSSDir()
{
#if ENABLE_NSS3
    return QString::fromLocal8Bit(NSSSignatureConfiguration::getNSSDir().c_str());
#else
    return QString();
#endif
}

void setNSSDir(const QString &path)
{
#if ENABLE_NSS3
    if (path.isEmpty()) {
        return;
    }

    const std::unique_ptr<GooString> goo = QStringToGooString(path);
    NSSSignatureConfiguration::setNSSDir(*goo);
#else
    (void)path;
#endif
}

namespace {
std::function<QString(const QString &)> nssPasswordCall;
}

void setNSSPasswordCallback(const std::function<char *(const char *)> &f)
{
#if ENABLE_NSS3
    NSSSignatureConfiguration::setNSSPasswordCallback(f);
#else
    qWarning() << "setNSSPasswordCallback called but this poppler is built without NSS support";
    (void)f;
#endif
}

void setPgpSignaturesAllowed(bool allowed)
{
#if ENABLE_GPGME
    GpgSignatureConfiguration::setPgpSignaturesAllowed(allowed);
#else
    qWarning() << "Trying to enable pgp signatures, but pgp not enabled in this build";
    (void)allowed;
#endif
}

bool arePgpSignaturesAllowed()
{
#if ENABLE_GPGME
    return GpgSignatureConfiguration::arePgpSignaturesAllowed();
#else
    return false;
#endif
}
}
