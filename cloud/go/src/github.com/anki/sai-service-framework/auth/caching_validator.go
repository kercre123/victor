package auth

import (
	"context"

	"github.com/jawher/mow.cli"
)

// CachingSessionValidator wraps an existing validator and adds a
// caching layer on top.
type CachingSessionValidator struct {
	wrapped SessionValidator
	cache   *AccountsCredentialsCache
}

func NewCachingSessionValidator(wrapped SessionValidator) *CachingSessionValidator {
	return &CachingSessionValidator{
		wrapped: wrapped,
		cache:   NewAccountsCredentialsCache(),
	}
}

func (v *CachingSessionValidator) CommandSpec() string {
	return v.cache.CommandSpec()
}

func (v *CachingSessionValidator) CommandInitialize(cmd *cli.Cmd) {
	v.cache.CommandInitialize(cmd)
}

func (v *CachingSessionValidator) ValidateSession(ctx context.Context, appkey, session string) (*AccountsCredentials, error) {
	if creds := v.cache.Get(appkey, session); creds != nil {
		return creds, nil
	} else {
		if creds, err := v.wrapped.ValidateSession(ctx, appkey, session); err != nil {
			return nil, err
		} else {
			v.cache.Set(appkey, session, creds)
			return creds, nil
		}
	}
}
