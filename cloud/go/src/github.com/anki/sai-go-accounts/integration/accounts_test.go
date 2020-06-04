// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// Integration tests for the Accounts API

package integration

import (
	"errors"
	"fmt"
	"net/http"
	"testing"
	"time"

	"github.com/anki/sai-go-accounts/integration/testutil"
	"github.com/anki/sai-go-util/strutil"
	"github.com/anki/sai-go-util/validate"
	"github.com/anki/sai-go-util/validate/validtest"
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/sqs"
)

type ClientTaskStart func(string) (*http.Response, testutil.Json, error)
type ClientTaskStatus func() (*http.Response, testutil.Json, error)

// this is for running one of the client tasks like purge-deactivated where oyu have to wait for it to complete
func runClientTask(startTask ClientTaskStart, taskStatus ClientTaskStatus) error {
	response, jdata, err := startTask("-1m")
	if err != nil {
		return err
	}
	if response.StatusCode != http.StatusOK {
		return fmt.Errorf("bad http status: %v", response.StatusCode)
	}
	if jdata == nil {
		return errors.New("no json response received")
	}
	if jdata["status"] != "Task started" {
		return errors.New("deactivate-unverified task failed to start")
	}

	timeoutChan := time.NewTimer(time.Second * 5).C
	ticker := time.NewTicker(time.Millisecond * 250)
	for {
		select {
		case <-ticker.C:
			response, jdata, err = taskStatus()
			if jdata == nil {
				continue
			}
			if jdata["status"] != "idle" {
				continue
			}
		case <-timeoutChan:
			return errors.New("timed out waiting for deactivate unverified")
		}
		ticker.Stop()
		break
	}
	return nil
}

