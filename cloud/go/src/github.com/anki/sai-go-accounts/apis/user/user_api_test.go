// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

package user

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"net/url"
	"reflect"
	"sort"
	"strconv"
	"strings"
	"testing"
	"time"

	"github.com/anki/sai-accounts-audit/audit"
	"github.com/anki/sai-go-accounts/apierrors"
	"github.com/anki/sai-go-accounts/apis"
	"github.com/anki/sai-go-accounts/db"
	"github.com/anki/sai-go-accounts/email"
	"github.com/anki/sai-go-accounts/models/session"
	"github.com/anki/sai-go-accounts/models/user"
	"github.com/anki/sai-go-accounts/models/user/usertest"
	"github.com/anki/sai-go-accounts/testutil"
	"github.com/anki/sai-go-accounts/util"
	autil "github.com/anki/sai-go-util"
	"github.com/anki/sai-go-util/jsonutil"
	"github.com/anki/sai-go-util/postmarkapp"
	"github.com/anki/sai-go-util/timedtoken"
	"github.com/anki/sai-go-util/validate"
	"github.com/anki/sai-go-util/validate/validtest"
	"github.com/kr/pretty"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"github.com/stretchr/testify/suite"
)

func init() {
	email.InitAnkiEmailer()
}

func strv(v interface{}) string {
	switch s := v.(type) {
	case string:
		return s
	case *string:
		if s == nil {
			return ""
		}
		return *s
	}
	return ""
}

/**
TODO:
Tests that are still needed
*
* Extend validation checks
* Validate correct fields returned
* JSON serialization check
*/
func runGetUserAPI(us *session.UserSession, userId string) (req *http.Request, record *jsonutil.JsonWriterRecorder) {
	return apitest.RunRequest("match", us, "GET", "/1/users/"+userId, nil, nil)
}

type updateUserFunc func(us *session.UserSession, userId string, jsonData []byte) (req *http.Request, record *jsonutil.JsonWriterRecorder)

func runUpdateUserAPI(us *session.UserSession, userId string, jsonData []byte) (req *http.Request, record *jsonutil.JsonWriterRecorder) {
	return apitest.RunRequest("match", us, "PATCH", "/1/users/"+userId, jsonData, nil)
}

// Test the non PATCH version of the update user endpoint (Android sucks)
func runUpdateUserAPINonPatch(us *session.UserSession, userId string, jsonData []byte) (req *http.Request, record *jsonutil.JsonWriterRecorder) {
	return apitest.RunRequest("match", us, "POST", "/1/users/"+userId+"/patch", jsonData, nil)
}

func runCreateUserAPI(us *session.UserSession, jsonData []byte) (req *http.Request, record *jsonutil.JsonWriterRecorder) {
	return apitest.RunRequest("match", us, "POST", "/1/users", jsonData, nil)
}

type auditRecorder struct {
	Records []*audit.Record
}

func (r *auditRecorder) WriteRecord(log *audit.Record) (string, error) {
	r.Records = append(r.Records, log)
	return "12345", nil
}

type UsersAPITestSuite struct {
	// APITestSuite does DB setup/teardown per test
	// and creates a standard set of test api keys and users with sessions
	apitest.APITestSuite
	Audit *auditRecorder
}

func TestUsersAPITestSuite(t *testing.T) {
	suite.Run(t, new(UsersAPITestSuite))
}

func (s *UsersAPITestSuite) SetupTest() {
	s.APITestSuite.SetupTest()
	s.Audit = &auditRecorder{}
	audit.DefaultAuditWriter = s.Audit
	RegisterAPI()
}

var listUserTests = []struct {
	name              string
	user              string
	params            string
	expectedHttpCode  int
	expectedErrorCode string
}{
	{"user list users", "u1-unlinked", "", http.StatusForbidden, apierrors.InvalidSession.Code}, // Try to list users with user scope
	{"admin list users", "admin", "", http.StatusOK, ""},                                        // Try to list users with admin scope
	{"list users invalid status", "admin", "status=fail", http.StatusBadRequest, apierrors.InvalidFieldsCode},
	{"list users bad order by ", "admin", "order_by=fail", http.StatusBadRequest, apierrors.InvalidFieldsCode},
	{"list users bad limit ", "admin", "limit=fail", http.StatusBadRequest, apierrors.InvalidFieldsCode},   // Set limit higher than the max
	{"list users bad offset ", "admin", "offset=fail", http.StatusBadRequest, apierrors.InvalidFieldsCode}, // Set limit higher than the max
	{"list users limit too high", "admin", fmt.Sprintf("limit=%d", user.MaxListLimit+1),
		http.StatusBadRequest, apierrors.InvalidFieldsCode}, // Try an invalid order type
}

// TODO: Add more tests with permutations of sort, limit, offset, filters, etc
func (s *UsersAPITestSuite) TestListUsersResponse() {
	t := s.T()
	for _, test := range listUserTests {
		_, record := apitest.RunRequest("match", s.Users[test.user], "GET", fmt.Sprintf("/1/users?%s", test.params), nil, nil)
		if record.Code != test.expectedHttpCode {
			t.Errorf("test=%q expected-status-code=%#v actual=%#v", test.name, test.expectedHttpCode, record.Code)
		}
		if test.expectedErrorCode == "" {
			_, ok := record.OkResponse.(ListUsersResponse)
			if !ok {
				t.Errorf("ListUsers OkResponse did not contain ListUsersResponse object; got %#v", record.OkResponse)
			}
			continue
		}
		if err, ok := record.ErrorResponse.(jsonutil.JsonErrorResponse); ok {
			if err.Code != test.expectedErrorCode {
				t.Errorf("Incorrect error returned, expected=%q actual=%q", apierrors.InvalidFieldsCode, err.Code)
			}
		}
	}
}

var listUserMatchTests = []struct {
	name          string
	params        url.Values
	exepctedUsers []string
}{

	{"username implicit match",
		url.Values{"status": {"active,deactivated"}, "username": {"gareth1"}},
		[]string{"gareth1"},
	},
	{"all accounts limit to 3",
		url.Values{"status": {"active,deactivated"}, "limit": {"3"}},
		[]string{"ben1", "ben2", "gareth1"},
	},
	{"all accounts limit to 3 offset 2",
		url.Values{"status": {"active,deactivated"}, "limit": {"3"}, "offset": {"2"}},
		[]string{"gareth1", "gareth2", "gareth3"},
	},
	{"default response", // same as active only
		url.Values{},
		[]string{"ben1", "ben2", "gareth1", "gareth2"},
	},
	{"active only",
		url.Values{"status": {"active"}},
		[]string{"ben1", "ben2", "gareth1", "gareth2"},
	},
	{"active only email sort",
		url.Values{"status": {"active"}, "order_by": {"email"}},
		[]string{"ben1", "ben2", "gareth2", "gareth1"},
	},
	{"active only email reverse sort",
		url.Values{"status": {"active"}, "order_by": {"email"}, "sort_descending": {"true"}},
		[]string{"gareth1", "gareth2", "ben2", "ben1"},
	},
	{"username exact",
		url.Values{"status": {"active"}, "username": {"gareth2"}, "username_mode": {"match"}},
		[]string{"gareth2"},
	},
	{"username exact no result",
		url.Values{"status": {"active"}, "username": {"gareth"}, "username_mode": {"match"}},
		nil,
	},
	{"username prefix",
		url.Values{"status": {"active"}, "username": {"gareth"}, "username_mode": {"prefix"}},
		[]string{"gareth1", "gareth2"},
	},
	{"family name prefix",
		url.Values{"status": {"active"}, "family_name": {"last"}, "family_name_mode": {"prefix"}},
		[]string{"gareth1", "gareth2"},
	},
	{"family name exact no result",
		url.Values{"status": {"active"}, "family_name": {"last"}},
		nil,
	},
	{"family name exact",
		url.Values{"status": {"active"}, "family_name": {"last1"}},
		[]string{"gareth1"},
	},
	/*
	 */
}

// Check the various filters are passed to the model correctly
//
// this just replicates the model tests for now :-/
func (s *UsersAPITestSuite) TestListUsersMatch() {
	t := s.T()
	// clear existing users
	db.Dbmap.Exec("DELETE FROM users")

	// load test specific users
	usertest.CreateUserWithState(t, user.AccountActive, "gareth1", "gareth3@example.com", "Last1")
	usertest.CreateUserWithState(t, user.AccountActive, "gareth2", "gareth2@example.com", "Last2")
	usertest.CreateUserWithState(t, user.AccountDeactivated, "gareth3", "gareth1@example.com", "Last3")
	usertest.CreateUserWithState(t, user.AccountDeactivated, "gareth4", "gareth3@example.com", "Last3")
	usertest.CreateUserWithState(t, user.AccountActive, "ben1", "ben1@example.com", "Another1")
	usertest.CreateUserWithState(t, user.AccountActive, "ben2", "ben2@example.com", "Another2")

	for _, test := range listUserMatchTests {
		_, record := apitest.RunRequest("match", s.Users["admin"], "GET", "/1/users?"+test.params.Encode(), nil, nil)
		if record.Code != http.StatusOK {
			t.Errorf("test=%q expected-status-code=200 actual=%#v", test.name, record.Code)
		}
		resp, ok := record.OkResponse.(ListUsersResponse)
		if !ok {
			t.Errorf("OkResponse did not contain ListUsersResponse; got %#v", record.OkResponse)
		}
		var usernames []string
		for _, account := range resp.Accounts {
			usernames = append(usernames, *account.Username)
		}
		if !reflect.DeepEqual(usernames, test.exepctedUsers) {
			t.Errorf("test=%s expected=%#v actual=%#v", test.name, test.exepctedUsers, usernames)
		}
	}
}
func (s *UsersAPITestSuite) TestListUsersMatchPurged() {
	t := s.T()
	purgeUser := s.Users["u1-unlinked"].User
	if err := user.DeactivateUserAccount(purgeUser.Id, "test"); err != nil {
		t.Fatal("Failed to deactivate account", err)
	}
	if err := user.PurgeUserAccount(purgeUser.Id, "test"); err != nil {
		t.Fatal("Failed to purge account", err)
	}

	params := url.Values{"status": {"purged"}}
	_, record := apitest.RunRequest("match", s.Users["admin"], "GET", "/1/users?"+params.Encode(), nil, nil)
	if record.Code != http.StatusOK {
		t.Errorf("expected-status-code=200 actual=%#v", record.Code)
	}
	resp, ok := record.OkResponse.(ListUsersResponse)
	if !ok {
		t.Errorf("OkResponse did not contain ListUsersResponse; got %#v", record.OkResponse)
	}
	if len(resp.Accounts) != 2 {
		t.Fatal("Incorrect number of acocunts returned", resp.Accounts)
	}
	if resp.Accounts[0].Id != purgeUser.Id && resp.Accounts[1].Id != purgeUser.Id {
		t.Fatalf("Incorrect id returned expected=%q actual=%q, %q", purgeUser.Id, resp.Accounts[0].Id, resp.Accounts[1].Id)
	}
}

