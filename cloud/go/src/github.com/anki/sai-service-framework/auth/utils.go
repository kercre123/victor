package auth

import (
	"context"

	"github.com/anki/sai-token-service/model"
)

// GetUserId returns the User Id associated with a call, regardless of
// whether the call has been authenticated via a user session or a JWT
// token.
func GetUserId(ctx context.Context) string {
	if tok := model.GetAccessToken(ctx); tok != nil {
		return tok.UserId
	}

	if creds := GetAccountsCredentials(ctx); creds != nil {
		return creds.UserId()
	}

	return ""
}
