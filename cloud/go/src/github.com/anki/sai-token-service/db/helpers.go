package db

import (
	"context"
	"github.com/anki/sai-go-util/log"
)

// RevokeTokens is a helper function which searches for tokens
// matching the given criteria and revokes them all.
func RevokeTokens(ctx context.Context, store TokenStore, searchBy TokenSearch, id string) (error, int) {
	var page *ResultPage = nil
	var err error
	tokensRevoked := 0
	tokensRetrieved := 0
	pagesProcessed := 0

	alog.Debug{
		"action": "revoke_tokens",
		"status": "start",
	}.LogC(ctx)

	for {
		page, err = store.ListTokens(ctx, searchBy, id, page)
		if err != nil {
			alog.Error{
				"action": "revoke_tokens",
				"status": alog.StatusError,

				"tokens_revoked":   tokensRevoked,
				"tokens_retrieved": tokensRetrieved,
				"pages_processed":  pagesProcessed,
				"msg":              "Failed to list tokens",
				"err":              err,
			}.LogC(ctx)
			return err, tokensRevoked
		}
		tokensRetrieved += len(page.Tokens)
		for _, tok := range page.Tokens {
			if err = store.Revoke(ctx, tok); err != nil {
				alog.Error{
					"action":           "revoke_tokens",
					"status":           alog.StatusError,
					"token_id":         tok.Id,
					"tokens_revoked":   tokensRevoked,
					"tokens_retrieved": tokensRetrieved,
					"pages_processed":  pagesProcessed,
					"msg":              "Failed to revoke token",
					"err":              err,
				}.LogC(ctx)
				return err, tokensRevoked
			}
			tokensRevoked++
		}
		if page.Done {
			break
		}
	} // end for
	alog.Info{
		"action":           "revoke_tokens",
		"status":           alog.StatusOK,
		"tokens_revoked":   tokensRevoked,
		"tokens_retrieved": tokensRetrieved,
		"pages_processed":  pagesProcessed,
	}.LogC(ctx)
	return nil, tokensRevoked
}
