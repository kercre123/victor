package auth

import (
	"context"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-service-framework/grpc/grpcutil"
	"github.com/anki/sai-service-framework/svc"
	"github.com/jawher/mow.cli"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

// AccountsAuthenticator implements gRPC interceptors to decorate the
// current context with user information extracted from an Accounts
// system user session and app key.
//
// The user session token and app key are obtained from the context,
// having been already bound by the service framework's standard
// interceptor package.
//
// This interceptor does not enforce access policies - that's up to
// the authorization component. It will deny access in the event that
// the user session fails validation or the appkey is missing, but
// otherwise performs no access level checks for the request resource.
type AccountsAuthenticator struct {
	enabled   *bool
	cache     *CachingSessionValidator
	validator SessionValidator
}

type AccountsAuthenticatorOpt func(a *AccountsAuthenticator)

func WithSessionValidator(v SessionValidator) AccountsAuthenticatorOpt {
	return func(a *AccountsAuthenticator) {
		a.validator = v
	}
}

func WithAccountsAuthenticationEnabled(value bool) AccountsAuthenticatorOpt {
	return func(a *AccountsAuthenticator) {
		a.enabled = &value
	}
}

func NewAccountsAuthenticator(opts ...AccountsAuthenticatorOpt) *AccountsAuthenticator {
	authenticator := &AccountsAuthenticator{}
	for _, opt := range opts {
		opt(authenticator)
	}
	if authenticator.validator == nil {
		authenticator.validator = NewAccountsValidator()
	}
	if authenticator.cache == nil {
		authenticator.cache = NewCachingSessionValidator(authenticator.validator)
	}
	return authenticator
}

func (a *AccountsAuthenticator) CommandSpec() string {
	spec := "[--accounts-authentication-enabled] " +
		a.cache.CommandSpec()
	if ci, ok := a.validator.(svc.CommandInitializer); ok {
		spec = spec + " " + ci.CommandSpec()
	}
	return spec
}

func (a *AccountsAuthenticator) CommandInitialize(cmd *cli.Cmd) {
	if a.enabled == nil {
		a.enabled = svc.BoolOpt(cmd, "accounts-authentication-enabled", false,
			"Enable authentication based on appkeys and sessions")
	}
	if ci, ok := a.validator.(svc.CommandInitializer); ok {
		ci.CommandInitialize(cmd)
	}
	a.cache.CommandInitialize(cmd)
}

func (a *AccountsAuthenticator) Start(ctx context.Context) error {
	alog.Debug{
		"action":  "accounts_authenticator_start",
		"enabled": *a.enabled,
	}.Log()
	if a.enabled != nil && *a.enabled {
		if so, ok := a.validator.(svc.ServiceObject); ok {
			return so.Start(ctx)
		}
	}
	return nil
}

func (a *AccountsAuthenticator) Stop() error {
	if so, ok := a.validator.(svc.ServiceObject); ok {
		return so.Stop()
	}
	return nil
}

func (a *AccountsAuthenticator) Kill() error {
	if so, ok := a.validator.(svc.ServiceObject); ok {
		return so.Kill()
	}
	return nil
}

func (a *AccountsAuthenticator) UnaryServerInterceptor() grpc.UnaryServerInterceptor {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
		if authctx, err := a.authenticate(ctx); err != nil {
			return nil, err
		} else {
			return handler(authctx, req)
		}
	}
}

func (a *AccountsAuthenticator) StreamServerInterceptor() grpc.StreamServerInterceptor {
	return func(srv interface{}, stream grpc.ServerStream, info *grpc.StreamServerInfo, handler grpc.StreamHandler) error {
		if authctx, err := a.authenticate(stream.Context()); err != nil {
			return err
		} else {
			return handler(srv, &grpcutil.ServerStreamWithContext{stream, authctx})
		}
	}
}

func (a *AccountsAuthenticator) authenticate(ctx context.Context) (context.Context, error) {
	ctx = WithSessionValidatorContext(ctx, a.cache) // UGH, this is gross

	if a.enabled == nil || !*a.enabled {
		return ctx, nil
	}

	appkey := grpcutil.GetAppKey(ctx)
	session := grpcutil.GetUserSessionKey(ctx)

	// If appkey and session are both empty, this may be a call
	// authenticated by JWT token, so just pass through.
	if appkey == "" && session == "" {
		return ctx, nil
	}

	credentials, err := a.cache.ValidateSession(ctx, appkey, session)
	if err != nil {
		return ctx, status.Errorf(codes.Unauthenticated, "Invalid accounts credentials: %s", err)
	}
	if credentials.AppKey == nil {
		return ctx, status.Errorf(codes.Unauthenticated, "Missing app key")
	}
	ctx = alog.WithLogMap(ctx, map[string]interface{}{
		"auth_user_id": credentials.UserId(),
	})
	return WithAccountsCredentials(ctx, credentials), nil
}
