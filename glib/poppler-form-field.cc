/* poppler-form-field.cc: glib interface to poppler
 *
 * Copyright (C) 2007 Carlos Garcia Campos <carlosgc@gnome.org>
 * Copyright (C) 2006 Julien Rebetez
 * Copyright (C) 2020 Oliver Sander <oliver.sander@tu-dresden.de>
 * Copyright (C) 2021 André Guerreiro <aguerreiro1985@gmail.com>
 * Copyright (C) 2021, 2023 Marek Kasik <mkasik@redhat.com>
 * Copyright (C) 2023, 2024 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
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

#include <memory>

#include "poppler.h"
#include "poppler-private.h"

#include <CertificateInfo.h>
#ifdef ENABLE_NSS3
#    include <NSSCryptoSignBackend.h>
#endif
#include <CryptoSignBackend.h>

/**
 * SECTION:poppler-form-field
 * @short_description: Form Field
 * @title: PopplerFormField
 */

typedef struct _PopplerFormFieldClass PopplerFormFieldClass;
struct _PopplerFormFieldClass
{
    GObjectClass parent_class;
};

G_DEFINE_TYPE(PopplerFormField, poppler_form_field, G_TYPE_OBJECT)

static void poppler_form_field_finalize(GObject *object)
{
    PopplerFormField *field = POPPLER_FORM_FIELD(object);

    if (field->document) {
        g_object_unref(field->document);
        field->document = nullptr;
    }
    if (field->action) {
        poppler_action_free(field->action);
        field->action = nullptr;
    }
    field->widget = nullptr;

    G_OBJECT_CLASS(poppler_form_field_parent_class)->finalize(object);
}

static void poppler_form_field_init(PopplerFormField *field) { }

static void poppler_form_field_class_init(PopplerFormFieldClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->finalize = poppler_form_field_finalize;
}

PopplerFormField *_poppler_form_field_new(PopplerDocument *document, FormWidget *field)
{
    PopplerFormField *poppler_field;

    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), NULL);
    g_return_val_if_fail(field != nullptr, NULL);

    poppler_field = POPPLER_FORM_FIELD(g_object_new(POPPLER_TYPE_FORM_FIELD, nullptr));

    poppler_field->document = (PopplerDocument *)g_object_ref(document);
    poppler_field->widget = field;

    return poppler_field;
}

/* Public methods */
/**
 * poppler_form_field_get_field_type:
 * @field: a #PopplerFormField
 *
 * Gets the type of @field
 *
 * Return value: #PopplerFormFieldType of @field
 **/
PopplerFormFieldType poppler_form_field_get_field_type(PopplerFormField *field)
{
    g_return_val_if_fail(POPPLER_IS_FORM_FIELD(field), POPPLER_FORM_FIELD_UNKNOWN);

    switch (field->widget->getType()) {
    case formButton:
        return POPPLER_FORM_FIELD_BUTTON;
    case formText:
        return POPPLER_FORM_FIELD_TEXT;
    case formChoice:
        return POPPLER_FORM_FIELD_CHOICE;
    case formSignature:
        return POPPLER_FORM_FIELD_SIGNATURE;
    default:
        g_warning("Unsupported Form Field Type");
    }

    return POPPLER_FORM_FIELD_UNKNOWN;
}

/**
 * poppler_form_field_get_id:
 * @field: a #PopplerFormField
 *
 * Gets the id of @field
 *
 * Return value: the id of @field
 **/
gint poppler_form_field_get_id(PopplerFormField *field)
{
    g_return_val_if_fail(POPPLER_IS_FORM_FIELD(field), -1);

    return field->widget->getID();
}

/**
 * poppler_form_field_get_font_size:
 * @field: a #PopplerFormField
 *
 * Gets the font size of @field
 *
 * WARNING: This function always returns 0. Contact the poppler
 * mailing list if you're interested in implementing it properly
 *
 * Return value: the font size of @field
 **/
gdouble poppler_form_field_get_font_size(PopplerFormField *field)
{
    g_return_val_if_fail(POPPLER_IS_FORM_FIELD(field), 0);

    return 0;
}

/**
 * poppler_form_field_is_read_only:
 * @field: a #PopplerFormField
 *
 * Checks whether @field is read only
 *
 * Return value: %TRUE if @field is read only
 **/
gboolean poppler_form_field_is_read_only(PopplerFormField *field)
{
    g_return_val_if_fail(POPPLER_IS_FORM_FIELD(field), FALSE);

    return field->widget->isReadOnly();
}

/**
 * poppler_form_field_get_action:
 * @field: a #PopplerFormField
 *
 * Retrieves the action (#PopplerAction) that shall be
 * performed when @field is activated, or %NULL
 *
 * Return value: (transfer none): the action to perform. The returned
 *               object is owned by @field and should not be freed
 *
 * Since: 0.18
 */
PopplerAction *poppler_form_field_get_action(PopplerFormField *field)
{
    LinkAction *action;

    if (field->action) {
        return field->action;
    }

    action = field->widget->getActivationAction();
    if (!action) {
        return nullptr;
    }

    field->action = _poppler_action_new(field->document, action, nullptr);

    return field->action;
}

/**
 * poppler_form_field_get_additional_action:
 * @field: a #PopplerFormField
 * @type: the type of additional action
 *
 * Retrieves the action (#PopplerAction) that shall be performed when
 * an additional action is triggered on @field, or %NULL.
 *
 * Return value: (transfer none): the action to perform. The returned
 *               object is owned by @field and should not be freed.
 *
 *
 * Since: 0.72
 */
PopplerAction *poppler_form_field_get_additional_action(PopplerFormField *field, PopplerAdditionalActionType type)
{
    Annot::FormAdditionalActionsType form_action;
    PopplerAction **action;

    switch (type) {
    case POPPLER_ADDITIONAL_ACTION_FIELD_MODIFIED:
        form_action = Annot::actionFieldModified;
        action = &field->field_modified_action;
        break;
    case POPPLER_ADDITIONAL_ACTION_FORMAT_FIELD:
        form_action = Annot::actionFormatField;
        action = &field->format_field_action;
        break;
    case POPPLER_ADDITIONAL_ACTION_VALIDATE_FIELD:
        form_action = Annot::actionValidateField;
        action = &field->validate_field_action;
        break;
    case POPPLER_ADDITIONAL_ACTION_CALCULATE_FIELD:
        form_action = Annot::actionCalculateField;
        action = &field->calculate_field_action;
        break;
    default:
        g_return_val_if_reached(nullptr);
        return nullptr;
    }

    if (*action) {
        return *action;
    }

    std::unique_ptr<LinkAction> link_action = field->widget->getAdditionalAction(form_action);
    if (!link_action) {
        return nullptr;
    }

    *action = _poppler_action_new(nullptr, link_action.get(), nullptr);

    return *action;
}

/* Button Field */
/**
 * poppler_form_field_button_get_button_type:
 * @field: a #PopplerFormField
 *
 * Gets the button type of @field
 *
 * Return value: #PopplerFormButtonType of @field
 **/
PopplerFormButtonType poppler_form_field_button_get_button_type(PopplerFormField *field)
{
    g_return_val_if_fail(field->widget->getType() == formButton, POPPLER_FORM_BUTTON_PUSH);

    switch (static_cast<FormWidgetButton *>(field->widget)->getButtonType()) {
    case formButtonPush:
        return POPPLER_FORM_BUTTON_PUSH;
    case formButtonCheck:
        return POPPLER_FORM_BUTTON_CHECK;
    case formButtonRadio:
        return POPPLER_FORM_BUTTON_RADIO;
    default:
        g_assert_not_reached();
    }
}

/**
 * poppler_form_field_button_get_state:
 * @field: a #PopplerFormField
 *
 * Queries a #PopplerFormField and returns its current state. Returns %TRUE if
 * @field is pressed in and %FALSE if it is raised.
 *
 * Return value: current state of @field
 **/
gboolean poppler_form_field_button_get_state(PopplerFormField *field)
{
    g_return_val_if_fail(field->widget->getType() == formButton, FALSE);

    return static_cast<FormWidgetButton *>(field->widget)->getState();
}

/**
 * poppler_form_field_button_set_state:
 * @field: a #PopplerFormField
 * @state: %TRUE or %FALSE
 *
 * Sets the status of @field. Set to %TRUE if you want the #PopplerFormField
 * to be 'pressed in', and %FALSE to raise it.
 **/
void poppler_form_field_button_set_state(PopplerFormField *field, gboolean state)
{
    g_return_if_fail(field->widget->getType() == formButton);

    static_cast<FormWidgetButton *>(field->widget)->setState((bool)state);
}

/**
 * poppler_form_field_get_partial_name:
 * @field: a #PopplerFormField
 *
 * Gets the partial name of @field.
 *
 * Return value: a new allocated string. It must be freed with g_free() when done.
 *
 * Since: 0.16
 **/
gchar *poppler_form_field_get_partial_name(PopplerFormField *field)
{
    const GooString *tmp;

    g_return_val_if_fail(POPPLER_IS_FORM_FIELD(field), NULL);

    tmp = field->widget->getPartialName();

    return tmp ? _poppler_goo_string_to_utf8(tmp) : nullptr;
}

