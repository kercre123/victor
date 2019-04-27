package auth

import (
	"context"

	"github.com/anki/sai-go-accounts/models/session"
)

// AccountsCredentials is a tuple holding the structured information
// about the appkey and optional user session. After a succesful call
// to ValidateSession, the either UserSession, AppKey, or both will be
// non-nil, depending on which values were present in the request.
type AccountsCredentials struct {
	AppKey      *session.ApiKey
	UserSession *session.UserSession
}

func (creds *AccountsCredentials) UserId() string {
	if creds.UserSession != nil {
		if creds.UserSession.Session.UserId != "" {
			return creds.UserSession.Session.UserId
		} else if creds.UserSession.Session.RemoteId != "" {
			return creds.UserSession.Session.RemoteId
		} else if creds.UserSession.User != nil {
			return creds.UserSession.User.Id
		}
	}
	return ""
}

type accountsCredsKeyType int

const accountsCredsKey accountsCredsKeyType = iota

// WithAccountsCredentials returns a context with an
// AccountsCredentials instance bound as a value.
func WithAccountsCredentials(ctx context.Context, creds *AccountsCredentials) context.Context {
	return context.WithValue(ctx, accountsCredsKey, creds)
}

// GetAccountsCredentials extracts an AccountsCredentials instance
// from the context, or nil if no credentials have been bound.
func GetAccountsCredentials(ctx context.Context) *AccountsCredentials {
	if value, ok := ctx.Value(accountsCredsKey).(*AccountsCredentials); ok {
		return value
	} else {
		return nil
	}
}
