// Package jdocserr provides error types and codes for JDOCS errors, and
// convenient ways to map these onto gRPC and HTTP protocols.
package jdocserr

import (
	"fmt"
	"strings"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

const (
	OkCode                   = "ok"
	BadJsonCode              = "bad_json"
	BadAccountCode           = "bad_account"
	BadThingCode             = "bad_thing"
	BadDocTypeCode           = "bad_doc_type"
	BadDocNameCode           = "bad_doc_name"
	DBCreateErrorCode        = "db_create_error"
	DBReadErrorCode          = "db_read_error"
	DBUpdateErrorCode        = "db_update_error"
	DBDeleteErrorCode        = "db_delete_error"
	DBFindErrorCode          = "db_find_error" // eg query/scan
	CreateDocNotAllowedCode  = "create_doc_not_allowed"
	ReadDocNotAllowedCode    = "read_doc_not_allowed"
	UpdateDocNotAllowedCode  = "update_doc_not_allowed"
	DeleteDocNotAllowedCode  = "delete_doc_not_allowed"
	ClientMetadataTooBigCode = "client_metadata_too_big"
	JsonDocTooBigCode        = "json_doc_too_big"
	JsonDocTooDeepCode       = "json_doc_too_deep"
)

func Errorf(sCode codes.Code, aCode string, format string, a ...interface{}) error {
	msg := fmt.Sprintf(format, a...)
	return status.Errorf(sCode, fmt.Sprintf("%s: %s", aCode, msg))
}

// CodesFromError extracts a GRPC code and an jdocserr code (string) from an
// error message.
func CodesFromError(err error) (codes.Code, string) {
	if sts, ok := status.FromError(err); ok {
		split := strings.Split(sts.Message(), ":")
		if len(split) == 0 {
			return sts.Code(), "no_code_present"
		}
		return sts.Code(), split[0]
	}
	return codes.Unknown, "no_code_present"
}
