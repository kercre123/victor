// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// Username existence tests
// TODO refactor to use a struct of funcs to reduce code duplication

package integration

import (
	"net/http"

	"github.com/anki/sai-go-util/strutil"
	"github.com/anki/sai-go-util/validate"
	"github.com/anki/sai-go-util/validate/validtest"

	. "github.com/anki/sai-go-accounts/integration/testutil"
)

// TestUsernameAvailable performs the following steps:
// 1) Generate a random username and check if it exists
// 2) Create a new account with that username
// 3) Check if the username exists
// 4) Delete the user account
// Each of the steps are done using both check username API methods
func (s *AccountsSuite) TestUsernameAvailable() {
	t := s.T()

	// Generate a random username and check if it is available
	// Do it first using method one (/1/username/:username)
	username := "ak" + strutil.LowerStr(10)
	response, _, err := s.Client().UsernameAvailable(username)
	if err != nil {
		t.Fatal("Error checking username existence method 1", err)
	}
	if response.StatusCode != http.StatusNotFound {
		t.Fatalf("Check for non-existent username using method 1 did not succeed, got status=%q", response.Status)
	}

	// Do it again using method 2 (/1/username-check?username=)
	var usernameJdata Json
	response, usernameJdata, err = s.Client().YetAnotherUsernameAvailable(username)
	if err != nil {
		t.Fatal("Error checking username existence using method 2", err)
	}
	if response.StatusCode != http.StatusOK {
		t.Fatalf("Check for non-existent username using method 2 did not succeed, got status=%q usernameJdata=%#v", response.Status, usernameJdata)
	}

	expected := validate.Map{
		"exists": validate.Equal{Val: false},
	}

	v := validate.New()
	v.Field("", usernameJdata, expected)
	validtest.ApplyToTest("User should not exist", v, t)

	// Generate remaining user details and create an account with the
	// random username
	password := strutil.LowerStr(30)
	email := "username-available-test@example.com"
	userInfo := map[string]interface{}{
		"username":    username,
		"password":    password,
		"given_name":  "First",
		"family_name": "Last",
		"email":       email,
	}

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

	// Check that the randomly generated username now exists using method 1
	response, _, err = s.Client().UsernameAvailable(username)
	if err != nil {
		t.Fatal("Error checking account existence method 1", err)
	}
	if response.StatusCode != http.StatusOK {
		t.Fatalf("Username should be available but is not method 1, got status=%q createJdata=%#v", response.Status, createJdata)
	}

	// Check that the randomly generated username now exists using method 2
	var usernameExistsJData Json
	response, usernameExistsJData, err = s.Client().YetAnotherUsernameAvailable(username)
	if err != nil {
		t.Fatal("Error checking account existence method 2", err)
	}
	if response.StatusCode != http.StatusOK {
		t.Fatalf("Username should be available but did not get OK status method 2, got status=%q createJdata=%#v", response.Status, createJdata)
	}

	// Username should now exists
	expected = validate.Map{
		"exists": validate.Equal{Val: true},
	}

	v = validate.New()
	v.Field("", usernameExistsJData, expected)
	validtest.ApplyToTest("User exists", v, t)
}

// When queried with Admin scope, /usernames/:username returns
// active and used_id fields
// 1) Generate a random username and check if it exists
// 2) Validate that the correct fields for Admin scope keys are returned
// 3) Delete the user account
func (s *AccountsSuite) TestAdminUsernameAvailable() {
	t := s.T()

	// Use Admin scope
	c := s.Client()
	c.AppKey = adminAppKey

	// Generate a random user
	username := "ak" + strutil.LowerStr(10)
	password := strutil.LowerStr(30)
	userInfo := map[string]interface{}{
		"username":    username,
		"password":    password,
		"given_name":  "First",
		"family_name": "Last",
		"email":       "gareth@example.com",
	}

	response, createJdata, err := c.CreateUser(userInfo)
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

	// Clean up when we're done
	defer s.DeleteUser(
		createJdata.FieldStrQ("session", "session_token"),
		createJdata.FieldStrQ("user", "user_id"),
	)

	// Check that the randomly generated username now exists using method 1
	var usernameExistsJdata Json
	response, usernameExistsJdata, err = c.UsernameAvailable(username)
	if err != nil {
		t.Fatal("Error checking account existence method 1", err)
	}
	if response.StatusCode != http.StatusOK {
		t.Fatalf("Username should be available but is not method 1, got status=%q createJdata=%#v", response.Status, createJdata)
	}

	UserId := createJdata.FieldStrQ("user", "user_id")
	// Username should now exist and be active
	expected := validate.Map{
		"exists":  validate.Equal{Val: true},
		"status":  validate.Equal{Val: "active"},
		"user_id": validate.Equal{Val: UserId},
	}

	v := validate.New()
	v.Field("", usernameExistsJdata, expected)
	validtest.ApplyToTest("Active user exists method 1", v, t)

	// Repeat using method 2
	response, usernameExistsJdata, err = c.YetAnotherUsernameAvailable(username)
	if err != nil {
		t.Fatal("Error checking account existence method 2", err)
	}
	if response.StatusCode != http.StatusOK {
		t.Fatalf("Username should be available but did not get OK status method 2, got status=%q createJdata=%#v", response.Status, createJdata)
	}

	v = validate.New()
	v.Field("", usernameExistsJdata, expected)
	validtest.ApplyToTest("Active user exists method 2", v, t)
}