func (s *UsersAPITestSuite) TestListUsersFilter() {
	t := s.T()
	for i := 0; i < 2; i++ {
		jsonData, _ := json.Marshal(map[string]interface{}{
			"username":    fmt.Sprintf("ListUser%d", i),
			"password":    "1234567",
			"email":       fmt.Sprintf("ListUser%d@example.com", i),
			"family_name": fmt.Sprintf("ListUser%d", i),
		})
		_, record := runCreateUserAPI(s.Users["u1-unlinked"], jsonData)
		if record.OkResponse == nil || record.ErrorResponse != nil {
			t.Fatalf("listUserFilterTests create accounts setup step did not get OK response,  ErrorResponse is %#v", record.ErrorResponse)
		}

	}

	filterTests := map[string]string{"username": "ListUser1", "email": "ListUser1@example.com", "family_name": "ListUser1"}

	for filter, value := range filterTests {
		// Test with exact match
		_, record := apitest.RunRequest("match", s.Users["admin"], "GET", fmt.Sprintf("/1/users?%s=%s", filter, value), nil, nil)
		if record.Code != http.StatusOK {
			t.Errorf("test=%q expected=%#v actual=%#v", fmt.Sprintf("list users filter %s=%s", filter, value), http.StatusOK, record.Code)
		}
		resp, ok := record.OkResponse.(ListUsersResponse)
		if !ok {
			t.Errorf("OkResponse did not contain ListUsersResponse; got %#v", record.OkResponse)
		}
		if len(resp.Accounts) != 1 {
			t.Errorf("List users %s=%s expected exact match, got %q, %#v", filter, value, len(resp.Accounts), record.OkResponse)
		}

		// Test with prefix match
		_, record = apitest.RunRequest("match", s.Users["admin"], "GET", fmt.Sprintf("/1/users?%s=%s&%s_mode=prefix", filter, value[0:8], filter), nil, nil)
		if record.Code != http.StatusOK {
			t.Errorf("test=%q expected=%#v actual=%#v", fmt.Sprintf("list users filter %s=%s with prefix", filter, value), http.StatusOK, record.Code)
		}
		resp, ok = record.OkResponse.(ListUsersResponse)
		if !ok {
			t.Errorf("OkResponse did not contain ListUsersResponse; got %#v", record.OkResponse)
		}
		if len(resp.Accounts) != 2 {
			t.Errorf("List users %s=%s with prefix expected 2 matches, got %d, %#v", filter, value, len(resp.Accounts), record.OkResponse)
		}
	}
}

// Limit the number of responses to 2, expect 2 results and an offset
func (s *UsersAPITestSuite) TestListUsersLimitOffset() {
	t := s.T()
	_, record := apitest.RunRequest("match", s.Users["admin"], "GET", "/1/users?limit=2", nil, nil)
	if record.Code != http.StatusOK {
		t.Errorf("test=%q expected-error=%#v actual=%#v", "list users limit 2", http.StatusOK, record.Code)
	}
	resp, ok := record.OkResponse.(ListUsersResponse)
	if !ok {
		t.Errorf("OkResponse did not contain ListUsersResponse; got %#v", record.OkResponse)
	}
	if len(resp.Accounts) != 2 {
		t.Errorf("Limit users request expected 2 accounts, got %q, %#v", len(resp.Accounts), record.OkResponse)
	}
	if resp.Offset != 2 {
		t.Errorf("Expected offset 2, got %q, %#v", resp.Offset, record.OkResponse)
	}
}

var createUserTests = []struct {
	name          string
	sessionUser   string
	expectedError error
}{
	{"logged-out-create-ok", "", nil},        // logged out users can create accounts
	{"user-create-fail", "u1-unlinked", nil}, // logged in users cannot create additional accounts
	{"admin-create-ok", "admin", nil},        // admins can create account
}

func (s *UsersAPITestSuite) TestCreateUserResponse() {
	t := s.T()
	captureEmail(true)
	defer captureEmail(false)

	for i, test := range createUserTests {
		sessionUser := s.Users[test.sessionUser]
		newUsername := fmt.Sprintf("user%d", i)
		jsonData, _ := json.Marshal(map[string]interface{}{
			"username": newUsername,
			"password": "123456",
			"email":    "a@example.com",
			"dob":      "1970-01-01",
		})
		_, record := runCreateUserAPI(sessionUser, jsonData)

		if test.expectedError == nil {
			if record.OkResponse == nil || record.ErrorResponse != nil {
				t.Errorf("test=%s Did not get OK response,  ErrorResponse is %#v",
					test.name, record.ErrorResponse)
				continue
			}

			ua, err := user.AccountByUsername(newUsername)
			if err != nil {
				t.Errorf("test=%s Failed to retrieve created user account: %s",
					test.name, err)
			}
			u := ua.User
			if [...]string{*u.Username, *u.Email} != [...]string{newUsername, "a@example.com"} {
				t.Errorf("User details don't match user=%v", *u)
			}
			e := getEmail(t, false)
			cont, etxt := emailContainsUrl(e, fmt.Sprintf("email-verifying?user_id=%s&token=%s&dloc=", u.Id, *u.EmailVerify))
			if !cont {
				t.Errorf("Email does not contain proper URL for vaidation: %v %v", etxt, e)
			}
			// The default users are all adults
			if e.Headers["X-Anki-Tmpl"][0] != "Verification" {
				t.Errorf("Email Header Mismatch Verification: %v", e.Headers)
			}
			if e.To[0].String() != "<a@example.com>" {
				t.Error("Create User Email not properly addressed ", e.To[0])
			}
			if ua.IsEmailVerify(*ua.EmailVerify) != true {
				t.Error("Newly created email verify token is not properly set")
			}
		} else {
			if record.ErrorResponse != test.expectedError {
				t.Errorf("test=%s Did not get expected error response.  expected=%#v error=%#v",
					test.name, test.expectedError, record.ErrorResponse)
				continue
			}
		}
	}
}

func (s *UsersAPITestSuite) TestCreateUserNoData() {
	t := s.T()
	jsonData := []byte("{}")
	_, record := runCreateUserAPI(nil, jsonData)

	if record.ErrorResponse == nil {
		t.Fatal("No error generated")
	}
	if err, ok := record.ErrorResponse.(jsonutil.JsonErrorResponse); ok {
		if err.Code != apierrors.InvalidFieldsCode {
			t.Errorf("Incorrect error returned expected=%q actual=%q", apierrors.InvalidFieldsCode, err.Code)
		}
	}
}

func (s *UsersAPITestSuite) TestCreateUserMetadata() {
	t := s.T()
	captureEmail(true)
	defer captureEmail(false)

	jsonData, _ := json.Marshal(map[string]interface{}{
		"username":                "metauser",
		"password":                "123456",
		"email":                   "a@example.com",
		"created_by_app_name":     "app-name",
		"created_by_app_version":  "app-version",
		"created_by_app_platform": "app-platform",
		"email_lang":              "en-GB",
		"dob":                     "1970-01-01",
	})
	_, record := runCreateUserAPI(nil, jsonData)
	if record.ErrorResponse != nil {
		t.Fatalf("Did not get OK response %s", record.ErrorResponse)
	}

	getEmail(t, false)

	ua, err := user.AccountByUsername("metauser")
	if err != nil {
		t.Fatalf("Failed to retrieve created user account: %s", err)
	}

	u := ua.User
	if u.CreatedByAppName == nil || *u.CreatedByAppName != "app-name" {
		t.Errorf("Incorrect app name %q", u.CreatedByAppName)
	}
	if u.CreatedByAppVersion == nil || *u.CreatedByAppVersion != "app-version" {
		t.Errorf("Incorrect app version %q", u.CreatedByAppVersion)
	}
	if u.CreatedByAppPlatform == nil || *u.CreatedByAppPlatform != "app-platform" {
		t.Errorf("Incorrect app platform %q", u.CreatedByAppPlatform)
	}
	if u.EmailLang == nil || *u.EmailLang != "en-GB" {
		t.Errorf("Incorrect email lang %q", u.EmailLang)
	}
}

func (s *UsersAPITestSuite) TestCreateUserDuplicateUsername() {
	t := s.T()
	existing := s.Users["u1-unlinked"]
	jsonData, _ := json.Marshal(map[string]interface{}{
		"username": *existing.User.Username,
		"password": "123456",
		"email":    "a@example.com",
	})

	_, record := runCreateUserAPI(nil, jsonData)

	if record.ErrorResponse == nil {
		t.Fatalf("Received error response: %v", record.ErrorResponse)
	}

	if record.ErrorResponse.(*jsonutil.JsonErrorResponse).Code != "duplicate_username" {
		t.Errorf("Incorrect error returned %#v\n", record.ErrorResponse)
	}
}

var fieldTests = []struct {
	fieldName  string
	required   bool
	goodValue1 interface{}
	goodValue2 interface{}
	badValue   interface{}
}{
	{"username", true, "auser", "buser", "toolongusername"},
	{"email", true, "test@example.com", "test2@example.com", "foobar"},
	{"email_lang", false, "en-US", "de-DE", "en-us"},
	{"drive_guest_id", false, "drive-guest-id", "diff-id", strings.Repeat("toolong", 10)},
	{"player_id", false, "drive-guest-id", "diff-id", strings.Repeat("toolong", 10)},
}

