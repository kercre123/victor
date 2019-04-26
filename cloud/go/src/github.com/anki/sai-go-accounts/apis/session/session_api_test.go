// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

package session

import (
	"net/http"
	"net/url"
	"strconv"
	"testing"

	"github.com/anki/sai-go-accounts/apierrors"
	"github.com/anki/sai-go-accounts/apis"
	"github.com/anki/sai-go-accounts/db"
	"github.com/anki/sai-go-accounts/models/session"
	"github.com/anki/sai-go-accounts/validate"
	"github.com/anki/sai-go-util/jsonutil"

	"github.com/stretchr/testify/suite"
)

func init() {
}

func runLoginAPI(us *session.UserSession, params url.Values) (req *http.Request, record *jsonutil.JsonWriterRecorder) {
	return apitest.RunRequest("match", us, "POST", "/1/sessions", nil, params)
}

func runDeleteAPI(us *session.UserSession, params url.Values) (req *http.Request, record *jsonutil.JsonWriterRecorder) {
	return apitest.RunRequest("match", us, "POST", "/1/sessions/delete", nil, params)
}

func runReupAPI(us *session.UserSession) (req *http.Request, record *jsonutil.JsonWriterRecorder) {
	return apitest.RunRequest("match", us, "POST", "/1/sessions/reup", nil, nil)
}

func runValidateAPI(apiKey string, us *session.UserSession, params url.Values) (req *http.Request, record *jsonutil.JsonWriterRecorder) {
	return apitest.RunRequest(apiKey, us, "POST", "/1/sessions/validate", nil, params)
}

type SessionAPISuite struct {
	// APITestSuite does  DB setup/teardown per test
	// and creates a standard set of test api keys and users with sessions
	apitest.APITestSuite
}

func TestSessionAPISuite(t *testing.T) {
	suite.Run(t, new(SessionAPISuite))
}

func (s *SessionAPISuite) SetupTest() {
	s.APITestSuite.SetupTest()
	RegisterAPI()
}

// missing password, invalid password, missing token, unexpected error, ok

func (s *SessionAPISuite) TestUsernameLoginNoParams() {
	t := s.T()
	_, resp := runLoginAPI(nil, nil)
	if resp.ErrorResponse != apierrors.MissingParameter {
		t.Errorf("Didn't get correct error response expected=%v actual=%v",
			apierrors.MissingParameter, resp.ErrorResponse)
	}
}

var loginErrTests = []struct {
	name          string
	apitoken      string
	password      string
	expectedError error
}{
	//{"no-api-key", "", "hashme", apierrors.MissingApiKey},
	{"no-password", "user-key", "", apierrors.MissingParameter},
	{"baduser", "user-key", "wrongpassword", apierrors.InvalidUser},
}

func (s *SessionAPISuite) TestPasswordLoginErrors() {
	t := s.T()
	for _, test := range loginErrTests {
		frm := url.Values{
			"username": {"user1"},
			"password": {test.password},
		}
		_, resp := runLoginAPI(nil, frm)

		if resp.ErrorResponse == nil {
			t.Errorf("test=%q no error response received", test.name)
			continue
		}

		if resp.ErrorResponse != test.expectedError {
			t.Errorf("test=%q expected_error=%#v actual=%#v",
				test.name, test.expectedError, resp.ErrorResponse)
			continue
		}
	}
}

func (s *SessionAPISuite) TestPasswordLoginOk() {
	// test logging in usign a username or a userid
	t := s.T()

	loginAs := s.Users["u1-unlinked"]
	for _, key := range []string{"username", "userid"} {
		val := *loginAs.User.Username
		if key == "userid" {
			val = loginAs.User.Id
		}
		frm := url.Values{
			key:        {val},
			"password": {"hashme"},
		}
		_, resp := runLoginAPI(nil, frm)

		if resp.ErrorResponse != nil {
			t.Fatalf("Unexpected error after login %v", resp.ErrorResponse)
		}

		if resp.OkResponse == nil {
			t.Fatalf("ok response not set")
		}

		us, ok := resp.OkResponse.(*session.UserSession)
		if !ok {
			t.Fatalf("didn't get a usersession response back: %#v", resp.OkResponse)
		}
		if *us.User.Username != *loginAs.User.Username {
			t.Errorf("Username mismatch expected=%q actual=%q",
				*loginAs.User.Username, *us.User.Username)
		}
	}
}


var deleteTests = []struct {
	deleteRelated bool
}{{true}, {false}}

