package main

// This integration test suite instantiates a robot instance in order to run some interaction
// flows (that can also be used for load testing).

import (
	"anki/robot"
	"anki/token"
	"ankidev/accounts"
	"clad/cloud"
	"testing"

	stoken "github.com/anki/sai-token-service/client/token"
	cli "github.com/jawher/mow.cli"
	"github.com/stretchr/testify/require"
	"github.com/stretchr/testify/suite"
)

type IntegrationTestSuite struct {
	suite.Suite

	options *options

	robotInstance *testableRobot
}

func (s *IntegrationTestSuite) SetupSuite() {
	app := cli.App("robot_integration_test", "Robot integration test")

	s.options = newFromEnvironment(app)

	// Enable client certs and set custom key pair dir (for this user)
	token.UseClientCert = true
	robot.DefaultCloudDir = *s.options.defaultCloudDir

	s.robotInstance = &testableRobot{}
	go s.robotInstance.run(*s.options.urlConfigFile)

	s.robotInstance.waitUntilReady()

	err := s.robotInstance.connectClients()
	require.NoError(s.T(), err)
}

func (s *IntegrationTestSuite) TearDownSuite() {
	s.robotInstance.closeClients()
}

func (s *IntegrationTestSuite) logIfNoError(err error, action, format string, a ...interface{}) {
	logIfNoError(err, *s.options.testUserName, action, format, a...)
}

func (s *IntegrationTestSuite) TestPrimaryPairingSequence() {
	require := require.New(s.T())

	// This test case attempts to simulate the Primary Pairing Sequence as documented in the
	// sequence diagram of the corresponding section of the "Voice System & Robot Client ERD"
	// document. The steps below correspond to the steps of the document (at implementation
	// time). Not all steps are included as some entities (e.g. switchboard and gateway) are
	// not part of the test setup.

	// Step 0: Create a new user test account
	if *s.options.enableAccountCreation {
		json, err := createTestAccount(*s.options.envName, *s.options.testUserName, *s.options.testUserPassword)
		s.logIfNoError(err, "create_account", "Created account %v\n", json)
		require.NoError(err)
	}

	// Step 1 & 2: User Authentication request to Accounts (user logs into Chewie)
	// Note: this is currently hardwired to the dev environment
	session, _, err := accounts.DoLogin(*s.options.envName, *s.options.testUserName, *s.options.testUserPassword)
	if session != nil {
		s.logIfNoError(err, "account_login", "Logged in user %q obtained session %q\n", session.UserID, session.Token)
	} else {
		s.logIfNoError(err, "account_login", "Login returned nil session\n")
	}
	require.NoError(err)
	require.NotNil(session)

	// Step 4 & 5: Switchboard sends a token request to the cloud process (no token present)
	jwtResponse, err := s.robotInstance.tokenClient.Jwt()
	s.logIfNoError(err, "token_jwt", "Token Jwt response=%v\n", jwtResponse)
	require.NoError(err)

	// Step 6 & 9: Switchboard sends an auth request to the cloud process (with session token)
	authResponse, err := s.robotInstance.tokenClient.Auth(session.Token)
	s.logIfNoError(err, "token_auth", "Token Auth response=%v\n", authResponse)
	require.NoError(err)
	s.Equal(cloud.TokenError_NoError, authResponse.Error)

	token, err := stoken.NewValidator().TokenFromString(authResponse.JwtToken)
	require.NoError(err)

	// Steps 11 & 12: Gateway sends a request for current client hashes stored in JDOCS
	readResponse, err := s.robotInstance.jdocsClient.Read(&cloud.ReadRequest{
		Account: session.UserID,
		Thing:   token.RequestorId,
		Items: []cloud.ReadItem{
			cloud.ReadItem{
				DocName:      "vic.AppTokens",
				MyDocVersion: 0,
			},
		},
	})
	s.logIfNoError(err, "jdocs_read", "JDOCS AppTokens Read response=%v\n", readResponse)
	require.NoError(err)
}

func (s *IntegrationTestSuite) TestLogCollector() {
	s3Url, err := s.robotInstance.logcollectorClient.upload(*s.options.testLogFile)
	s.NoError(err)

	s.logIfNoError(err, "log_upload", "File uploaded, url=%q (err=%v)\n", s3Url, err)
	require.NoError(s.T(), err)
	s.NotEmpty(s3Url)
}

func (s *IntegrationTestSuite) TestJdocsReadAndWriteSettings() {
	require := require.New(s.T())

	token, err := getCredentials(s.robotInstance.tokenClient)
	require.NoError(err)

	readResponse, err := s.robotInstance.jdocsClient.Read(&cloud.ReadRequest{
		Account: token.UserId,
		Thing:   token.RequestorId,
		Items: []cloud.ReadItem{
			cloud.ReadItem{
				DocName:      "vic.RobotSettings",
				MyDocVersion: 0,
			},
		},
	})
	s.logIfNoError(err, "jdocs_read", "JDOCS RobotSettings Read response=%v\n", readResponse)
	require.NoError(err)
	require.Len(readResponse.Items, 1)

	writeResponse, err := s.robotInstance.jdocsClient.Write(&cloud.WriteRequest{
		Account: token.UserId,
		Thing:   token.RequestorId,
		DocName: "vic.RobotSettings",
		Doc:     readResponse.Items[0].Doc,
	})
	s.logIfNoError(err, "jdocs_write", "JDOCS RobotSettings Write response=%v\n", writeResponse)
	require.NoError(err)
}

func (s *IntegrationTestSuite) TestTokenRefresh() {
	// Note: this is also tested as part of primary pairing sequence

	jwtResponse, err := s.robotInstance.tokenClient.Jwt()
	s.logIfNoError(err, "token_jwt", "Token Jwt response=%v\n", jwtResponse)
	s.NoError(err)
}

func TestIntegrationTestSuite(t *testing.T) {
	suite.Run(t, new(IntegrationTestSuite))
}
