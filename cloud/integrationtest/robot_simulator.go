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

func newRobotSimulator(options *options, instanceOptions *instanceOptions) (*robotSimulator, error) {
	simulator := &robotSimulator{
		simulator:     newSimulator(),
		robotInstance: newTestableRobot(options, instanceOptions),
	}

	go simulator.robotInstance.run()

	simulator.robotInstance.waitUntilReady()

	return simulator, simulator.robotInstance.connectClients()
}

func (s *robotSimulator) logIfNoError(err error, action, format string, a ...interface{}) {
	logIfNoError(err, s.robotInstance.instanceOptions.testUserName, action, format, a...)
}

func (s *robotSimulator) testPrimaryPairingSequence() error {
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
		s.logIfNoError(err, "create_account", "Created account %v\n", json)
		if err != nil {
			return err
		}
	}

	// Step 1 & 2: User Authentication request to Accounts (user logs into Chewie)
	// Note: this is currently hardwired to the dev environment
	session, _, err := accounts.DoLogin(*options.envName, instanceOptions.testUserName, *options.testUserPassword)
	s.logIfNoError(err, "account_login", "Logged in\n")
	if err != nil {
		return err
	}
	if session != nil {
		s.logIfNoError(err, "account_login", "Logged in user %q obtained session %q\n", session.UserID, session.Token)
	} else {
		s.logIfNoError(err, "account_login", "Login returned nil session\n")
		return fmt.Errorf("Login did not return a session")
	}

	// Step 4 & 5: Switchboard sends a token request to the cloud process (no token present)
	jwtResponse, err := s.robotInstance.tokenClient.Jwt()
	s.logIfNoError(err, "token_jwt", "Token Jwt response=%v\n", jwtResponse)
	if err != nil {
		return err
	}

	// Step 6 & 9: Switchboard sends an auth request to the cloud process (with session token)
	authResponse, err := s.robotInstance.tokenClient.Auth(session.Token)
	s.logIfNoError(err, "token_auth", "Token Auth response=%v\n", authResponse)
	if err != nil {
		return err
	}

	token, err := token.NewValidator().TokenFromString(authResponse.JwtToken)
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
	s.logIfNoError(err, "jdocs_read", "JDOCS AppTokens Read response=%v\n", readResponse)
	return err
}

func (s *robotSimulator) tearDownAction() error {
	// nothing to be done
	s.logIfNoError(nil, "tear_down", "Tearing down (nothing to do)")
	return nil
}

func (s *robotSimulator) testLogCollector() error {
	s3Url, err := s.robotInstance.logcollectorClient.upload(*s.robotInstance.options.testLogFile)
	s.logIfNoError(err, "log_upload", "File uploaded, url=%q (err=%v)\n", s3Url, err)
	return err
}

func (s *robotSimulator) testJdocsReadAndWriteSettings() error {
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
	s.logIfNoError(err, "jdocs_read", "JDOCS RobotSettings Read response=%v\n", readResponse)
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
	s.logIfNoError(err, "jdocs_write", "JDOCS RobotSettings Write response=%v\n", writeResponse)
	return err
}

func (s *robotSimulator) testTokenRefresh() error {
	jwtResponse, err := s.robotInstance.tokenClient.Jwt()
	s.logIfNoError(err, "token_jwt", "Token Jwt response=%v\n", jwtResponse)
	return err
}

func (s *robotSimulator) testMicConnectionCheck() error {
	err := s.robotInstance.micClient.connectionCheck()
	s.logIfNoError(err, "mic_connection_check", "Microphone connection checked\n")
	return err
}

func (s *robotSimulator) heartBeat() error {
	s.logIfNoError(nil, "heart_beat", "Heart beat")
	return nil
}