/**
 * poppler_form_field_get_mapping_name:
 * @field: a #PopplerFormField
 *
 * Gets the mapping name of @field that is used when
 * exporting interactive form field data from the document
 *
 * Return value: a new allocated string. It must be freed with g_free() when done.
 *
 * Since: 0.16
 **/
gchar *poppler_form_field_get_mapping_name(PopplerFormField *field)
{
    const GooString *tmp;

    g_return_val_if_fail(POPPLER_IS_FORM_FIELD(field), NULL);

    tmp = field->widget->getMappingName();

    return tmp ? _poppler_goo_string_to_utf8(tmp) : nullptr;
}

/**
 * poppler_form_field_get_name:
 * @field: a #PopplerFormField
 *
 * Gets the fully qualified name of @field. It's constructed by concatenating
 * the partial field names of the field and all of its ancestors.
 *
 * Return value: a new allocated string. It must be freed with g_free() when done.
 *
 * Since: 0.16
 **/
gchar *poppler_form_field_get_name(PopplerFormField *field)
{
    GooString *tmp;

    g_return_val_if_fail(POPPLER_IS_FORM_FIELD(field), NULL);

    tmp = field->widget->getFullyQualifiedName();

    return tmp ? _poppler_goo_string_to_utf8(tmp) : nullptr;
}

/**
 * poppler_form_field_get_alternate_ui_name:
 * @field: a #PopplerFormField
 *
 * Gets the alternate ui name of @field. This name is also commonly
 * used by pdf producers/readers to show it as a tooltip when @field area
 * is hovered by a pointing device (eg. mouse).
 *
 * Return value: a new allocated string. It must be freed with g_free() when done.
 *
 * Since: 0.88
 **/
gchar *poppler_form_field_get_alternate_ui_name(PopplerFormField *field)
{
    const GooString *tmp;

    g_return_val_if_fail(POPPLER_IS_FORM_FIELD(field), NULL);

    tmp = field->widget->getAlternateUiName();

    return tmp ? _poppler_goo_string_to_utf8(tmp) : nullptr;
}

/**
 * PopplerCertificateInfo:
 *
 * PopplerCertificateInfo contains detailed info about a signing certificate.
 *
 * Since: 23.07.0
 */
struct _PopplerCertificateInfo
{
    char *id;
    char *subject_common_name;
    char *subject_organization;
    char *subject_email;
    char *issuer_common_name;
    char *issuer_organization;
    char *issuer_email;
    GDateTime *issued;
    GDateTime *expires;
};

typedef struct _PopplerCertificateInfo PopplerCertificateInfo;

G_DEFINE_BOXED_TYPE(PopplerCertificateInfo, poppler_certificate_info, poppler_certificate_info_copy, poppler_certificate_info_free)

/**
 * PopplerSignatureInfo:
 *
 * PopplerSignatureInfo contains detailed info about a signature
 * contained in a form field.
 *
 * Since: 21.12.0
 **/
struct _PopplerSignatureInfo
{
    PopplerSignatureStatus sig_status;
    PopplerCertificateStatus cert_status;
    char *signer_name;
    GDateTime *local_signing_time;
    PopplerCertificateInfo *certificate_info;
};

static PopplerSignatureInfo *_poppler_form_field_signature_validate(PopplerFormField *field, PopplerSignatureValidationFlags flags, gboolean force_revalidation, GError **error)
{
    FormFieldSignature *sig_field;
    SignatureInfo *sig_info;
    PopplerSignatureInfo *poppler_sig_info;
    const X509CertificateInfo *certificate_info;

    if (poppler_form_field_get_field_type(field) != POPPLER_FORM_FIELD_SIGNATURE) {
        g_set_error(error, POPPLER_ERROR, POPPLER_ERROR_INVALID, "Wrong FormField type");
        return nullptr;
    }

    sig_field = static_cast<FormFieldSignature *>(field->widget->getField());

    sig_info = sig_field->validateSignatureAsync(flags & POPPLER_SIGNATURE_VALIDATION_FLAG_VALIDATE_CERTIFICATE, force_revalidation, -1, flags & POPPLER_SIGNATURE_VALIDATION_FLAG_WITHOUT_OCSP_REVOCATION_CHECK,
                                                 flags & POPPLER_SIGNATURE_VALIDATION_FLAG_USE_AIA_CERTIFICATE_FETCH, {});
    CertificateValidationStatus certificateStatus = sig_field->validateSignatureResult();

    poppler_sig_info = g_new0(PopplerSignatureInfo, 1);
    switch (sig_info->getSignatureValStatus()) {
    case SIGNATURE_VALID:
        poppler_sig_info->sig_status = POPPLER_SIGNATURE_VALID;
        break;
    case SIGNATURE_INVALID:
        poppler_sig_info->sig_status = POPPLER_SIGNATURE_INVALID;
        break;
    case SIGNATURE_DIGEST_MISMATCH:
        poppler_sig_info->sig_status = POPPLER_SIGNATURE_DIGEST_MISMATCH;
        break;
    case SIGNATURE_DECODING_ERROR:
        poppler_sig_info->sig_status = POPPLER_SIGNATURE_DECODING_ERROR;
        break;
    case SIGNATURE_GENERIC_ERROR:
        poppler_sig_info->sig_status = POPPLER_SIGNATURE_GENERIC_ERROR;
        break;
    case SIGNATURE_NOT_FOUND:
        poppler_sig_info->sig_status = POPPLER_SIGNATURE_NOT_FOUND;
        break;
    case SIGNATURE_NOT_VERIFIED:
        poppler_sig_info->sig_status = POPPLER_SIGNATURE_NOT_VERIFIED;
        break;
    }

    switch (certificateStatus) {
    case CERTIFICATE_TRUSTED:
        poppler_sig_info->cert_status = POPPLER_CERTIFICATE_TRUSTED;
        break;
    case CERTIFICATE_UNTRUSTED_ISSUER:
        poppler_sig_info->cert_status = POPPLER_CERTIFICATE_UNTRUSTED_ISSUER;
        break;
    case CERTIFICATE_UNKNOWN_ISSUER:
        poppler_sig_info->cert_status = POPPLER_CERTIFICATE_UNKNOWN_ISSUER;
        break;
    case CERTIFICATE_REVOKED:
        poppler_sig_info->cert_status = POPPLER_CERTIFICATE_REVOKED;
        break;
    case CERTIFICATE_EXPIRED:
        poppler_sig_info->cert_status = POPPLER_CERTIFICATE_EXPIRED;
        break;
    case CERTIFICATE_GENERIC_ERROR:
        poppler_sig_info->cert_status = POPPLER_CERTIFICATE_GENERIC_ERROR;
        break;
    case CERTIFICATE_NOT_VERIFIED:
        poppler_sig_info->cert_status = POPPLER_CERTIFICATE_NOT_VERIFIED;
        break;
    }

    std::string signerName = sig_info->getSignerName();
    poppler_sig_info->signer_name = g_strdup(signerName.c_str());
    poppler_sig_info->local_signing_time = g_date_time_new_from_unix_local(sig_info->getSigningTime());

    certificate_info = sig_info->getCertificateInfo();
    if (certificate_info != nullptr) {
        const X509CertificateInfo::EntityInfo &subject_info = certificate_info->getSubjectInfo();
        const X509CertificateInfo::EntityInfo &issuer_info = certificate_info->getIssuerInfo();
        const X509CertificateInfo::Validity &validity = certificate_info->getValidity();

        poppler_sig_info->certificate_info = poppler_certificate_info_new();
        poppler_sig_info->certificate_info->subject_common_name = g_strdup(subject_info.commonName.c_str());
        poppler_sig_info->certificate_info->subject_organization = g_strdup(subject_info.organization.c_str());
        poppler_sig_info->certificate_info->subject_email = g_strdup(subject_info.email.c_str());
        poppler_sig_info->certificate_info->issuer_common_name = g_strdup(issuer_info.commonName.c_str());
        poppler_sig_info->certificate_info->issuer_email = g_strdup(issuer_info.email.c_str());
        poppler_sig_info->certificate_info->issuer_organization = g_strdup(issuer_info.organization.c_str());
        poppler_sig_info->certificate_info->issued = g_date_time_new_from_unix_utc(validity.notBefore);
        poppler_sig_info->certificate_info->expires = g_date_time_new_from_unix_utc(validity.notAfter);
    }

    return poppler_sig_info;
}

static void signature_validate_thread(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
    PopplerSignatureValidationFlags flags = (PopplerSignatureValidationFlags)GPOINTER_TO_INT(task_data);
    PopplerSignatureInfo *signature_info;
    PopplerFormField *field = (PopplerFormField *)source_object;
    GError *error = nullptr;

    signature_info = _poppler_form_field_signature_validate(field, flags, FALSE, &error);
    if (signature_info == nullptr && error != nullptr) {
        g_task_return_error(task, error);
        return;
    }

    if (g_task_set_return_on_cancel(task, FALSE)) {
        g_task_return_pointer(task, signature_info, (GDestroyNotify)poppler_signature_info_free);
    }
}

