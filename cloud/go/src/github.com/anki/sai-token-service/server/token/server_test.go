package token

import (
	"context"
	"crypto/x509"
	"crypto/x509/pkix"
	"testing"
	"time"

	"github.com/anki/sai-go-accounts/models/session"
	"github.com/anki/sai-go-util/testutils/testtime"
	"github.com/anki/sai-service-framework/auth"
	"github.com/anki/sai-service-framework/grpc/grpcsvc"
	"github.com/anki/sai-service-framework/grpc/grpcutil"
	"github.com/anki/sai-service-framework/grpc/test"
	client "github.com/anki/sai-token-service/client/token"
	"github.com/anki/sai-token-service/db"
	"github.com/anki/sai-token-service/model"
	"github.com/anki/sai-token-service/proto/tokenpb"
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/sts"

	"github.com/dgrijalva/jwt-go"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"github.com/stretchr/testify/suite"
	"google.golang.org/grpc"
)

type TokenServerSuite struct {
	grpctest.Suite
	client      client.Client
	certStore   *testCertStore
	tokenStore  db.TokenStore
	credentials *auth.AccountsCredentials
}

type testCertStore struct {
	data map[string][]byte
}

func (s *testCertStore) Store(ctx context.Context, product, robotID string, cert []byte) error {
	if s.data == nil {
		s.data = make(map[string][]byte)
	}
	s.data[product+robotID] = cert
	return nil
}

var fakeCert = &x509.Certificate{
	Subject: pkix.Name{
		CommonName: "vic:12345678",
	},
}

type fakePeerCertificateInterceptor struct{}

func (f *fakePeerCertificateInterceptor) UnaryServerInterceptor() grpc.UnaryServerInterceptor {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
		return handler(grpcutil.BindPeerCertificate(ctx, fakeCert), req)
	}
}

func (f *fakePeerCertificateInterceptor) StreamServerInterceptor() grpc.StreamServerInterceptor {
	return func(srv interface{}, stream grpc.ServerStream, info *grpc.StreamServerInfo, handler grpc.StreamHandler) error {
		return handler(srv, &grpcutil.ServerStreamWithContext{
			ServerStream: stream,
			Ctx:          grpcutil.BindPeerCertificate(stream.Context(), fakeCert),
		})
	}
}

type fakeStsGenerator struct {
}

func (s *fakeStsGenerator) NewToken(ctx context.Context, userID, requestorID string) (*sts.Credentials, error) {
	return &sts.Credentials{
		AccessKeyId: aws.String("access-" + userID + "-" + requestorID),
		Expiration:  aws.Time(time.Date(2030, 01, 02, 03, 04, 05, 0, time.UTC)),
	}, nil
}

func (s *fakeStsGenerator) Enabled() bool { return true }

func (s *TokenServerSuite) ValidateSession(ctx context.Context, appkey, session string) (*auth.AccountsCredentials, error) {
	return s.getCredentials(), nil
}

var defaultCredentials = makeCredentials("appkey", "user", "session")

func makeCredentials(appkey, user, sessionKey string) *auth.AccountsCredentials {
	return &auth.AccountsCredentials{
		AppKey: &session.ApiKey{
			Token:  appkey,
			Scopes: auth.UserScopeMask,
		},
		UserSession: &session.UserSession{
			Session: &session.Session{
				Id:     sessionKey,
				UserId: user,
				Scope:  session.UserScope,
			},
		},
	}
}

func (s *TokenServerSuite) getCredentials() *auth.AccountsCredentials {
	if s.credentials == nil {
		return defaultCredentials
	}
	return s.credentials
}

func (s *TokenServerSuite) getContext() context.Context {
	creds := s.getCredentials()
	ctx := context.Background()
	ctx = grpcutil.WithUserSessionKey(ctx, creds.UserSession.Session.Id)
	ctx = grpcutil.WithAppKey(ctx, creds.AppKey.Token)
	return ctx
}

// ----------------------------------------------------------------------
// Test Methods
// ----------------------------------------------------------------------

