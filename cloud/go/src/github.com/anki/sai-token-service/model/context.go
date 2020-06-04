package model

import (
	"context"
	"google.golang.org/grpc/metadata"
)

// ----------------------------------------------------------------------
// Context
// ----------------------------------------------------------------------

type tokenContextKey int

const (
	tokenKey               tokenContextKey = iota
	AccessTokenMetadataKey                 = "anki-access-token"
)

// WithAccessToken returns a new context with the token object bound
// as an immediate value on the context, and with the token's raw form
// set in the outgoing metadata, for passing the authentication token
// to any downstream gRPC services.
func WithAccessToken(ctx context.Context, token *Token) context.Context {
	ctx = WithOutboundAccessToken(ctx, token.Raw)
	return context.WithValue(ctx, tokenKey, token)
}

// WithOutboundAccessToken sets a signed JWT token in the outgoing
// metadata for authenticating to service endpoints requiring an
// access token.
func WithOutboundAccessToken(ctx context.Context, token string) context.Context {
	return metadata.AppendToOutgoingContext(ctx, AccessTokenMetadataKey, token)
}

func GetAccessToken(ctx context.Context) *Token {
	if t, ok := ctx.Value(tokenKey).(*Token); ok {
		return t
	}
	return nil
}