/**
 * poppler_form_field_signature_validate_sync:
 * @field: a #PopplerFormField that represents a signature annotation
 * @flags: #PopplerSignatureValidationFlags flags influencing process of validation of the field signature
 * @cancellable: (nullable): optional #GCancellable object
 * @error: a #GError
 *
 * Synchronously validates the cryptographic signature contained in @signature_field.
 *
 * Return value: (transfer full): a #PopplerSignatureInfo structure containing signature metadata and validation status
 *                                Free the returned structure with poppler_signature_info_free().
 *
 * Since: 21.12.0
 **/
PopplerSignatureInfo *poppler_form_field_signature_validate_sync(PopplerFormField *field, PopplerSignatureValidationFlags flags, GCancellable *cancellable, GError **error)
{
    PopplerSignatureInfo *signature_info;
    GTask *task;

    g_return_val_if_fail(error == NULL || *error == NULL, NULL);

    task = g_task_new(field, cancellable, nullptr, nullptr);
    g_task_set_task_data(task, GINT_TO_POINTER(flags), nullptr);
    g_task_set_return_on_cancel(task, TRUE);

    g_task_run_in_thread_sync(task, signature_validate_thread);

    signature_info = (PopplerSignatureInfo *)g_task_propagate_pointer(task, error);
    g_object_unref(task);

    return signature_info;
}

/**
 * poppler_form_field_signature_validate_async:
 * @field: a #PopplerFormField that represents a signature annotation
 * @flags: #PopplerSignatureValidationFlags flags influencing process of validation of the field signature
 * @cancellable: (nullable): optional #GCancellable object
 * @callback: (scope async): a #GAsyncReadyCallback to call when the signature is validated
 * @user_data: (closure): the data to pass to callback function
 *
 * Asynchronously validates the cryptographic signature contained in @signature_field.
 *
 * Since: 21.12.0
 **/
void poppler_form_field_signature_validate_async(PopplerFormField *field, PopplerSignatureValidationFlags flags, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    GTask *task;

    task = g_task_new(field, cancellable, callback, user_data);
    g_task_set_task_data(task, GINT_TO_POINTER(flags), nullptr);
    g_task_set_return_on_cancel(task, TRUE);

    g_task_run_in_thread(task, signature_validate_thread);

    g_object_unref(task);
}

/**
 * poppler_form_field_signature_validate_finish:
 * @field: a #PopplerFormField that represents a signature annotation
 * @result: a #GAsyncResult
 * @error: a #GError
 *
 * Finishes validation of the cryptographic signature contained in @signature_field.
 * See poppler_form_field_signature_validate_async().
 *
 * Return value: (transfer full): a #PopplerSignatureInfo structure containing signature metadata and validation status
 *                                Free the returned structure with poppler_signature_info_free().
 *
 * Since: 21.12.0
 **/
PopplerSignatureInfo *poppler_form_field_signature_validate_finish(PopplerFormField *field, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail(g_task_is_valid(result, field), NULL);

    return (PopplerSignatureInfo *)g_task_propagate_pointer(G_TASK(result), error);
}

G_DEFINE_BOXED_TYPE(PopplerSignatureInfo, poppler_signature_info, poppler_signature_info_copy, poppler_signature_info_free)

/**
 * poppler_signature_info_copy:
 * @siginfo: a #PopplerSignatureInfo structure containing signature metadata and validation status
 *
 * Copies @siginfo, creating an identical #PopplerSignatureInfo.
 *
 * Return value: (transfer full): a new #PopplerSignatureInfo structure identical to @siginfo
 *
 * Since: 21.12.0
 **/
PopplerSignatureInfo *poppler_signature_info_copy(const PopplerSignatureInfo *siginfo)
{
    PopplerSignatureInfo *new_info;

    g_return_val_if_fail(siginfo != NULL, NULL);

    new_info = g_new(PopplerSignatureInfo, 1);
    new_info->sig_status = siginfo->sig_status;
    new_info->cert_status = siginfo->cert_status;
    new_info->signer_name = g_strdup(siginfo->signer_name);
    new_info->local_signing_time = g_date_time_ref(siginfo->local_signing_time);
    new_info->certificate_info = poppler_certificate_info_copy(siginfo->certificate_info);

    return new_info;
}

/**
 * poppler_signature_info_free:
 * @siginfo: a #PopplerSignatureInfo structure containing signature metadata and validation status
 *
 * Frees @siginfo
 *
 * Since: 21.12.0
 **/
void poppler_signature_info_free(PopplerSignatureInfo *siginfo)
{
    if (siginfo == nullptr) {
        return;
    }

    g_date_time_unref(siginfo->local_signing_time);
    g_free(siginfo->signer_name);
    poppler_certificate_info_free(siginfo->certificate_info);
    g_free(siginfo);
}

/**
 * poppler_signature_info_get_signature_status:
 * @siginfo: a #PopplerSignatureInfo
 *
 * Returns status of the signature for given PopplerSignatureInfo.
 *
 * Return value: signature status of the signature
 *
 * Since: 21.12.0
 **/
PopplerSignatureStatus poppler_signature_info_get_signature_status(const PopplerSignatureInfo *siginfo)
{
    g_return_val_if_fail(siginfo != NULL, POPPLER_SIGNATURE_GENERIC_ERROR);

    return siginfo->sig_status;
}

/**
 * poppler_signature_info_get_certificate_info:
 * @siginfo: a #PopplerSignatureInfo
 *
 * Returns PopplerCertificateInfo for given PopplerSignatureInfo.
 *
 * Return value: (transfer none): certificate info of the signature
 *
 * Since: 23.08.0
 **/
PopplerCertificateInfo *poppler_signature_info_get_certificate_info(const PopplerSignatureInfo *siginfo)
{
    g_return_val_if_fail(siginfo != NULL, NULL);

    return siginfo->certificate_info;
}

/**
 * poppler_signature_info_get_certificate_status:
 * @siginfo: a #PopplerSignatureInfo
 *
 * Returns status of the certificate for given PopplerSignatureInfo.
 *
 * Return value: certificate status of the signature
 *
 * Since: 21.12.0
 **/
PopplerCertificateStatus poppler_signature_info_get_certificate_status(const PopplerSignatureInfo *siginfo)
{
    g_return_val_if_fail(siginfo != NULL, POPPLER_CERTIFICATE_GENERIC_ERROR);

    return siginfo->cert_status;
}

/**
 * poppler_signature_info_get_signer_name:
 * @siginfo: a #PopplerSignatureInfo
 *
 * Returns name of signer for given PopplerSignatureInfo.
 *
 * Return value: (transfer none): A string.
 *
 * Since: 21.12.0
 **/
const gchar *poppler_signature_info_get_signer_name(const PopplerSignatureInfo *siginfo)
{
    g_return_val_if_fail(siginfo != NULL, NULL);

    return siginfo->signer_name;
}

/**
 * poppler_signature_info_get_local_signing_time:
 * @siginfo: a #PopplerSignatureInfo
 *
 * Returns local time of signing as GDateTime. This does not
 * contain information about time zone since it has not been
 * preserved during conversion.
 * Do not modify returned value since it is internal to
 * PopplerSignatureInfo.
 *
 * Return value: (transfer none): GDateTime
 *
 * Since: 21.12.0
 **/
GDateTime *poppler_signature_info_get_local_signing_time(const PopplerSignatureInfo *siginfo)
{
    g_return_val_if_fail(siginfo != NULL, NULL);

    return siginfo->local_signing_time;
}

/* Text Field */
/**
 * poppler_form_field_text_get_text_type:
 * @field: a #PopplerFormField
 *
 * Gets the text type of @field.
 *
 * Return value: #PopplerFormTextType of @field
 **/
PopplerFormTextType poppler_form_field_text_get_text_type(PopplerFormField *field)
{
    FormWidgetText *text_field;

    g_return_val_if_fail(field->widget->getType() == formText, POPPLER_FORM_TEXT_NORMAL);

    text_field = static_cast<FormWidgetText *>(field->widget);

    if (text_field->isMultiline()) {
        return POPPLER_FORM_TEXT_MULTILINE;
    } else if (text_field->isFileSelect()) {
        return POPPLER_FORM_TEXT_FILE_SELECT;
    }

    return POPPLER_FORM_TEXT_NORMAL;
}

/**
 * poppler_form_field_text_get_text:
 * @field: a #PopplerFormField
 *
 * Retrieves the contents of @field.
 *
 * Return value: a new allocated string. It must be freed with g_free() when done.
 **/
gchar *poppler_form_field_text_get_text(PopplerFormField *field)
{
    FormWidgetText *text_field;
    const GooString *tmp;

    g_return_val_if_fail(field->widget->getType() == formText, NULL);

    text_field = static_cast<FormWidgetText *>(field->widget);
    tmp = text_field->getContent();

    return tmp ? _poppler_goo_string_to_utf8(tmp) : nullptr;
}

/**
 * poppler_form_field_text_set_text:
 * @field: a #PopplerFormField
 * @text: the new text
 *
 * Sets the text in @field to the given value, replacing the current contents.
 **/
