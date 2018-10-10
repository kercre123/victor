package main

import (
	"anki/robot"
	"anki/token"
	"ankidev/accounts"
	"clad/cloud"
	"fmt"
	"os"
	"os/signal"
	"syscall"
)

type robotSimulator struct {
	simulator

	options *options

	robotInstance *testableRobot
}

func newRobotSimulator() (*robotSimulator, error) {
	simulator := new(robotSimulator)

	options := newFromEnvironment()
	options.printOptions()

	simulator.options = options

	// Enable client certs and set custom key pair dir (for this user)
	token.UseClientCert = true
	robot.DefaultCloudDir = options.defaultCloudDir

	simulator.robotInstance = &testableRobot{}
	go simulator.robotInstance.run(options.urlConfigFile)

	simulator.robotInstance.waitUntilReady()

	return simulator, simulator.robotInstance.connectClients()
}

func (s *robotSimulator) logIfNoError(err error, action, format string, a ...interface{}) {
	logIfNoError(err, s.options.testUserName, action, format, a...)
}

func (s *robotSimulator) testPrimaryPairingSequence() error {
	// This test action attempts to simulate the Primary Pairing Sequence as documented in the
	// sequence diagram of the corresponding section of the "Voice System & Robot Client ERD"
	// document. The steps below correspond to the steps of the document (at implementation
	// time). Not all steps are included as some entities (e.g. switchboard and gateway) are
	// not part of the test setup.

	// Step 0: Create a new user test account
	if s.options.enableAccountCreation {
		json, err := createTestAccount(s.options.envName, s.options.testUserName, s.options.testUserPassword)
		s.logIfNoError(err, "create_account", "Created account %v\n", json)
		if err != nil {
			return err
		}
	}

	// Step 1 & 2: User Authentication request to Accounts (user logs into Chewie)
	// Note: this is currently hardwired to the dev environment
	session, _, err := accounts.DoLogin(s.options.envName, s.options.testUserName, s.options.testUserPassword)
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

	token, err := parseToken(authResponse.JwtToken)
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

func (s *robotSimulator) testLogCollector() error {
	s3Url, err := s.robotInstance.logcollectorClient.upload(s.options.testLogFile)
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

func (s *robotSimulator) heartBeat() error {
	var err error
	s.logIfNoError(err, "heart_beat", "Heart beat")
	return err
}

func waitForSignal() {
	signalChannel := make(chan os.Signal, 1)
	signal.Notify(signalChannel, os.Interrupt, syscall.SIGTERM)

	<-signalChannel

	fmt.Println("Received SIGTERM signal, stopping simulator")
}

func main() {
	simulator, err := newRobotSimulator()
	simulator.logIfNoError(err, "main", "New robot created")

	if err == nil {
		return
	}

	// At startup we go through the primary pairing sequence action
	simulator.addSetupAction(simulator.testPrimaryPairingSequence)

	// After that we periodically run the following actions
	options := simulator.options
	simulator.addPeriodicAction(options.heartBeatInterval, simulator.heartBeat)
	simulator.addPeriodicAction(options.jdocsInterval, simulator.testJdocsReadAndWriteSettings)
	simulator.addPeriodicAction(options.logCollectorInterval, simulator.testLogCollector)
	simulator.addPeriodicAction(options.tokenRefreshInterval, simulator.testTokenRefresh)

	simulator.start()

	waitForSignal()

	simulator.stop()

	fmt.Println("All periodic actions have stopped, exiting")
}
