package db

import (
	"context"

	"github.com/anki/sai-token-service/model"
)

type ResultPage struct {
	Tokens []*model.Token
	Done   bool
}

type TokenSearch int

const (
	SearchByUser TokenSearch = iota
	SearchByRequestor
)

type TokenStore interface {

	// Fetch retrieves a token from the backing store by id. Returns a
	// non-nil error if the token does not exist.
	Fetch(ctx context.Context, id string) (*model.Token, error)

	// Store writes a token to the backing store. If a token with the
	// same id already exists it will be over-written. Tokens are
	// stored for up to one year, i.e. Fetch() will return the token
	// value even after it has expired.
	Store(ctx context.Context, t *model.Token) error

	// Changes the status of a token to revoked, and issues a
	// notification.
	Revoke(ctx context.Context, t *model.Token) error

	// ListTokens queries for a list of tokens associated with a
	// particular key, either a user ID or robot ID. It is a paged
	// interface, returning one page of results at a time. To fetch
	// the first page, supply nil for the prev parameter.
	ListTokens(ctx context.Context, searchBy TokenSearch, key string, prev *ResultPage) (*ResultPage, error)

	// ListRevoked lists all tokens which have been recently
	// revoked. It is a paged interface, returning one page of results
	// at a time. To fetch the first page, supply nil for the prev
	// parameter.
	ListRevoked(ctx context.Context, prev *ResultPage) (*ResultPage, error)

	// IsHealthy implements the StatusChecker interface for reporting
	// service status to monitoring systems.
	IsHealthy() error
}