void poppler_form_field_text_set_text(PopplerFormField *field, const gchar *text)
{
    GooString *goo_tmp;
    gchar *tmp;
    gsize length = 0;

    g_return_if_fail(field->widget->getType() == formText);

    tmp = text ? g_convert(text, -1, "UTF-16BE", "UTF-8", nullptr, &length, nullptr) : nullptr;
    goo_tmp = new GooString(tmp, length);
    g_free(tmp);
    static_cast<FormWidgetText *>(field->widget)->setContent(goo_tmp);
    delete goo_tmp;
}

/**
 * poppler_form_field_text_get_max_len:
 * @field: a #PopplerFormField
 *
 * Retrieves the maximum allowed length of the text in @field
 *
 * Return value: the maximum allowed number of characters in @field, or -1 if there is no maximum.
 **/
gint poppler_form_field_text_get_max_len(PopplerFormField *field)
{
    g_return_val_if_fail(field->widget->getType() == formText, 0);

    return static_cast<FormWidgetText *>(field->widget)->getMaxLen();
}

/**
 * poppler_form_field_text_do_spell_check:
 * @field: a #PopplerFormField
 *
 * Checks whether spell checking should be done for the contents of @field
 *
 * Return value: %TRUE if spell checking should be done for @field
 **/
gboolean poppler_form_field_text_do_spell_check(PopplerFormField *field)
{
    g_return_val_if_fail(field->widget->getType() == formText, FALSE);

    return !static_cast<FormWidgetText *>(field->widget)->noSpellCheck();
}

gboolean poppler_form_field_text_do_scroll(PopplerFormField *field)
{
    g_return_val_if_fail(field->widget->getType() == formText, FALSE);

    return !static_cast<FormWidgetText *>(field->widget)->noScroll();
}

/**
 * poppler_form_field_text_is_rich_text:
 * @field: a #PopplerFormField
 *
 * Checks whether the contents of @field are rich text
 *
 * Return value: %TRUE if the contents of @field are rich text
 **/
gboolean poppler_form_field_text_is_rich_text(PopplerFormField *field)
{
    g_return_val_if_fail(field->widget->getType() == formText, FALSE);

    return static_cast<FormWidgetText *>(field->widget)->isRichText();
}

/**
 * poppler_form_field_text_is_password:
 * @field: a #PopplerFormField
 *
 * Checks whether content of @field is a password and it must be hidden
 *
 * Return value: %TRUE if the content of @field is a password
 **/
gboolean poppler_form_field_text_is_password(PopplerFormField *field)
{
    g_return_val_if_fail(field->widget->getType() == formText, FALSE);

    return static_cast<FormWidgetText *>(field->widget)->isPassword();
}

/* Choice Field */
/**
 * poppler_form_field_choice_get_choice_type:
 * @field: a #PopplerFormField
 *
 * Gets the choice type of @field
 *
 * Return value: #PopplerFormChoiceType of @field
 **/
PopplerFormChoiceType poppler_form_field_choice_get_choice_type(PopplerFormField *field)
{
    g_return_val_if_fail(field->widget->getType() == formChoice, POPPLER_FORM_CHOICE_COMBO);

    if (static_cast<FormWidgetChoice *>(field->widget)->isCombo()) {
        return POPPLER_FORM_CHOICE_COMBO;
    } else {
        return POPPLER_FORM_CHOICE_LIST;
    }
}

/**
 * poppler_form_field_choice_is_editable:
 * @field: a #PopplerFormField
 *
 * Checks whether @field is editable
 *
 * Return value: %TRUE if @field is editable
 **/
gboolean poppler_form_field_choice_is_editable(PopplerFormField *field)
{
    g_return_val_if_fail(field->widget->getType() == formChoice, FALSE);

    return static_cast<FormWidgetChoice *>(field->widget)->hasEdit();
}

/**
 * poppler_form_field_choice_can_select_multiple:
 * @field: a #PopplerFormField
 *
 * Checks whether @field allows multiple choices to be selected
 *
 * Return value: %TRUE if @field allows multiple choices to be selected
 **/
gboolean poppler_form_field_choice_can_select_multiple(PopplerFormField *field)
{
    g_return_val_if_fail(field->widget->getType() == formChoice, FALSE);

    return static_cast<FormWidgetChoice *>(field->widget)->isMultiSelect();
}

/**
 * poppler_form_field_choice_do_spell_check:
 * @field: a #PopplerFormField
 *
 * Checks whether spell checking should be done for the contents of @field
 *
 * Return value: %TRUE if spell checking should be done for @field
 **/
gboolean poppler_form_field_choice_do_spell_check(PopplerFormField *field)
{
    g_return_val_if_fail(field->widget->getType() == formChoice, FALSE);

    return !static_cast<FormWidgetChoice *>(field->widget)->noSpellCheck();
}

gboolean poppler_form_field_choice_commit_on_change(PopplerFormField *field)
{
    g_return_val_if_fail(field->widget->getType() == formChoice, FALSE);

    return static_cast<FormWidgetChoice *>(field->widget)->commitOnSelChange();
}

/**
 * poppler_form_field_choice_get_n_items:
 * @field: a #PopplerFormField
 *
 * Returns the number of items on @field
 *
 * Return value: the number of items on @field
 **/
gint poppler_form_field_choice_get_n_items(PopplerFormField *field)
{
    g_return_val_if_fail(field->widget->getType() == formChoice, -1);

    return static_cast<FormWidgetChoice *>(field->widget)->getNumChoices();
}

/**
 * poppler_form_field_choice_get_item:
 * @field: a #PopplerFormField
 * @index: the index of the item
 *
 * Returns the contents of the item on @field at the given index
 *
 * Return value: a new allocated string. It must be freed with g_free() when done.
 **/
gchar *poppler_form_field_choice_get_item(PopplerFormField *field, gint index)
{
    const GooString *tmp;

    g_return_val_if_fail(field->widget->getType() == formChoice, NULL);
    g_return_val_if_fail(index >= 0 && index < poppler_form_field_choice_get_n_items(field), NULL);

    tmp = static_cast<FormWidgetChoice *>(field->widget)->getChoice(index);
    return tmp ? _poppler_goo_string_to_utf8(tmp) : nullptr;
}

/**
 * poppler_form_field_choice_is_item_selected:
 * @field: a #PopplerFormField
 * @index: the index of the item
 *
 * Checks whether the item at the given index on @field is currently selected
 *
 * Return value: %TRUE if item at @index is currently selected
 **/
gboolean poppler_form_field_choice_is_item_selected(PopplerFormField *field, gint index)
{
    g_return_val_if_fail(field->widget->getType() == formChoice, FALSE);
    g_return_val_if_fail(index >= 0 && index < poppler_form_field_choice_get_n_items(field), FALSE);

    return static_cast<FormWidgetChoice *>(field->widget)->isSelected(index);
}

/**
 * poppler_form_field_choice_select_item:
 * @field: a #PopplerFormField
 * @index: the index of the item
 *
 * Selects the item at the given index on @field
 **/
void poppler_form_field_choice_select_item(PopplerFormField *field, gint index)
{
    g_return_if_fail(field->widget->getType() == formChoice);
    g_return_if_fail(index >= 0 && index < poppler_form_field_choice_get_n_items(field));

    static_cast<FormWidgetChoice *>(field->widget)->select(index);
}

/**
 * poppler_form_field_choice_unselect_all:
 * @field: a #PopplerFormField
 *
 * Unselects all the items on @field
 **/
void poppler_form_field_choice_unselect_all(PopplerFormField *field)
{
    g_return_if_fail(field->widget->getType() == formChoice);

    static_cast<FormWidgetChoice *>(field->widget)->deselectAll();
}

/**
 * poppler_form_field_choice_toggle_item:
 * @field: a #PopplerFormField
 * @index: the index of the item
 *
 * Changes the state of the item at the given index
 **/
void poppler_form_field_choice_toggle_item(PopplerFormField *field, gint index)
{
    g_return_if_fail(field->widget->getType() == formChoice);
    g_return_if_fail(index >= 0 && index < poppler_form_field_choice_get_n_items(field));

    static_cast<FormWidgetChoice *>(field->widget)->toggle(index);
}

/**
 * poppler_form_field_choice_set_text:
 * @field: a #PopplerFormField
 * @text: the new text
 *
 * Sets the text in @field to the given value, replacing the current contents
 **/
void poppler_form_field_choice_set_text(PopplerFormField *field, const gchar *text)
{
    GooString *goo_tmp;
    gchar *tmp;
    gsize length = 0;

    g_return_if_fail(field->widget->getType() == formChoice);

    tmp = text ? g_convert(text, -1, "UTF-16BE", "UTF-8", nullptr, &length, nullptr) : nullptr;
    goo_tmp = new GooString(tmp, length);
    g_free(tmp);
    static_cast<FormWidgetChoice *>(field->widget)->setEditChoice(goo_tmp);
    delete goo_tmp;
}

/**
 * poppler_form_field_choice_get_text:
 * @field: a #PopplerFormField
 *
 * Retrieves the contents of @field.
 *
 * Return value: a new allocated string. It must be freed with g_free() when done.
 **/
gchar *poppler_form_field_choice_get_text(PopplerFormField *field)
{
    const GooString *tmp;

    g_return_val_if_fail(field->widget->getType() == formChoice, NULL);

    tmp = static_cast<FormWidgetChoice *>(field->widget)->getEditChoice();
    return tmp ? _poppler_goo_string_to_utf8(tmp) : nullptr;
}

