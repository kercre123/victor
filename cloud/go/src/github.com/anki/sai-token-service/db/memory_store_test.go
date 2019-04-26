package db

import (
	"context"
	"testing"
	"time"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-token-service/model"

	"github.com/nu7hatch/gouuid"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func init() {
	alog.ToStdout()
}

func TestMemoryStore(t *testing.T) {
	s := NewMemoryTokenStore()
	tok := dummyToken("a", "b")
	assert.NoError(t, s.Store(context.Background(), tok))
	tok2, err := s.Fetch(context.Background(), tok.Id)
	assert.NoError(t, err)
	assert.Equal(t, tok, tok2)
	assert.NoError(t, s.Revoke(context.Background(), tok))
	tok3, err := s.Fetch(context.Background(), tok.Id)
	assert.Error(t, err)
	assert.Nil(t, tok3)
}

func TestMemoryStore_ListRevoked(t *testing.T) {
	var (
		t1 = dummyToken("a", "b")
		t2 = dummyToken("1", "2")
		t3 = dummyToken("fred", "barney")
	)
	s := NewMemoryTokenStore()
	s.Store(context.Background(), t1)
	s.Store(context.Background(), t2)
	s.Store(context.Background(), t3)

	s.Revoke(context.Background(), t1)
	s.Revoke(context.Background(), t2)

	page, err := s.ListRevoked(context.Background(), nil)
	assert.NotNil(t, page)
	assert.NoError(t, err)
	assert.Len(t, page.Tokens, 2)
	assert.True(t, page.Done)
}

func TestMemoryStore_ListByUser(t *testing.T) {
	s := NewMemoryTokenStore()
	data := map[string][]*model.Token{
		"user1": dummyTokens("user1", "robot1", 10),
		"user2": dummyTokens("user2", "robot2", 12),
		"user3": dummyTokens("user3", "robot3", 23),
	}
	for _, tokens := range data {
		for _, tok := range tokens {
			require.NoError(t, s.Store(context.Background(), tok))
		}
	}
	for userId, tokens := range data {
		page, err := s.ListTokens(context.Background(), SearchByUser, userId, nil)
		require.NoError(t, err)
		require.NotNil(t, page)
		require.NotNil(t, page.Tokens)
		assert.Equal(t, len(tokens), len(page.Tokens))
		assert.ElementsMatch(t, tokens, page.Tokens)
	}

	for userId, tokens := range data {
		for _, tok := range tokens {
			err := s.Revoke(context.Background(), tok)
			require.NoError(t, err)
		}
		page, err := s.ListTokens(context.Background(), SearchByUser, userId, nil)
		require.NoError(t, err)
		require.NotNil(t, page)
		assert.Len(t, page.Tokens, 0)
	}
}

func TestMemoryStore_ListByRobot(t *testing.T) {
	s := NewMemoryTokenStore()
	data := map[string][]*model.Token{
		"robot1": dummyTokens("user1", "robot1", 10),
		"robot2": dummyTokens("user2", "robot2", 12),
		"robot3": dummyTokens("user3", "robot3", 23),
	}
	for _, tokens := range data {
		for _, tok := range tokens {
			require.NoError(t, s.Store(context.Background(), tok))
		}
	}
	for robotId, tokens := range data {
		page, err := s.ListTokens(context.Background(), SearchByRequestor, robotId, nil)
		require.NoError(t, err)
		require.NotNil(t, page)
		require.NotNil(t, page.Tokens)
		assert.Equal(t, len(tokens), len(page.Tokens))
		assert.ElementsMatch(t, tokens, page.Tokens)
	}

	for robotId, tokens := range data {
		for _, tok := range tokens {
			err := s.Revoke(context.Background(), tok)
			require.NoError(t, err)
		}
		page, err := s.ListTokens(context.Background(), SearchByRequestor, robotId, nil)
		require.NoError(t, err)
		require.NotNil(t, page)
		assert.Len(t, page.Tokens, 0)
	}
}

func TestMemoryStore_RevokeHelper(t *testing.T) {
	s := NewMemoryTokenStore()
	data := map[string][]*model.Token{
		"user1": dummyTokens("user1", "robot1", 10),
		"user2": dummyTokens("user2", "robot1", 10),
		"user3": dummyTokens("user3", "robot3", 10),
	}
	// Create 30 tokens
	// Each user has 10 tokens
	// robot1 has 20 tokens
	// robot3 has 10 tokens
	// robot2 does not exist
	for _, tokens := range data {
		for _, tok := range tokens {
			require.NoError(t, s.Store(context.Background(), tok))
		}
	}

	// Verify user token counts
	for user, _ := range data {
		page, err := s.ListTokens(context.Background(), SearchByUser, user, nil)
		require.NoError(t, err)
		assert.Len(t, page.Tokens, 10)
	}

	// Verify robot token counts
	page, err := s.ListTokens(context.Background(), SearchByRequestor, "robot1", nil)
	require.NoError(t, err)
	assert.Len(t, page.Tokens, 20)

	page, err = s.ListTokens(context.Background(), SearchByRequestor, "robot3", nil)
	require.NoError(t, err)
	assert.Len(t, page.Tokens, 10)

	page, err = s.ListTokens(context.Background(), SearchByRequestor, "robot2", nil)
	require.NoError(t, err)
	assert.Len(t, page.Tokens, 0)

	// Revoke user1's tokens
	err, count := RevokeTokens(context.Background(), s, SearchByUser, "user1")
	require.NoError(t, err)
	assert.Equal(t, 10, count)

	// Verify token counts
	page, err = s.ListTokens(context.Background(), SearchByUser, "user1", nil)
	require.NoError(t, err)
	assert.Len(t, page.Tokens, 0)

	page, err = s.ListTokens(context.Background(), SearchByUser, "user2", nil)
	require.NoError(t, err)
	assert.Len(t, page.Tokens, 10)

	page, err = s.ListTokens(context.Background(), SearchByRequestor, "robot1", nil)
	require.NoError(t, err)
	assert.Len(t, page.Tokens, 10)
}

func dummyTokens(userId, requestorId string, count int) []*model.Token {
	tokens := []*model.Token{}
	for i := 0; i < count; i++ {
		tokens = append(tokens, dummyToken(userId, requestorId))
	}
	return tokens
}

func dummyToken(userId, requestorId string) *model.Token {
	var (
		iat        = time.Now().UTC()
		expires    = iat.Add(time.Hour * 24)
		tokenId, _ = uuid.NewV4()
	)

	return &model.Token{
		Id:          tokenId.String(),
		Type:        "user+robot",
		RequestorId: requestorId,
		UserId:      userId,
		IssuedAt:    iat,
		ExpiresAt:   expires,
	}
}