func (s *UsersAPITestSuite) TestFields() {
	// TODO: break this up into separate tests
	t := s.T()

	var entry map[string]interface{}
	reset := func() {
		entry = make(map[string]interface{})
		for _, test := range fieldTests {
			if test.required {
				entry[test.fieldName] = test.goodValue1
			}
		}
		entry["password"] = "a password"
	}

	for _, test := range fieldTests {
		if test.fieldName != "player_id" {
			continue
		}
		_, err := db.Dbmap.Exec(
			`DELETE FROM users WHERE username IN ("auser", "buser", "toolongusername")`)
		if err != nil {
			t.Fatal("Got error deleting users: ", err)
		}
		reset()

		// try to create an entry with the field missing
		delete(entry, test.fieldName)
		jdata, _ := json.Marshal(entry)
		_, record := runCreateUserAPI(nil, jdata)
		if test.required {
			// if its required the operation should have failed
			if record.ErrorResponse == nil {
				t.Errorf("field=%s op=create_required_fail did not fail; got okresponse=%#v",
					test.fieldName, record.OkResponse)
				continue
			}
		} else {
			// if its optional the operation should have succeeded
			if record.ErrorResponse != nil {
				t.Errorf("field=%s op=create_required_fail failed; got okresponse=%#v",
					test.fieldName, record.OkResponse)
				continue
			}
		}

		// reset
		db.Dbmap.Exec(`DELETE FROM users WHERE username IN ("auser", "buser", "toolongusername")`)

		// test create with bad value
		entry[test.fieldName] = test.badValue

		jdata, _ = json.Marshal(entry)
		_, record = runCreateUserAPI(nil, jdata)
		if record.ErrorResponse == nil {
			t.Errorf("field=%s op=create_fail baddata=%q did not fail; got okresponse=%#v",
				test.fieldName, test.badValue, record.OkResponse)
			continue
		}

		// actually create the entry
		entry[test.fieldName] = test.goodValue1
		jdata, _ = json.Marshal(entry)
		_, record = runCreateUserAPI(nil, jdata)
		if record.ErrorResponse != nil {
			t.Errorf("field=%s op=create_ok data=%q failed; got err=%#v",
				test.fieldName, test.goodValue1, record.ErrorResponse)
			continue
		}
		us, ok := record.OkResponse.(*session.UserSession)
		if !ok {
			t.Errorf("OkResponse was not a *UserSession; got %#v", record.OkResponse)
			continue
		}

		val, err := autil.StructFieldByJsonField(us.User.User, test.fieldName)
		if err != nil || strv(val) != test.goodValue1 {
			t.Errorf("field=%q Did not get expected value after create, actual=%q err=%v",
				test.fieldName, val, err)
			continue
		}

		// capture the new userId
		userId := us.User.Id

		// attempt a good update
		update := map[string]interface{}{
			test.fieldName: test.goodValue2,
		}
		jdata, _ = json.Marshal(update)
		_, record = runUpdateUserAPI(us, userId, jdata)
		if record.ErrorResponse != nil {
			t.Errorf("field=%s op=update1_ok data=%q failed; got err=%#v",
				test.fieldName, test.goodValue2, record.ErrorResponse)
			continue
		}
		ua, ok := record.OkResponse.(*user.UserAccount)
		if !ok {
			t.Errorf("OkResponse was not a *UserAccount; got %#v", record.OkResponse)
			continue
		}

		val, err = autil.StructFieldByJsonField(ua.User, test.fieldName)
		if err != nil || strv(val) != test.goodValue2 {
			t.Errorf("field=%q Did not get expected value after update, expected=%q actual=%q",
				test.fieldName, test.goodValue2, strv(val))
			continue
		}

		// attempt a bad update; should fail
		update = map[string]interface{}{
			test.fieldName: test.badValue,
		}
		jdata, _ = json.Marshal(update)
		_, record = runUpdateUserAPI(us, userId, jdata)
		if record.ErrorResponse == nil {
			t.Errorf("field=%q Did not return error after using bad value for update okResponse=%#v",
				test.fieldName, record)
			continue
		}
	}
}

// Check we can't influence non user-settable fields
func (s *UsersAPITestSuite) TestCreateUserBadFields() {
	t := s.T()
	jsonData, _ := json.Marshal(map[string]interface{}{
		"username":             "auser",
		"password":             "123456",
		"email":                "a@example.com",
		"status":               "purged",
		"deactivated_username": "deactivate",
		"email_is_verified":    true,
		"password_is_complex":  true,
	})

	_, record := runCreateUserAPI(nil, jsonData)

	if record.ErrorResponse == nil {
		t.Fatalf("Did not receive error response: %v", record)
	}

	bad_fields, ok := record.ErrorResponse.(jsonutil.BadFieldsError)
	if !ok {
		t.Fatal("Did not get a json error", record.ErrorResponse)
	}
	fields := []string(bad_fields)
	sort.Strings(fields)
	expected := []string{"status", "deactivated_username", "email_is_verified", "password_is_complex"}
	sort.Strings(expected)
	if !reflect.DeepEqual(fields, expected) {
		t.Errorf("Incorrect field list, expected=%#v actual=%#v", expected, fields)
	}
}

func (s *UsersAPITestSuite) TestUpdateUserBadFields() {
	t := s.T()
	us := s.Users["u1-unlinked"]
	jsonData, _ := json.Marshal(map[string]interface{}{
		"username":             "auser",
		"password":             "123456",
		"email":                "a@example.com",
		"status":               "purged",
		"deactivated_username": "deactivate",
		"email_is_verified":    true,
		"password_is_complex":  true,
		// Fields that can only be set on creation; not updated
		"created_by_app_name":     "app-name",
		"created_by_app_version":  "app-version",
		"created_by_app_platform": "app-platform",
	})

	_, record := runUpdateUserAPI(us, us.User.Id, jsonData)

	if record.ErrorResponse == nil {
		t.Fatalf("Did not receive error response: %v", record)
	}

	bad_fields, ok := record.ErrorResponse.(jsonutil.BadFieldsError)
	if !ok {
		t.Fatal("Did not get a json error", record.ErrorResponse)
	}
	fields := []string(bad_fields)
	sort.Strings(fields)
	expected := []string{"status", "deactivated_username", "email_is_verified", "password_is_complex",
		"created_by_app_name", "created_by_app_version", "created_by_app_platform"}
	sort.Strings(expected)
	if !reflect.DeepEqual(fields, expected) {
		t.Errorf("Incorrect field list, expected=%#v actual=%#v", expected, fields)
	}
}

var updateUserTests = []struct {
	name          string
	sessionUser   string
	targetUser    string
	expectedError error
	expectedState user.AccountStatus
}{
	{"user-update-self", "u3-unlinked", "u3-unlinked", nil, user.AccountActive},                      // should succeed
	{"user-update-me", "u3-unlinked", "me", nil, user.AccountActive},                                 // should succeed
	{"user-update-deleted", "d5-unlinked", "me", apierrors.AccountNotFound, user.AccountDeactivated}, // should fail
	{"user-update-other", "u3-unlinked", "u4-unlinked",
		apierrors.InvalidSessionPerms, user.AccountActive}, // a user can't update another user's account
	{"admin-update-purged", "admin", "p1-unlinked", apierrors.CannotReactivatePurged, user.AccountPurged},
	{"admin-update-any", "admin", "u4-unlinked", nil, user.AccountActive}, // admins can update any account
}

// Test that a call to the update endpoint just updates the requested fields
func (s *UsersAPITestSuite) testUpdateUserAPI(updateFunc updateUserFunc) {
	var data *user.UserAccount
	var ok bool
	t := s.T()

	for i, test := range updateUserTests {
		var (
			ua                    *user.UserAccount
			userId, requestUserId string
		)
		// Which user are we pretending to be when running the test?
		userSession := s.Users[test.sessionUser]

		// Which user do we want to try and update?
		if test.targetUser == "me" {
			if u := s.Users[test.sessionUser]; u == nil {
				t.Fatalf("Bad sessionUser name %q", test.sessionUser)
			}
			ua = s.Users[test.sessionUser].User
			requestUserId = "me"
		} else {
			if u := s.Users[test.targetUser]; u == nil {
				t.Fatalf("Bad targetUser name %q", test.targetUser)
			}
			ua = s.Users[test.targetUser].User
			requestUserId = ua.User.Id
		}
		userId = ua.User.Id

		newUsername := fmt.Sprintf("edit%d", i)
		jsonData, _ := json.Marshal(map[string]interface{}{
			"username":    newUsername,
			"family_name": "updated",
			"email":       "test-updated@example.com",
			"dob":         over13dob(),
		})
		_, record := updateFunc(userSession, requestUserId, jsonData)

		if test.expectedError == nil {
			if record.OkResponse == nil {
				t.Errorf("test=%s Did not get OK response,  ErrorResponse is %#v",
					test.name, record.ErrorResponse)
				continue
			}
			if record.Code != http.StatusOK {
				t.Errorf("test=%s Unexpected HTTP response code %d", test.name, record.Code)
			}
			resp := record.OkResponse
			if data, ok = resp.(*user.UserAccount); !ok {
				t.Fatalf("test=%s Didn't get a user.UserAccount object type=%T val=%#v", test.name, resp, resp)
			}

			ua, err := user.AccountById(userId)
			if err != nil || ua == nil {
				t.Errorf("test=%s Failed to laod updated account error=%v", test.name, err)
				continue
			}
			u := ua.User
			expected := [...]string{newUsername, "updated", "given name", "test-updated@example.com"}
			actual := [...]string{*u.Username, *u.FamilyName, *u.GivenName, *u.Email}
			if expected != actual {
				t.Errorf("test=%s Stored fields don't match expected=%#v actual=%#v",
					test.name, expected, actual)
			}

			// check the response shows the updated data too
			u = data.User
			actual = [...]string{*u.Username, *u.FamilyName, *u.GivenName, *u.Email}
			if expected != actual {
				t.Errorf("test=%s Returned fields don't match expected=%#v actual=%#v", test.name, expected, actual)
			}
		} else {
			if record.ErrorResponse != test.expectedError {
				t.Errorf("test=%s Did not get expected error response.  expected=%#v error=%#v",
					test.name, test.expectedError, record.ErrorResponse)
				continue
			}
			if ua.User.Status != test.expectedState {
				t.Errorf("test=%s Account in unexpected state %q expected %q", test.name, ua.User.Status, test.expectedState)
			}
		}

	}
}

func (s *UsersAPITestSuite) TestUpdateUserAPIWithPatch() {
	s.testUpdateUserAPI(runUpdateUserAPI)
}

func (s *UsersAPITestSuite) TestUpdateUserAPINoPatch() {
	s.testUpdateUserAPI(runUpdateUserAPINonPatch)
}

func over13dob() string {
	return "1999-01-01"
}

func under13dob() string {
	return "2013-01-01"
}

var updateUserCheckEmailTests = []struct {
	sessionUser   string
	targetUser    string
	newDob        string
	expectedEmail bool
	emailName     string
}{
	{"u1-unlinked", "u1-unlinked", over13dob(), true, "Verification"},
	{"u1-unlinked", "u1-unlinked", under13dob(), true, "VerificationUnder13"},
	{"u1-unlinked", "u1-unlinked", over13dob(), true, "Verification"}, // test flip flopping
	{"d5-unlinked", "me", over13dob(), false, ""},                     // should not send an email - deleted user update attempt
	{"d5-unlinked", "d5-unlinked", under13dob(), false, ""},           // should not send an email - deleted user update attempt
}