// TestNoAutodelete performs the following steps:
// 1) Create four new accounts with random usernames
// 2) Set the no_autodelete flag on two of the accounts via SQS message
// 3) Run deactivate-unverified and check results
// 4) Deactivated one of the no_autodelete accounts, re-activate one of the deactivated accounts
// 5) Run purge-deactivated and check results
func (s *AccountsSuite) TestNoAutodelete() {
	t := s.T()

	client := s.Client()
	client.AppKey = adminAppKey
	client.SessionToken = adminSessionToken

	// 1) Create four new accounts with random usernames
	var users [4]string
	for i := 0; i < 4; i++ {
		userName := "ak" + strutil.LowerStr(10)
		password := strutil.LowerStr(10)
		userInfo := map[string]interface{}{
			"username": userName,
			"password": password,
			"email":    "test-no-autodelete-" + userName + "@example.com",
		}
		response, createJdata, err := client.CreateUser(userInfo)
		if err != nil {
			t.Fatal("Error creating account", err)
		}
		if createJdata == nil {
			t.Fatal("No json response received")
		}
		if response.StatusCode != http.StatusOK {
			// createJdata should hold a json error response
			t.Fatalf("Create did not get 200 response, got status=%q createJdata=%#v", response.Status, createJdata)
		}
		users[i] = createJdata.FieldStrQ("user", "user_id")
	}

	client.AppKey = cronAppKey
	client.SessionToken = ""
	var err error

	// notify unverified accounts twice, so they are liable to be deactivated/purged
	err = runClientTask(client.NotifyUnverified, client.NotifyUnverifiedStatus)
	if err != nil {
		t.Fatal("runClientTask failed", err)
	}

	err = runClientTask(client.NotifyUnverified, client.NotifyUnverifiedStatus)
	if err != nil {
		t.Fatal("runClientTask failed", err)
	}

	// 2) Set the no_autodelete flag on two of the accounts via SQS message
	SQSTestSuite := s.server.SQSSuite()

	// set visibility timeout to 0, this lets us "poll" the queue until it's empty
	SQSTestSuite.SQS.SetQueueAttributes(&sqs.SetQueueAttributesInput{
		QueueUrl: aws.String(s.server.SQSQueueUrl),
		Attributes: aws.StringMap(map[string]string{
			"VisibilityTimeout": "0",
		}),
	})

	// create sqs message to set no_autodelete
	msg1 := &sqs.SendMessageInput{
		QueueUrl:    aws.String(s.server.SQSQueueUrl),
		MessageBody: aws.String(`{"user_id":"` + users[0] + `","order_id":"100001014"}`),
	}
	msg2 := &sqs.SendMessageInput{
		QueueUrl:    aws.String(s.server.SQSQueueUrl),
		MessageBody: aws.String(`{"user_id":"` + users[2] + `","order_id":"100001015"}`),
	}

	// send SQS message
	_, err = SQSTestSuite.SQS.SendMessage(msg1)
	if err != nil {
		t.Fatal("couldn't send sqs message: ", err)
	}
	_, err = SQSTestSuite.SQS.SendMessage(msg2)
	if err != nil {
		t.Fatal("couldn't send sqs message: ", err)
	}

	timeoutChan := time.NewTimer(time.Second * 5).C
	ticker := time.NewTicker(time.Millisecond * 250)
	for {
		select {
		case <-ticker.C:
			result, err := SQSTestSuite.SQS.ReceiveMessage(&sqs.ReceiveMessageInput{
				QueueUrl: aws.String(s.server.SQSQueueUrl),
			})
			if err != nil {
				t.Fatal("couldn't receive message")
				break
			}
			if len(result.Messages) != 0 {
				continue
			}
		case <-timeoutChan:
			t.Fatal("timed out waiting for SQS message to be processed")
		}
		ticker.Stop()
		break
	}

	client.AppKey = adminAppKey
	client.SessionToken = adminSessionToken

	// check that the no_autodelete flag was set or not set, after the SQS message was processed
	for i, expectedVal := range map[int]bool{0: true, 1: false, 2: true, 3: false} {
		// fetch the user and check flag
		response, jdata, err := client.UserById(users[i])
		if err != nil {
			t.Fatalf("Error fetching account: %v", err)
		}
		if response.StatusCode != http.StatusOK {
			t.Fatal("bad http status")
		}

		if jdata == nil {
			t.Fatal("no json response received")
		}

		if jdata["no_autodelete"] != expectedVal {
			t.Fatalf("no_autodelete unexpected value, user %d, got %v expected %v", i, jdata["no_autodelete"], expectedVal)
		}
	}

	client.AppKey = cronAppKey
	client.SessionToken = ""

	// 3) Run deactivate-unverified and check results
	err = runClientTask(client.DeactivateUnverified, client.DeactivateUnverifiedStatus)
	if err != nil {
		t.Fatal("runClientTask failed", err)
	}

	// users[0] should be active
	// users[1] should be deactivated
	// users[2] should be active
	// users[3] should be deactivated
	response, jdata, err := client.UserById(users[0])
	if err != nil {
		t.Fatalf("Error fetching user: %v", err)
	}
	if response.StatusCode != http.StatusOK {
		t.Fatal("bad http status")
	}
	if jdata["status"] != "active" {
		t.Fatalf("users[0] was %v, but expected 'active' after deactivate-unverified", jdata["status"])
	}
	response, jdata, err = client.UserById(users[1])
	if err != nil {
		t.Fatalf("Error fetching user: %v", err)
	}
	if response.StatusCode != http.StatusOK {
		t.Fatal("bad http status")
	}
	if jdata["status"] != "deleted" {
		t.Fatalf("users[1] was %v, but expected 'deleted' after deactivate-unverified", jdata["status"])
	}
	response, jdata, err = client.UserById(users[2])
	if err != nil {
		t.Fatalf("Error fetching user: %v", err)
	}
	if response.StatusCode != http.StatusOK {
		t.Fatal("bad http status")
	}
	if jdata["status"] != "active" {
		t.Fatalf("users[2] was %v, but expected 'active' after deactivate-unverified", jdata["status"])
	}
	response, jdata, err = client.UserById(users[3])
	if err != nil {
		t.Fatalf("Error fetching user: %v", err)
	}
	if response.StatusCode != http.StatusOK {
		t.Fatal("bad http status")
	}
	if jdata["status"] != "deleted" {
		t.Fatalf("users[3] was %v, but expected 'deleted' after deactivate-unverified", jdata["status"])
	}

	// 4) Deactivate one of the no_autodelete accounts (users[2]), re-activate one of the deactivated accounts (users[3])
	client.AppKey = adminAppKey
	client.SessionToken = adminSessionToken
	response, jdata, err = client.DeleteUser(users[2])
	if err != nil {
		t.Fatal("Error reactivating account", err)
	}
	if jdata == nil {
		t.Fatal("No json response received")
	}
	if response.StatusCode != http.StatusOK {
		t.Fatalf("Create did not get 200 response, got status=%q jdata=%#v", response.Status, jdata)
	}

	response, jdata, err = client.ReactivateUser(users[3], "ak"+strutil.LowerStr(10))
	if err != nil {
		t.Fatal("Error reactivating account", err)
	}
	if jdata == nil {
		t.Fatal("No json response received")
	}
	if response.StatusCode != http.StatusOK {
		t.Fatalf("Create did not get 200 response, got status=%q jdata=%#v", response.Status, jdata)
	}

	client.AppKey = cronAppKey
	client.SessionToken = ""

	// 5) Run purge-deactivated and check results
	// at this point we should have:
	// users[0]: no_autodelete=true, deactivated=false
	// users[1]: no_autodelete=false, deactivated=true
	// users[2]: no_autodelete=true, deactivated=true
	// users[3]: no_autodelete=false, deactivated=false

	// start the user purge
	err = runClientTask(client.PurgeDeactivated, client.PurgeDeactivatedStatus)
	if err != nil {
		t.Fatal("runClientTask failed", err)
	}

	// users[0] should be "active"
	// users[1] should be "purged"
	// users[2] should be "deleted"
	// users[3] should be "active"
	response, jdata, err = client.UserById(users[0])
	if err != nil {
		t.Fatalf("Error fetching user: %v", err)
	}
	if response.StatusCode != http.StatusOK {
		t.Fatal("bad http status")
	}
	if jdata["status"] != "active" {
		t.Fatalf("users[0] was %v, but expected 'active' after purge", jdata["status"])
	}
	response, jdata, err = client.UserById(users[1])
	if err != nil {
		t.Fatalf("Error fetching user: %v", err)
	}
	if response.StatusCode != http.StatusOK {
		t.Fatal("bad http status")
	}
	if jdata["status"] != "purged" {
		t.Fatalf("users[1] was %v, but expected 'purged' after purge", jdata["status"])
	}
	response, jdata, err = client.UserById(users[2])
	if err != nil {
		t.Fatalf("Error fetching user: %v", err)
	}
	if response.StatusCode != http.StatusOK {
		t.Fatal("bad http status")
	}
	if jdata["status"] != "deleted" {
		t.Fatalf("users[2] was %v, but expected 'deleted' after purge", jdata["status"])
	}
	response, jdata, err = client.UserById(users[3])
	if err != nil {
		t.Fatalf("Error fetching user: %v", err)
	}
	if response.StatusCode != http.StatusOK {
		t.Fatal("bad http status")
	}
	if jdata["status"] != "active" {
		t.Fatalf("users[3] was %v, but expected 'active' after purge", jdata["status"])
	}
}