/* Signing Data */

struct _PopplerSigningData
{
    char *destination_filename;
    PopplerCertificateInfo *certificate_info;
    int page;

    char *signature_text;
    char *signature_text_left;
    PopplerRectangle signature_rect;

    PopplerColor font_color;
    gdouble font_size;
    gdouble left_font_size;

    PopplerColor border_color;
    gdouble border_width;

    PopplerColor background_color;

    char *field_partial_name;
    char *reason;
    char *location;
    char *image_path;
    char *password;
    char *document_owner_password;
    char *document_user_password;
};

typedef struct _PopplerSigningData PopplerSigningData;

G_DEFINE_BOXED_TYPE(PopplerSigningData, poppler_signing_data, poppler_signing_data_copy, poppler_signing_data_free)

/**
 * poppler_signing_data_new:
 *
 * Creates a new #PopplerSigningData with default content.
 *
 * Return value: a new #PopplerSigningData. It must be freed with poppler_signing_data_free() when done.
 *
 * Since: 23.07.0
 **/
PopplerSigningData *poppler_signing_data_new(void)
{
    PopplerSigningData *data = (PopplerSigningData *)g_malloc0(sizeof(PopplerSigningData));

    data->password = g_strdup("");
    data->page = 0;

    data->font_size = 10.0;
    data->left_font_size = 20.0;
    data->border_width = 1.5;

    /* Grey background */
    auto background_color = PopplerColor();
    background_color.red = 0xEF;
    background_color.green = 0xEF;
    background_color.blue = 0xEF;
    poppler_signing_data_set_background_color(data, &background_color);

    /* Red border color */
    auto border_color = PopplerColor();
    border_color.red = 0xFF;
    border_color.green = 0x00;
    border_color.blue = 0x00;
    poppler_signing_data_set_border_color(data, &border_color);

    /* Red font color */
    auto font_color = PopplerColor();
    font_color.red = 0xFF;
    font_color.green = 0x00;
    border_color.blue = 0x00;
    poppler_signing_data_set_font_color(data, &font_color);

    return data;
}

/**
 * poppler_signing_data_copy:
 * @signing_data: a #PopplerSigningData structure containing signing data
 *
 * Copies @signing_data, creating an identical #PopplerSigningData.
 *
 * Return value: (transfer full): a new #PopplerSigningData structure identical to @signing_data
 *
 * Since: 23.07.0
 **/
PopplerSigningData *poppler_signing_data_copy(const PopplerSigningData *signing_data)
{
    PopplerSigningData *data;

    g_return_val_if_fail(signing_data != nullptr, nullptr);

    data = (PopplerSigningData *)g_malloc0(sizeof(PopplerSigningData));
    data->destination_filename = g_strdup(signing_data->destination_filename);
    data->certificate_info = poppler_certificate_info_copy(signing_data->certificate_info);
    data->page = signing_data->page;

    data->signature_text = g_strdup(signing_data->signature_text);
    data->signature_text_left = g_strdup(signing_data->signature_text_left);
    memcpy(&data->signature_rect, &signing_data->signature_rect, sizeof(PopplerRectangle));

    memcpy(&data->font_color, &signing_data->font_color, sizeof(PopplerColor));
    data->font_size = signing_data->font_size;
    data->left_font_size = signing_data->left_font_size;

    memcpy(&data->border_color, &signing_data->border_color, sizeof(PopplerColor));
    data->border_width = signing_data->border_width;

    memcpy(&data->background_color, &signing_data->background_color, sizeof(PopplerColor));

    data->field_partial_name = g_strdup(signing_data->field_partial_name);
    data->reason = g_strdup(signing_data->reason);
    data->location = g_strdup(signing_data->location);
    data->image_path = g_strdup(signing_data->image_path);
    data->password = g_strdup(signing_data->password);
    data->document_owner_password = g_strdup(signing_data->document_owner_password);
    data->document_user_password = g_strdup(signing_data->document_user_password);

    return data;
}

/**
 * poppler_signing_data_free:
 * @signing_data: (nullable): a #PopplerSigningData structure containing signing data
 *
 * Frees @signing_data
 *
 * Since: 23.07.0
 **/
void poppler_signing_data_free(PopplerSigningData *signing_data)
{
    if (!signing_data) {
        return;
    }

    g_clear_pointer(&signing_data->destination_filename, g_free);
    g_clear_pointer(&signing_data->certificate_info, poppler_certificate_info_free);
    g_clear_pointer(&signing_data->signature_text, g_free);
    g_clear_pointer(&signing_data->signature_text_left, g_free);
    g_clear_pointer(&signing_data->field_partial_name, g_free);
    g_clear_pointer(&signing_data->reason, g_free);
    g_clear_pointer(&signing_data->location, g_free);
    g_clear_pointer(&signing_data->image_path, g_free);

    if (signing_data->password) {
#ifdef HAVE_EXPLICIT_BZERO
        explicit_bzero(signing_data->password, strlen(signing_data->password));
#else
        memset(signing_data->password, 0, strlen(signing_data->password));
#endif
        g_clear_pointer(&signing_data->password, g_free);
    }

    if (signing_data->document_owner_password) {
#ifdef HAVE_EXPLICIT_BZERO
        explicit_bzero(signing_data->document_owner_password, strlen(signing_data->document_owner_password));
#else
        memset(signing_data->document_owner_password, 0, strlen(signing_data->document_owner_password));
#endif
        g_clear_pointer(&signing_data->document_owner_password, g_free);
    }

    if (signing_data->document_user_password) {
#ifdef HAVE_EXPLICIT_BZERO
        explicit_bzero(signing_data->document_user_password, strlen(signing_data->document_user_password));
#else
        memset(signing_data->document_user_password, 0, strlen(signing_data->document_user_password));
#endif
        g_clear_pointer(&signing_data->document_user_password, g_free);
    }

    g_free(signing_data);
}

/**
 * poppler_signing_data_set_destination_filename:
 * @signing_data: a #PopplerSigningData structure containing signing data
 * @filename: destination filename
 *
 * Set destination file name.
 *
 * Since: 23.07.0
 **/
void poppler_signing_data_set_destination_filename(PopplerSigningData *signing_data, const gchar *filename)
{
    g_return_if_fail(signing_data != nullptr);
    g_return_if_fail(filename != nullptr);

    if (signing_data->destination_filename == filename) {
        return;
    }

    g_clear_pointer(&signing_data->destination_filename, g_free);
    signing_data->destination_filename = g_strdup(filename);
}

/**
 * poppler_signing_data_get_destination_filename:
 * @signing_data: a #PopplerSigningData structure containing signing data
 *
 * Get destination file name.
 *
 * Return value: destination filename
 *
 * Since: 23.07.0
 **/
const gchar *poppler_signing_data_get_destination_filename(const PopplerSigningData *signing_data)
{
    g_return_val_if_fail(signing_data != nullptr, nullptr);

    return signing_data->destination_filename;
}

/**
 * poppler_signing_data_set_certificate_info:
 * @signing_data: a #PopplerSigningData structure containing signing data
 * @certificate_info: a #PopplerCertificateInfo
 *
 * Set certification information.
 *
 * Since: 23.07.0
 **/
void poppler_signing_data_set_certificate_info(PopplerSigningData *signing_data, const PopplerCertificateInfo *certificate_info)
{
    g_return_if_fail(signing_data != nullptr);
    g_return_if_fail(certificate_info != nullptr);

    if (signing_data->certificate_info == certificate_info) {
        return;
    }

    g_clear_pointer(&signing_data->certificate_info, poppler_certificate_info_free);
    signing_data->certificate_info = poppler_certificate_info_copy(certificate_info);
}

/**
 * poppler_signing_data_get_certificate_info:
 * @signing_data: a #PopplerSigningData structure containing signing data
 *
 * Get certification information.
 *
 * Return value: a #PopplerCertificateInfo
 *
 * Since: 23.07.0
 **/
const PopplerCertificateInfo *poppler_signing_data_get_certificate_info(const PopplerSigningData *signing_data)
{
    g_return_val_if_fail(signing_data != nullptr, nullptr);
    return signing_data->certificate_info;
}

/**
 * poppler_signing_data_set_page:
 * @signing_data: a #PopplerSigningData structure containing signing data
 * @page: a page number
 *
 * Set page (>=0).
 *
 * Since: 23.07.0
 **/
void poppler_signing_data_set_page(PopplerSigningData *signing_data, int page)
{
    g_return_if_fail(signing_data != nullptr);

    if (page < 0) {
        return;
    }

    signing_data->page = page;
}

/**
 * poppler_signing_data_get_page:
 * @signing_data: a #PopplerSigningData structure containing signing data
 *
 * Get page.
 *
 * Return value: page number
 *
 * Since: 23.07.0
 **/
int poppler_signing_data_get_page(const PopplerSigningData *signing_data)
{
    g_return_val_if_fail(signing_data != nullptr, 0);
    return signing_data->page;
}

/**
 * poppler_signing_data_set_signature_text:
 * @signing_data: a #PopplerSigningData structure containing signing data
 * @signature_text: text to show as main signature
 *
 * Set signature text.
 *
 * Since: 23.07.0
 **/