// Test that a call to the update endpoint just updates the requested fields
func (s *UsersAPITestSuite) TestUpdateUserAPIEmailVerification() {
	t := s.T()
	captureEmail(true)
	defer captureEmail(false)

	for i, test := range updateUserCheckEmailTests {
		var ua *user.UserAccount
		var requestUserId string

		// Which user are we pretending to be when running the test?
		userSession := s.Users[test.sessionUser]

		// Which user do we want to try and update?
		if test.targetUser == "me" {
			if u := s.Users[test.sessionUser]; u == nil {
				t.Fatalf("Bad sessionUser name %q", test.sessionUser)
			}
			ua = s.Users[test.sessionUser].User
			requestUserId = "me"
		} else {
			if u := s.Users[test.targetUser]; u == nil {
				t.Fatalf("Bad targetUser name %q", test.targetUser)
			}
			ua = s.Users[test.targetUser].User
			requestUserId = ua.User.Id
		}

		newUsername := fmt.Sprintf("edit%d", i)
		jsonData, _ := json.Marshal(map[string]interface{}{
			"username":    newUsername,
			"family_name": "updated",
			"email":       fmt.Sprintf("test-%d-updated@example.com", i),
			"dob":         test.newDob,
		})
		runUpdateUserAPI(userSession, requestUserId, jsonData)

		if test.expectedEmail {
			// Check email
			e := getEmail(t, false)

			// The default users are all under13/no dob
			if e.Headers["X-Anki-Tmpl"][0] != test.emailName {
				// failing with Verification not equal to VertificationUnder13
				t.Errorf("Email Header Mismatch '%s': '%v'", test.emailName, e.Headers["X-Anki-Tmpl"][0])
			}
		} else {
			if e := getEmail(t, true); e != nil {
				t.Errorf("Test %d should not have sent an email", i)
			}
		}
	}
}

// Check that changing the user's email address clears the email failure code
func (s *UsersAPITestSuite) TestUpdateUserAPIEmailFailureReset() {
	t := s.T()
	captureEmail(true)
	defer captureEmail(false)

	us := s.Users["u1-unlinked"]
	u := us.User
	userId := u.Id
	// set failure code
	if _, err := db.Dbmap.Exec(`UPDATE users SET email_failure_code="codeset" WHERE user_id=?`, userId); err != nil {
		t.Fatal("Failed to set email_failure_ocde", err)
	}

	jsonData, _ := json.Marshal(map[string]interface{}{
		"email": "test-updated-email@example.com",
	})

	runUpdateUserAPI(us, u.Id, jsonData)
	getEmail(t, false)

	// refetch user
	ua, err := user.AccountById(userId)
	if err != nil || ua == nil {
		t.Fatalf("Failed to laod updated account error=%v", err)
	}
	if ua.User.EmailFailureCode != nil {
		t.Fatal("Failure code was not reset; current value", *ua.User.EmailFailureCode)
	}

}

// change the user's password and attempt to login using it
func (s *UsersAPITestSuite) TestChangePassword() {
	t := s.T()
	us1 := s.Users["u1-unlinked"]

	jdata, _ := json.Marshal(map[string]interface{}{
		"password": "new password",
	})
	_, record := runUpdateUserAPI(us1, us1.User.Id, jdata)

	if record.Code != http.StatusOK {
		t.Errorf("Unexpected HTTP response code %d", record.Code)
	}
	resp := record.OkResponse
	if _, ok := resp.(*user.UserAccount); !ok {
		t.Fatalf("Didn't get a user.UserAccount object type=%T val=%#v", resp, resp)
	}
	ua, _ := user.AccountById(us1.User.Id)
	if ok, err := ua.IsPasswordCorrect("new password"); !ok || err != nil {
		oldOk, _ := ua.IsPasswordCorrect("hashme")
		t.Fatalf("Password did not validate ok=%t still_old_password=%t err=%v", ok, oldOk, err)
	}
}

// Test that changing a user's password invalidates any existing sessions
// updating other fields, but not the password should not invalidate existing sessions
func (s *UsersAPITestSuite) TestPasswordSessionInvalidation() {
	t := s.T()
	us1 := s.Users["u1-unlinked"]

	for i, changePassword := range []bool{false, true} {
		// create an extra session
		us2, err := session.NewUserSession(us1.User)
		if err != nil {
			t.Fatal("Failed to create extra session: ", err)
		}

		jdata := map[string]interface{}{
			"username": "user" + strconv.Itoa(i),
		}
		if changePassword {
			jdata["password"] = "updated"
		}

		jsonData, _ := json.Marshal(jdata)
		_, record := runUpdateUserAPI(us1, us1.User.Id, jsonData)

		if record.ErrorResponse != nil {
			t.Fatal("Unexpected error", record.ErrorResponse)
		}

		// only the secondary sessions should now be unvalid
		check1, err1 := session.ByToken(us1.Session.Token())
		check2, err2 := session.ByToken(us2.Session.Token())
		if err1 != nil || err2 != nil {
			t.Fatal("Unexpected error from ByToken", err1, err2)
		}

		if changePassword && (check1 == nil || check2 != nil) {
			if check1 == nil {
				t.Error("Unexpecteldy logged out of primary session after password reset")
			}
			if check2 != nil {
				t.Error("Secondary session was not invalidated after password reset")
			}

		} else if !changePassword && (check1 == nil || check2 == nil) {
			t.Error("Session tokens were invalidated when password wasn't changed")
		}
	}
}

func (s *UsersAPITestSuite) TestUpdateUserAPIDuplicateUsername() {
	t := s.T()
	us := s.Users["u1-unlinked"]
	userId := us.User.Id

	jsonData, _ := json.Marshal(map[string]interface{}{
		"username": s.Users["u2-unlinked"].User.Username,
	})

	_, record := runUpdateUserAPI(us, userId, jsonData)

	if record.ErrorResponse == nil {
		t.Fatal("Did not receive error response")
	}

	if record.ErrorResponse.(*jsonutil.JsonErrorResponse).Code != "duplicate_username" {
		t.Errorf("Incorrect error returned %#v\n", record.ErrorResponse)
	}
}

func (s *UsersAPITestSuite) TestUpdateUserAPIInvalidUser() {
	t := s.T()
	us := s.Users["admin"]

	jsonData, _ := json.Marshal(map[string]interface{}{
		"email": "updated@example.com",
	})

	_, record := runUpdateUserAPI(us, "invalid", jsonData)

	if record.ErrorResponse == nil {
		t.Fatal("Did not receive error response")
	}

	if record.ErrorResponse.(*jsonutil.JsonErrorResponse) != apierrors.AccountNotFound {
		t.Errorf("Incorrect error returned %#v\n", record.ErrorResponse)
	}
}

var getUserTests = []struct {
	name           string
	sessionUser    string
	targetUser     string
	restrictedView bool
	expectedError  error
	expectAudit    bool
}{
	{"user-get-self", "u3-unlinked", "u3-unlinked", false, nil, false},                      // should succeed
	{"user-get-me", "u3-unlinked", "me", false, nil, false},                                 // should succeed
	{"user-get-invalid", "u3-unlinked", "invalid", false, apierrors.AccountNotFound, false}, // should succeed
	{"no-user-get-user", "", "u3-unlinked", true, nil, true},                                // should succeed
	{"no-user-get-me", "", "me", false, apierrors.AccountNotFound, false},                   // not logged in; "me" meaningless
	{"user-get-other", "u3-unlinked", "u4-unlinked",
		true, nil, true}, // a user can't access another full details of another user's account
	{"admin-get-any", "admin", "u4-unlinked", false, nil, true}, // admins can get any account
}

func (s *UsersAPITestSuite) TestGetUserAPIResponses() {
	t := s.T()

	user.DeactivateUserAccount(s.Users["u1-unlinked"].User.Id, "test")
	user.DeactivateUserAccount(s.Users["u2-unlinked"].User.Id, "test")
	user.PurgeUserAccount(s.Users["u2-unlinked"].User.Id, "test")

	expectedStatus := map[string]user.AccountStatus{
		s.Users["u1-unlinked"].User.Id: user.AccountDeactivated,
		s.Users["u2-unlinked"].User.Id: user.AccountPurged,
		s.Users["u3-unlinked"].User.Id: user.AccountActive,
		s.Users["u4-unlinked"].User.Id: user.AccountActive,
	}

	for _, test := range getUserTests {
		var (
			ua                    *user.UserAccount
			userId, requestUserId string
		)
		// Which user are we pretending to be when running the test?
		userSession := s.Users[test.sessionUser]

		// Which user do we want to try and get?
		if test.targetUser == "me" {
			if userSession != nil {
				ua = userSession.User
				userId = ua.User.Id
			}
			requestUserId = "me"
		} else {
			if targetUser := s.Users[test.targetUser]; targetUser != nil {
				ua = targetUser.User
				userId = ua.User.Id
				requestUserId = userId
			} else {
				requestUserId = test.targetUser
			}
		}

		s.Audit.Records = nil
		_, record := runGetUserAPI(userSession, requestUserId)
		if test.expectAudit {
			assert.Equal(t, 1, len(s.Audit.Records), "Failed to record audit of account access from non-owner")
		}
		if test.expectedError == nil {
			if record.OkResponse == nil {
				t.Errorf("test=%s Did not get OK response,  ErrorResponse is %#v", test.name, record.ErrorResponse)
				continue
			}
			if record.Code != http.StatusOK {
				t.Errorf("Unexpected HTTP response code %d", record.Code)
			}
			resp := record.OkResponse

			var status user.AccountStatus
			switch data := resp.(type) {
			case *user.UserAccount:
				if test.restrictedView {
					t.Errorf("test=%q Got full user object instead of restricted view", test.name)
				}
				status = data.Status
				if status == user.AccountActive {
					if *data.User.Username != *ua.User.Username {
						t.Errorf("test=%q Username mismatch expected=%q actual=%q", test.name, *ua.User.Username, *data.User.Username)
					}
				}
			case restrictedUserView:
				if !test.restrictedView {
					t.Errorf("test=%q Got restricted view instead of full user object", test.name)
				}
				status = data.Status
			default:
				t.Fatalf("test=%q Unknown response type %T for %#v", test.name, resp, resp)
			}

			actualRequestUserId := requestUserId
			if actualRequestUserId == "me" {
				actualRequestUserId = s.Users[test.sessionUser].User.Id
			}
			fmt.Println(actualRequestUserId)
			if status != expectedStatus[actualRequestUserId] {
				t.Errorf("test=%q incorrect account status expected=%v actual=%v", test.name, expectedStatus[actualRequestUserId], status)
			}

		} else {
			if record.ErrorResponse != test.expectedError {
				t.Errorf("test=%s Did not get expected error response.  expected=%#v error=%#v",
					test.name, test.expectedError, record.ErrorResponse)
				continue
			}
		}
	}
}