func (s *TokenServerSuite) SetupTest() {
	s.credentials = defaultCredentials
	s.tokenStore = db.NewMemoryTokenStore()
	s.Setup(
		grpcsvc.WithRegisterFn(func(gs *grpc.Server) {
			certStore := &testCertStore{}
			s.certStore = certStore
			tokenpb.RegisterTokenServer(gs, &Server{
				Store:       s.tokenStore,
				Signer:      &dummyTokenSigner{},
				CertStore:   certStore,
				ClientStore: &DummyClientTokenStore{},
				StsGen:      &fakeStsGenerator{},
			})
		}),
		grpcsvc.WithInterceptor(&fakePeerCertificateInterceptor{}),
		grpcsvc.WithInterceptor(
			auth.NewAccountsAuthenticator(
				auth.WithAccountsAuthenticationEnabled(true),
				auth.WithSessionValidator(s))),
		grpcsvc.WithInterceptor(
			auth.NewTokenAuthenticator(
				auth.WithTokenAuthenticationEnabled(true),
				auth.WithTokenValidator(
					client.NewValidator(client.WithKey(dummySecret))))),
	)
	s.client = client.NewClient(client.WithClientConn(s.ClientConn))
}

func TestTokenServerSuite(t *testing.T) {
	suite.Run(t, new(TokenServerSuite))
}

func (s *TokenServerSuite) TestAssociatePrimaryUser() {
	t := s.T()
	rsp, err := s.client.AssociatePrimaryUser(s.getContext(), &tokenpb.AssociatePrimaryUserRequest{
		SessionCertificate: []byte("certdata"), // TODO: Update when cert validation is implemented
	})
	require.NoError(t, err)
	require.NotNil(t, rsp)
	require.NotNil(t, rsp.Data)
	assert.NotEqual(t, "", rsp.Data.Token)
	_, err = s.client.TokenFromString(rsp.Data.Token)
	assert.NoError(t, err)
	assert.True(t, s.client.IsValidToken(rsp.Data.Token))
	assert.Nil(t, rsp.Data.StsToken, "got unexpected STS token")

	certData, ok := s.certStore.data["vic12345678"]
	require.True(t, ok)
	assert.Equal(t, []byte("certdata"), certData)
}

func (s *TokenServerSuite) TestAssociatePrimaryStsToken() {
	t := s.T()
	rsp, err := s.client.AssociatePrimaryUser(s.getContext(), &tokenpb.AssociatePrimaryUserRequest{
		SessionCertificate: []byte("certdata"), // TODO: Update when cert validation is implemented
		GenerateStsToken:   true,
	})
	require.NoError(t, err)
	require.NotNil(t, rsp)
	require.NotNil(t, rsp.Data)
	assert.Equal(t, "access-user-vic:12345678", rsp.Data.StsToken.AccessKeyId)
	assert.Equal(t, "2030-01-02T03:04:05Z", rsp.Data.StsToken.Expiration)
}

func (s *TokenServerSuite) TestDisassociatePrimaryUser() {
	rid := "vic:12345678"
	t := s.T()

	// Verify there are no tokens in the token store
	page, err := s.tokenStore.ListTokens(s.getContext(), db.SearchByRequestor, rid, nil)
	require.NoError(t, err)
	require.NotNil(t, page)
	require.Len(t, page.Tokens, 0)

	rsp, err := s.client.AssociatePrimaryUser(s.getContext(), &tokenpb.AssociatePrimaryUserRequest{})
	require.NoError(t, err)
	require.NotNil(t, rsp)
	require.NotNil(t, rsp.Data)
	require.NotEmpty(t, rsp.Data.Token)
	tokenData := rsp.Data.Token

	// Verify there is an issued token in the store for this robot
	page, err = s.tokenStore.ListTokens(s.getContext(), db.SearchByRequestor, rid, nil)
	require.NoError(t, err)
	require.NotNil(t, page)
	require.Len(t, page.Tokens, 1)

	ctx := model.WithOutboundAccessToken(s.getContext(), rsp.Data.Token)
	rsp2, err2 := s.client.DisassociatePrimaryUser(ctx, &tokenpb.DisassociatePrimaryUserRequest{})
	require.NoError(t, err2)
	require.NotNil(t, rsp2)

	// Verify that the token has been revoked
	page, err = s.tokenStore.ListTokens(ctx, db.SearchByRequestor, rid, nil)
	require.NoError(t, err)
	require.NotNil(t, page)
	require.Len(t, page.Tokens, 0)

	// Verify that the token has been revoked. Need to call the
	// ListRevokedTokens gRPC method here to get the signed form back.
	rsp3, err := s.client.ListRevokedTokens(s.getContext(), &tokenpb.ListRevokedTokensRequest{})
	require.NoError(t, err)
	require.NotNil(t, rsp3)
	require.NotNil(t, rsp3.Data)
	require.NotNil(t, rsp3.Data.Tokens)
	require.Len(t, rsp3.Data.Tokens, 1)
	require.Equal(t, tokenData, rsp3.Data.Tokens[0])
}