void poppler_signing_data_set_signature_text(PopplerSigningData *signing_data, const gchar *signature_text)
{
    g_return_if_fail(signing_data != nullptr);
    g_return_if_fail(signature_text != nullptr);

    if (signing_data->signature_text == signature_text) {
        return;
    }

    g_clear_pointer(&signing_data->signature_text, g_free);
    signing_data->signature_text = g_strdup(signature_text);
}

/**
 * poppler_signing_data_get_signature_text:
 * @signing_data: a #PopplerSigningData structure containing signing data
 *
 * Get signature text.
 *
 * Return value: signature text
 *
 * Since: 23.07.0
 **/
const gchar *poppler_signing_data_get_signature_text(const PopplerSigningData *signing_data)
{
    g_return_val_if_fail(signing_data != nullptr, nullptr);
    return signing_data->signature_text;
}

/**
 * poppler_signing_data_set_signature_text_left:
 * @signing_data: a #PopplerSigningData structure containing signing data
 * @signature_text_left: text to show as small left signature
 *
 * Set small signature text on the left hand.
 *
 * Since: 23.07.0
 **/
void poppler_signing_data_set_signature_text_left(PopplerSigningData *signing_data, const gchar *signature_text_left)
{
    g_return_if_fail(signing_data != nullptr);
    g_return_if_fail(signature_text_left != nullptr);

    if (signing_data->signature_text_left == signature_text_left) {
        return;
    }

    g_clear_pointer(&signing_data->signature_text_left, g_free);
    signing_data->signature_text_left = g_strdup(signature_text_left);
}

/**
 * poppler_signing_data_get_signature_text_left:
 * @signing_data: a #PopplerSigningData structure containing signing data
 *
 * Get signature text left.
 *
 * Return value: signature text left
 *
 * Since: 23.07.0
 **/
const gchar *poppler_signing_data_get_signature_text_left(const PopplerSigningData *signing_data)
{
    g_return_val_if_fail(signing_data != nullptr, nullptr);
    return signing_data->signature_text_left;
}

/**
 * poppler_signing_data_set_signature_rectangle:
 * @signing_data: a #PopplerSigningData structure containing signing data
 * @signature_rect: a #PopplerRectangle where signature should be shown
 *
 * Set signature rectangle.
 *
 * Since: 23.07.0
 **/
void poppler_signing_data_set_signature_rectangle(PopplerSigningData *signing_data, const PopplerRectangle *signature_rect)
{
    g_return_if_fail(signing_data != nullptr);
    g_return_if_fail(signature_rect != nullptr);

    memcpy(&signing_data->signature_rect, signature_rect, sizeof(PopplerRectangle));
}

/**
 * poppler_signing_data_get_signature_rectangle:
 * @signing_data: a #PopplerSigningData structure containing signing data
 *
 * Get signature rectangle.
 *
 * Return value: a #PopplerRectangle
 *
 * Since: 23.07.0
 **/
const PopplerRectangle *poppler_signing_data_get_signature_rectangle(const PopplerSigningData *signing_data)
{
    g_return_val_if_fail(signing_data != nullptr, nullptr);
    return &signing_data->signature_rect;
}

/**
 * poppler_signing_data_set_font_color:
 * @signing_data: a #PopplerSigningData structure containing signing data
 * @font_color: a #PopplerColor to be used as signature font color
 *
 * Set signature font color.
 *
 * Since: 23.07.0
 **/
void poppler_signing_data_set_font_color(PopplerSigningData *signing_data, const PopplerColor *font_color)
{
    g_return_if_fail(signing_data != nullptr);
    g_return_if_fail(font_color != nullptr);

    memcpy(&signing_data->font_color, font_color, sizeof(PopplerColor));
}

/**
 * poppler_signing_data_get_font_color:
 * @signing_data: a #PopplerSigningData structure containing signing data
 *
 * Get signature font color.
 *
 * Return value: a #PopplerColor
 *
 * Since: 23.07.0
 **/
const PopplerColor *poppler_signing_data_get_font_color(const PopplerSigningData *signing_data)
{
    g_return_val_if_fail(signing_data != nullptr, nullptr);
    return &signing_data->font_color;
}

/**
 * poppler_signing_data_set_font_size:
 * @signing_data: a #PopplerSigningData structure containing signing data
 * @font_size: signature font size
 *
 * Set signature font size (>0).
 *
 * Since: 23.07.0
 **/
void poppler_signing_data_set_font_size(PopplerSigningData *signing_data, gdouble font_size)
{
    g_return_if_fail(signing_data != nullptr);

    if (font_size <= 0) {
        return;
    }

    signing_data->font_size = font_size;
}

/**
 * poppler_signing_data_get_font_size:
 * @signing_data: a #PopplerSigningData structure containing signing data
 *
 * Get signature font size.
 *
 * Return value: font size
 *
 * Since: 23.07.0
 **/
gdouble poppler_signing_data_get_font_size(const PopplerSigningData *signing_data)
{
    g_return_val_if_fail(signing_data != nullptr, 20.0f);
    return signing_data->font_size;
}

/**
 * poppler_signing_data_set_left_font_size:
 * @signing_data: a #PopplerSigningData structure containing signing data
 * @font_size: signature font size
 *
 * Set signature left font size (> 0).
 *
 * Since: 23.07.0
 **/
void poppler_signing_data_set_left_font_size(PopplerSigningData *signing_data, gdouble left_font_size)
{
    g_return_if_fail(signing_data != nullptr);

    if (left_font_size <= 0) {
        return;
    }

    signing_data->left_font_size = left_font_size;
}

/**
 * poppler_signing_data_get_left_font_size:
 * @signing_data: a #PopplerSigningData structure containing signing data
 *
 * Get signature left font size.
 *
 * Return value: left font size
 *
 * Since: 23.07.0
 **/
gdouble poppler_signing_data_get_left_font_size(const PopplerSigningData *signing_data)
{
    g_return_val_if_fail(signing_data != nullptr, 12.0);
    return signing_data->left_font_size;
}

/**
 * poppler_signing_data_set_border_color:
 * @signing_data: a #PopplerSigningData structure containing signing data
 * @border_color: a #PopplerColor to be used for signature border
 *
 * Set signature border color.
 *
 * Since: 23.07.0
 **/
void poppler_signing_data_set_border_color(PopplerSigningData *signing_data, const PopplerColor *border_color)
{
    g_return_if_fail(signing_data != nullptr);
    g_return_if_fail(border_color != nullptr);

    memcpy(&signing_data->border_color, border_color, sizeof(PopplerColor));
}

/**
 * poppler_signing_data_get_border_color:
 * @signing_data: a #PopplerSigningData structure containing signing data
 *
 * Get signature border color.
 *
 * Return value: a #PopplerColor
 *
 * Since: 23.07.0
 **/
const PopplerColor *poppler_signing_data_get_border_color(const PopplerSigningData *signing_data)
{
    g_return_val_if_fail(signing_data != nullptr, nullptr);
    return &signing_data->border_color;
}

/**
 * poppler_signing_data_set_border_width:
 * @signing_data: a #PopplerSigningData structure containing signing data
 * @border_width: border width
 *
 * Set signature border width.
 *
 * Since: 23.07.0
 **/
void poppler_signing_data_set_border_width(PopplerSigningData *signing_data, gdouble border_width)
{
    g_return_if_fail(signing_data != nullptr);

    if (border_width < 0) {
        return;
    }

    signing_data->border_width = border_width;
}

/**
 * poppler_signing_data_get_border_width:
 * @signing_data: a #PopplerSigningData structure containing signing data
 *
 * Get signature border width.
 *
 * Return value: border width
 *
 * Since: 23.07.0
 **/
gdouble poppler_signing_data_get_border_width(const PopplerSigningData *signing_data)
{
    g_return_val_if_fail(signing_data != nullptr, 12);
    return signing_data->border_width;
}

/**
 * poppler_signing_data_set_background_color:
 * @signing_data: a #PopplerSigningData structure containing signing data
 * @background_color: a #PopplerColor to be used for signature background
 *
 * Set signature background color.
 *
 * Since: 23.07.0
 **/
void poppler_signing_data_set_background_color(PopplerSigningData *signing_data, const PopplerColor *background_color)
{
    g_return_if_fail(signing_data != nullptr);
    g_return_if_fail(background_color != nullptr);

    memcpy(&signing_data->background_color, background_color, sizeof(PopplerColor));
}

/**
 * poppler_signing_data_get_background_color:
 * @signing_data: a #PopplerSigningData structure containing signing data
 *
 * Get signature background color.
 *
 * Return value: a #PopplerColor
 *
 * Since: 23.07.0
 **/
const PopplerColor *poppler_signing_data_get_background_color(const PopplerSigningData *signing_data)
{
    g_return_val_if_fail(signing_data != nullptr, nullptr);
    return &signing_data->background_color;
}

/**
 * poppler_signing_data_set_field_partial_name:
 * @signing_data: a #PopplerSigningData structure containing signing data
 * @field_partial_name: a field partial name
 *
 * Set field partial name (existing field id or a new one) where signature is placed.
 *
 * Since: 23.07.0
 **/