// TestCreateLogin performs the following steps:
// 1) Create a new account with a random username
// 2) Validate the response
// 3) Attempt to login using that username & password
// 4) Validate the returned session id is differnet from the one create generated
// 5) Delete the user account
func (s *AccountsSuite) TestCreateLogin() {
	t := s.T()
	// Create a new account and attempt to login to it
	userName := "ak" + strutil.LowerStr(10)
	password := strutil.LowerStr(30)
	driveGuestId := strutil.LowerStr(36)
	email := "test-create-login@example.com"
	userInfo := map[string]interface{}{
		"username":       userName,
		"password":       password,
		"drive_guest_id": driveGuestId,
		"given_name":     "First",
		"family_name":    "Last",
		"email":          email,
	}

	now := time.Now()
	response, createJdata, err := s.Client().CreateUser(userInfo)
	if err != nil {
		t.Fatal("Error creating account", err)
	}
	if response.StatusCode != http.StatusOK {
		// createJdata should hold a json error response
		t.Fatalf("Create did not get 200 response, got status=%q createJdata=%#v", response.Status, createJdata)
	}

	if createJdata == nil {
		t.Fatal("No json response received")
	}

	// make sure we cleanup when we're done
	defer s.DeleteUser(
		createJdata.FieldStrQ("session", "session_token"),
		createJdata.FieldStrQ("user", "user_id"),
	)

	expected := validate.Map{
		"session": validate.Map{
			"session_token": validate.LengthBetween{Min: 10, Max: 30},
			"time_expires":  validate.ParsedTimeAfter{Time: now.Add(-time.Second), Format: time.RFC3339Nano},
			"scope":         validate.Equal{Val: "user"},
		},
		"user": validate.Map{
			"drive_guest_id":    validate.Equal{Val: driveGuestId},
			"username":          validate.Equal{Val: userName},
			"given_name":        validate.Equal{Val: "First"},
			"family_name":       validate.Equal{Val: "Last"},
			"email":             validate.Equal{Val: email},
			"status":            validate.Equal{Val: "active"},
			"email_is_verified": validate.Equal{Val: false},
		},
	}

	v := validate.New()
	v.Field("", createJdata, expected)
	validtest.ApplyToTest("create user", v, t)

	// Attempt to login
	response, loginJdata, err := s.Client().NewUserSession(userName, password)
	if err != nil {
		t.Fatal("Error submitting login request", err)
	}

	if response.StatusCode != http.StatusOK {
		// createJdata should hold a json error response
		t.Fatalf("Login did not get 200 response, got status=%q createJdata=%#v", response.Status, createJdata)
	}

	// should have the same resposne as create account, but with a different session id
	v = validate.New()
	v.Field("", createJdata, expected)
	validtest.ApplyToTest("session login", v, t)

	loginSession := loginJdata["session"].(map[string]interface{})
	createSession := createJdata["session"].(map[string]interface{})

	if loginSession["session_token"] == createSession["session_token"] {
		t.Errorf("New session has same session id as create returned")
	}

	// attempt to login using userid
	userId := createJdata.FieldStrQ("user", "user_id")
	response, loginJdata, err = s.Client().NewUserIdSession(userId, password)
	if err != nil {
		t.Fatal("Error submitting login request", err)
	}

	if response.StatusCode != http.StatusOK {
		// createJdata should hold a json error response
		t.Fatalf("Login did not get 200 response, got status=%q createJdata=%#v", response.Status, createJdata)
	}

	// attempt to login using an incorrect username
	response, loginJdata, err = s.Client().NewUserSession("InvalidUsername", password)
	if err != nil {
		t.Fatal("Error submitting login request", err)
	}
	if response.StatusCode != http.StatusForbidden {
		t.Fatalf("Login with incorrect username resulted in unexpected OK, status=%q loginJdata=%#v", response.Status, loginJdata)
	}

	// attempt to login using an incorrect password
	response, loginJdata, err = s.Client().NewUserSession(userName, "InvalidPassword")
	if err != nil {
		t.Fatal("Error submitting login request", err)
	}
	if response.StatusCode != http.StatusForbidden {
		t.Fatalf("Login with incorrect password resulted in unexpected OK, status=%q loginJdata=%#v", response.Status, loginJdata)
	}
}

