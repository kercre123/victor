package identity

import (
	"fmt"
	"math/rand"
	"os"
	"path"
	"runtime"
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

const testToken3 = "eyJhbGciOiJSUzUxMiIsInR5cCI6IkpXVCJ9.eyJleHBpcmVzIjoiMjAxOC0xMC0yNVQwNjo0Mjo0Ni40NDM2NTE2MzRaIiwiaWF0IjoiMjAxOC0xMC0yNFQwNjo0Mjo0Ni40NDM2NTE2MzRaIiwicmVxdWVzdG9yX2lkIjoidmljOjAwZTIwMTI0IiwidG9rZW5faWQiOiIzOThiMDI5YS02MDYxLTRhMjEtYjc5ZC01NzhhMzI1MmY5YmQiLCJ0b2tlbl90eXBlIjoidXNlcityb2JvdCIsInVzZXJfaWQiOiIyZ3FOTXlRaHRwY0NRTVh2cVhjckpFQSJ9.Oy2wz2OgWoEjjU0efY7kclahyFjUYgNXq5rRIHQxxK8sf0g95C8c3AQ20FWEJj9crcJLJKu0l7FtNAIEpiBMlex2FetSjvl8BCKBh4abnIsLHqSrLwxEd3pnZY5rAYBGHiY6AkOGN1JJksR46gQKhK2grwJtDa-_vso_Zg_SoFN38S9dqPGmhv5l_ypLnR1NwPIhDp_GAWEHc5N7a3TTOxKHblCMmFsxLr8_C8rgDF6Je_-xQ5Y7i9N-J0anC8woqwD9fPoXieP_cXUIzjtM7iQs9JcSRxhVb4yGyyjTHk9xBzMS_J5CG-0nmeAjMzIPuzKBY0xwz1r1_0A7M28Lmw"

type IdentityProviderSuite struct {
	suite.Suite

	certPath   string
	tokenPaths []string
}

func (s *IdentityProviderSuite) createRandomTokenPath() string {
	tokenPath := fmt.Sprintf("%s_%d", testTokenPathPrefix, rand.Intn(100000))
	s.tokenPaths = append(s.tokenPaths, tokenPath)
	return tokenPath
}

func (s *IdentityProviderSuite) findTestCertDir() string {
	// make sure we find the cert test files, regardless where we run this...
	// test certs taken from: github.com/anki/sai-token-service/integration/robotcerts
	_, filename, _, _ := runtime.Caller(1)
	return path.Join(path.Dir(filename), "testdata")
}

func (s *IdentityProviderSuite) SetupSuite() {
	rand.Seed(time.Now().UnixNano())
	s.certPath = s.findTestCertDir()
}

func (s *IdentityProviderSuite) TearDownSuite() {
	for _, tokenPath := range s.tokenPaths {
		os.Remove(tokenPath)
	}
}

func (s *IdentityProviderSuite) TestCertCommonName() {
	require := require.New(s.T())

	tokenPath := s.createRandomTokenPath()
	provider, err := NewFileProvider(tokenPath, s.certPath)
	require.NoError(err)

	s.Equal("vic:adam@anki.com:1", provider.CertCommonName())
}

func (s *IdentityProviderSuite) TestSingleInstance() {
	require := require.New(s.T())

	tokenPath := s.createRandomTokenPath()
	provider, err := NewFileProvider(tokenPath, s.certPath)
	require.NoError(err)

	err = provider.Init()
	require.NoError(err)
	s.Equal(tokenPath, provider.jwtPath)

	storedToken := provider.GetToken()
	s.Nil(storedToken)

	token, err := provider.ParseAndStoreToken(testToken1)
	require.NoError(err)
	s.NotNil(token)
	s.Equal(testToken1, token.String())
	s.Equal("test_user_1", token.UserID())

	storedToken = provider.GetToken()
	s.Equal(token, storedToken)
}

func (s *IdentityProviderSuite) TestTimestamps() {
	require := require.New(s.T())

	tokenPath := s.createRandomTokenPath()
	provider, err := NewFileProvider(tokenPath, s.certPath)
	require.NoError(err)

	err = provider.Init()
	require.NoError(err)
	s.Equal(tokenPath, provider.jwtPath)

	storedToken := provider.GetToken()
	s.Nil(storedToken)

	token, err := provider.ParseAndStoreToken(testToken3)
	require.NoError(err)
	s.NotNil(token)
	s.Equal(testToken3, token.String())
	s.Equal("2gqNMyQhtpcCQMXvqXcrJEA", token.UserID())
	s.Equal("2018-10-24 06:42:46.443651634 +0000 UTC", token.IssuedAt().String())
	s.Equal("2018-10-25 03:42:46.443651634 +0000 UTC", token.RefreshTime().String())

	storedToken = provider.GetToken()
	s.Equal(token, storedToken)
}

func (s *IdentityProviderSuite) TestMultipleInstances() {
	require := require.New(s.T())

	tokenPath1 := s.createRandomTokenPath()
	provider1, err := NewFileProvider(tokenPath1, s.certPath)
	require.NoError(err)

	err = provider1.Init()
	require.NoError(err)
	s.Equal(tokenPath1, provider1.jwtPath)

	tokenPath2 := s.createRandomTokenPath()
	provider2, err := NewFileProvider(tokenPath2, s.certPath)
	require.NoError(err)

	err = provider2.Init()
	require.NoError(err)
	s.Equal(tokenPath2, provider2.jwtPath)

	token1, err := provider1.ParseAndStoreToken(testToken1)
	require.NoError(err)
	token2, err := provider2.ParseAndStoreToken(testToken2)
	require.NoError(err)

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
