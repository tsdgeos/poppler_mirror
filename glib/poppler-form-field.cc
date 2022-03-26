/* poppler-form-field.cc: glib interface to poppler
 *
 * Copyright (C) 2007 Carlos Garcia Campos <carlosgc@gnome.org>
 * Copyright (C) 2006 Julien Rebetez
 * Copyright (C) 2020 Oliver Sander <oliver.sander@tu-dresden.de>
 * Copyright (C) 2021 André Guerreiro <aguerreiro1985@gmail.com>
 * Copyright (C) 2021 Marek Kasik <mkasik@redhat.com>
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
};

static PopplerSignatureInfo *_poppler_form_field_signature_validate(PopplerFormField *field, PopplerSignatureValidationFlags flags, gboolean force_revalidation, GError **error)
{
    FormFieldSignature *sig_field;
    SignatureInfo *sig_info;
    PopplerSignatureInfo *poppler_sig_info;

    if (poppler_form_field_get_field_type(field) != POPPLER_FORM_FIELD_SIGNATURE) {
        g_set_error(error, POPPLER_ERROR, POPPLER_ERROR_INVALID, "Wrong FormField type");
        return nullptr;
    }

    sig_field = static_cast<FormFieldSignature *>(field->widget->getField());

    sig_info = sig_field->validateSignature(flags & POPPLER_SIGNATURE_VALIDATION_FLAG_VALIDATE_CERTIFICATE, force_revalidation, -1, flags & POPPLER_SIGNATURE_VALIDATION_FLAG_WITHOUT_OCSP_REVOCATION_CHECK,
                                            flags & POPPLER_SIGNATURE_VALIDATION_FLAG_USE_AIA_CERTIFICATE_FETCH);

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

    switch (sig_info->getCertificateValStatus()) {
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

    poppler_sig_info->signer_name = g_strdup(sig_info->getSignerName());
    poppler_sig_info->local_signing_time = g_date_time_new_from_unix_local(sig_info->getSigningTime());

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
