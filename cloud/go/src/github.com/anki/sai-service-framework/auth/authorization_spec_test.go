package auth

import (
	"context"
	"testing"
	"time"

	"github.com/anki/sai-go-accounts/models/session"
	"github.com/anki/sai-go-accounts/models/user"
	"github.com/anki/sai-token-service/model"
	"github.com/stretchr/testify/assert"
)

// Sigh. Can't take the address of a constant in Go, so we need
// variable aliases for all these constants for the sake of the test
// setup.
var (
	userMask               = UserScopeMask
	adminMask              = AdminScopeMask
	serviceMask            = ServiceScopeMask
	userOrAdminMask        = UserOrAdminScopeMask
	userSupportOrAdminMask = UserSupportOrAdminScopeMask

	userScope    = session.UserScope
	adminScope   = session.AdminScope
	serviceScope = session.ServiceScope
	supportScope = session.SupportScope
)

func makeContext(sessionScope *session.Scope, appScopeMask *session.ScopeMask, tok *model.Token) context.Context {
	ctx := context.Background()
	if sessionScope == nil && appScopeMask == nil {
		return ctx
	}
	if tok != nil {
		ctx = model.WithAccessToken(ctx, tok)
	}
	creds := &AccountsCredentials{}
	if appScopeMask != nil {
		creds.AppKey = &session.ApiKey{
			Scopes: *appScopeMask,
		}
	}
	if sessionScope != nil {
		creds.UserSession = &session.UserSession{
			Session: &session.Session{
				Id:     "session",
				UserId: "user",
				Scope:  *sessionScope,
			},
			User: &user.UserAccount{
				User: &user.User{
					Id: "user",
				},
			},
		}
	}
	if appScopeMask == nil && sessionScope == nil {
		return ctx
	}
	return WithAccountsCredentials(ctx, creds)
}