var getUserTests = []struct {
	name            string
	setAppKey       bool
	asSelf          bool
	expectTruncated bool
	expectedStatus  int
}{
	{"logged in user; full result", true, true, false, http.StatusOK},
	{"user fetch other account; truncate", true, false, true, http.StatusOK},
	{"no api key; denied", false, false, true, http.StatusBadRequest},
}

// Check that a logged in user fetching his own data gets an untrucated response
// and that fetching any other account, or not being logged in at all only reveals account status info
// also check that an app key is required regardless.
func (s *AccountsSuite) TestGetUser() {
	t := s.T()
	s.SeedAccounts(t)
	for _, test := range getUserTests {
		c := s.Client()
		if !test.setAppKey {
			c.AppKey = ""
		} else {
			if test.asSelf {
				c.SessionToken = s.seedAccounts[0].Token
			} else {
				c.SessionToken = s.seedAccounts[1].Token
			}
		}
		response, jdata, err := c.UserById(s.seedAccounts[0].UserId)
		if err != nil {
			t.Fatalf("test=%q Error fetching account: %v", test.name, err)
		}

		if response.StatusCode != test.expectedStatus {
			t.Errorf("test=%q expected status=%d actual=%d jdata=%v", test.name, test.expectedStatus, response.StatusCode, jdata)
			continue
		}

		if response.StatusCode != http.StatusOK {
			continue
		}

		if jdata == nil {
			t.Errorf("test=%q no json response received", test.name)
			continue
		}

		if jdata.FieldStrQ("status") != "active" {
			t.Errorf("test=%q did not get active response - jdata=%v", test.name, jdata)
		}

		if test.expectTruncated && len(jdata) > 1 {
			t.Errorf("test=%q expected truncated result, received %v", test.name, jdata)
		} else if !test.expectTruncated && len(jdata) < 2 {
			t.Errorf("test=%q expected full result, received %v", test.name, jdata)
		}

	}
}

func (s *AccountsSuite) TestLoginRateLimit() {
	t := s.T()
	usernames := []string{"ak" + strutil.LowerStr(10), "ak" + strutil.LowerStr(10)}
	password := strutil.LowerStr(30)
	for _, username := range usernames {
		driveGuestId := strutil.LowerStr(36)
		email := "test-create-login@example.com"
		userInfo := map[string]interface{}{
			"username":       username,
			"password":       password,
			"drive_guest_id": driveGuestId,
			"given_name":     "First",
			"family_name":    "Last",
			"email":          email,
		}

		response, createJdata, err := s.Client().CreateUser(userInfo)
		if err != nil {
			t.Fatal("Error creating account", err)
		}
		if response.StatusCode != http.StatusOK {
			// createJdata should hold a json error response
			t.Fatalf("Create did not get 200 response, got status=%q createJdata=%#v", response.Status, createJdata)
		}
	}

	client := s.Client()
	client.AppKey = client.RateLimitedKey()
	// 5 logins in a fast loop should succeed without error
	for i := 0; i < 8; i++ {
		response, _, err := client.NewUserSession(usernames[0], password)
		if err != nil {
			t.Fatalf("i=%d Error submitting login request: %v", i, err)
		}

		if response.StatusCode != http.StatusOK {
			// createJdata should hold a json error response
			t.Fatalf("i=%d Login did not get 200 response, got status=%q", i, response.Status)
		}
	}

	// next should hit the rate limiter
	response, _, _ := client.NewUserSession(usernames[0], password)
	if response.StatusCode != 429 {
		// createJdata should hold a json error response
		t.Fatalf("Login did not get 429 response, got status=%q", response.Status)
	}

	// A login using the other user account should not be rate limited
	response, _, _ = client.NewUserSession(usernames[1], password)
	if response.StatusCode != 200 {
		// createJdata should hold a json error response
		t.Fatalf("Login did not get 200 response, got status=%q", response.Status)
	}
}