void poppler_signing_data_set_field_partial_name(PopplerSigningData *signing_data, const gchar *field_partial_name)
{
    g_return_if_fail(signing_data != nullptr);
    g_return_if_fail(field_partial_name != nullptr);

    g_clear_pointer(&signing_data->field_partial_name, g_free);
    signing_data->field_partial_name = g_strdup(field_partial_name);
}

/**
 * poppler_signing_data_get_field_partial_name:
 * @signing_data: a #PopplerSigningData structure containing signing data
 *
 * Get field partial name.
 *
 * Return value: field partial name
 *
 * Since: 23.07.0
 **/
const gchar *poppler_signing_data_get_field_partial_name(const PopplerSigningData *signing_data)
{
    g_return_val_if_fail(signing_data != nullptr, "");
    return signing_data->field_partial_name;
}

/**
 * poppler_signing_data_set_reason:
 * @signing_data: a #PopplerSigningData structure containing signing data
 * @reason: a reason
 *
 * Set reason for signature (e.g. I'm approver).
 *
 * Since: 23.07.0
 **/
void poppler_signing_data_set_reason(PopplerSigningData *signing_data, const gchar *reason)
{
    g_return_if_fail(signing_data != nullptr);
    g_return_if_fail(reason != nullptr);

    if (signing_data->reason == reason) {
        return;
    }

    g_clear_pointer(&signing_data->reason, g_free);
    signing_data->reason = g_strdup(reason);
}

/**
 * poppler_signing_data_get_reason:
 * @signing_data: a #PopplerSigningData structure containing signing data
 *
 * Get reason.
 *
 * Return value: reason
 *
 * Since: 23.07.0
 **/
const gchar *poppler_signing_data_get_reason(const PopplerSigningData *signing_data)
{
    g_return_val_if_fail(signing_data != nullptr, nullptr);
    return signing_data->reason;
}

/**
 * poppler_signing_data_set_location:
 * @signing_data: a #PopplerSigningData structure containing signing data
 * @location: a location
 *
 * Set signature location (e.g. "At my desk").
 *
 * Since: 23.07.0
 **/
void poppler_signing_data_set_location(PopplerSigningData *signing_data, const gchar *location)
{
    g_return_if_fail(signing_data != nullptr);
    g_return_if_fail(location != nullptr);

    if (signing_data->location == location) {
        return;
    }

    g_clear_pointer(&signing_data->location, g_free);
    signing_data->location = g_strdup(location);
}

/**
 * poppler_signing_data_get_location:
 * @signing_data: a #PopplerSigningData structure containing signing data
 *
 * Get location.
 *
 * Return value: location
 *
 * Since: 23.07.0
 **/
const gchar *poppler_signing_data_get_location(const PopplerSigningData *signing_data)
{
    g_return_val_if_fail(signing_data != nullptr, nullptr);
    return signing_data->location;
}

/**
 * poppler_signing_data_set_image_path:
 * @signing_data: a #PopplerSigningData structure containing signing data
 * @image_path: signature image path
 *
 * Set signature background (watermark) image path.
 *
 * Since: 23.07.0
 **/
void poppler_signing_data_set_image_path(PopplerSigningData *signing_data, const gchar *image_path)
{
    g_return_if_fail(signing_data != nullptr);
    g_return_if_fail(image_path != nullptr);

    if (signing_data->image_path == image_path) {
        return;
    }

    g_clear_pointer(&signing_data->image_path, g_free);
    signing_data->image_path = g_strdup(image_path);
}

/**
 * poppler_signing_data_get_image_path:
 * @signing_data: a #PopplerSigningData structure containing signing data
 *
 * Get image path.
 *
 * Return value: image path
 *
 * Since: 23.07.0
 **/
const gchar *poppler_signing_data_get_image_path(const PopplerSigningData *signing_data)
{
    g_return_val_if_fail(signing_data != nullptr, nullptr);
    return signing_data->image_path;
}

/**
 * poppler_signing_data_set_password:
 * @signing_data: a #PopplerSigningData structure containing signing data
 * @password: a password
 *
 * Set password for the signing key.
 *
 * Since: 23.07.0
 **/
void poppler_signing_data_set_password(PopplerSigningData *signing_data, const gchar *password)
{
    g_return_if_fail(signing_data != nullptr);
    g_return_if_fail(password != nullptr);

    if (signing_data->password == password) {
        return;
    }

    g_clear_pointer(&signing_data->password, g_free);
    signing_data->password = g_strdup(password);
}

/**
 * poppler_signing_data_get_password:
 * @signing_data: a #PopplerSigningData structure containing signing data
 *
 * Get signing key password.
 *
 * Return value: password
 *
 * Since: 23.07.0
 **/
const gchar *poppler_signing_data_get_password(const PopplerSigningData *signing_data)
{
    g_return_val_if_fail(signing_data != nullptr, nullptr);
    return signing_data->password;
}

/**
 * poppler_signing_data_set_document_owner_password:
 * @signing_data: a #PopplerSigningData structure containing signing data
 * @document_owner_password: document owner password
 *
 * Set document owner password (for encrypted files).
 *
 * Since: 23.07.0
 **/
void poppler_signing_data_set_document_owner_password(PopplerSigningData *signing_data, const gchar *document_owner_password)
{
    g_return_if_fail(signing_data != nullptr);
    g_return_if_fail(document_owner_password != nullptr);

    if (signing_data->document_owner_password == document_owner_password) {
        return;
    }

    g_clear_pointer(&signing_data->document_owner_password, g_free);
    signing_data->document_owner_password = g_strdup(document_owner_password);
}

/**
 * poppler_signing_data_get_document_owner_password:
 * @signing_data: a #PopplerSigningData structure containing signing data
 *
 * Get document owner password.
 *
 * Return value: document owner password (for encrypted files)
 *
 * Since: 23.07.0
 **/
const gchar *poppler_signing_data_get_document_owner_password(const PopplerSigningData *signing_data)
{
    g_return_val_if_fail(signing_data != nullptr, nullptr);
    return signing_data->document_owner_password;
}

/**
 * poppler_signing_data_set_document_user_password:
 * @signing_data: a #PopplerSigningData structure containing signing data
 * @document_user_password: document user password
 *
 * Set document user password (for encrypted files).
 *
 * Since: 23.07.0
 **/
void poppler_signing_data_set_document_user_password(PopplerSigningData *signing_data, const gchar *document_user_password)
{
    g_return_if_fail(signing_data != nullptr);
    g_return_if_fail(document_user_password != nullptr);

    if (signing_data->document_user_password == document_user_password) {
        return;
    }

    g_clear_pointer(&signing_data->document_user_password, g_free);
    signing_data->document_user_password = g_strdup(document_user_password);
}

/**
 * poppler_signing_data_get_document_user_password:
 * @signing_data: a #PopplerSigningData structure containing signing data
 *
 * Get document user password.
 *
 * Return value: document user password (for encrypted files)
 *
 * Since: 23.07.0
 **/
const gchar *poppler_signing_data_get_document_user_password(const PopplerSigningData *signing_data)
{
    g_return_val_if_fail(signing_data != nullptr, "");
    return signing_data->document_user_password;
}

/* Certificate Information */

/**
 * poppler_certificate_info_new:
 *
 * Creates a new #PopplerCertificateInfo
 *
 * Return value: a new #PopplerCertificateInfo. It must be freed with poppler_certificate_info_free() when done.
 *
 * Since: 23.07.0
 **/
PopplerCertificateInfo *poppler_certificate_info_new(void)
{
    return (PopplerCertificateInfo *)g_malloc0(sizeof(PopplerCertificateInfo));
}

/**
 * poppler_certificate_info_get_id:
 * @certificate_info: a #PopplerCertificateInfo structure containing certificate information
 *
 * Get certificate nick name
 *
 * Return value: certificate nick name
 *
 * Since: 23.07.0
 **/
const char *poppler_certificate_info_get_id(const PopplerCertificateInfo *certificate_info)
{
    g_return_val_if_fail(certificate_info != nullptr, nullptr);
    return certificate_info->id;
}

/**
 * poppler_certificate_info_get_subject_common_name:
 * @certificate_info: a #PopplerCertificateInfo structure containing certificate information
 *
 * Get certificate subject common name
 *
 * Return value: certificate subject common name
 *
 * Since: 23.07.0
 **/
const char *poppler_certificate_info_get_subject_common_name(const PopplerCertificateInfo *certificate_info)
{
    g_return_val_if_fail(certificate_info != nullptr, nullptr);
    return certificate_info->subject_common_name;
}

/**
 * poppler_certificate_info_get_subject_organization:
 * @certificate_info: a #PopplerCertificateInfo structure containing certificate information
 *
 * Get certificate subject organization
 *
 * Return value: certificate subject organization
 *
 * Since: 23.08.0
 **/
const char *poppler_certificate_info_get_subject_organization(const PopplerCertificateInfo *certificate_info)
{
    g_return_val_if_fail(certificate_info != nullptr, nullptr);
    return certificate_info->subject_organization;
}

/**
 * poppler_certificate_info_get_subject_email:
 * @certificate_info: a #PopplerCertificateInfo structure containing certificate information
 *
 * Get certificate subject email
 *
 * Return value: certificate subject email
 *
 * Since: 23.08.0
 **/
