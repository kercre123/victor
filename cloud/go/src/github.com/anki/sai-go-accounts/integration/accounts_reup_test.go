// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// Integration tests for the Accounts API

package integration

import (
	"net/http"
	"time"

	"github.com/anki/sai-go-util/strutil"
	"github.com/anki/sai-go-util/validate"
	"github.com/anki/sai-go-util/validate/validtest"
	"github.com/kr/pretty"
)

// TestReupSession performs the following steps:
// 1) Create a new account with a random username
// 2) Validate the response
// 3) Attempt to reup the session token returned by create account
// 4) Confirm that the old session token has been invalidated
// 5) Validate the fields returned by /session/validate
// 6) Delete the user account
func (s *AccountsSuite) TestReupSession() {
	t := s.T()
	// Create a new account and attempt to login to it
	userName := "ak" + strutil.LowerStr(10)
	password := strutil.LowerStr(30)
	email := "reup-session-test@example.com"
	userInfo := map[string]interface{}{
		"username":    userName,
		"password":    password,
		"given_name":  "First",
		"family_name": "Last",
		"email":       email,
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

	// Clean up if reup fails
	defer s.DeleteUser(
		createJdata.FieldStrQ("session", "session_token"),
		createJdata.FieldStrQ("user", "user_id"),
	)

	expectedUserObject := validate.Map{
		"session": validate.Map{
			"session_token": validate.LengthBetween{Min: 10, Max: 30},
			"time_expires":  validate.ParsedTimeAfter{Time: now.Add(-time.Second), Format: time.RFC3339Nano},
			"scope":         validate.Equal{Val: "user"},
		},
		"user": validate.Map{
			"username":          validate.Equal{Val: userName},
			"given_name":        validate.Equal{Val: "First"},
			"family_name":       validate.Equal{Val: "Last"},
			"email":             validate.Equal{Val: email},
			"status":            validate.Equal{Val: "active"},
			"email_is_verified": validate.Equal{Val: false},
		},
	}

	v := validate.New()
	v.Field("", createJdata, expectedUserObject)
	validtest.ApplyToTest("create user", v, t)

	client := s.Client()
	client.SessionToken = createJdata.FieldStrQ("session", "session_token")

	// Attempt to reup token
	response, reupJdata, err := client.ReupSession()
	if err != nil {
		t.Fatal("Error submitting reup request", err)
	}
	pretty.Println(reupJdata)

	if response.StatusCode != http.StatusOK {
		t.Fatalf("Reup did not get 200 response, got status=%q reupJdata=%#v", response.Status, reupJdata)
	}

	// Need to run delete again now as the original token run by create is invalid!
	defer s.DeleteUser(
		reupJdata.FieldStrQ("session_token"),
		createJdata.FieldStrQ("user", "user_id"),
	)

	expectedReupObject := validate.Map{
		"session_token": validate.LengthBetween{Min: 10, Max: 30},
		"time_expires":  validate.ParsedTimeAfter{Time: now.Add(-time.Second), Format: time.RFC3339Nano},
	}

	// should have a new session token
	v = validate.New()
	v.Field("", reupJdata, expectedReupObject)
	validtest.ApplyToTest("session reup", v, t)

	reupSession := reupJdata["session_token"]
	createSession := createJdata["session"].(map[string]interface{})

	if reupSession == createSession["session_token"] {
		t.Errorf("Reupped session token is the same as the original token")
	}

}