// TODO Add update tests that check that users cannot set fields they shouldn't be able to.
func (s *UsersAPITestSuite) TestGetUserAPIGetNotFound() {
	t := s.T()
	//users := genUsers(t)

	_, record := runGetUserAPI(s.Users["admin"], "random")
	if record.ErrorResponse == nil {
		t.Fatalf("No error response set")
	}

	if err, ok := record.ErrorResponse.(*jsonutil.JsonErrorResponse); ok {
		if err.Code != "account_not_found" {
			t.Errorf("Incorrect error response %#v", err)
		}
	} else {
		t.Fatalf("Error resposne was not a JsonError: %#v", record.ErrorResponse)
	}
}

var deactivateUserTests = []struct {
	name          string
	sessionUser   string
	deleteUser    string
	expectedError error
}{
	{"user-delete-self", "u1-unlinked", "u1-unlinked", nil}, // should succeed
	{"user-delete-me", "u2-unlinked", "me", nil},            // should succeed
	{"user-delete-other", "u3-unlinked", "u4-unlinked",
		apierrors.InvalidSessionPerms}, // a user can't delete another user's account
	{"admin-delete-any", "admin", "u4-unlinked", nil}, // admins can delete any account
}

func (s *UsersAPITestSuite) TestDeactivateUserAPIResponse() {
	t := s.T()
	for _, test := range deactivateUserTests {
		var (
			ua                    *user.UserAccount
			userId, requestUserId string
		)
		// Which user are we pretending to be when running the test?
		userSession := s.Users[test.sessionUser]

		// Which user do we want to try and delete?
		if test.deleteUser == "me" {
			ua = s.Users[test.sessionUser].User
			userId = ua.User.Id
			requestUserId = "me"
		} else {
			ua = s.Users[test.deleteUser].User
			userId = ua.User.Id
			requestUserId = userId
		}

		_, record := apitest.RunRequest("match", userSession, "DELETE", "/1/users/"+requestUserId, nil, nil)

		ua, err := user.AccountById(userId)
		if ua == nil || err != nil {
			t.Fatalf("Failed to fetch user ua=%#v err=%#v", ua, err)
		}
		if test.expectedError == nil {
			if record.OkResponse == nil {
				t.Errorf("test=%s Did not get OK response,  ErrorResponse is %#v",
					test.name, record.ErrorResponse)
				continue
			}
			if ua.User.Status != user.AccountDeactivated {
				t.Errorf("Account in unexpected state %q", ua.User.Status)
			}
			if util.NS(ua.User.DeactivationReason) != apiDeactivationReason {
				t.Error("Incorrect deactivation reason", util.NS(ua.User.DeactivationReason))
			}
		} else {
			if record.ErrorResponse != test.expectedError {
				t.Errorf("test=%s Did not get expected error response.  expected=%#v error=%#v",
					test.name, test.expectedError, record.ErrorResponse)
				continue
			}
			if ua.User.Status != user.AccountActive {
				t.Errorf("Account in unexpected state %q", ua.User.Status)
			}

		}
	}
}

var reactivateUserTests = []struct {
	name          string
	sessionUser   string
	user          string
	expectedError error
}{
	{"bad-perms", "u1-unlinked", "u1-unlinked", apierrors.InvalidSessionPerms}, // test will make u1-unlinked deactivated at start
	{"ok", "admin", "u1-unlinked", nil},                                        // test will make u1-unlinked deactivated at start
	{"already-active", "admin", "u2-unlinked", apierrors.AccountAlreadyActive},
	{"not-found", "admin", "", apierrors.AccountNotFound},
}

func (s *UsersAPITestSuite) TestReactivateUserAPIResponse() {
	t := s.T()
	ua := s.Users["u1-unlinked"]
	user.DeactivateUserAccount(ua.User.Id, "test")
	for _, test := range reactivateUserTests {
		ua := s.Users[test.user]
		userId := "notfound"

		if ua != nil {
			userId = ua.User.Id
		}

		userSession := s.Users[test.sessionUser]
		frm := url.Values{"username": {"auser"}}
		_, record := apitest.RunRequest("admin-key", userSession, "POST", "/1/users/"+userId+"/reactivate", nil, frm)

		if record.ErrorResponse != test.expectedError {
			t.Errorf("test=%q expected-error=%#v actual=%#v",
				test.name, test.expectedError, record.ErrorResponse)
		}
	}
}

// Check that deactivating an account causes all sessions for that account to be terminated.
func (s *UsersAPITestSuite) TestDeactivateUserAPISession() {
	t := s.T()

	ua := s.Users["u1-unlinked"].User
	userId := ua.User.Id
	s1, err1 := session.NewUserSession(ua)
	s2, err2 := session.NewUserSession(ua)
	if err1 != nil || err2 != nil {
		t.Fatalf("Failed to create sessions err1=%#v err2=%#v", err1, err2)
	}

	_, record := apitest.RunRequest("user-key", s1, "DELETE", "/1/users/"+userId, nil, nil)
	if record.Code != 200 {
		t.Fatalf("Did not get 200 response got=%#v", record)
	}

	c1, err2 := session.ByToken(s1.Session.Token())
	c2, err2 := session.ByToken(s2.Session.Token())
	if err1 != nil || err2 != nil {
		t.Errorf("Error fetching session data err1=%v err2=%v", err1, err2)
	}
	if c1 != nil || c2 != nil {
		t.Errorf("Sessions weren't deleted c1=%#v c2=%#v", c1, c2)
	}
}

func (s *UsersAPITestSuite) TestDeactivateUserAPI404() {
	t := s.T()
	//users := genUsers(t)
	_, record := apitest.RunRequest("match", s.Users["admin"], "DELETE", "/1/users/invalid", nil, nil)

	if record.Code != http.StatusNotFound {
		t.Errorf("Incorrect response code expected=404 actual=%d", record.Code)
	}
}

func (s *UsersAPITestSuite) TestPurgeUserOk() {
	t := s.T()
	us := s.Users["u1-unlinked"]
	userId := us.User.Id
	// deactivate first
	if err := user.DeactivateUserAccount(userId, "test"); err != nil {
		t.Fatal("failed to deactivate user", err)
	}

	_, record := apitest.RunRequest("admin-key", s.Users["admin"], "POST", "/1/users/"+userId+"/purge", nil, nil)
	if record.Code != 200 {
		t.Fatalf("Did not get 200 response got=%#v", record)
	}

	ua, err := user.AccountById(userId)
	if ua == nil || err != nil {
		t.Fatalf("Failed to fetch user ua=%#v err=%#v", ua, err)
	}
	if ua.User.Status != user.AccountPurged {
		t.Errorf("Account status was not updated to purged, got %v", ua.User.Status)
	}
	if util.NS(ua.User.PurgeReason) != apiPurgeReason {
		t.Errorf("Purge reason incorrect expected=%q actual=%q", apiPurgeReason, util.NS(us.User.PurgeReason))
	}
}

func (s *UsersAPITestSuite) TestPurgeUserErr() {
	t := s.T()
	us := s.Users["u1-unlinked"]
	userId := us.User.Id
	// don't deactivate first

	_, record := apitest.RunRequest("admin-key", s.Users["admin"], "POST", "/1/users/"+userId+"/purge", nil, nil)

	err, ok := record.ErrorResponse.(*jsonutil.JsonErrorResponse)
	if !ok {
		t.Fatalf("Did not get expected error respones, got record=%v response=%v", pretty.Formatter(record), pretty.Formatter(record.OkResponse))
	}
	if err != apierrors.CannotPurgeActive {
		t.Errorf("Did not get correct error response, got %v", err)
	}
}

// Only support/admin roles should be able to access the purge endpoint
func (s *UsersAPITestSuite) TestPurgeUserPerms() {
	t := s.T()
	us := s.Users["u1-unlinked"]
	userId := us.User.Id
	// don't deactivate first

	_, record := apitest.RunRequest("user-key", us, "POST", "/1/users/"+userId+"/purge", nil, nil)

	if record.Code != 403 {
		t.Fatal("Didn't get permission denied response")
	}
}

func (s *UsersAPITestSuite) TestIsUserAvailableAPI() {
	t := s.T()
	//users := genUsers(t)
	ua := usertest.CreateModelUser(t)
	username := *ua.User.Username

	_, record := apitest.RunRequest("user-key", nil, "GET", "/1/usernames/"+username, nil, nil)

	resp := record.OkResponse
	if record.Code != http.StatusOK {
		t.Errorf("Endpoint did not return 200 status")
	}

	if resp == nil {
		t.Fatalf("Got nil OK response.  error respnose = %#v", record.ErrorResponse)
	}

	expected := validate.Map{
		"exists": validate.Equal{Val: true},
	}

	v := validate.New()
	v.Field("", resp, expected)
	validtest.ApplyToTest("username exists 1 User scope", v, t)

	_, record = apitest.RunRequest("admin-key", nil, "GET", "/1/usernames/"+username, nil, nil)
	resp = record.OkResponse
	if record.Code != http.StatusOK {
		t.Errorf("Endpoint did not return 200 status")
	}
	if resp == nil {
		t.Fatalf("Got nil OK response.  error respnose = %#v", record.ErrorResponse)
	}

	expected = validate.Map{
		"exists":  validate.Equal{Val: true},
		"status":  validate.Equal{Val: "active"},
		"user_id": validate.Equal{Val: ua.User.Id},
	}

	v = validate.New()
	v.Field("", resp, expected)
	validtest.ApplyToTest("username exists 1 Admin scope", v, t)

	_, record = apitest.RunRequest(
		"user-key", nil, "GET", "/1/usernames/invalid", nil, nil)
	if record.Code != http.StatusNotFound {
		t.Errorf("endpoint did not return 404 status status=%d", record.Code)
	}
	_, ok := record.ErrorResponse.(*jsonutil.JsonErrorResponse)
	if !ok {
		t.Fatal("error response was not set")
	}
}

