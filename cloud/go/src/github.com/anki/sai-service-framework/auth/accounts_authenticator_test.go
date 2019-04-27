// +build integration

package auth

import (
	"context"
	"testing"

	"github.com/anki/sai-go-accounts/dockerutil"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-service-framework/grpc/grpcutil"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"github.com/stretchr/testify/suite"
)

func init() {
	alog.ToStdout()
}

type AccountsAuthenticatorSuite struct {
	dockerutil.AccountsSuite
}

func TestAccountsAuthenticatorSuite(t *testing.T) {
	suite.Run(t, new(AccountsAuthenticatorSuite))
}

func makeAuthContext(appkey, sessionid string) context.Context {
	ctx := context.Background()
	ctx = grpcutil.WithAppKey(ctx, appkey)
	ctx = grpcutil.WithUserSessionKey(ctx, sessionid)
	return ctx
}

func (s *AccountsAuthenticatorSuite) makeAuthenticator() *AccountsAuthenticator {
	return NewAccountsAuthenticator(
		WithAccountsAuthenticationEnabled(true),
		WithSessionValidator(
			NewAccountsValidator(
				WithAccountsClient(
					s.GetClient(dockerutil.ServiceAppkey, "")))))
}

func (s *AccountsAuthenticatorSuite) Test_ValidAppkey_ValidSession() {
	t := s.T()
	s.CreateUser("user1", "password", "test@example.com")
	usersession := s.NewUserSession("user1", "password")

	auth := s.makeAuthenticator()
	ctx, err := auth.authenticate(makeAuthContext(dockerutil.ServiceAppkey, usersession.Session.Id))
	require.NoError(t, err)
	require.NotNil(t, ctx)
	creds := GetAccountsCredentials(ctx)
	require.NotNil(t, creds)
	require.NotNil(t, creds.UserSession)
	require.NotNil(t, creds.AppKey)
	assert.Equal(t, usersession.Session.Id, creds.UserSession.Session.Id)
	assert.Equal(t, usersession.Session.UserId, creds.UserId())
}

func (s *AccountsAuthenticatorSuite) Test_ValidAppkey_NoSession() {
	t := s.T()
	auth := s.makeAuthenticator()
	ctx, err := auth.authenticate(makeAuthContext(dockerutil.UserAppkey, ""))
	require.NoError(t, err)
	require.NotNil(t, ctx)
	creds := GetAccountsCredentials(ctx)
	require.NotNil(t, creds)
	require.Nil(t, creds.UserSession)
	require.NotNil(t, creds.AppKey)
}

func (s *AccountsAuthenticatorSuite) Test_NoAppkey() {
	t := s.T()
	s.CreateUser("user1", "password", "test@example.com")
	usersession := s.NewUserSession("user1", "password")

	auth := s.makeAuthenticator()
	_, err := auth.authenticate(makeAuthContext("", usersession.Session.Id))
	require.Error(t, err)
}

func (s *AccountsAuthenticatorSuite) Test_NoAppkey_NoSession() {
	t := s.T()
	auth := s.makeAuthenticator()
	ctx, err := auth.authenticate(makeAuthContext("", ""))
	require.NoError(t, err)
	require.NotNil(t, ctx)
	creds := GetAccountsCredentials(ctx)
	require.Nil(t, creds)
}