// TestUpdateLogin performs the following steps:
// 1) Create a new account with a random username
// 2) Update the account with a new username, password, given_name and DOB and verify the response
// 3) Try to query the get endpoint; should fail as changing the password logged out the account
// 4) Delete the user account
func (s *AccountsSuite) TestUpdateLogin() {
	t := s.T()
	client := s.Client()
	// Create a new account and attempt to login to it
	userName := "ak" + strutil.LowerStr(10)
	password := strutil.LowerStr(30)
	driveGuestId := strutil.LowerStr(36)
	email := "test-update-login@example.com"
	userInfo := map[string]interface{}{
		"username":       userName,
		"password":       password,
		"drive_guest_id": driveGuestId,
		"given_name":     "First",
		"family_name":    "Last",
		"email":          email,
	}

	response, createJdata, err := s.Client().CreateUser(userInfo)
	if err != nil {
		t.Fatal("Error creating account", err)
	}
	if createJdata == nil {
		t.Fatal("No json response received")
	}
	if response.StatusCode != http.StatusOK {
		// createJdata should hold a json error response
		t.Fatalf("Create did not get 200 response, got status=%q createJdata=%#v", response.Status, createJdata)
	}

	userId := createJdata.FieldStrQ("user", "user_id")
	client.SessionToken = createJdata.FieldStrQ("session", "session_token")

	// make sure we cleanup when we're done
	defer s.DeleteUser(
		createJdata.FieldStrQ("session", "session_token"),
		createJdata.FieldStrQ("user", "user_id"),
	)

	newUsername := "ak" + strutil.LowerStr(10)
	newPassword := strutil.LowerStr(30)
	newGivenName := "NewFirst"
	newDob := "1970-01-01"
	update := map[string]interface{}{
		"username":   newUsername,
		"password":   newPassword,
		"given_name": newGivenName,
		"dob":        newDob,
	}

	response, updateJdata, err := client.UpdateUser(userId, update)
	if err != nil {
		t.Errorf("Error with update user api err=%s", err)
	}
	if response.StatusCode != http.StatusOK {
		t.Fatalf("Update user did not get 200 response, got status=%q updateJdata=%#v", response.Status, updateJdata)
	}

	response, _, err = client.UserById(userId)
	if err != nil {
		t.Fatal("Error submitting get request", err)
	}
	// As of SAIAC-235 the existing session should still be good
	if response.StatusCode != http.StatusOK {
		t.Fatalf("Get did not get 403 response, got status=%q response=%#v", response.Status, response)
	}

	// Login again to get a new session with the updated username/password
	response, loginJdata, err := s.Client().NewUserSession(newUsername, newPassword)
	if err != nil {
		t.Fatal("Error submitting login request", err)
	}

	if response.StatusCode != http.StatusOK {
		t.Fatalf("Login did not issue a 200 response, got status=%q response=%#v", response.Status, response)
	}

	// Clean up the new token since the old one is now invalid
	defer s.DeleteUser(
		loginJdata.FieldStrQ("session", "session_token"),
		createJdata.FieldStrQ("user", "user_id"),
	)
}

// TestCrossAccountUpdate performs the following steps:
// 1) Create two new accounts with random usernames
// 2) Attempt to update the first account with the session token from the second
// 3) Delete the user account
func (s *AccountsSuite) TestCrossAccountUpdate() {
	t := s.T()
	client := s.Client()

	var userId string
	for i := 0; i < 2; i++ {
		userName := "ak" + strutil.LowerStr(10)
		password := strutil.LowerStr(10)
		userInfo := map[string]interface{}{
			"username": userName,
			"password": password,
			"email":    "test-cross-account-update-" + userName + "@example.com",
		}
		response, createJdata, err := s.Client().CreateUser(userInfo)
		if err != nil {
			t.Fatal("Error creating account", err)
		}
		if createJdata == nil {
			t.Fatal("No json response received")
		}
		if response.StatusCode != http.StatusOK {
			// createJdata should hold a json error response
			t.Fatalf("Create did not get 200 response, got status=%q createJdata=%#v", response.Status, createJdata)
		}

		if i == 0 {
			// Save the first user ID
			userId = createJdata.FieldStrQ("user", "user_id")
		} else if i == 1 {
			// Save the second session token
			client.SessionToken = createJdata.FieldStrQ("session", "session_token")
		}

		// make sure we cleanup when we're done
		defer s.DeleteUser(
			createJdata.FieldStrQ("session", "session_token"),
			createJdata.FieldStrQ("user", "user_id"),
		)
	}

	newUsername := "ak" + strutil.LowerStr(10)
	update := map[string]interface{}{
		"username": newUsername,
	}

	// Try to update the first user with the second user's token
	response, updateJdata, err := client.UpdateUser(userId, update)
	if err != nil {
		t.Errorf("Error with cross account update user api err=%s", err)
	}
	if response.StatusCode != http.StatusForbidden {
		t.Fatalf("Cross account user update resulted in unexpected OK, status=%q updateJdata=%#v", response.Status, updateJdata)
	}
}

