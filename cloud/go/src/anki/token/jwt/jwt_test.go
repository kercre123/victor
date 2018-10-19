package jwt

import (
	"fmt"
	"math/rand"
	"os"
	"testing"
	"time"

	"github.com/stretchr/testify/require"
	"github.com/stretchr/testify/suite"
)

const testTokenPathPrefix = "/tmp/test_token"

// Note that these test tokens are not according to the JWT specifications (RFC 7519) because our
// implementation does only accept ISO strings for the "iat" clause where the RFC states that it MUST be
// a number containing a NumericDate value (see section 4.1.6).

const testToken1 = "eyJhbGciOiJSUzUxMiIsInR5cCI6IkpXVCJ9.eyJleHBpcmVzIjoiMjAyMC0wMS0wMVQwMDowMDowMC4wWi" +
	"IsImlhdCI6IjIwMTgtMDktMTlUMDg6NTI6MDAuODk3MDc0Njc2WiIsInJlcXVlc3Rvcl9pZCI6InZpYzowMDAwMDAwMSIsInRv" +
	"a2VuX2lkIjoiNzcxYjQ5NjgtYmM1MS0xMWU4LWI4OTgtNDdjNDI0N2VjMTg0IiwidG9rZW5fdHlwZSI6InVzZXIrcm9ib3QiLC" +
	"J1c2VyX2lkIjoidGVzdF91c2VyXzEifQ.0IoGDdAQXqrUb7V91azqZNDgGnrJfVeGpf0CLVGXS_4JB6kj35XYNT-txHlua08Em" +
	"PhcuLRrZvWVJZKcTnJF2XsReRt4Ek06kQ2fWbEQb7NjRiNGCEhW2a4t-kuCSTsGIOZdjSDVf7jGSxxlt5cVhqV2awqEmxo6NAZh" +
	"u1Go_T4GBfQuzZ5fMtA2LMU8BzMUG8TcAmWemYsUdsmvrwMUI-V97zlWbh5JcSxlpd4SsgY_4-inCODKUxy5Rz7n-MyIVRDFmUI" +
	"VsRgNP-ns1vmlM0L-tjHEgc49S5eMmLYGHa-m_pLvm1muaeGbYybB8ZLBcXcTMVau13HCd1A0lExSTg"

const testToken2 = "eyJhbGciOiJSUzUxMiIsInR5cCI6IkpXVCJ9.eyJleHBpcmVzIjoiMjAyMC0wMS0wMVQwMDowMDowMC4wWi" +
	"IsImlhdCI6IjIwMTgtMDktMTlUMDg6NTI6MDAuODk3MDc0Njc2WiIsInJlcXVlc3Rvcl9pZCI6InZpYzowMDAwMDAwMiIsInRv" +
	"a2VuX2lkIjoiZmM5ZDFjMjgtYmM1Ny0xMWU4LTk1NmUtODdkOWQ0ZTYzMzFhIiwidG9rZW5fdHlwZSI6InVzZXIrcm9ib3QiLC" +
	"J1c2VyX2lkIjoidGVzdF91c2VyXzIifQ.GKuncmbiid7vLMminwsg5ptL81jeA8DvCA8ru-n7m0EF2po2cOwCf-4pAQal0Ptj2" +
	"iyPDpUjmW2cM7GoS-YJMxB8xowQlxN-Nx_0gTyTk7p8hrjWqvF0n-2v_FFAyitpC1umMx5faq9k3tCMgBw08o4GUIUg14olnaU" +
	"Tqv_MOsDAZYNrCjCvfAbFg5jkj2YkKAFqiq4VN6WIgvSnqJORNmtIuJQ3uXf_a6BIlbvev-dTcaGL9XyGW-ZWj2P_3N8aHdL-6" +
	"lbA0Js1lbPist4dyTRXfdAP_8xZNks89oik3ZaBfRRWxhiEr4nuxMGLU1B5vvnbj1EFCxFAoOM_-uQTTw"

type IdentityProviderSuite struct {
	suite.Suite

	tokenPaths []string
}

func (s *IdentityProviderSuite) SetupSuite() {
	rand.Seed(time.Now().UnixNano())
}

func (s *IdentityProviderSuite) TearDownSuite() {
	for _, tokenPath := range s.tokenPaths {
		os.Remove(tokenPath)
	}
}

func (s *IdentityProviderSuite) createRandomTokenPath() string {
	tokenPath := fmt.Sprintf("%s_%d", testTokenPathPrefix, rand.Intn(100000))
	s.tokenPaths = append(s.tokenPaths, tokenPath)
	return tokenPath
}

func (s *IdentityProviderSuite) TestSingleInstance() {
	tokenPath := s.createRandomTokenPath()
	provider := NewIdentityProvider(tokenPath)

	err := provider.Init()
	require.NoError(s.T(), err)
	s.Equal(tokenPath, provider.path)

	storedToken := provider.GetToken()
	s.Nil(storedToken)

	token, err := provider.ParseToken(testToken1)
	require.NoError(s.T(), err)
	s.NotNil(token)
	s.Equal(testToken1, token.String())
	s.Equal("test_user_1", token.UserID())

	storedToken = provider.GetToken()
	s.Equal(token, storedToken)
}

func (s *IdentityProviderSuite) TestMultipleInstances() {
	tokenPath1 := s.createRandomTokenPath()
	provider1 := NewIdentityProvider(tokenPath1)
	err := provider1.Init()
	require.NoError(s.T(), err)
	s.Equal(tokenPath1, provider1.path)

	tokenPath2 := s.createRandomTokenPath()
	provider2 := NewIdentityProvider(tokenPath2)
	err = provider2.Init()
	require.NoError(s.T(), err)
	s.Equal(tokenPath2, provider2.path)

	token1, err := provider1.ParseToken(testToken1)
	require.NoError(s.T(), err)
	token2, err := provider2.ParseToken(testToken2)
	require.NoError(s.T(), err)

	s.NotEqual(token1.UserID(), token2.UserID())

	// force re-read from storage (flush provider)
	provider1.init()
	provider2.init()

	storedToken1 := provider1.GetToken()
	s.Equal(token1, storedToken1)

	storedToken2 := provider2.GetToken()
	s.Equal(token2, storedToken2)

	// the stored tokens should not be the same
	s.NotEqual(storedToken1.UserID(), storedToken2.UserID())

	s.NotNil(storedToken1)
	s.Equal(testToken1, storedToken1.String())
	s.Equal("test_user_1", storedToken1.UserID())

	s.NotNil(storedToken2)
	s.Equal(testToken2, storedToken2.String())
	s.Equal("test_user_2", storedToken2.UserID())
}

func TestIdentityProviderSuite(t *testing.T) {
	suite.Run(t, new(IdentityProviderSuite))
}
