// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// Any errors that may be returned through the API should be defined here
package apierrors

import (
	"net/http"

	"github.com/anki/sai-go-util/jsonutil"
)

var (
	ValidateSessionErrorCode = "validate_user_failed"
	InvalidFieldsCode        = "invalid_fields"
	BadGatewayCode           = "bad_gateway"
)

var (
	AccountNotFound = &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusNotFound,
		Code:       "account_not_found",
		Message:    "Account not found",
	}
	DuplicateUsername = &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusBadRequest,
		Code:       "duplicate_username",
		Message:    "Username already in use",
	}
	MissingParameter = &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusBadRequest,
		Code:       "missing_params",
		Message:    "Missing one or more required parameters",
	}
	MissingApiKey = &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusBadRequest,
		Code:       "no_app_key",
		Message:    "Anki-App-Key not provided",
	}
	InvalidSession = &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusForbidden,
		Code:       "invalid_session",
		Message:    "Session token is invalid",
	}
	InvalidSessionPerms = &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusForbidden,
		Code:       "permission_denied",
		Message:    "Session token has insufficient permissions",
	}
	InvalidApiKey = &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusUnauthorized,
		Code:       "invalid_app_key",
		Message:    "Anki-App-Key is invalid",
	}
	PasswordTooShort = &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusBadRequest,
		Code:       "password_too_short",
		Message:    "Passwords must be at least 6 characters",
	}
	PasswordContainsUsername = &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusBadRequest,
		Code:       "password_contains_username",
		Message:    "Passwords cannot contain the username",
	}
	CannotReactivatePurged = &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusBadRequest,
		Code:       "cannot_reactivate_purged",
		Message:    "Account is purged; cannot be reactivated",
	}
	CannotPurgeActive = &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusBadRequest,
		Code:       "cannot_purge_active",
		Message:    "Cannot purge an active account; must first be deactivated",
	}
	AccountAlreadyPurged = &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusBadRequest,
		Code:       "already_purged",
		Message:    "Account is already purged",
	}
	AccountAlreadyActive = &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusBadRequest,
		Code:       "already_active",
		Message:    "Account is already active",
	}
	// InvalidUser and InvalidPassword present the same error to the user
	// but have different variable names so tests can tell them apart.
	InvalidUser = &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusForbidden,
		Code:       "invalid_username_or_password",
		Message:    "Invalid username or password",
	}
	InvalidPassword = &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusForbidden,
		Code:       "invalid_username_or_password",
		Message:    "Invalid username or password",
	}
	InvalidScopeForApiKey = &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusUnauthorized,
		Code:       "invalid_api_key_scope",
		Message:    "Anki-App-Key cannot be used by this user.",
	}
	EmailAlreadyVerified = &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusConflict,
		Code:       "email_already_verified",
		Message:    "The email address is already verified",
	}
	EmailBlocked = &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusBadRequest,
		Code:       "email_address_blocked",
		Message:    "The email address is blocked",
	}
	InvaldSAML = &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusUnauthorized,
		Code:       "invalid_saml_response",
		Message:    "SAML response could not be parsed or verified",
	}
	InvaldSAMLGroup = &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusUnauthorized,
		Code:       "invalid_saml_group",
		Message:    "Not a member of a suitable SAML group",
	}
	InvalidUTF8 = &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusBadRequest,
		Code:       "invalid_utf8_string",
		Message:    "Could not decode UTF8 input",
	}
	PlayerIdConflict = &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusBadRequest,
		Code:       "player_id_conflict",
		Message:    "player_id value conflicts with drive_guest_id",
	}
)

func NewValidateSessionError(err jsonutil.JsonError) jsonutil.JsonError {
	return &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusBadRequest,
		Code:       ValidateSessionErrorCode,
		Message:    "Invalid session for validation",
		Metadata: map[string]interface{}{
			"original_error": err, // will be marshalled into json
		},
	}
}

func NewFieldValidationError(message string) jsonutil.JsonError {
	return &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusBadRequest,
		Code:       InvalidFieldsCode,
		Message:    message,
	}
}
