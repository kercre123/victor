package auth

import (
	"context"
	"testing"

	"github.com/anki/sai-go-util/metricstesting"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/suite"
)

type CachingSessionValidatorSuite struct {
	suite.Suite
}

func (s *CachingSessionValidatorSuite) SetupTest() {
	countCacheHit.Clear()
	countCacheMiss.Clear()
	countCacheUpdate.Clear()
}

func TestCachingSessionValidatorSuite(t *testing.T) {
	suite.Run(t, new(CachingSessionValidatorSuite))
}

type testSessionValidator struct {
	validateSessionCount int
}

func (v *testSessionValidator) ValidateSession(ctx context.Context, appkey, session string) (*AccountsCredentials, error) {
	v.validateSessionCount++
	return &AccountsCredentials{}, nil
}

func (s *CachingSessionValidatorSuite) Test_Init() {
	t := s.T()
	v := &testSessionValidator{}
	validator := NewCachingSessionValidator(v)
	assert.NotNil(t, validator)
	assert.NotNil(t, validator.cache)
	assert.Equal(t, v, validator.wrapped)
}

func (s *CachingSessionValidatorSuite) Test_Caching() {
	t := s.T()

	validator := &testSessionValidator{}
	assert.Equal(t, 0, validator.validateSessionCount)
	metricstesting.AssertCounts(t, authMetrics, map[string]int64{
		"svc.auth.accounts.cache.hit":    0,
		"svc.auth.accounts.cache.miss":   0,
		"svc.auth.accounts.cache.update": 0,
	})

	v := NewCachingSessionValidator(validator)
	for i := 0; i < 100; i++ {
		_, err := v.ValidateSession(context.Background(), "a1", "s1")
		assert.NoError(t, err)
	}
	assert.Equal(t, 1, validator.validateSessionCount)
	metricstesting.AssertCounts(t, authMetrics, map[string]int64{
		"svc.auth.accounts.cache.hit":    99,
		"svc.auth.accounts.cache.miss":   1,
		"svc.auth.accounts.cache.update": 1,
	})

	for i := 0; i < 100; i++ {
		_, err := v.ValidateSession(context.Background(), "a1", "s2")
		assert.NoError(t, err)
	}
	assert.Equal(t, 2, validator.validateSessionCount)
	metricstesting.AssertCounts(t, authMetrics, map[string]int64{
		"svc.auth.accounts.cache.hit":    198,
		"svc.auth.accounts.cache.miss":   2,
		"svc.auth.accounts.cache.update": 2,
	})

	for i := 0; i < 100; i++ {
		_, err := v.ValidateSession(context.Background(), "a2", "s1")
		assert.NoError(t, err)
	}
	assert.Equal(t, 3, validator.validateSessionCount)
	metricstesting.AssertCounts(t, authMetrics, map[string]int64{
		"svc.auth.accounts.cache.hit":    297,
		"svc.auth.accounts.cache.miss":   3,
		"svc.auth.accounts.cache.update": 3,
	})
}
