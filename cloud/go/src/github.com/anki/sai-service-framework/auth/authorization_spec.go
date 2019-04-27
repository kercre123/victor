package auth

import (
	"context"

	"github.com/anki/sai-go-accounts/models/session"
	"github.com/anki/sai-go-accounts/validate"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-service-framework/grpc/grpcutil"
	tokenmodel "github.com/anki/sai-token-service/model"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

// AuthorizationMethod is a type enumerating which authentication
// mechanisms are allowed for a gRPC endpoint.
type AuthorizationMethod int

const (
	// AuthAccounts indicates that authentication must be via Accounts
	// session - JWT tokens will not be accepted.
	AuthAccounts AuthorizationMethod = iota
	// AuthToken indicates that authentication must be via a JWT token
	// issued by the Token Management Service. Accounts sessions will
	// not be accepted.
	AuthToken
	// AuthEither indicates that either an appropriate accounts
	// session or a JWT token may be provided.
	AuthEither
	// AuthBoth indicates that an accounts session AND a JWT token are
	// required.
	AuthBoth

	MaybeSession   = validate.MaybeSession
	NoSession      = validate.NoSession
	RequireSession = validate.RequireSession

	ValidScopeMask              = session.ValidSessionScope
	UserScopeMask               = session.ScopeMask(session.UserScope)
	AdminScopeMask              = session.ScopeMask(session.AdminScope)
	ServiceScopeMask            = session.ScopeMask(session.ServiceScope)
	UserOrAdminScopeMask        = session.ScopeMask(session.UserScope | session.AdminScope)
	UserSupportOrAdminScopeMask = session.ScopeMask(session.UserScope | session.SupportScope | session.AdminScope)
)

// GrpcAuthorizationSpec specifies authorization requirements for a
// specific gRPC endpoint.
type GrpcAuthorizationSpec struct {
	// Method is the full method name of a gRPC method, in the
	// canonical form of "/package.service/method".
	Method string

	// Auth contains the specification of the authentication
	// requirements for authorizing a call of the given method.
	Auth AuthorizationSpec
}

func (spec GrpcAuthorizationSpec) Authorize(ctx context.Context) error {
	return spec.Auth.Authorize(ctx)
}

// AuthorizationSpec specifies authorization requirements and can
// evaluate whether a given request context meets those requirements.
type AuthorizationSpec struct {
	// AuthMethod specifies whether the method should be authorized by
	// an accounts appkey+session, a JWT token, or either.
	AuthMethod AuthorizationMethod

	// ScopeMask is a mask which either the user's session or appkey
	// Scope must match for validation to pass.
	ScopeMask session.ScopeMask

	// SessionStatus indicates whether a session is required for
	// validation to pass.
	//
	// One of:
	// RequireSession - The user must have a session and it must comply with ScopeMask
	// MaybeSession - The user may or may not supply a session, but if they do
	//     it'll be validated and must match the ScopeMask
	// NoSession - The user must not supply a session token at all.
	SessionStatus validate.SessionStatus

	// AllowExpiredTokens indicates that the authorizer should allow a
	// call to proceed if it was authenticated with an expired JWT
	// token. This pretty much exists only for the RefreshToken case
	// in the token service.
	AllowExpiredTokens bool

	// AllowRevokedTokens indicates that the authorizer should allow a
	// call to proceed with any token whose signature can be
	// validated, even if it's been revoked.
	AllowRevokedTokens bool
}

var (
	errorBadAuthorizationSpec = status.Errorf(codes.Internal, "Bad authorization spec")
	errorNoCredentials        = status.Errorf(codes.Unauthenticated, "No credentials found")
	errorNoAppkey             = status.Errorf(codes.Unauthenticated, "No appkey found")
	errorNoToken              = status.Errorf(codes.Unauthenticated, "No token found")
)

func (spec AuthorizationSpec) Authorize(ctx context.Context) error {

	// Retrieve the accounts credentials retrieved by validating the
	// appkey present on every request and optional session.
	creds := GetAccountsCredentials(ctx)
	if creds == nil {
		// Every call must have accounts credentials present -- even
		// if the invocation is authorized via JWT token, or without a
		// user session, an app key is still present to identify the
		// calling application and its privileges.
		return errorNoCredentials
	}

	if creds.AppKey == nil {
		return errorNoAppkey
	}

	// Retrieve the validated access token, if one is present. This
	// will be nil if no token was previously extracted by the token
	// authenticator.
	tok := tokenmodel.GetAccessToken(ctx)

	// Dispatch to the appropriate authorization function depending on
	// the auth spec and the provided credentials.
	switch spec.AuthMethod {
	case AuthAccounts:
		return spec.authorizeAccounts(ctx, creds)
	case AuthToken:
		return spec.authorizeToken(ctx, creds, tok)
	case AuthEither:
		// In the case of accepting either a session or a token, the
		// token is always used if it is present.
		if tok != nil {
			return spec.authorizeToken(ctx, creds, tok)
		} else {
			return spec.authorizeAccounts(ctx, creds)
		}
	case AuthBoth:
		// AuthBoth requires a session AND a token, so call both auth
		// methods in succession.
		if err := spec.authorizeAccounts(ctx, creds); err != nil {
			return err
		}
		return spec.authorizeToken(ctx, creds, tok)
	}

	return errorBadAuthorizationSpec
}

var (
	errorMissingSession              = status.Errorf(codes.Unauthenticated, "Missing session")
	errorSessionSupplied             = status.Errorf(codes.PermissionDenied, "Session was supplied")
	errorInsufficientUserPermissions = status.Errorf(codes.PermissionDenied, "Insufficient user permissions")
	errorInsufficientAppPermissions  = status.Errorf(codes.PermissionDenied, "Insufficient app permissions")
)

// authorizeAccounts validates acounts credentials against the session
// rules specified. This basically replicates the logic in
// github.com/anki/sai-go-accounts/validate/validate.go, but
// independent of the transport mechanism.
func (spec AuthorizationSpec) authorizeAccounts(ctx context.Context, creds *AccountsCredentials) error {
	// TODO: Handle getting the request ID from either the gRPC bound
	// values or from eventual service framework updates for REST
	// APIs, if we ever do them, so the logging here will work equally
	// well for authorizing gRPC or REST calls.
	requestId := grpcutil.GetRequestId(ctx)
	userId := "none"

	if spec.SessionStatus == RequireSession && creds.UserSession == nil {
		alog.Info{
			"action":     "authorize",
			"status":     "session_required",
			"request_id": requestId,
			"error":      "no_session_supplied",
		}.Log()
		return errorMissingSession
	}

	if spec.SessionStatus == NoSession && creds.UserSession != nil {
		alog.Info{
			"action":     "authorize",
			"status":     "no_session_required",
			"request_id": requestId,
			"error":      "session_was_supplied",
		}.Log()
		return errorSessionSupplied
	}

	activeScope := session.NoSessionScope
	// Check session scope if a session is set
	if creds.UserSession != nil {
		activeScope = creds.UserSession.Session.Scope

		if creds.UserSession.User != nil {
			userId = creds.UserSession.User.Id
		} else {
			userId = creds.UserSession.Session.RemoteId
		}

		// Check the user's session scope against the required scope
		// (with an exemption for root scope)
		if activeScope != session.RootScope && !spec.ScopeMask.Contains(activeScope) {
			alog.Info{
				"action":             "authorize",
				"status":             "permission_denied",
				"request_id":         requestId,
				"active_scope":       activeScope,
				"required_scopemask": spec.ScopeMask.String(),
				"error":              "invalid_session_scope",
			}.Log()
			return errorInsufficientUserPermissions
		}

		// If the user's scope meets the requirements, make sure the
		// app key does as well (e.g. so you can't use an admin
		// session with a user-scoped app)
		if !creds.AppKey.Scopes.Contains(activeScope) {
			alog.Info{
				"action":             "authorize",
				"status":             "permission_denied",
				"request_id":         requestId,
				"active_scope":       activeScope,
				"appkey_scopemask":   creds.AppKey.Scopes.String(),
				"required_scopemask": spec.ScopeMask.String(),
				"error":              "invalid_appkey_scope",
			}.Log()
			return errorInsufficientAppPermissions
		}
	}

	// Else, if there was no session but the spec status is
	// MaybeSession, check that the app key scope overlaps the
	// requested scope mask.
	if (creds.UserSession == nil && spec.SessionStatus == MaybeSession) || spec.SessionStatus == NoSession {
		if (spec.ScopeMask & creds.AppKey.Scopes) == 0 {
			alog.Info{
				"action":             "authorize",
				"status":             "permission_denied",
				"request_id":         requestId,
				"appkey_scopemask":   creds.AppKey.Scopes.String(),
				"required_scopemask": spec.ScopeMask.String(),
				"error":              "invalid_appkey_scope",
				"with_session":       false,
			}.Log()
			return errorInsufficientAppPermissions
		}
	}

	alog.Info{
		"action":             "authorize",
		"status":             "ok",
		"request_id":         requestId,
		"user_id":            userId,
		"session_scope":      activeScope,
		"appkey_scopemask":   creds.AppKey.Scopes.String(),
		"required_scopemask": spec.ScopeMask.String(),
	}.Log()

	return nil
}

var (
	errorInvalidToken = status.Errorf(codes.PermissionDenied, "Invalid token")
	errorExpiredToken = status.Errorf(codes.PermissionDenied, "Expired token")
)

func (spec AuthorizationSpec) authorizeToken(ctx context.Context, creds *AccountsCredentials, tok *tokenmodel.Token) error {
	requestId := grpcutil.GetRequestId(ctx)

	if tok == nil {
		alog.Info{
			"action":     "authorize",
			"status":     "unauthenticated",
			"request_id": requestId,
			"error":      "missing_token",
		}.Log()
		return errorNoToken
	}

	// Check for invalid token
	if tok.Id == "" {
		alog.Error{
			"action":     "authorize",
			"status":     "permission_denied",
			"request_id": requestId,
			"token":      tok.Raw,
			"error":      "invalid_token",
		}.Log()
		return errorInvalidToken
	}

	// Check for token expiration
	if tok.IsExpired() && !spec.AllowExpiredTokens {
		alog.Info{
			"action":     "authorize",
			"status":     "permission_denied",
			"request_id": requestId,
			"token":      tok.Id,
			"error":      "expired_token",
		}.Log()
		return errorExpiredToken
	}

	// Check that the app has the correct permissions
	if (spec.ScopeMask & creds.AppKey.Scopes) == 0 {
		alog.Info{
			"action":             "authorize",
			"status":             "permission_denied",
			"request_id":         requestId,
			"appkey_scopemask":   creds.AppKey.Scopes.String(),
			"required_scopemask": spec.ScopeMask.String(),
			"error":              "invalid_appkey_scope",
			"with_token":         true,
			"token_id":           tok.Id,
		}.Log()
		return errorInsufficientAppPermissions
	}

	alog.Info{
		"action":             "authorize",
		"status":             "ok",
		"request_id":         requestId,
		"user_id":            tok.UserId,
		"appkey_scopemask":   creds.AppKey.Scopes.String(),
		"required_scopemask": spec.ScopeMask.String(),
		"token_id":           tok.Id,
	}.Log()

	return nil
}
