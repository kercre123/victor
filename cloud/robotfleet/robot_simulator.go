package main

import (
	"ankidev/accounts"
	"clad/cloud"
	"fmt"

	"github.com/anki/sai-token-service/client/token"
)

type robotSimulator struct {
	*simulator

	robotInstance *testableRobot
}

func newRobotSimulator(actionRegistry *actionMetricRegistry, options *options, instanceOptions *instanceOptions) (*robotSimulator, error) {
	simulator := &robotSimulator{
		simulator:     newSimulator(actionRegistry, options, instanceOptions),
		robotInstance: newTestableRobot(options, instanceOptions),
	}

	go simulator.robotInstance.run()

	simulator.robotInstance.waitUntilReady()

	return simulator, simulator.robotInstance.connectClients()
}

func (s *robotSimulator) logIfNoError(err error, action, subAction, format string, a ...interface{}) {
	logDebugIfNoError(err, s.taskID, s.requestor, action, subAction, format, a...)
}

func (s *robotSimulator) testPrimaryPairingSequence(action action) error {
	// This test action attempts to simulate the Primary Pairing Sequence as documented in the
	// sequence diagram of the corresponding section of the "Voice System & Robot Client ERD"
	// document. The steps below correspond to the steps of the document (at implementation
	// time). Not all steps are included as some entities (e.g. switchboard and gateway) are
	// not part of the test setup.

	options := s.robotInstance.options
	instanceOptions := s.robotInstance.instanceOptions

	// Step 0: Create a new user test account
	if *options.enableAccountCreation {
		json, err := createTestAccount(*options.envName, instanceOptions.testUserName, *options.testUserPassword)
		s.logIfNoError(err, action.id, "create_account", "Created account %v\n", json)
		if err != nil {
			return err
		}
	}

	// Step 1 & 2: User Authentication request to Accounts (user logs into Chewie)
	session, _, err := accounts.DoLogin(*options.envName, instanceOptions.testUserName, *options.testUserPassword)
	s.logIfNoError(err, action.id, "account_login", "Logged in\n")
	if err != nil {
		return err
	}
	if session != nil {
		s.logIfNoError(err, action.id, "account_login", "Logged in user %q obtained session %q\n", session.UserID, session.Token)
	} else {
		s.logIfNoError(err, action.id, "account_login", "Login returned nil session\n")
		return fmt.Errorf("Login did not return a session")
	}

	// Step 4 & 5: Switchboard sends a token request to the cloud process (no token present)
	jwtResponse, err := s.robotInstance.tokenClient.Jwt()
	s.logIfNoError(err, action.id, "token_jwt", "Token Jwt response=%v\n", jwtResponse)
	if err != nil {
		return err
	}

	// Step 6 & 9: Switchboard sends an auth request to the cloud process (with session token)
	authResponse, err := s.robotInstance.tokenClient.Auth(session.Token)
	if err == nil && authResponse.Error != cloud.TokenError_NoError {
		err = fmt.Errorf("token server auth error %v", authResponse.Error)
	}
	s.logIfNoError(err, action.id, "token_auth", "Token Auth response for session token=%q: %v\n", session.Token, authResponse)
	if err != nil {
		return err
	}

	token, err := token.NewValidator().TokenFromString(authResponse.JwtToken)
	s.logIfNoError(err, action.id, "token_from_string", "authResponse %v (jwt=%q)", authResponse, authResponse.JwtToken)
	if err != nil {
		return err
	}

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
	s.logIfNoError(err, action.id, "jdocs_read", "JDOCS AppTokens Read response=%v\n", readResponse)
	return err
}

func (s *robotSimulator) tearDownAction(action action) error {
	// nothing to be done
	s.logIfNoError(nil, action.id, "none", "Tearing down (nothing to do)")
	return nil
}

func (s *robotSimulator) testLogCollector(action action) error {
	s3Url, err := s.robotInstance.logcollectorClient.upload(*s.robotInstance.options.testLogFile)
	s.logIfNoError(err, action.id, "none", "File uploaded, url=%q (err=%v)\n", s3Url, err)
	return err
}

func (s *robotSimulator) testJdocsReadAndWriteSettings(action action) error {
	token, err := getCredentials(s.robotInstance.tokenClient)
	if err != nil {
		return err
	}

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
	s.logIfNoError(err, action.id, "jdocs_read", "JDOCS RobotSettings Read response=%v\n", readResponse)
	if err != nil {
		return err
	}
	if len(readResponse.Items) != 1 {
		return fmt.Errorf("Expected 1 JDOC item, got %d", len(readResponse.Items))
	}

	writeResponse, err := s.robotInstance.jdocsClient.Write(&cloud.WriteRequest{
		Account: token.UserId,
		Thing:   token.RequestorId,
		DocName: "vic.RobotSettings",
		Doc:     readResponse.Items[0].Doc,
	})
	s.logIfNoError(err, action.id, "jdocs_write", "JDOCS RobotSettings Write response=%v\n", writeResponse)
	return err
}

func (s *robotSimulator) testTokenRefresh(action action) error {
	jwtResponse, err := s.robotInstance.tokenClient.Jwt()
	s.logIfNoError(err, action.id, "none", "Token Jwt response=%v\n", jwtResponse)
	return err
}

func (s *robotSimulator) testMicConnectionCheck(action action) error {
	err := s.robotInstance.micClient.connectionCheck()
	s.logIfNoError(err, action.id, "none", "Microphone connection checked\n")
	return err
}

func (s *robotSimulator) heartBeat(action action) error {
	s.logIfNoError(nil, action.id, "none", "Heart beat")
	return nil
}