// TestSessionLogout performs the following steps:
// 1) Create a new account with a random username, save its session token
// 2) Login to the new account, save the second session token
// 3) Delete the first session token and confirm that it is inactive but the second session remains active
// 4) Delete the second session token and confirm that it is inactive
func (s *AccountsSuite) TestSessionLogout() {
	t := s.T()
	client := s.Client()

	userName := "ak" + strutil.LowerStr(10)
	password := strutil.LowerStr(10)
	userInfo := map[string]interface{}{
		"username": userName,
		"password": password,
		"email":    "test-session-logout@example.com",
	}
	response, createJdata, err := client.CreateUser(userInfo)
	if err != nil {
		t.Fatal("Error creating account", err)
	}
	if createJdata == nil {
		t.Fatal("No json response received")
	}
	if response.StatusCode != http.StatusOK {
		// createJdata should hold a json error response
		t.Fatalf("Create did not get 200 response, got status=%q createJdata=%#v", response.Status, createJdata)
	}

	// make sure we cleanup when we're done
	defer s.DeleteUser(
		createJdata.FieldStrQ("session", "session_token"),
		createJdata.FieldStrQ("user", "user_id"),
	)

	sessionOne := createJdata.FieldStrQ("session", "session_token")

	// Log in to obtain a second session
	response, loginJdata, err := client.NewUserSession(userName, password)
	if err != nil {
		t.Fatal("Error submitting login request", err)
	}
	if response.StatusCode != http.StatusOK {
		t.Fatalf("Login for session test resulted in an error, status=%q, loginJdata=%#v", response.Status, loginJdata)
	}
	defer s.DeleteUser(
		loginJdata.FieldStrQ("session", "session_token"),
		loginJdata.FieldStrQ("user", "user_id"),
	)

	sessionTwo := loginJdata.FieldStrQ("session", "session_token")

	// Validate that the first session is active
	client.SessionToken = ""
	client.AppKey = serviceAppKey
	response, validateJdata, err := client.ValidateSession(sessionOne)
	if err != nil {
		t.Fatal("Error submitting validation request", err)
	}
	if response.StatusCode != http.StatusOK {
		t.Errorf("Attempt to validate the first token should have succeeded but did not, got status=%q, validateJdata=%#v", response.Status, validateJdata)
	}

	// Delete the first session
	client.SessionToken = sessionOne
	client.AppKey = userAppKey
	response, deleteJdata, err := client.DeleteSession(sessionOne, false)
	if err != nil {
		t.Fatal("Error submitting validation request", err)
	}
	if response.StatusCode != http.StatusOK {
		t.Errorf("Attempt to delete the first token should have succeeded but did not, got status=%q, deleteJdata=%#v", response.Status, deleteJdata)
	}

	// Validate that the first session is no longer valid
	client.AppKey = serviceAppKey
	client.SessionToken = ""
	response, validateJdata, err = client.ValidateSession(sessionOne)
	if err != nil {
		t.Fatal("Error submitting validation request", err)
	}
	if response.StatusCode != http.StatusBadRequest {
		t.Errorf("Validated the session using a deleted token, got status=%q deleteJdata=%#v", response.Status, validateJdata)
	}

	// Validate that the second session is still valid
	response, validateJdata, err = client.ValidateSession(sessionTwo)
	if err != nil {
		t.Fatal("Error submitting validation request", err)
	}
	if response.StatusCode != http.StatusOK {
		t.Errorf("Unable to validate the session even though it should be valid, got status=%q deleteJdata=%#v", response.Status, validateJdata)
	}
}

// Check that the pprof http server is only exposed on the pprof port
func (s *AccountsSuite) TestPprofServer() {
	t := s.T()
	apiUrl := s.server.AccountsApiUrl
	profileUrl := s.server.ProfileUrl

	resp, err := http.Get(profileUrl + "/debug/pprof")
	if err != nil {
		t.Fatal("Failed to query profile server: ", err)
	}
	if resp.StatusCode != http.StatusOK {
		t.Errorf("Did not get 200 response from profile server, got code=%d", resp.StatusCode)
	}

	resp, err = http.Get(apiUrl + "/debug/pprof")
	if err != nil {
		t.Fatal("Failed to query api server: ", err)
	}
	if resp.StatusCode != http.StatusNotFound {
		t.Errorf("Did not get 404 response from api server, got code=%d", resp.StatusCode)
	}
}

