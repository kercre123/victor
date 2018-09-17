package integrationtest

// This integration test suite instantiates a robot instance in order to run some interaction
// flows (that can also be used for load testing).

import (
	"ankidev/accounts"
	"clad/cloud"
	"fmt"
	"testing"

	"github.com/anki/sai-token-service/model"
	jwt "github.com/dgrijalva/jwt-go"
	"github.com/stretchr/testify/require"
	"github.com/stretchr/testify/suite"
)

const (
	testUserID       = "joep.vangassel@anki.com"
	testUserPassword = "ankisecret"

	testLogFile   = "/var/log/syslog"
	urlConfigFile = "integrationtest/server_config.json"
)

func logIfNoError(err error, format string, a ...interface{}) {
	if err != nil {
		fmt.Println("Error", err)
	} else {
		fmt.Printf(format, a...)
	}
}

// Copied from "anki/token/jwt" (unexported)
func parseToken(token string) (*model.Token, error) {
	t, _, err := new(jwt.Parser).ParseUnverified(token, jwt.MapClaims{})
	if err != nil {
		return nil, err
	}
	tok, err := model.FromJwtToken(t)
	if err != nil {
		return nil, err
	}
	return tok, nil
}

type IntegrationTestSuite struct {
	suite.Suite

	robotInstance *testableRobot
}

func (s *IntegrationTestSuite) SetupSuite() {

	s.robotInstance = &testableRobot{}
	go s.robotInstance.run(urlConfigFile)

	s.robotInstance.waitUntilReady()

	err := s.robotInstance.connectClients()
	require.NoError(s.T(), err)
}

func (s *IntegrationTestSuite) TearDownSuite() {
	s.robotInstance.closeClients()
}

func (s *IntegrationTestSuite) getCredentials() (*model.Token, error) {
	jwtResponse, err := s.robotInstance.tokenClient.Jwt()
	if err != nil {
		return nil, err
	}

	return parseToken(jwtResponse.JwtToken)
}

func (s *IntegrationTestSuite) TestPrimaryPairingSequence() {
	require := require.New(s.T())

	// This test case attempts to simulate the Primary Pairing Sequence as documented in the
	// sequence diagram of the corresponding section of the "Voice System & Robot Client ERD"
	// document. The steps below correspond to the steps of the document (at implementation
	// time). Not all steps are included as some entities (e.g. switchboard and gateway) are
	// not part of the test setup.

	// Step 1 & 2: User Authentication request to Accounts (user logs into Chewie)
	// Note: this is currently hardwired to the dev environment
	session, _, err := accounts.DoLogin(testUserID, testUserPassword)
	require.NoError(err)
	logIfNoError(err, "Logged in %q obtained session token %q\n", session.UserID, session.Token)

	// Step 4 & 5: Switchboard sends a token request to the cloud process (no token present)
	jwtResponse, err := s.robotInstance.tokenClient.Jwt()
	require.NoError(err)
	logIfNoError(err, "Token Jwt response=%v\n", jwtResponse)

	// Step 6 & 9: Switchboard sends an auth request to the cloud process (with session token)
	authResponse, err := s.robotInstance.tokenClient.Auth(session.Token)
	require.NoError(err)
	logIfNoError(err, "Token Auth response=%v\n", authResponse)
	s.Equal(cloud.TokenError_NoError, authResponse.Error)

	token, err := parseToken(authResponse.JwtToken)
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
	require.NoError(err)

	logIfNoError(err, "JDOCS AppTokens Read response=%v\n", readResponse)
}

func (s *IntegrationTestSuite) TestLogCollector() {
	s3Url, err := s.robotInstance.logcollectorClient.upload(testLogFile)
	s.NoError(err)

	logIfNoError(err, "File uploaded, url=%q (err=%v)", s3Url, err)
	require.NoError(s.T(), err)
	s.NotEmpty(s3Url)
}

func (s *IntegrationTestSuite) JdocsReadAndWriteSettings() {
	require := require.New(s.T())

	token, err := s.getCredentials()
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
	require.NoError(err)
	logIfNoError(err, "JDOCS RobotSettings Read response=%v\n", readResponse)
	require.Len(readResponse.Items, 1)

	writeResponse, err := s.robotInstance.jdocsClient.Write(&cloud.WriteRequest{
		Account: token.UserId,
		Thing:   token.RequestorId,
		DocName: "vic.RobotSettings",
		Doc:     readResponse.Items[0].Doc,
	})
	require.NoError(err)
	logIfNoError(err, "JDOCS RobotSettings Write response=%v\n", writeResponse)
}

func (s *IntegrationTestSuite) TestTokenRefresh() {
	// Note: this is also tested as part of primary pairing sequence

	jwtResponse, err := s.robotInstance.tokenClient.Jwt()
	s.NoError(err)
	logIfNoError(err, "Token Jwt response=%v\n", jwtResponse)
}

func TestIntegrationTestSuite(t *testing.T) {
	suite.Run(t, new(IntegrationTestSuite))
}