func (s *SessionAPISuite) TestDeleteSession() {
	t := s.T()

	ua1 := s.Users["u1-unlinked"].User

	for _, test := range deleteTests {
		s1, _ := session.NewUserSession(ua1)
		s2, _ := session.NewUserSession(ua1)

		frm := url.Values{
			"session_token":  {s1.Session.Token()},
			"delete_related": {strconv.FormatBool(test.deleteRelated)},
		}
		_, resp := runDeleteAPI(s1, frm)

		if resp.ErrorResponse != nil {
			t.Fatalf("Unexpected error after delete %v", resp.ErrorResponse)
		}

		v1, _ := session.ByToken(s1.Session.Token())
		v2, _ := session.ByToken(s2.Session.Token())
		if v1 != nil {
			t.Errorf("session1 still active! %#v", v1)
		}
		if test.deleteRelated && v2 != nil {
			t.Errorf("session2 still active when delete_releted=true! %#v", v1)
		} else if !test.deleteRelated && v2 == nil {
			t.Errorf("session2 not active when delete_related=fales!")
		}
	}
}

var deletePermissionTests = []struct {
	name          string
	activeUser    string
	targetUser    string
	expectedError error
}{
	{"logged-out", "", "u1-unlinked",
		apierrors.InvalidSession}, // Logged out users can't delete any sessions
	{"user-delete-other", "u2-unlinked", "u1-unlinked",
		apierrors.InvalidSessionPerms}, // Users can't delete other user's sessions
	{"user-delete-self", "u1-unlinked", "u1-unlinked", nil}, // Users can delete their own sessions
	{"admin-delete-any", "admin", "u2-unlinked", nil},       // Admins can delete any session
}

func (s *SessionAPISuite) TestDeletePermissions() {
	t := s.T()

	for _, test := range deletePermissionTests {
		activeSession := s.Users[test.activeUser]
		targetUser := s.Users[test.targetUser]

		frm := url.Values{
			"session_token": {targetUser.Session.Token()},
		}
		_, resp := runDeleteAPI(activeSession, frm)

		if test.expectedError == nil {
			if resp.ErrorResponse != nil {
				t.Fatalf("test=%q Unexpected error after delete %v",
					test.name, resp.ErrorResponse)
			}

			// Check delete actually happened
			if v1, _ := session.ByToken(targetUser.Session.Token()); v1 != nil {
				t.Errorf("test=%q session1 still active! %#v", test.name, v1)
			}

		} else {
			if resp.ErrorResponse != test.expectedError {
				t.Errorf("test=%q incorrect error expected=%#v actual=%#v",
					test.name, test.expectedError, resp.ErrorResponse)
				continue
			}
			// check delete didn't happen anyway
			if v1, _ := session.ByToken(targetUser.Session.Token()); v1 == nil {
				t.Errorf("test=%q session was deleted when it shouldn't of been", test.name)
			}
		}
	}
}

var missingTokenTests = []struct {
	sessionUser  string
	endpointPath string
}{
	{"u1-unlinked", "/1/sessions/delete"}, // delete requires a logged in session
}

func (s *SessionAPISuite) TestMissingToken() {
	// don't send the session_token form value
	t := s.T()
	for _, test := range missingTokenTests {
		var us *session.UserSession = nil
		if test.sessionUser != "" {
			us = s.Users[test.sessionUser]
		}
		_, resp := apitest.RunRequest("user-key", us, "POST", test.endpointPath, nil, nil)

		if resp.ErrorResponse == nil {
			t.Errorf("No error received for endpoint=%q", test.endpointPath)
			continue
		}
		if resp.ErrorResponse != apierrors.MissingParameter {
			t.Errorf("Incorrect error received endpoint=%q expected=%v actual=%v",
				test.endpointPath, apierrors.MissingParameter, resp.ErrorResponse)
		}
	}
}

var invalidTokenTests = []struct {
	sessionUser  string
	endpointPath string
}{
	{"", "/1/sessions/delete"}, // delete requires a logged in session
}

func (s *SessionAPISuite) TestInvalidToken() {
	t := s.T()
	apiKey := session.ApiKey{
		Token:  "user-key",
		Scopes: session.NewScopeMask(session.UserScope),
	}
	db.Dbmap.Insert(&apiKey)
	for _, test := range invalidTokenTests {
		frm := url.Values{
			"session_token": {"invalid"},
		}
		var us *session.UserSession = nil
		if test.sessionUser != "" {
			us = s.Users[test.sessionUser]
		}
		_, resp := apitest.RunRequest(
			"user-key", us, "POST", test.endpointPath, nil, frm)

		if resp.ErrorResponse == nil {
			t.Errorf("No error received for endpoint=%q", test.endpointPath)
			continue
		}
		if resp.ErrorResponse != apierrors.InvalidSession {
			t.Errorf("Incorrect error received endpoint=%q expected=%#v actual=%#v",
				test.endpointPath, apierrors.InvalidSession, resp.ErrorResponse)
		}
	}
}