func (s *UsersAPITestSuite) TestYetAnotherIsUserAvailableAPI() {
	t := s.T()
	ua := usertest.CreateModelUser(t)
	username := *ua.User.Username

	_, record := apitest.RunRequest(
		"user-key", s.Users["u1-unlinked"], "GET", "/1/username-check?username="+username, nil, nil)

	resp := record.OkResponse
	if record.Code != http.StatusOK {
		t.Errorf("Endpoint test 1 did not return 200 status")
	}

	if resp == nil {
		t.Fatalf("Got nil OK response.  error response = %#v", record.ErrorResponse)
	}
	exists, ok := resp.(map[string]interface{})["exists"]
	if !exists.(bool) || !ok {
		t.Errorf("endpoint test 2 did not return correct response: %#v", resp)
	}

	_, record = apitest.RunRequest(
		"user-key", s.Users["u1-unlinked"], "GET", "/1/username-check?username=invalid", nil, nil)
	if record.Code != http.StatusOK {
		t.Errorf("endpoint for invalid did not return 200 status status=%d", record.Code)
	}
	resp = record.OkResponse
	exists, ok = resp.(map[string]interface{})["exists"]
	if exists.(bool) || !ok {
		t.Errorf("endpoint did not return correct response for invalid: %#v", resp)
	}
}

var suggestionsTests = []struct {
	username        string
	count           string
	expected        []string
	expectedErrCode string
}{
	// Note: if DefaultSuggestionCount or MaxSuggestionCount are changed
	// then these tests will need to be updated
	{"taken", "", []string{"taken1", "taken2", "taken3", "taken4", "taken5"}, ""},
	{"taken", "1", []string{"taken1"}, ""},
	{"taken", "2", []string{"taken1", "taken2"}, ""},
	{"taken", "0", nil, "invalid_fields"},
	{"taken", "20", nil, "invalid_fields"},
	{"taken", "badnum", nil, "invalid_fields"},
	{"free", "", []string{"free", "free1", "free2", "free3", "free4"}, ""},
}

func (s *UsersAPITestSuite) TestUserSuggestionsAPI() {
	t := s.T()
	u := s.Users["u1-unlinked"].User
	// change username
	un := "taken"
	u.Username = &un
	user.UpdateUserAccount(u)
	for _, test := range suggestionsTests {
		frm := url.Values{
			"username": []string{test.username},
		}
		if test.count != "" {
			frm["count"] = []string{test.count}
		}

		_, record := apitest.RunRequest(
			"user-key", nil, "POST", "/1/suggestions", nil, frm)

		if test.expectedErrCode == "" {
			if record.OkResponse == nil {
				t.Fatalf("Failed to get OK response for username=%q error=%#v", test.username, record.ErrorResponse)
			}

			if data, ok := record.OkResponse.(map[string]interface{}); ok {
				if !reflect.DeepEqual(data["suggestions"], test.expected) {
					t.Errorf("username=%q count=%q did not get expected result expected=%#v result=%#v",
						test.username, test.count, test.expected, data["suggestions"])
				}
			} else {
				t.Errorf("Could not parse data response data=%#v", record.OkResponse)
			}
		} else {
			if err, ok := record.ErrorResponse.(*jsonutil.JsonErrorResponse); ok {
				if err.Code != test.expectedErrCode {
					t.Errorf("Did not get expected error for username=%q count=%s expected=%q actual=%q",
						test.username, test.count, test.expectedErrCode, err.Code)
				}
			} else {
				t.Errorf("Did not get expected error for username=%q count=%s expected=%q actual=%#v",
					test.username, test.count, test.expectedErrCode, record.ErrorResponse)
			}
		}

	}
}

var relatedUserTests = []struct {
	key            string
	username       string
	expectedStatus int
	expectedCount  int
}{
	{"service-key", "active1", http.StatusOK, 1},
	{"service-key", "active2", http.StatusOK, 0},
	{"service-key", "inactive", http.StatusNotFound, 0},
	{"service-key", "invalid", http.StatusNotFound, 0},
	{"user-key", "active1", http.StatusForbidden, 0},
}

func (s *UsersAPITestSuite) TestRelatedUsersAPI() {
	t := s.T()

	// clear existing users
	db.Dbmap.Exec("DELETE FROM users")

	usertest.CreateUserWithState(t, user.AccountActive, "active1", "active1@example.com", "Last1")
	usertest.CreateUserWithState(t, user.AccountActive, "active1-1", "active1@example.com", "Last1")
	usertest.CreateUserWithState(t, user.AccountActive, "active2", "active2@example.com", "Last1")
	usertest.CreateUserWithState(t, user.AccountDeactivated, "inactive", "active1@example.com", "Last1")

	for _, test := range relatedUserTests {
		var id string
		ua, err := user.AccountByUsername(test.username)
		if err != nil {
			t.Fatalf("Failed to fetch username=%s %v", test.username, err)
		}
		if ua == nil {
			id = "invalid"
		} else {
			id = ua.Id
		}
		_, result := apitest.RunRequest(
			test.key, nil, "GET", "/1/users/"+id+"/related", nil, nil)
		if result.Code != test.expectedStatus {
			t.Errorf("expected status=%d for test=%v, got %d", test.expectedStatus, test, result.Code)
			continue
		}
		if test.expectedStatus != http.StatusOK {
			continue
		}
		if result.ErrorResponse != nil {
			t.Errorf("Unexpected error response for test=%v error=%v", test, result.ErrorResponse)
			continue
		}

		data, ok := result.OkResponse.(map[string]interface{})
		if !ok {
			t.Errorf("Failed to parse ok response test=%v data=%#v", test, result.OkResponse)
			continue
		}

		entries, ok := data["user_ids"].([]string)
		if !ok {
			t.Errorf("Failed to parse user_ids response test=%v data=%#v", test, result.OkResponse)
			continue
		}

		if len(entries) != test.expectedCount {
			t.Errorf("Failed to parse user_ids response test=%v data=%#v", test, result.OkResponse)
		}

	}

}

var emailValidationTests = []struct {
	username   string
	email1     string
	email2     string
	emailLang  string
	dob        string
	emailname  string
	revalidate bool
	adult      string
}{
	{"TestEmailVa1", "testuservalidation@example.com", "testuservalidation@example.com",
		"en-US", "1950-01-01", "Verification", false, "T"},
	{"TestEmailVa2", "testuservalidation@example.com", "testuservalidation@example.com",
		"en-US", "2014-01-01", "VerificationUnder13", false, "F"},
	{"TestEmailVa4", "testuservalidation@example.com", "NEWEMAIL@example.com",
		"us-US", "2014-01-01", "Verification", true, "F"},
	{"TestEmailVa5", "testuservalidation@example.com", "TESTUSERVALIDATION@EXAMPLE.COM",
		"en-US", "2014-01-01", "Verification", false, "F"},
}

func (s *UsersAPITestSuite) TestEmailValidation() {
	t := s.T()
	captureEmail(true)
	defer captureEmail(false)

	for _, test := range emailValidationTests {
		username := test.username
		email := test.email1
		jsonData, _ := json.Marshal(map[string]interface{}{
			"username":   username,
			"password":   "123456",
			"email":      email,
			"email_lang": test.emailLang,
			"dob":        test.dob,
		})

		_, record := runCreateUserAPI(nil, jsonData)
		if record.OkResponse == nil {
			t.Fatalf("Cannot runCreateUserAPI %#v", record.ErrorResponse)
		}

		u, err := user.UserByUsername(username)
		if err != nil {
			t.Fatal("Error getting user by username ", err)
		}
		if u == nil {
			t.Fatal("Error getting user by username, user is nil")
		}
		initial_token := u.EmailVerify
		if initial_token == nil || *initial_token == "" {
			t.Fatal("No initial email verification token set")
		}

		e := getEmail(t, false)
		cont, etxt := emailContainsUrl(e, fmt.Sprintf("email-verifying?user_id=%s&token=%s&dloc=", u.Id, *initial_token))
		if !cont {
			t.Errorf("Email does not contain proper URL for vaidation: %q %q", etxt, fmt.Sprintf("email-verifying?user_id=%s&token=%s&dloc=", u.Id, *initial_token))
		}

		// Request a new verification code - OK no conflict here
		ua, _ := user.AccountByUsername(username) // todo, replace above user lookup with this
		session, _ := session.NewUserSession(ua)
		errr, record := apitest.RunRequest("user-key", session, "POST", fmt.Sprintf("/1/users/%s/verify_email", u.Id), nil, nil)
		if record.OkResponse == nil {
			t.Fatalf("Failed to get OK response for username=%q error=%+v, response=%+v", u.Username, errr, record.ErrorResponse)
		}

		// Burn the new email - fail if we didn't get one
		getEmail(t, false)

		u, err = user.UserByUsername(username)
		new_token := u.EmailVerify
		if new_token == nil || *new_token == "" {
			t.Fatal("Token not set for /verify_email")
		}

		if *new_token == *initial_token {
			t.Fatal("Tokens weren't actually changed in /verify_email")
		}

		// Validate email using new token
		_, record = apitest.RunRequest("user-key", nil, "GET", fmt.Sprintf("/1/users/%s/verify_email_confirm/%s", u.Id, *new_token), nil, nil)

		if record.Code != 302 {
			t.Fatalf("We didn't get a 302 redirect from calling verify_email_confirm for %s %+v",
				test.username, record.ResponseRecorder)
		}
		if !strings.Contains(record.HeaderMap["Location"][0], "email-validation-ok") {
			t.Fatalf("verify_email_confirm %s didn't send us to an ok response page %+v",
				test.username, record.ResponseRecorder)
		}
		if !strings.Contains(record.HeaderMap["Location"][0], fmt.Sprintf("u=%s", username)) {
			t.Fatalf("verify_email_confirm %s didn't have the right u param: %s",
				test.username, string(record.HeaderMap["Location"][0]))
		}
		if !strings.Contains(record.HeaderMap["Location"][0], "e=testuservalidation%40example.com") {
			t.Fatalf("verify_email_confirm %s didn't have the right e param: %s",
				test.username, record.HeaderMap["Location"][0])
		}
		if !strings.Contains(record.HeaderMap["Location"][0], fmt.Sprintf("a=%s", test.adult)) {
			t.Fatalf("verify_email_confirm %s didn't have the right a param: %s",
				test.username, record.HeaderMap["Location"][0])
		}

		// Confirm post-verification emails are sent to under-13 accounts
		e = getEmail(t, true)
		if test.adult == "F" && e == nil {
			t.Fatalf("Expected post-activation email for a child account %s", username)
		}
		if test.adult == "T" && e != nil {
			t.Fatalf("Should not have a post-activation email for an adult account %s", username)
		}

		// Try to validate again - should get 302 OK - already validated users will always be told they are OK
		_, record = apitest.RunRequest("user-key", nil, "GET", fmt.Sprintf("/1/users/%s/verify_email_confirm/%s", u.Id, *new_token), nil, nil)
		if record.Code != 302 {
			t.Fatalf("We didn't get a 302 redirect from calling verify_email_confirm again %+v", record.ResponseRecorder)
		}

		if !strings.Contains(record.HeaderMap["Location"][0], "email-validation-ok") {
			t.Fatalf("verify_email_confirm didn't send us to an ok response page %+v", record.ResponseRecorder)
		}
		if !strings.Contains(record.HeaderMap["Location"][0], fmt.Sprintf("u=%s", username)) {
			t.Fatalf("verify_email_confirm didn't have the right u param: %s", record.HeaderMap["Location"][0])
		}

		// Request a new verification code - 409 Conflict, already verified
		errr, record = apitest.RunRequest("user-key", session, "POST", fmt.Sprintf("/1/users/%s/verify_email", u.Id), nil, nil)
		if record.Code != 409 {
			t.Fatalf("Failed to get 409 response for username=%q error=%+v, response=%+v", u.Username, errr, record.ErrorResponse)
		}

		// Try to vaidate with Old token, should get an ok even though the token was invalid, because already-validated users are allowed to pass
		_, record = apitest.RunRequest("user-key", nil, "GET", fmt.Sprintf("/1/users/%s/verify_email_confirm/%s", u.Id, *new_token), nil, nil)
		if record.Code != 302 {
			t.Fatalf("We didn't get a 302 redirect from calling verify_email_confirm again %+v", record.ResponseRecorder)
		}
		if !strings.Contains(record.HeaderMap["Location"][0], "email-validation-ok") {
			t.Fatalf("verify_email_confirm didn't send us to an ok response page %+v", record.ResponseRecorder)
		}
		if !strings.Contains(record.HeaderMap["Location"][0], fmt.Sprintf("u=%s", username)) {
			t.Fatalf("verify_email_confirm didn't have the right u param: %s", record.HeaderMap["Location"][0])
		}

		// Update account to (potentially new) email - check if an update is sent
		jsonData, _ = json.Marshal(map[string]interface{}{
			"email": test.email2,
		})
		_, record = runUpdateUserAPI(session, u.Id, jsonData)
		e = getEmail(t, true)
		if test.revalidate && e == nil {
			t.Fatalf("Expected revalidation email on email update for username: %s", username)
		}
		if !test.revalidate && e != nil {
			t.Fatalf("Expected no revalidation email on email update for username but got one: %s %s", username, e)
		}
	}
}

