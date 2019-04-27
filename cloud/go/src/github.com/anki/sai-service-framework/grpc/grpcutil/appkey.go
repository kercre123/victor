package grpcutil

import (
	"context"
	"fmt"

	"google.golang.org/grpc/metadata"
)

type appKeyContextKey int

const (
	appKeyKey appKeyContextKey = iota
	userSessionKey

	AppKeyMetadataKey      = "anki-app-key"
	UserSessionMetadataKey = "anki-user-session"
)

// GetAppKey extracts the Anki app key from a context, first checking
// for an immediate value, then the incoming metadata, and finally the
// outgoing metadata.
func GetAppKey(ctx context.Context) string {
	return GetStringValueWithFallbackToMetadata(ctx, appKeyKey, AppKeyMetadataKey)
}

// GetUserSessionKey extracts the Anki user session key (the string
// identifier for the session) from a context, if there is one. First
// checking for an immediate value, then falling back to incoming,
// then outgoing metadata if necessary.
func GetUserSessionKey(ctx context.Context) string {
	return GetStringValueWithFallbackToMetadata(ctx, userSessionKey, UserSessionMetadataKey)
}

// WithAppKey returns a context with the the Anki app key set in the
// outgoing metadata, for identification to downstream Anki gRPC
// services.
func WithAppKey(ctx context.Context, appKey string) context.Context {
	return metadata.AppendToOutgoingContext(ctx, AppKeyMetadataKey, appKey)
}

// WithUserSession returns a context with the the Anki user session
// set in the outgoing metadata, for identification to downstream Anki
// gRPC services.
func WithUserSessionKey(ctx context.Context, session string) context.Context {
	return metadata.AppendToOutgoingContext(ctx, UserSessionMetadataKey, session)
}

// SanitizeAppKey returns a truncated version of the app key which is
// safe for logging.
func SanitizeAppKey(key string) string {
	if len(key) > 3 {
		return fmt.Sprintf("%s...", key[:3])
	}
	return key
}