func (s *TokenServerSuite) TestAssociateSecondaryClient_SameUser() {
	t := s.T()

	// Use default credentials
	rsp, err := s.client.AssociatePrimaryUser(s.getContext(), &tokenpb.AssociatePrimaryUserRequest{})
	require.NoError(t, err)
	require.NotNil(t, rsp)
	require.NotNil(t, rsp.Data)
	require.NotEmpty(t, rsp.Data.Token)
	assert.True(t, s.client.IsValidToken(rsp.Data.Token))

	// Use default credentials
	rsp2, err2 := s.client.AssociateSecondaryClient(model.WithOutboundAccessToken(s.getContext(), rsp.Data.Token),
		&tokenpb.AssociateSecondaryClientRequest{
			ClientName:  "TestSuite",
			AppId:       "vicapp",
			UserSession: "session",
		})
	require.NoError(t, err2)
	require.NotNil(t, rsp2)
	require.NotNil(t, rsp2.Data)
	assert.Empty(t, rsp2.Data.Token)
	assert.NotEmpty(t, rsp2.Data.ClientToken)
}

func (s *TokenServerSuite) TestAssociateSecondaryClient_DifferentUser() {
	t := s.T()

	// Use default credentials
	rsp, err := s.client.AssociatePrimaryUser(s.getContext(), &tokenpb.AssociatePrimaryUserRequest{})
	require.NoError(t, err)
	require.NotNil(t, rsp)
	require.NotNil(t, rsp.Data)
	require.NotEmpty(t, rsp.Data.Token)
	assert.True(t, s.client.IsValidToken(rsp.Data.Token))

	// Use different credentials
	s.credentials = makeCredentials("app", "another-user", "session-whatever")
	rsp2, err2 := s.client.AssociateSecondaryClient(model.WithOutboundAccessToken(s.getContext(), rsp.Data.Token),
		&tokenpb.AssociateSecondaryClientRequest{
			ClientName: "TestSuite",
			AppId:      "vicapp",
		})
	require.Error(t, err2)
	require.Nil(t, rsp2)
}

var refreshTests = []struct {
	name       string
	jwtAge     time.Duration
	requestJwt bool
	refreshJwt bool
	requestSts bool
	refreshSts bool
}{
	{
		// fresh jwt; doesn't need renewal; don't ask for sts
		name:       "fresh-jwt-no-sts",
		jwtAge:     1 * time.Second,
		requestJwt: true,
		refreshJwt: false,
		requestSts: false,
		refreshSts: false,
	}, {
		// fresh jwt; doesn't need renewal; request sts
		name:       "fresh-jwt-gen-sts",
		jwtAge:     1 * time.Second,
		requestJwt: true,
		refreshJwt: false,
		requestSts: true,
		refreshSts: true,
	}, {
		// no lnoger fresh, but eligable for renwal
		name:       "old-jwt-gen-sts",
		jwtAge:     DefaultTokenExpiration - TokenRefreshWindowStart + time.Hour,
		requestJwt: true,
		refreshJwt: true, // should get a new JWT
		requestSts: true,
		refreshSts: true,
	},
}

func init() {
	// replace the passthrough time.Now setup used by refreshToken with something
	// test-friendly.
	refreshTokenTime = testtime.NewTestable()
}