const char *poppler_certificate_info_get_subject_email(const PopplerCertificateInfo *certificate_info)
{
    g_return_val_if_fail(certificate_info != nullptr, nullptr);
    return certificate_info->subject_email;
}

/**
 * poppler_certificate_info_get_issuer_common_name:
 * @certificate_info: a #PopplerCertificateInfo structure containing certificate information
 *
 * Get certificate issuer common name
 *
 * Return value: certificate issuer common name
 *
 * Since: 23.08.0
 **/
const char *poppler_certificate_info_get_issuer_common_name(const PopplerCertificateInfo *certificate_info)
{
    g_return_val_if_fail(certificate_info != nullptr, nullptr);
    return certificate_info->issuer_common_name;
}

/**
 * poppler_certificate_info_get_issuer_organization:
 * @certificate_info: a #PopplerCertificateInfo structure containing certificate information
 *
 * Get certificate issuer organization
 *
 * Return value: certificate issuer organization
 *
 * Since: 23.08.0
 **/
const char *poppler_certificate_info_get_issuer_organization(const PopplerCertificateInfo *certificate_info)
{
    g_return_val_if_fail(certificate_info != nullptr, nullptr);
    return certificate_info->issuer_organization;
}

/**
 * poppler_certificate_info_get_issuer_email:
 * @certificate_info: a #PopplerCertificateInfo structure containing certificate information
 *
 * Get certificate issuer email
 *
 * Return value: certificate issuer email
 *
 * Since: 23.08.0
 **/
const char *poppler_certificate_info_get_issuer_email(const PopplerCertificateInfo *certificate_info)
{
    g_return_val_if_fail(certificate_info != nullptr, nullptr);
    return certificate_info->issuer_email;
}

/**
 * poppler_certificate_info_get_issuance_time:
 * @certificate_info: a #PopplerCertificateInfo structure containing certificate information
 *
 * Get certificate issuance time
 *
 * Return value: (transfer none): certificate issuance time
 *
 * Since: 23.08.0
 **/
GDateTime *poppler_certificate_info_get_issuance_time(const PopplerCertificateInfo *certificate_info)
{
    g_return_val_if_fail(certificate_info != nullptr, nullptr);
    return certificate_info->issued;
}

/**
 * poppler_certificate_info_get_expiration_time:
 * @certificate_info: a #PopplerCertificateInfo structure containing certificate information
 *
 * Get certificate expiration time
 *
 * Return value: (transfer none): certificate expiration time
 *
 * Since: 23.08.0
 **/
GDateTime *poppler_certificate_info_get_expiration_time(const PopplerCertificateInfo *certificate_info)
{
    g_return_val_if_fail(certificate_info != nullptr, nullptr);
    return certificate_info->expires;
}

static PopplerCertificateInfo *create_certificate_info(const X509CertificateInfo *ci)
{
    PopplerCertificateInfo *certificate_info;

    g_return_val_if_fail(ci != nullptr, nullptr);

    const X509CertificateInfo::EntityInfo &subject_info = ci->getSubjectInfo();
    const X509CertificateInfo::EntityInfo &issuer_info = ci->getIssuerInfo();
    const X509CertificateInfo::Validity &validity = ci->getValidity();

    certificate_info = poppler_certificate_info_new();
    certificate_info->id = g_strdup(ci->getNickName().c_str());
    certificate_info->subject_common_name = g_strdup(subject_info.commonName.c_str());
    certificate_info->subject_organization = g_strdup(subject_info.organization.c_str());
    certificate_info->subject_email = g_strdup(subject_info.email.c_str());
    certificate_info->issuer_common_name = g_strdup(issuer_info.commonName.c_str());
    certificate_info->issuer_organization = g_strdup(issuer_info.organization.c_str());
    certificate_info->issuer_email = g_strdup(issuer_info.email.c_str());
    certificate_info->issued = g_date_time_new_from_unix_utc(validity.notBefore);
    certificate_info->expires = g_date_time_new_from_unix_utc(validity.notAfter);

    return certificate_info;
}

/**
 * poppler_certificate_info_copy:
 * @certificate_info: a #PopplerCertificateInfo structure containing certificate information
 *
 * Copies @certificate_info, creating an identical #PopplerCertificateInfo.
 *
 * Return value: (transfer full): a new #PopplerCertificateInfo structure identical to @certificate_info
 *
 * Since: 23.07.0
 **/
PopplerCertificateInfo *poppler_certificate_info_copy(const PopplerCertificateInfo *certificate_info)
{
    PopplerCertificateInfo *dup;

    g_return_val_if_fail(certificate_info != nullptr, nullptr);

    dup = (PopplerCertificateInfo *)g_malloc0(sizeof(PopplerCertificateInfo));
    dup->id = g_strdup(certificate_info->id);
    dup->subject_common_name = g_strdup(certificate_info->subject_common_name);
    dup->subject_organization = g_strdup(certificate_info->subject_organization);
    dup->subject_email = g_strdup(certificate_info->subject_email);
    dup->issuer_common_name = g_strdup(certificate_info->issuer_common_name);
    dup->issuer_organization = g_strdup(certificate_info->issuer_organization);
    dup->issuer_email = g_strdup(certificate_info->issuer_email);
    dup->issued = g_date_time_ref(certificate_info->issued);
    dup->expires = g_date_time_ref(certificate_info->expires);

    return dup;
}

/**
 * poppler_certificate_info_free:
 * @certificate_info: a #PopplerCertificateInfo structure containing certificate information
 *
 * Frees @certificate_info
 *
 * Since: 23.07.0
 **/
void poppler_certificate_info_free(PopplerCertificateInfo *certificate_info)
{
    if (certificate_info == nullptr) {
        return;
    }

    g_clear_pointer(&certificate_info->id, g_free);
    g_clear_pointer(&certificate_info->subject_common_name, g_free);
    g_clear_pointer(&certificate_info->subject_organization, g_free);
    g_clear_pointer(&certificate_info->subject_email, g_free);
    g_clear_pointer(&certificate_info->issuer_common_name, g_free);
    g_clear_pointer(&certificate_info->issuer_organization, g_free);
    g_clear_pointer(&certificate_info->issuer_email, g_free);
    g_clear_pointer(&certificate_info->issued, g_date_time_unref);
    g_clear_pointer(&certificate_info->expires, g_date_time_unref);

    g_free(certificate_info);
}

/**
 * poppler_get_available_signing_certificates:
 *
 * Get all available signing certificate information
 *
 * Returns: (transfer full) (element-type PopplerCertificateInfo): all available signing certificate information
 **/
GList *poppler_get_available_signing_certificates(void)
{
    GList *list = nullptr;
    auto backend = CryptoSign::Factory::createActive();

    if (!backend) {
        return nullptr;
    }

    std::vector<std::unique_ptr<X509CertificateInfo>> vCerts = backend->getAvailableSigningCertificates();
    for (auto &cert : vCerts) {
        PopplerCertificateInfo *certificate_info = create_certificate_info(cert.get());
        list = g_list_append(list, certificate_info);
    }
    return list;
}

/**
 * poppler_get_certificate_info_by_id:
 *
 * Get certificate by nick name
 *
 * Returns: (transfer full): a #PopplerCertificateInfo or %NULL if not found
 **/
PopplerCertificateInfo *poppler_get_certificate_info_by_id(const char *id)
{
    PopplerCertificateInfo *ret = nullptr;
    GList *certificate_info = poppler_get_available_signing_certificates();
    GList *list;

    for (list = certificate_info; list != nullptr; list = list->next) {
        PopplerCertificateInfo *info = (PopplerCertificateInfo *)list->data;

        if (g_strcmp0(info->id, id) == 0) {
            ret = poppler_certificate_info_copy(info);
            break;
        }
    }

    g_list_free_full(certificate_info, (GDestroyNotify)poppler_certificate_info_free);

    return ret;
}

/* NSS functions */

/**
 * poppler_set_nss_dir:
 *
 * Set NSS directory
 *
 * Since: 23.07.0
 **/
void poppler_set_nss_dir(const char *path)
{
#ifdef ENABLE_NSS3
    NSSSignatureConfiguration::setNSSDir(GooString(path));
#else
    (void)path;
#endif
}

/**
 * poppler_get_nss_dir:
 *
 * Get NSS directory
 *
 * Return value: (transfer full): nss directroy.
 *
 * Since: 23.07.0
 **/
char *poppler_get_nss_dir(void)
{
#ifdef ENABLE_NSS3
    return g_strdup(NSSSignatureConfiguration::getNSSDir().c_str());
#else
    return nullptr;
#endif
}

/**
 * poppler_set_nss_password_callback:
 * @func: (scope call): a #PopplerNssPasswordFunc that represents a signature annotation
 *
 * A callback which asks for certificate password
 *
 * Since: 23.07.0
 **/
void poppler_set_nss_password_callback(PopplerNssPasswordFunc func)
{
#ifdef ENABLE_NSS3
    NSSSignatureConfiguration::setNSSPasswordCallback(func);
#else
    g_warning("poppler_set_nss_password_callback called but this poppler is built without NSS support");
    (void)func;
#endif
}