// TestListUsers performs the following steps:
// 1) List accounts with no parameters
// 2) List accounts and filter for a specific username
// 3) Delete an account and confirm that it still shows when show_inactive=true
// 4) List accounts and limit results to 2, confirm that two results are returned with an offset
type ListUserTests struct {
	name      string
	modifiers testutil.ListUserMods
	handler   func(s *AccountsSuite, test ListUserTests, jdata testutil.Json)
}

var listUserTests = []ListUserTests{
	{name: "list all users",
		modifiers: testutil.ListUserMods{
			Status:   nil,
			Filters:  nil,
			Limit:    0,
			OrderBy:  "",
			Offset:   0,
			SortDesc: false},
		handler: nil},
	{name: "list user with email filter",
		modifiers: testutil.ListUserMods{
			Status:   nil,
			Filters:  map[string]string{"email": "seed-account-0@example.com"},
			Limit:    0,
			OrderBy:  "",
			Offset:   0,
			SortDesc: false},
		handler: listWithEmailFilter},
	{name: "list deleted user without setting status",
		modifiers: testutil.ListUserMods{
			Status:   nil,
			Filters:  map[string]string{"email": "seed-account-1@example.com"},
			Limit:    0,
			OrderBy:  "",
			Offset:   0,
			SortDesc: false},
		handler: listDeletedUser},
	{name: "list deleted user with setting status",
		modifiers: testutil.ListUserMods{
			Status:   []string{"deactivated"},
			Filters:  map[string]string{"email": "seed-account-1@example.com"},
			Limit:    0,
			OrderBy:  "",
			Offset:   0,
			SortDesc: false},
		handler: listDeletedUser},
	{name: "list deleted user with active and deactivated",
		modifiers: testutil.ListUserMods{
			Status:   []string{"active,deactivated"},
			Filters:  map[string]string{"email": "seed-account-1@example.com"},
			Limit:    0,
			OrderBy:  "",
			Offset:   0,
			SortDesc: false},
		handler: listDeletedUser},
	{name: "list with limit",
		modifiers: testutil.ListUserMods{
			Status:   nil,
			Filters:  nil,
			Limit:    2,
			OrderBy:  "",
			Offset:   0,
			SortDesc: false},
		handler: listWithLimit},
}

func (s *AccountsSuite) TestListUsers() {
	t := s.T()
	client := s.Client()
	client.AppKey = adminAppKey
	client.SessionToken = adminSessionToken
	s.SeedAccounts(t)

	// Delete one user for a later test
	s.DeleteUser(s.seedAccounts[1].Token, s.seedAccounts[1].UserId)

	for _, test := range listUserTests {
		response, jdata, err := client.ListUsers(test.modifiers)
		if err != nil {
			t.Errorf("test=%q Error listing accounts: %v", test.name, err)
			continue
		}
		if jdata == nil {
			t.Errorf("test=%q No json response received", test.name)
			continue
		}
		if response.StatusCode != http.StatusOK {
			t.Errorf("test=%q, got status=%q userListJdata=%#v", test.name, response.Status, jdata)
			continue
		}
		if test.handler != nil {
			test.handler(s, test, jdata)
		}
	}
}

// TestListUsers handlers

func listWithEmailFilter(s *AccountsSuite, test ListUserTests, jdata testutil.Json) {
	t := s.T()
	accounts, ok := jdata["accounts"].([]interface{})
	if !ok {
		t.Errorf("test=%q, expected list of accounts, got %v, jdata=%#v", test.name, jdata["accounts"], jdata)
	}
	if len(accounts) != 1 {
		t.Errorf("ListUsers with email filter returned incorrect number of results, expected 1, got %v, jdata=%#v", len(accounts), jdata)
	}
	email := accounts[0].(map[string]interface{})["email"]
	if email != test.modifiers.Filters["email"] {
		t.Errorf("ListUsers with filter did not return correct address, expected %s, got %s, jdata=%#v", test.modifiers.Filters["email"], email, jdata)
	}
}

func listDeletedUser(s *AccountsSuite, test ListUserTests, jdata testutil.Json) {
	t := s.T()
	if test.modifiers.Status == nil {
		if jdata["accounts"] != nil {
			t.Errorf("test=%q, expected no results, jdata=%#v", test.name, jdata)
		}
	} else {
		accounts, ok := jdata["accounts"].([]interface{})
		if !ok {
			t.Fatalf("test=%q, expected list of accounts, got %v, jdata=%#v", test.name, jdata["accounts"], jdata)
		}
		email := accounts[0].(map[string]interface{})["email"]
		if email != test.modifiers.Filters["email"] {
			t.Errorf("ListUsers with filter did not return correct address, expected %s, got %s, jdata=%#v", test.modifiers.Filters["email"], email, jdata)
		}
	}

}