func (s *TokenServerSuite) TestRefreshToken() {
	t := s.T()
	refreshTokenTime := refreshTokenTime.(testtime.TestableTime)

	rsp, err := s.client.AssociatePrimaryUser(s.getContext(), &tokenpb.AssociatePrimaryUserRequest{})
	require.NoError(t, err)
	require.NotNil(t, rsp)
	require.NotNil(t, rsp.Data)
	require.NotEmpty(t, rsp.Data.Token)
	assert.True(t, s.client.IsValidToken(rsp.Data.Token))
	lastJwt := rsp.Data.Token

	for _, test := range refreshTests {
		// run the test with the clock that RefreshToken uses offset by some amount
		refreshTokenTime.WithNowDelta(test.jwtAge, func() {
			ctx := model.WithOutboundAccessToken(s.getContext(), lastJwt)
			rsp2, err2 := s.client.RefreshToken(ctx, &tokenpb.RefreshTokenRequest{
				RefreshJwtTokens: test.requestJwt,
				RefreshStsTokens: test.requestSts,
			})
			require.NoError(t, err2, test.name)
			require.NotNil(t, rsp2, test.name)
			require.NotNil(t, rsp2.Data, test.name)
			assert.NotEqual(t, "", rsp2.Data.Token, test.name)

			if test.refreshJwt {
				assert.NotEqual(t, lastJwt, rsp2.Data.Token, test.name+" jwt not changed")
			} else {
				assert.Equal(t, lastJwt, rsp2.Data.Token, test.name+" jwt was changed")
			}

			if test.refreshSts {
				assert.NotNil(t, rsp2.Data.StsToken, test.name)
				if rsp2.Data.StsToken != nil {
					assert.NotEqual(t, "", rsp2.Data.StsToken.AccessKeyId, test.name)
				}
			} else {
				assert.Nil(t, rsp2.Data.StsToken, test.name)
			}
			assert.True(t, s.client.IsValidToken(rsp2.Data.Token), test.name)

			lastJwt = rsp2.Data.Token
		})
	}
}

func (s *TokenServerSuite) TestExpiredRefreshToken() {
	t := s.T()
	refreshTokenTime := refreshTokenTime.(testtime.TestableTime)
	rsp, err := s.client.AssociatePrimaryUser(s.getContext(), &tokenpb.AssociatePrimaryUserRequest{})
	require.NoError(t, err)
	require.NotNil(t, rsp)
	require.NotNil(t, rsp.Data)
	require.NotEmpty(t, rsp.Data.Token)
	assert.True(t, s.client.IsValidToken(rsp.Data.Token))
	lastJwt := rsp.Data.Token

	// run the test with the clock pushed forward past the token expiration time and
	// renewal window time
	refreshTokenTime.WithNowDelta(TokenRefreshWindowEnd+DefaultTokenExpiration+time.Hour, func() {
		ctx := model.WithOutboundAccessToken(s.getContext(), lastJwt)
		_, err2 := s.client.RefreshToken(ctx, &tokenpb.RefreshTokenRequest{
			RefreshJwtTokens: true,
		})
		assert.NotNil(t, err2)
	})
}

func (s *TokenServerSuite) TestListRevokedTokens() {
	t := s.T()
	rsp, err := s.client.ListRevokedTokens(s.getContext(), &tokenpb.ListRevokedTokensRequest{})
	require.NoError(t, err)
	require.NotNil(t, rsp)
	require.NotNil(t, rsp.Data)
	assert.Len(t, rsp.Data.Tokens, 0)
}

func (s *TokenServerSuite) TestRevokeFactoryCertificate() {
	_, err := s.client.RevokeFactoryCertificate(s.getContext(), &tokenpb.RevokeFactoryCertificateRequest{
		CertificateId: "test",
	})
	assert.Error(s.T(), err) // Currently unimplemented
}

// ----------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------

var dummySecret = []byte("secret")

// DummyTokenSigner provides a simple implementation using a static
// secret to sign tokens in unit tests.
type dummyTokenSigner struct{}

func (s *dummyTokenSigner) SignToken(token *model.Token) (string, error) {
	return token.JwtToken(jwt.SigningMethodHS256).SignedString(dummySecret)
}
