package apierrors

import (
	"net/http"

	"github.com/anki/sai-go-util/jsonutil"
)

const (
	InvalidJsonCode      = "bad_json"
	BadSearchModeCode    = "bad_search_mode"
	BlobIDNotFoundCode   = "blob_id_not_found"
	ValidationErrorCode  = "validation_error"
	InvalidNamespaceCode = "invalid_namespace"
	InvalidMetadata      = "bad_metadata"
)

var (
	BadSearchMode = &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusNotFound,
		Code:       BadSearchModeCode,
		Message:    "Invalid search mode",
	}
	BlobIDNotFound = &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusNotFound,
		Code:       BlobIDNotFoundCode,
		Message:    "Blob ID not found",
	}
	InvalidNamespace = &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusBadRequest,
		Code:       InvalidNamespaceCode,
		Message:    "Invalid namespace",
	}
	InvalidSessionPerms = &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusForbidden,
		Code:       "permission_denied",
		Message:    "Session token has insufficient permissions",
	}
)

func NewInvalidJSON(msg string) *jsonutil.JsonErrorResponse {
	return &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusBadRequest,
		Code:       InvalidJsonCode,
		Message:    "Failed to parse JSON request: " + msg,
	}
}

func NewValidationError(msg string) *jsonutil.JsonErrorResponse {
	return &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusBadRequest,
		Code:       ValidationErrorCode,
		Message:    "Failed to validate request: " + msg,
	}
}

func NewInvalidMetadata(msg string) *jsonutil.JsonErrorResponse {
	return &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusBadRequest,
		Code:       InvalidMetadata,
		Message:    "Invalid metadata supplied: " + msg,
	}
}