func (s *SessionAPISuite) TestReupOk() {
	t := s.T()
	us := s.Users["u1-unlinked"]
	_, resp := runReupAPI(us)

	if resp.ErrorResponse != nil {
		t.Fatalf("Unexpected error after reup %v", resp.ErrorResponse)
	}

	newSession, ok := resp.OkResponse.(*session.Session)
	if !ok {
		t.Fatalf("Did not get session in ok response, got %#v", resp.OkResponse)
	}

	v1, _ := session.ByToken(us.Session.Token())
	if v1 != nil {
		t.Fatalf("Original session was not deleted")
	}

	if newSession.Token() == us.Session.Token() {
		t.Fatalf("New session has same token as old")
	}
}

var validateOkTests = []struct {
	apiKey string
	userid string
}{
	{"user-key", "u1-unlinked"},
	{"", "u1-unlinked"},
	{"user-key", ""},
	{"", ""},
}

// Test that submiting an appkey token, session token or both
// gets the right response back with nil for the omitted entries
func (s *SessionAPISuite) TestValidateOk() {
	t := s.T()
	for _, test := range validateOkTests {
		frm := url.Values{}

		var us *session.UserSession
		var apiKey *session.ApiKey

		if test.apiKey != "" {
			apiKey = s.ApiKeys[test.apiKey]
			frm.Set("app_token", test.apiKey)
		}
		if test.userid != "" {
			us = s.Users[test.userid]
			frm.Set("session_token", us.Session.Token())
		}

		_, resp := runValidateAPI("service-key", nil, frm)

		if resp.ErrorResponse != nil {
			t.Fatalf("Unexpected error after validate %v", resp.ErrorResponse)
		}

		response, ok := resp.OkResponse.(validate.ValidateResponse)
		if !ok {
			t.Fatalf("Did not get session in ok response, got %#v", resp.OkResponse)
		}

		if us == nil {
			if response.UserSession != nil {
				t.Errorf("test=%#v response.UsersSession set unexpectedly", test)
			}
		} else {
			if response.UserSession == nil {
				t.Errorf("test=%#v response.UserSession was not set", test)
			} else if *response.UserSession.User.Username != *us.User.Username {
				t.Errorf("test=%#v username mismatch %q", test, *response.UserSession.User.Username)
			}
		}

		if apiKey == nil {
			if response.ApiKey != nil {
				t.Errorf("test=%#v response.ApiKey set unexpectedly", test)
			}
		} else {
			if response.ApiKey.Scopes != apiKey.Scopes {
				t.Errorf("test=%#v scopes mismatch %s", test, response.ApiKey.Scopes)
			}
		}
	}
}

var validateErrTests = []struct {
	validAppToken     bool
	validSessionToken bool
}{
	{true, false},
	{false, true},
	{false, false},
}

func (s *SessionAPISuite) TestValidateErrors() {
	t := s.T()
	for _, test := range validateErrTests {
		frm := url.Values{}

		if test.validAppToken {
			frm.Set("app_token", "user-key")
		} else {
			frm.Set("app_token", "invalid-key")
		}

		if test.validSessionToken {
			frm.Set("session_token", s.Users["u1-unlinked"].Session.Token())
		} else {
			frm.Set("session_token", "invalid-session-token")
		}

		_, resp := runValidateAPI("service-key", nil, frm)

		if resp.ErrorResponse == nil {
			t.Errorf("test=%#v Did not get error response", test)
			continue
		}

		if err, ok := resp.ErrorResponse.(*jsonutil.JsonErrorResponse); ok {
			if err.Code != apierrors.ValidateSessionErrorCode {
				t.Errorf("test=%v incorrect errorcode expected=%q actual=%q",
					test, apierrors.ValidateSessionErrorCode, err.Code)
			}
		}
	}
}

var validatePermErrTests = []struct {
	apiToken    string
	sessionUser string
}{
	{"service-key", "u1-unlinked"}, // u1-unlinked has user scope, not service
	{"user-key", "u1-unlinked"},    // u1-unlinked has user scope, not service
	{"user-key", ""},               // not logged in, but invalid apikey scope
	{"service-key", "s1-unlinked"}, // s1-unlinked has service scope(somehow), but still shouldn't be logged in
}

// validate should only run with a service apikey and no session
func (s *SessionAPISuite) TestValidatePermErr() {
	t := s.T()
	userSessionToken := s.Users["u1-unlinked"].Session.Token()
	for _, test := range validatePermErrTests {
		var us *session.UserSession = nil
		if test.sessionUser != "" {
			us = s.Users[test.sessionUser]
		}

		frm := url.Values{
			"session_token": {userSessionToken},
		}
		_, resp := runValidateAPI(test.apiToken, us, frm)
		if resp.ErrorResponse == nil {
			t.Errorf("Did not get error response for apiToken=%q sessionUser=%q",
				test.apiToken, test.sessionUser)
			continue
		}

		if resp.ErrorResponse != apierrors.InvalidSessionPerms {
			t.Errorf("Incorrect error received  for apiToken=%q sessionUser=%q expected=%#v actual=%#v",
				test.apiToken, test.sessionUser, apierrors.InvalidSessionPerms, resp.ErrorResponse)
		}
	}
}
