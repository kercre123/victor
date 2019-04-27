// +build integration

package auth

import (
	"context"
	"testing"

	"github.com/anki/sai-go-accounts/dockerutil"
	"github.com/anki/sai-go-util/log"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"github.com/stretchr/testify/suite"
)

func init() {
	alog.ToStdout()
}

type AccountsValidatorSuite struct {
	dockerutil.AccountsSuite
}

func TestAccountsValidatorSuite(t *testing.T) {
	suite.Run(t, new(AccountsValidatorSuite))
}

func (s *AccountsValidatorSuite) TestValidSession() {
	t := s.T()
	s.CreateUser("user1", "password", "test@example.com")
	usersession := s.NewUserSession("user1", "password")
	require.NotNil(t, usersession)
	require.NotNil(t, usersession.Session)
	sessionid := usersession.Session.Id

	validator := NewAccountsValidator(WithAccountsClient(s.GetClient(dockerutil.ServiceAppkey, "")))
	creds, err := validator.ValidateSession(context.Background(), dockerutil.UserAppkey, sessionid)
	assert.NoError(t, err)
	require.NotNil(t, creds)
	require.NotNil(t, creds.AppKey)
	require.NotNil(t, creds.UserSession)
	assert.Equal(t, usersession.Session.Id, creds.UserSession.Session.Id)
	assert.Equal(t, usersession.Session.UserId, creds.UserId())
}

func (s *AccountsValidatorSuite) TestInvalidSession() {
	t := s.T()
	validator := NewAccountsValidator(WithAccountsClient(s.GetClient(dockerutil.ServiceAppkey, "")))
	creds, err := validator.ValidateSession(context.Background(), dockerutil.UserAppkey, "invalid-session")
	assert.Error(t, err)
	require.Nil(t, creds)
}

func (s *AccountsValidatorSuite) TestInvalidAppKey() {
	t := s.T()
	sessionid := s.CreateUser("user1", "password", "test@example.com").Session.Id

	validator := NewAccountsValidator(WithAccountsClient(s.GetClient(dockerutil.ServiceAppkey, "")))
	creds, err := validator.ValidateSession(context.Background(), "invalid-app", sessionid)
	assert.Error(t, err)
	assert.Nil(t, creds)
}