var ResetUserPasswordTestsUsers = []struct {
	username string
	email    string
	verified bool
	deleted  bool
}{
	{"sam", "sam@example.com", true, false},
	{"bob", "bob@example.com", false, false},
	{"deleted", "deleted@example.com", false, true},
	{"kid1", "mom@example.com", true, false},
	{"kid2", "mom@example.com", false, false},
	{"kid3", "mom@example.com", false, true},
}

var ResetUserPasswordTests = []struct {
	username             string
	email                string
	userid               string
	expectedresult       int
	expectedemailto      string
	expectedlinkstouname []string // username we expect to see reset urls for
}{
	{"SAM", "", "", 200, "sam@example.com", []string{"sam"}},
	{"BOB", "", "", 200, "bob@example.com", []string{"bob"}},
	{"KID1", "", "", 200, "mom@example.com", []string{"kid1"}},
	{"kid2", "", "", 200, "mom@example.com", []string{"kid2"}},
	{"", "", "kid2", 200, "mom@example.com", []string{"kid2"}},
	{"", "SAM@example.com", "", 200, "sam@example.com", []string{"sam"}},
	{"", "MOM@example.com", "", 200, "mom@example.com", []string{"kid1", "kid2"}},
	{"", "not-exists@example.com", "", 404, "", []string{}},
	{"not_exists", "", "", 404, "", []string{}},
	{"", "", "", 404, "", []string{}},
	{"deleted", "", "", 404, "", []string{}},
}

func (s *UsersAPITestSuite) TestResetUserPassword() {
	defer captureEmail(false)
	t := s.T()
	var ua *user.UserAccount

	// map usernames to user ids
	byUsername := make(map[string]string)

	for _, userdata := range ResetUserPasswordTestsUsers {
		now := time.Now()
		status := user.AccountActive
		if userdata.deleted {
			status = user.AccountDeactivated
		}
		ua := &user.UserAccount{
			NewPassword: util.Pstring("hashme"),
			User: &user.User{
				Status:    status,
				Username:  util.Pstring(userdata.username),
				GivenName: util.Pstring("given name"),
				Email:     util.Pstring(userdata.email),
			}}
		if userdata.verified {
			ua.TimeEmailFirstVerified = &now
		}

		user.CreateUserAccount(ua)
		byUsername[userdata.username] = ua.Id
	}

	var record *jsonutil.JsonWriterRecorder
	for i, userdata := range ResetUserPasswordTests {
		frm := url.Values{"username": []string{userdata.username}, "email": []string{userdata.email}}
		if userdata.userid != "" {
			frm["userid"] = []string{byUsername[userdata.userid]}
		}
		fmt.Println("Frm", frm)
		msgs, _ := testutil.CaptureEmails(func() error {
			_, record = apitest.RunRequest("user-key", nil, "POST", "/1/reset_user_password", nil, frm)
			return nil
		})
		if record.Code != userdata.expectedresult {
			t.Fatalf("%d Failed to get expected code response from reset_user_password exp: %v got: %v test=%v", i, userdata.expectedresult, record.Code, userdata)
		}

		if userdata.expectedresult != 200 {
			continue
		}

		if record.OkResponse == nil {
			t.Fatalf("%d Failed to get OK response from reset_user_password  %#v test=%v", i, record.ErrorResponse, userdata)
		}

		if len(msgs) != 1 {
			t.Error("Multiple messages returned for test", userdata)
		}

		e := msgs[0]
		if !util.CaseInsensitiveContains(e.To[0].String(), userdata.expectedemailto) {
			t.Errorf("%d Create User Email not properly addressed %s %s", i, e.To[0], userdata.expectedemailto)
		}
		for _, expecteduname := range userdata.expectedlinkstouname {
			ua, _ = user.AccountByUsername(expecteduname)
			reseturl := fmt.Sprintf("password-reset?user_id=%s&token=%s&dloc=", ua.User.Id, *ua.User.PasswordReset)
			cont, etxt := emailContainsUrl(e, reseturl)
			if !cont {
				t.Errorf("%d Email does not contain proper URL for vaidation: email:%v reseturl:%v", i, etxt, reseturl)
			}
		}
	}
}

var ResetPasswordConfirmTokenOk = timedtoken.Token()
var ResetPasswordConfirmTokenNOMATCH = timedtoken.Token()
var ResetPasswordConfirmTokenExpired = timedtoken.TokenFromTime(time.Now().AddDate(-5, 0, 0))

var ResetPasswordConfirmTests = []struct {
	token       string
	password    string
	userid      string // "ok", "nil" or "invalid"
	expectedErr string // error code that should be in the redirect url
}{
	{ResetPasswordConfirmTokenOk, "pwoeirpwe!@T", "ok", ""}, // no error
	{ResetPasswordConfirmTokenOk, "93802398402", "ok", ""},  // no error
	{ResetPasswordConfirmTokenOk, "", "ok", "no_password"},
	{ResetPasswordConfirmTokenOk, "pwoeirpwe", "nil", "no_user_id"},
	{ResetPasswordConfirmTokenOk, "pwoeirpwe", "invalid", "user_not_found"},
	{"", "pwoeirpwe", "ok", "no_token"},
	{ResetPasswordConfirmTokenOk, "p", "ok", "err_pw_update"}, // password too short
	{ResetPasswordConfirmTokenExpired, "93802398402", "ok", "invalid_token"},
	{ResetPasswordConfirmTokenExpired, "", "nil", "no_user_id"},
	{ResetPasswordConfirmTokenNOMATCH, "wewr34o4!@R", "ok", "invalid_token"}, //This will map to a token that doesn't match the reset token
}

func (s *UsersAPITestSuite) TestResetPasswordConfirm() {
	t := s.T()

	for i, testdata := range ResetPasswordConfirmTests {
		username := fmt.Sprintf("bob_loop_%d", i)
		jsonData, _ := json.Marshal(map[string]interface{}{
			"username": username,
			"password": "123456",
			"email":    "bobefkjhw32@example.com",
		})
		_, record := runCreateUserAPI(nil, jsonData)
		if record.OkResponse == nil {
			t.Fatalf("Cannot runCreateUserAPI %#v", record.ErrorResponse)
		}

		ua, _ := user.AccountByUsername(username)
		err := ua.SetPasswordResetToken(testdata.token)
		if err != nil {
			t.Fatal("Could not set ResetToken", err)
		}

		//reset_password_confirm
		var frmUserId string
		switch testdata.userid {
		case "ok":
			frmUserId = ua.User.Id
		case "nil":
			frmUserId = ""
		case "invalid":
			frmUserId = "invalid"
		default:
			t.Fatal("Invalid setting for userid field in test", testdata.userid)
		}

		frmToken := testdata.token
		if frmToken == ResetPasswordConfirmTokenNOMATCH {
			frmToken = "NOMATCH"
		}
		frm := url.Values{
			"user_id":  []string{frmUserId},
			"token":    []string{frmToken},
			"password": []string{testdata.password}}
		_, record = apitest.RunRequest("user-key", nil, "POST", "/1/reset_password_confirm", nil, frm)
		if record.Code != 302 {
			t.Fatalf("Failed to get a redirect after hitting reset_password_confirm endpoint %d", record.Code)
		}

		ua, _ = user.AccountByUsername(username)
		pwChanged, err := ua.IsPasswordCorrect(testdata.password)
		if err != nil {
			t.Fatalf("cannot load username %s: %s", username, err)
		}

		if testdata.expectedErr == "" {
			// Expected succesful update
			if !strings.Contains(record.HeaderMap["Location"][0], "ok") {
				t.Fatalf("reset_password_confirm didn't send us to an ok response page %+v %+v", username, record.ResponseRecorder)
			}
			if !pwChanged {
				t.Fatalf("Password not set right for loop %s", username)
			}

		} else {
			// Expected no updates
			if !strings.Contains(record.HeaderMap["Location"][0], "error") {
				t.Fatalf("reset_password_confirm didn't send us to an error response page %+v %+v", username, record.ResponseRecorder)
			}
			// location url should end in err=code
			u, _ := url.Parse(record.HeaderMap["Location"][0])
			if err := u.Query().Get("err"); err != testdata.expectedErr {
				t.Errorf("Incorrect error code received for test=%s expected=%s actual=%s", username, testdata.expectedErr, err)
			}

			if pwChanged {
				t.Fatalf("Password was set when it shouldn't have been for loop %s", username)
			}
		}
	} // end for each test

	// token, password, user_id
	// test empty of each
	// test empty of all
	//
	// Text token not match user
}

