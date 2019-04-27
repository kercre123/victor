package auth

import (
	"testing"
	"time"

	"github.com/anki/sai-go-util/metricstesting"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/suite"
)

type AccountsCredentialsCacheSuite struct {
	suite.Suite
}

func (s *AccountsCredentialsCacheSuite) SetupTest() {
	countCacheHit.Clear()
	countCacheMiss.Clear()
	countCacheUpdate.Clear()
}

func TestAccountsCredentialsCacheSuite(t *testing.T) {
	suite.Run(t, new(AccountsCredentialsCacheSuite))
}

func (s *AccountsCredentialsCacheSuite) Test_Init() {
	t := s.T()
	c := NewAccountsCredentialsCache()
	assert.NotNil(t, c.enabled)
	assert.NotNil(t, c.cacheTTL)
	assert.NotNil(t, c.cleanupInterval)
	assert.NotNil(t, c.cache)

	c = NewAccountsCredentialsCache(WithCacheEnabled(false))
	assert.False(t, *c.enabled)
	assert.Nil(t, c.cache)

	c = NewAccountsCredentialsCache(WithCacheEnabled(true))
	assert.True(t, *c.enabled)
	assert.NotNil(t, c.cache)

	c = NewAccountsCredentialsCache(
		WithCacheTTL(time.Second*30),
		WithCacheCleanupInterval(time.Second*20))
	assert.Equal(t, time.Second*30, c.cacheTTL.Duration())
	assert.Equal(t, time.Second*20, c.cleanupInterval.Duration())
}

func (s *AccountsCredentialsCacheSuite) Test_Disabled() {
	t := s.T()
	c := NewAccountsCredentialsCache(WithCacheEnabled(false))
	assert.Nil(t, c.Get("appkey", "session"))
	c.Set("appkey", "session", &AccountsCredentials{})
	assert.Nil(t, c.Get("appkey", "session"))
	metricstesting.AssertCounts(t, authMetrics, map[string]int64{
		"svc.auth.accounts.cache.hit":    0,
		"svc.auth.accounts.cache.miss":   0,
		"svc.auth.accounts.cache.update": 0,
	})
}

func (s *AccountsCredentialsCacheSuite) Test_cacheKey() {
	t := s.T()
	c := NewAccountsCredentialsCache()
	assert.Equal(t, "appkeysession", c.cacheKey("appkey", "session"))
	assert.Equal(t, "appkey", c.cacheKey("appkey", ""))
	assert.Equal(t, "session", c.cacheKey("", "session"))
}

func (s *AccountsCredentialsCacheSuite) Test_BasicCaching() {
	t := s.T()
	c := NewAccountsCredentialsCache()
	var (
		creds1 = &AccountsCredentials{}
		creds2 = &AccountsCredentials{}
		creds3 = &AccountsCredentials{}
	)
	metricstesting.AssertCounts(t, authMetrics, map[string]int64{
		"svc.auth.accounts.cache.hit":    0,
		"svc.auth.accounts.cache.miss":   0,
		"svc.auth.accounts.cache.update": 0,
	})
	assert.Nil(t, c.Get("app", "s1"))
	assert.Nil(t, c.Get("app", "s2"))
	assert.Nil(t, c.Get("app", "s3"))
	metricstesting.AssertCounts(t, authMetrics, map[string]int64{
		"svc.auth.accounts.cache.hit":    0,
		"svc.auth.accounts.cache.miss":   3,
		"svc.auth.accounts.cache.update": 0,
	})
	c.Set("app", "s1", creds1)
	c.Set("app", "s2", creds2)
	c.Set("app", "s3", creds3)
	metricstesting.AssertCounts(t, authMetrics, map[string]int64{
		"svc.auth.accounts.cache.hit":    0,
		"svc.auth.accounts.cache.miss":   3,
		"svc.auth.accounts.cache.update": 3,
	})
	assert.Equal(t, creds1, c.Get("app", "s1"))
	assert.Equal(t, creds2, c.Get("app", "s2"))
	assert.Equal(t, creds3, c.Get("app", "s3"))
	metricstesting.AssertCounts(t, authMetrics, map[string]int64{
		"svc.auth.accounts.cache.hit":    3,
		"svc.auth.accounts.cache.miss":   3,
		"svc.auth.accounts.cache.update": 3,
	})
	assert.Nil(t, c.Get("otherapp", "s1"))
	assert.Nil(t, c.Get("otherapp", "s2"))
	assert.Nil(t, c.Get("otherapp", "s3"))
	metricstesting.AssertCounts(t, authMetrics, map[string]int64{
		"svc.auth.accounts.cache.hit":    3,
		"svc.auth.accounts.cache.miss":   6,
		"svc.auth.accounts.cache.update": 3,
	})
}
