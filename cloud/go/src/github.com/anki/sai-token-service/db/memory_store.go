package db

import (
	"context"
	"fmt"

	"github.com/anki/sai-token-service/model"
)

type MemoryTokenStore struct {
	data    map[string]*model.Token
	revoked map[string]*model.Token
}

func NewMemoryTokenStore() *MemoryTokenStore {
	return &MemoryTokenStore{
		data:    map[string]*model.Token{},
		revoked: map[string]*model.Token{},
	}
}

func (s *MemoryTokenStore) Fetch(ctx context.Context, id string) (*model.Token, error) {
	if tok, ok := s.data[id]; ok {
		return tok, nil
	}
	return nil, fmt.Errorf("Token %s not found", id)
}

func (s *MemoryTokenStore) Store(ctx context.Context, t *model.Token) error {
	s.data[t.Id] = t
	return nil
}

func (s *MemoryTokenStore) Revoke(ctx context.Context, t *model.Token) error {
	delete(s.data, t.Id)
	s.revoked[t.Id] = t
	return nil
}

func (s *MemoryTokenStore) ListTokens(ctx context.Context, searchBy TokenSearch, key string, prev *ResultPage) (*ResultPage, error) {
	page := &ResultPage{
		Tokens: []*model.Token{},
		Done:   true,
	}
	switch searchBy {
	case SearchByRequestor:
		for _, t := range s.data {
			if t.RequestorId == key {
				page.Tokens = append(page.Tokens, t)
			}
		}
	case SearchByUser:
		for _, t := range s.data {
			if t.UserId == key {
				page.Tokens = append(page.Tokens, t)
			}
		}
	}

	return page, nil
}

func (s *MemoryTokenStore) ListRevoked(ctx context.Context, prev *ResultPage) (*ResultPage, error) {
	page := &ResultPage{
		Tokens: []*model.Token{},
		Done:   true,
	}
	for _, t := range s.revoked {
		page.Tokens = append(page.Tokens, t)
	}
	return page, nil
}

func (s *MemoryTokenStore) IsHealthy() error {
	return nil
}