func listWithLimit(s *AccountsSuite, test ListUserTests, jdata testutil.Json) {
	t := s.T()
	accounts, ok := jdata["accounts"].([]interface{})
	if !ok {
		t.Errorf("test=%q, expected list of accounts, got %v, jdata=%#v", test.name, jdata["accounts"], jdata)
	}
	if len(accounts) != 2 {
		t.Errorf("test=%q returned incorrect number of results, expected 2, got %v, jdata=%#v", test.name, len(accounts), jdata)
	}
	offset := jdata["offset"].(float64)
	if offset != 2 {
		t.Errorf("test=%q did not return offset, jdata=%#v", test.name, jdata)
	}
}

// Benchmarks

var (
	benchAccountsSuite *AccountsSuite
	benchServer        *testutil.AccountsDockerEnv
)

func benchSetupAccountsSuite() *AccountsSuite {
	server, client := setupEnv()
	benchServer = server
	benchAccountsSuite = new(AccountsSuite)
	benchAccountsSuite.setclient(client, server)
	return benchAccountsSuite
}

func benchTeardown() {
	if benchServer != nil {
		tearDownEnv(benchServer)
		benchServer = nil
		benchAccountsSuite = nil
	}
}

// Benchmark logging into an existing account
// run as "make integration-benchmark BENCHTEST=BenchmarkLogin"
func BenchmarkLogin(b *testing.B) {
	s := benchSetupAccountsSuite()
	defer benchTeardown()

	userName := "ak" + strutil.LowerStr(10)
	password := strutil.LowerStr(30)
	driveGuestId := strutil.LowerStr(36)
	email := "test-create-login@example.com"
	userInfo := map[string]interface{}{
		"username":       userName,
		"password":       password,
		"drive_guest_id": driveGuestId,
		"given_name":     "First",
		"family_name":    "Last",
		"email":          email,
	}

	response, createJdata, err := s.Client().CreateUser(userInfo)
	if err != nil {
		b.Fatal("Error creating account", err)
	}
	if response.StatusCode != http.StatusOK {
		// createJdata should hold a json error response
		b.Fatalf("Create did not get 200 response, got status=%q createJdata=%#v", response.Status, createJdata)
	}

	//sessionToken := createJdata.FieldStrQ("session", "session_token")
	//userId := createJdata.FieldStrQ("user", "user_id"),

	client := s.Client()
	client.AppKey = serviceAppKey

	// reset the clock ready for the main loop
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		/*
			response, _, err := client.ValidateSession(sessionToken)
			if err != nil {
				b.Fatal("Error submitting login request", err)
			}
		*/
		response, _, err := s.Client().NewUserSession(userName, password)
		if err != nil {
			b.Fatal("Error submitting login request", err)
		}

		if response.StatusCode != http.StatusOK {
			// createJdata should hold a json error response
			b.Fatalf("Login did not get 200 response, got status=%q createJdata=%#v", response.Status, createJdata)
		}
	}
}

// Benchmark validating a session
// run as "make integration-benchmark BENCHTEST=BenchmarkValidateSession"
func BenchmarkValidateSession(b *testing.B) {
	s := benchSetupAccountsSuite()
	defer benchTeardown()

	userName := "ak" + strutil.LowerStr(10)
	password := strutil.LowerStr(30)
	driveGuestId := strutil.LowerStr(36)
	email := "test-create-login@example.com"
	userInfo := map[string]interface{}{
		"username":       userName,
		"password":       password,
		"drive_guest_id": driveGuestId,
		"given_name":     "First",
		"family_name":    "Last",
		"email":          email,
	}

	response, createJdata, err := s.Client().CreateUser(userInfo)
	if err != nil {
		b.Fatal("Error creating account", err)
	}
	if response.StatusCode != http.StatusOK {
		// createJdata should hold a json error response
		b.Fatalf("Create did not get 200 response, got status=%q createJdata=%#v", response.Status, createJdata)
	}

	sessionToken := createJdata.FieldStrQ("session", "session_token")

	client := s.Client()
	client.AppKey = serviceAppKey

	// reset the clock ready for the main loop
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		response, _, err := client.ValidateSession(sessionToken)
		if err != nil {
			b.Fatal("Error submitting login request", err)
		}

		if response.StatusCode != http.StatusOK {
			// createJdata should hold a json error response
			b.Fatalf("Login did not get 200 response, got status=%q createJdata=%#v", response.Status, createJdata)
		}
	}
}