var authorizationTests = []struct {
	name          string
	spec          AuthorizationSpec
	sessionScope  *session.Scope
	appScopeMask  *session.ScopeMask
	tok           *model.Token
	expectedError error
}{
	{
		name: "fail-no-credentials",
		spec: AuthorizationSpec{
			AuthMethod: AuthEither,
		},
		expectedError: errorNoCredentials,
	},
	{
		name: "fail-no-appkey",
		spec: AuthorizationSpec{
			AuthMethod: AuthEither,
		},
		sessionScope:  &userScope,
		expectedError: errorNoAppkey,
	},
	{
		name: "accounts-fail-no-session-when-required",
		spec: AuthorizationSpec{
			AuthMethod:    AuthAccounts,
			ScopeMask:     UserScopeMask,
			SessionStatus: RequireSession,
		},
		appScopeMask:  &userMask,
		expectedError: errorMissingSession,
	},
	{
		name: "accounts-fail-session-supplied-when-not-required",
		spec: AuthorizationSpec{
			AuthMethod:    AuthAccounts,
			SessionStatus: NoSession,
		},
		sessionScope:  &userScope,
		appScopeMask:  &userMask,
		expectedError: errorSessionSupplied,
	},
	{
		name: "accounts-fail-appkey-insufficient-permissions",
		spec: AuthorizationSpec{
			AuthMethod:    AuthAccounts,
			SessionStatus: MaybeSession,
			ScopeMask:     adminMask,
		},
		appScopeMask:  &userMask,
		expectedError: errorInsufficientAppPermissions,
	},
	{
		name: "accounts-fail-session-insufficient-permissions",
		spec: AuthorizationSpec{
			AuthMethod:    AuthAccounts,
			SessionStatus: RequireSession,
			ScopeMask:     adminMask,
		},
		appScopeMask:  &adminMask,
		sessionScope:  &userScope,
		expectedError: errorInsufficientUserPermissions,
	},
	{
		name: "accounts-fail-session-ok-but-app-insufficient-permissions",
		spec: AuthorizationSpec{
			AuthMethod:    AuthAccounts,
			SessionStatus: RequireSession,
			ScopeMask:     adminMask,
		},
		appScopeMask:  &userMask,
		sessionScope:  &adminScope,
		expectedError: errorInsufficientAppPermissions,
	},
	{
		name: "accounts-ok-session-match",
		spec: AuthorizationSpec{
			AuthMethod:    AuthAccounts,
			SessionStatus: RequireSession,
			ScopeMask:     userMask,
		},
		appScopeMask: &userMask,
		sessionScope: &userScope,
	},
	{
		name: "accounts-ok-one-of-session-scope-match",
		spec: AuthorizationSpec{
			AuthMethod:    AuthAccounts,
			SessionStatus: RequireSession,
			ScopeMask:     userOrAdminMask,
		},
		appScopeMask: &adminMask,
		sessionScope: &adminScope,
	},
	{
		name: "accounts-ok-app-scope-match",
		spec: AuthorizationSpec{
			AuthMethod:    AuthAccounts,
			SessionStatus: NoSession,
			ScopeMask:     AdminScopeMask,
		},
		appScopeMask: &adminMask,
	},
	{
		name: "accounts-ok-on-of-app-scope-match",
		spec: AuthorizationSpec{
			AuthMethod:    AuthAccounts,
			SessionStatus: NoSession,
			ScopeMask:     UserOrAdminScopeMask,
		},
		appScopeMask: &userMask,
	},
	{
		name: "token-fail-no-appkey",
		spec: AuthorizationSpec{
			AuthMethod: AuthToken,
			ScopeMask:  UserScopeMask,
		},
		tok: &model.Token{
			Id:        "token",
			UserId:    "token-user",
			ExpiresAt: time.Now().UTC().Add(time.Hour),
		},
		expectedError: errorNoCredentials,
	},
	{
		name: "token-fail-missing-token",
		spec: AuthorizationSpec{
			AuthMethod: AuthToken,
			ScopeMask:  UserScopeMask,
		},
		tok:           nil,
		appScopeMask:  &userMask,
		expectedError: errorNoToken,
	},
	{
		name: "token-fail-invalid-token",
		spec: AuthorizationSpec{
			AuthMethod: AuthToken,
			ScopeMask:  UserScopeMask,
		},
		tok:           &model.Token{},
		appScopeMask:  &userMask,
		expectedError: errorInvalidToken,
	},
	{
		name: "token-fail-expired",
		spec: AuthorizationSpec{
			AuthMethod: AuthToken,
			ScopeMask:  UserScopeMask,
		},
		appScopeMask: &userMask,
		tok: &model.Token{
			Id:        "token",
			UserId:    "token-user",
			ExpiresAt: time.Now().UTC().Add(-time.Hour),
		},
		expectedError: errorExpiredToken,
	},
	{
		name: "token-ok-expired",
		spec: AuthorizationSpec{
			AuthMethod:         AuthToken,
			ScopeMask:          UserScopeMask,
			AllowExpiredTokens: true,
		},
		appScopeMask: &userMask,
		tok: &model.Token{
			Id:        "token",
			UserId:    "token-user",
			ExpiresAt: time.Now().UTC().Add(-time.Hour),
		},
	},
	{
		name: "token-fail-insufficient-app-permissions",
		spec: AuthorizationSpec{
			AuthMethod: AuthToken,
			ScopeMask:  AdminScopeMask,
		},
		appScopeMask: &userMask,
		tok: &model.Token{
			Id:        "token",
			UserId:    "token-user",
			ExpiresAt: time.Now().UTC().Add(time.Hour),
		},
		expectedError: errorInsufficientAppPermissions,
	},
	{
		name: "token-ok",
		spec: AuthorizationSpec{
			AuthMethod: AuthToken,
			ScopeMask:  UserScopeMask,
		},
		appScopeMask: &userMask,
		tok: &model.Token{
			Id:        "token",
			UserId:    "token-user",
			ExpiresAt: time.Now().UTC().Add(time.Hour),
		},
	},
	{
		name: "either-ok-accounts",
		spec: AuthorizationSpec{
			AuthMethod: AuthEither,
			ScopeMask:  UserScopeMask,
		},
		appScopeMask: &userMask,
		sessionScope: &userScope,
	},
	{
		name: "either-ok-token",
		spec: AuthorizationSpec{
			AuthMethod: AuthEither,
			ScopeMask:  UserScopeMask,
		},
		appScopeMask: &userMask,
		tok: &model.Token{
			Id:        "token",
			UserId:    "token-user",
			ExpiresAt: time.Now().UTC().Add(time.Hour),
		},
	},
	{
		name: "either-ok-both",
		spec: AuthorizationSpec{
			AuthMethod: AuthEither,
			ScopeMask:  UserScopeMask,
		},
		appScopeMask: &userMask,
		sessionScope: &userScope,
		tok: &model.Token{
			Id:        "token",
			UserId:    "token-user",
			ExpiresAt: time.Now().UTC().Add(time.Hour),
		},
	},
	{
		name: "either-ok-token-overrides-session",
		spec: AuthorizationSpec{
			AuthMethod: AuthEither,
			ScopeMask:  UserScopeMask,
		},
		appScopeMask: &userMask,
		sessionScope: &adminScope,
		tok: &model.Token{
			Id:        "token",
			UserId:    "token-user",
			ExpiresAt: time.Now().UTC().Add(time.Hour),
		},
	},
	{
		name: "both-ok",
		spec: AuthorizationSpec{
			AuthMethod:    AuthBoth,
			SessionStatus: RequireSession,
			ScopeMask:     UserScopeMask,
		},
		appScopeMask: &userMask,
		sessionScope: &userScope,
		tok: &model.Token{
			Id:        "token",
			UserId:    "token-user",
			ExpiresAt: time.Now().UTC().Add(time.Hour),
		},
	},
	{
		name: "both-fail-missing-session",
		spec: AuthorizationSpec{
			AuthMethod:    AuthBoth,
			SessionStatus: RequireSession,
			ScopeMask:     UserScopeMask,
		},
		appScopeMask: &userMask,
		tok: &model.Token{
			Id:        "token",
			UserId:    "token-user",
			ExpiresAt: time.Now().UTC().Add(time.Hour),
		},
		expectedError: errorMissingSession,
	},
	{
		name: "both-fail-missing-token",
		spec: AuthorizationSpec{
			AuthMethod:    AuthBoth,
			SessionStatus: RequireSession,
			ScopeMask:     UserScopeMask,
		},
		appScopeMask:  &userMask,
		sessionScope:  &userScope,
		expectedError: errorNoToken,
	},
	{
		name: "both-fail-missing-both",
		spec: AuthorizationSpec{
			AuthMethod:    AuthBoth,
			SessionStatus: RequireSession,
			ScopeMask:     UserScopeMask,
		},
		appScopeMask:  &userMask,
		expectedError: errorMissingSession,
	},
}

func TestAuthorize(t *testing.T) {
	for _, test := range authorizationTests {
		ctx := makeContext(test.sessionScope, test.appScopeMask, test.tok)

		err := test.spec.Authorize(ctx)
		assert.Equal(t, test.expectedError, err, test.name)

		grpcSpec := GrpcAuthorizationSpec{
			Method: "/package.service/method",
			Auth:   test.spec,
		}
		assert.Equal(t, test.expectedError, grpcSpec.Authorize(ctx), test.name+"_GrpcAuthorizationSpec")
	}
}
