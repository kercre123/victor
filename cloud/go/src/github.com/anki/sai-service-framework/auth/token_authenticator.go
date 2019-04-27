package auth

import (
	"context"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-service-framework/grpc/grpcutil"
	"github.com/anki/sai-service-framework/svc"
	"github.com/anki/sai-token-service/client/token"
	tokenmodel "github.com/anki/sai-token-service/model"
	"github.com/jawher/mow.cli"
	"google.golang.org/grpc"
	"google.golang.org/grpc/metadata"
)

// TokenAuthenticator implements gRPC interceptors to decorate the
// current context with a parsed and validated JWT token.
type TokenAuthenticator struct {
	svc.CompositeServiceObject

	enabled *bool

	// validator is a TokenValidator, which verifies and parses the
	// incoming JWT token. In the case of the token service itself,
	// this will be a local implementation; for other services it will
	// be the Token Service client.
	validator token.TokenValidator
}

type TokenAuthenticatorOpt func(*TokenAuthenticator)

// WithTokenClient returns an option to initialize the token
// authenticator with a token service client for keeping the
// revocation list up-to-date from the Token Service. This should be
// used by most gRPC services.
func WithTokenClient() TokenAuthenticatorOpt {
	return WithTokenValidator(token.NewClient())
}

// WithPublicKeyValidator returns an option to initialize the token
// authenticator with a token validator that is initialized from a
// local config, but does not include a token service client. This
// should only be used by the token service itself.
func WithPublicKeyValidator() TokenAuthenticatorOpt {
	return WithTokenValidator(token.NewValidator())
}

// WithTokenValidator returns an option to initialize the token
// authenticator with the supplied validator. Useful for testing.
func WithTokenValidator(v token.TokenValidator) TokenAuthenticatorOpt {
	return func(a *TokenAuthenticator) {
		if a.validator != nil {
			panic("Programming error - more than one token validator specified")
		}
		a.validator = v
		a.Add(v)
	}
}

func WithTokenAuthenticationEnabled(value bool) TokenAuthenticatorOpt {
	return func(a *TokenAuthenticator) {
		a.enabled = &value
	}
}

func NewTokenAuthenticator(opts ...TokenAuthenticatorOpt) *TokenAuthenticator {
	auth := &TokenAuthenticator{}
	for _, opt := range opts {
		opt(auth)
	}
	if auth.validator == nil {
		panic("Programming error - no token validator specified")
	}
	return auth
}

func (a *TokenAuthenticator) CommandSpec() string {
	return "[--token-authentication-enabled] " + a.CompositeServiceObject.CommandSpec()
}

func (a *TokenAuthenticator) CommandInitialize(cmd *cli.Cmd) {
	if a.enabled == nil {
		a.enabled = svc.BoolOpt(cmd, "token-authentication-enabled", false,
			"Enable authentication based on tokens from the Token Service")
	}
	a.CompositeServiceObject.CommandInitialize(cmd)
}

func (a *TokenAuthenticator) Start(ctx context.Context) error {
	if a.enabled == nil || !*a.enabled {
		return nil
	}
	return a.CompositeServiceObject.Start(ctx)
}

func (a *TokenAuthenticator) UnaryServerInterceptor() grpc.UnaryServerInterceptor {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
		if authctx, err := a.authenticate(ctx); err != nil {
			return nil, err
		} else {
			return handler(authctx, req)
		}
	}
}

func (a *TokenAuthenticator) StreamServerInterceptor() grpc.StreamServerInterceptor {
	return func(srv interface{}, stream grpc.ServerStream, info *grpc.StreamServerInfo, handler grpc.StreamHandler) error {
		if authctx, err := a.authenticate(stream.Context()); err != nil {
			return err
		} else {
			return handler(srv, &grpcutil.ServerStreamWithContext{stream, authctx})
		}
	}
}

func (a *TokenAuthenticator) authenticate(ctx context.Context) (context.Context, error) {
	if a.enabled == nil || !*a.enabled {
		return ctx, nil
	}

	// Check that the token hasn't already been extracted
	if tok := tokenmodel.GetAccessToken(ctx); tok != nil {
		return ctx, nil
	}
	// Check the incoming metadata for a token
	if md, ok := metadata.FromIncomingContext(ctx); ok {
		if tokstr, ok := md[tokenmodel.AccessTokenMetadataKey]; ok {
			if tok, err := a.validator.TokenFromString(tokstr[0]); err == nil {
				ctx = alog.WithLogMap(ctx, map[string]interface{}{
					"auth_token_id":     tok.Id,
					"auth_requestor_id": tok.RequestorId,
					"auth_user_id":      tok.UserId,
				})
				return tokenmodel.WithAccessToken(ctx, tok), nil
			} else {
				alog.Warn{
					"action": "token_authenticate",
					"status": "invalid_token",
					"error":  err,
				}.LogC(ctx)
				// Use an empty token instance to represent invalid
				// token, so rejecting the request can be centralized
				// in the authorizer.
				return tokenmodel.WithAccessToken(ctx, &tokenmodel.Token{Raw: tokstr[0]}), nil
			}
		}
	}
	return ctx, nil
}
