// +build integration

package db

import (
	"context"
	"testing"

	"github.com/anki/sai-go-util/dockerutil"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-token-service/model"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"github.com/stretchr/testify/suite"
)

func init() {
	alog.ToStdout()
	alog.SetLevelByString("DEBUG")
}

type DynamoTokenStoreSuite struct {
	dockerutil.DynamoSuite
	Store *DynamoTokenStore
	ctx   context.Context
}

func TestDynamoTokenStoreSuite(t *testing.T) {
	suite.Run(t, new(DynamoTokenStoreSuite))
}

func (s *DynamoTokenStoreSuite) SetupTest() {
	s.DynamoSuite.DeleteAllTables()
	if store, err := NewDynamoTokenStore("test-", s.DynamoSuite.Config); err != nil {
		s.T().Fatalf("Failed to initialize dynamo token store: %s", err)
	} else {
		s.Store = store
		if err = s.Store.CreateTables(); err != nil {
			s.T().Fatalf("Failed to initialize dynamo tables: %s", err)
		}
	}
	s.ctx = context.Background()
}

func (s *DynamoTokenStoreSuite) TestCreateTables() {
	require.NoError(s.T(), s.Store.db.CheckTables())
	require.Len(s.T(), s.ListTableNames(), 2)
}

func (s *DynamoTokenStoreSuite) TestStoreFetch() {
	tok := dummyToken("user", "robot")
	require.NoError(s.T(), s.Store.Store(s.ctx, tok))
	tok2, err := s.Store.Fetch(s.ctx, tok.Id)
	require.NoError(s.T(), err)
	assert.Equal(s.T(), tok, tok2)
}

func (s *DynamoTokenStoreSuite) TestRevoke() {
	t := s.T()
	tok := dummyToken("user", "robot")
	require.NoError(t, s.Store.Store(s.ctx, tok))
	require.NoError(t, s.Store.Revoke(s.ctx, tok))
	_, err := s.Store.Fetch(s.ctx, tok.Id)
	require.Error(t, err)
}

func (s *DynamoTokenStoreSuite) TestListRevoked() {
	t := s.T()

	// Store 100 tokens
	tokens := dummyTokens("user", "robot", 100)
	for _, tok := range tokens {
		require.NoError(t, s.Store.Store(s.ctx, tok))
	}

	// Verify them
	page, err := s.Store.ListTokens(s.ctx, SearchByUser, "user", nil)
	require.NoError(t, err)
	assert.Len(t, page.Tokens, 100)

	// Revoke them all and verify
	for _, tok := range tokens {
		require.NoError(t, s.Store.Revoke(s.ctx, tok))
	}
	page, err = s.Store.ListTokens(s.ctx, SearchByUser, "user", nil)
	require.NoError(t, err)
	assert.Len(t, page.Tokens, 0)

	// Verify revoked list
	page, err = s.Store.ListRevoked(s.ctx, nil)
	require.NoError(t, err)
	assert.Len(t, page.Tokens, 100)
}

func (s *DynamoTokenStoreSuite) TestListTokens_ByUser() {
	t := s.T()
	data := map[string][]*model.Token{
		"user1": dummyTokens("user1", "robot1", 10),
		"user2": dummyTokens("user2", "robot2", 12),
		"user3": dummyTokens("user3", "robot3", 23),
	}
	for _, tokens := range data {
		for _, tok := range tokens {
			require.NoError(t, s.Store.Store(s.ctx, tok))
		}
	}
	for userId, tokens := range data {
		page, err := s.Store.ListTokens(s.ctx, SearchByUser, userId, nil)
		require.NoError(t, err)
		require.NotNil(t, page)
		require.NotNil(t, page.Tokens)
		assert.Equal(t, len(tokens), len(page.Tokens))
		// assert.ElementsMatch(t, tokens, page.Tokens)
	}

	for userId, tokens := range data {
		for _, tok := range tokens {
			err := s.Store.Revoke(s.ctx, tok)
			require.NoError(t, err)
		}
		page, err := s.Store.ListTokens(s.ctx, SearchByUser, userId, nil)
		require.NoError(t, err)
		require.NotNil(t, page)
		assert.Len(t, page.Tokens, 0)
	}
}

func (s *DynamoTokenStoreSuite) TestListTokens_ByRobot() {
	t := s.T()
	data := map[string][]*model.Token{
		"robot1": dummyTokens("user1", "robot1", 10),
		"robot2": dummyTokens("user2", "robot2", 12),
		"robot3": dummyTokens("user3", "robot3", 23),
	}
	for _, tokens := range data {
		for _, tok := range tokens {
			require.NoError(t, s.Store.Store(s.ctx, tok))
		}
	}
	for robotId, tokens := range data {
		page, err := s.Store.ListTokens(s.ctx, SearchByRequestor, robotId, nil)
		require.NoError(t, err)
		require.NotNil(t, page)
		require.NotNil(t, page.Tokens)
		assert.Equal(t, len(tokens), len(page.Tokens))
		// assert.ElementsMatch(t, tokens, page.Tokens)
	}

	for robotId, tokens := range data {
		for _, tok := range tokens {
			err := s.Store.Revoke(s.ctx, tok)
			require.NoError(t, err)
		}
		page, err := s.Store.ListTokens(s.ctx, SearchByRequestor, robotId, nil)
		require.NoError(t, err)
		require.NotNil(t, page)
		assert.Len(t, page.Tokens, 0)
	}
}

func (s *DynamoTokenStoreSuite) TestRevokeHelper() {
	t := s.T()
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
			require.NoError(t, s.Store.Store(s.ctx, tok))
		}
	}

	// Verify user token counts
	for user, _ := range data {
		page, err := s.Store.ListTokens(s.ctx, SearchByUser, user, nil)
		require.NoError(t, err)
		assert.Len(t, page.Tokens, 10)
	}

	// Verify robot token counts
	page, err := s.Store.ListTokens(s.ctx, SearchByRequestor, "robot1", nil)
	require.NoError(t, err)
	assert.Len(t, page.Tokens, 20)

	page, err = s.Store.ListTokens(s.ctx, SearchByRequestor, "robot3", nil)
	require.NoError(t, err)
	assert.Len(t, page.Tokens, 10)

	page, err = s.Store.ListTokens(s.ctx, SearchByRequestor, "robot2", nil)
	require.NoError(t, err)
	assert.Len(t, page.Tokens, 0)

	// Revoke user1's tokens
	err, count := RevokeTokens(s.ctx, s.Store, SearchByUser, "user1")
	require.NoError(t, err)
	assert.Equal(t, 10, count)

	// Verify token counts
	page, err = s.Store.ListTokens(s.ctx, SearchByUser, "user1", nil)
	require.NoError(t, err)
	assert.Len(t, page.Tokens, 0)

	page, err = s.Store.ListTokens(s.ctx, SearchByUser, "user2", nil)
	require.NoError(t, err)
	assert.Len(t, page.Tokens, 10)

	page, err = s.Store.ListTokens(s.ctx, SearchByRequestor, "robot1", nil)
	require.NoError(t, err)
	assert.Len(t, page.Tokens, 10)
}
