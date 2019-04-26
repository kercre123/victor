package db

import (
	"context"
	"github.com/anki/sai-token-service/model"
)

type CachingTokenStore struct {
	TokenStore
}

func NewCachingTokenStore(store TokenStore) *CachingTokenStore {
	return &CachingTokenStore{
		TokenStore: store,
	}
}

func (s *CachingTokenStore) Fetch(ctx context.Context, id string) (*model.Token, error) {
	// TODO: caching
	return s.TokenStore.Fetch(ctx, id)
}