func (s *UsersAPITestSuite) TestResetPasswordSessionInvalidation() {
	t := s.T()
	us1 := s.Users["u1-unlinked"]
	ua1 := us1.User
	token := ResetPasswordConfirmTokenOk
	if err := ua1.SetPasswordResetToken(token); err != nil {
		t.Fatal("Could not set ResetToken", err)
	}

	// create an extra session
	us2, err := session.NewUserSession(ua1)
	if err != nil {
		t.Fatal("Failed to create additional session", err)
	}

	newPassword := "changed"
	frm := url.Values{
		"user_id":  []string{ua1.Id},
		"token":    []string{token},
		"password": []string{newPassword},
	}
	_, record := apitest.RunRequest("user-key", nil, "POST", "/1/reset_password_confirm", nil, frm)
	if record.Code != 302 {
		t.Fatalf("Failed to get a redirect after hitting reset_password_confirm endpoint %d", record.Code)
	}

	if !strings.Contains(record.HeaderMap["Location"][0], "ok") {
		t.Fatalf("reset_password_confirm didn't send us to an ok response page %+v", record.ResponseRecorder)
	}
	// both sessions should now be unvalid
	check1, err1 := session.ByToken(us1.Session.Token())
	check2, err2 := session.ByToken(us2.Session.Token())
	if err1 != nil || err2 != nil {
		t.Fatal("Unexpected error from ByToken", err1, err2)
	}

	if check1 != nil || check2 != nil {
		t.Error("Original session tokens are still valid")
	}
}

// Check that a deactivated user can not trigger a password reset
func (s *UsersAPITestSuite) TestInactiveResetUserPassword() {
	defer captureEmail(false)
	t := s.T()
	us1 := usertest.CreateUserWithState(t, user.AccountDeactivated, "resetpw", "reset-inactive-pw@example.com", "Last1")

	// deactivate account
	//user.DeactivateUserAccount(us1.User.Id)
	// attempt to trigger password reset

	var record *jsonutil.JsonWriterRecorder
	msgs, _ := testutil.CaptureEmails(func() error {
		frm := url.Values{"email": []string{*us1.User.Email}}
		_, record = apitest.RunRequest("user-key", nil, "POST", "/1/reset_user_password", nil, frm)
		return nil
	})
	if len(msgs) != 0 {
		t.Errorf("Received %d unexpected messages", len(msgs))
	}

	if record.Code != 404 {
		t.Errorf("Did not get 404 resposne")
	}
}

func (s *UsersAPITestSuite) TestAccountUpdatedEmails() {
	// Currently accounts are notified of updates only if the password changes, which can be in two ways, via update or PW reset
	t := s.T()
	username := "tstusa"
	ua := &user.UserAccount{
		NewPassword: util.Pstring("hashme"),
		User: &user.User{
			Status:    user.AccountActive,
			Username:  util.Pstring(username),
			GivenName: util.Pstring("given name"),
			Email:     util.Pstring("testau@example.com"),
		}}
	user.CreateUserAccount(ua)

	ua, _ = user.AccountByUsername(username)
	session, _ := session.NewUserSession(ua)

	jsonUpdateUsername, _ := json.Marshal(map[string]interface{}{
		"username": "tstusb",
	})
	captureEmail(true)
	defer captureEmail(false)
	runUpdateUserAPI(session, ua.Id, jsonUpdateUsername)
	e := getEmail(t, true)
	if e != nil {
		t.Errorf("Not expecting any email from username update: %+v", e)
	}

	jsonUpdatePassword, _ := json.Marshal(map[string]interface{}{
		"password": "thisisatet123",
	})
	captureEmail(true)
	runUpdateUserAPI(session, ua.Id, jsonUpdatePassword)
	e = getEmail(t, true)
	cont, _ := emailContainsUrl(e, "An Anki account associated with this email address has been updated.")
	if !cont {
		t.Errorf("Email does not contain account updated text: %+v", e)
	}
}

var pmBounceTests = []struct {
	btype         string
	shouldDisable bool
}{
	{"HardBounce", true},
	{"SpamNotification", true},
	{"SpamComplaint", true},
	{"BadEmailAddress", true},
	{"ManuallyDeactivated", true},
	{"ManuallyDeactivated", true},
	{"Transient", false},
	{"SoftBounce", false},
}

func (s *UsersAPITestSuite) TestPostmarkBounceWebhook() {
	t := s.T()

	ua := usertest.CreateUserWithState(t, user.AccountActive, "test1", "test1@example.com", "ln")
	ua2 := usertest.CreateUserWithState(t, user.AccountActive, "test2", "test2@example.com", "ln")

	PostmarkHookAuthUsers["testuser"] = "testpw"
	for _, test := range pmBounceTests {
		ua.EmailFailureCode = nil
		user.UpdateUserAccount(ua)

		s.Audit.Records = nil
		jsonRequest, _ := json.Marshal(map[string]interface{}{
			"Type":  test.btype,
			"Email": "test1@example.com",
		})
		req := apitest.MakeRequest("match", nil, "POST", "/1/postmark_bounce_webhook", jsonRequest, nil)
		req.SetBasicAuth("testuser", "testpw")
		rec := req.Run()

		if rec.Code != http.StatusOK {
			pretty.Println(rec)
			t.Fatalf("btype=%q unexpected error %v", test.btype, rec)
		}

		if test.shouldDisable {
			require.NotEqual(t, 0, len(s.Audit.Records), "Failed to record audit record of bounce for "+test.btype)
			assert.Equal(t, ua.User.Id, s.Audit.Records[0].AccountID, "Audit record has wrong account ID for "+test.btype)
			assert.Equal(t, audit.ActionAccountEmailBounced, s.Audit.Records[0].Action, "Audit record has wrong action "+test.btype)
		}

		ua, _ = user.AccountById(ua.User.Id)
		if test.shouldDisable && util.NS(ua.EmailFailureCode) == "" {
			t.Errorf("btype=%q failure code not set", test.btype)
		} else if !test.shouldDisable && util.NS(ua.EmailFailureCode) != "" {
			t.Errorf("btype=%q failure code set to %q", test.btype, util.NS(ua.EmailFailureCode))
		}

		ua2, _ = user.AccountById(ua2.User.Id)
		if code := util.NS(ua2.EmailFailureCode); code != "" {
			t.Errorf("btype=%q unexpectedly updated failure code for ua2 to %q", test.btype, code)
		}
	}
}

// make sure user keys cannot be used to run/query tasks
func (s *UsersAPITestSuite) TestTaskPerms() {
	t := s.T()
	for _, taskName := range []string{"deactivate-unverified", "purge-deactivated", "notify-unverified", "email-csv"} {
		endpoint := "/1/tasks/users/" + taskName
		_, resp := apitest.RunRequest("user-key", nil, "GET", endpoint, nil, nil)
		if resp.Code != http.StatusForbidden {
			t.Errorf("task=%s did not return forbidden for user-key with no session got code=%v", taskName, resp.Code)
		}

		_, resp = apitest.RunRequest("match", s.Users["u1-unlinked"], "GET", endpoint, nil, nil)
		if resp.Code != http.StatusForbidden {
			t.Errorf("task=%s did not return forbidden for user-key with session got code=%v", taskName, resp.Code)
		}
	}
}

// Regression test for SAIP-103 - validate that password reset attempt
// on an account with an invalid email account results in an HTTP 400,
// not a 500.
func (s *UsersAPITestSuite) TestPasswordResetInvalidEmailResponseCode() {
	t := s.T()

	ua := usertest.CreateUserWithState(t, user.AccountActive, "test-invalid", "invalid@example.com", "ln")

	err := ua.User.SetEmailFailureCode("invalid_email")
	if err != nil {
		t.Fatalf("Unable to set email failure code. %s.", err)
	}

	_, resp := apitest.RunRequest("user-key", nil, "POST",
		"/1/reset_user_password", nil, url.Values{"email": {"invalid@example.com"}})
	if resp.Code != http.StatusBadRequest {
		t.Fatalf("Unexpected HTTP status code. %v", resp)
	}
}

//////// EMAIL UTILITY METHODS ////////////

// getEmail get the email from the default email.Email skyhook after captureEmail has been called
// This fails (if returnOnNill is false) if no email is sent within 1 second.
func getEmail(t *testing.T, returnOnNil bool) *postmarkapp.Message {
	var e *postmarkapp.Message
	pmc := email.Emailer.PostmarkC.(*postmarkapp.Client)
	select {
	case e = <-pmc.LastEmail:
	case <-time.After(time.Second):
		if !returnOnNil {
			t.Fatalf("No message was created!")
		}
	}
	return e
}

// captureEmail sets up email capture
// This will impact the rest of the tests but none of the tests actually send emails
// so that's ok
func captureEmail(cap bool) {
	email.InitAnkiEmailer()

	if cap {
		pmc := email.Emailer.PostmarkC.(*postmarkapp.Client)
		pmc.LastEmail = make(chan *postmarkapp.Message)
	}
}

// emailContainsUrl checks if the txt and html part of an email contain a string
func emailContainsUrl(e *postmarkapp.Message, ustr string) (bool, string) {
	htmlUrl := strings.Replace(ustr, "&", "&amp;", -1)
	if e == nil {
		return false, "NO EMAIL BODY!"
	}
	byt, _ := ioutil.ReadAll(e.TextBody)
	txtt := string(byt)
	e.TextBody = bytes.NewReader(byt)
	if !strings.Contains(txtt, ustr) {
		return false, txtt
	}
	byt, _ = ioutil.ReadAll(e.HtmlBody)
	txth := string(byt)
	e.HtmlBody = bytes.NewReader(byt)
	if !strings.Contains(txth, htmlUrl) {
		return false, txth
	}
	return true, txtt
}

func emailTxt(e *postmarkapp.Message) string {
	if e == nil {
		return "NULL"
	}
	byt, _ := ioutil.ReadAll(e.TextBody)
	return string(byt)
}

////////// END EMAIL UTILITY METHODS //////////
