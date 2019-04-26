// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

package user

import (
	"bytes"
	"encoding/csv"
	"fmt"
	"io/ioutil"
	"net/url"
	"reflect"
	"sort"
	"strconv"
	"strings"
	"testing"
	"time"

	"github.com/anki/sai-go-accounts/apierrors"
	"github.com/anki/sai-go-accounts/db"
	"github.com/anki/sai-go-accounts/db/dbtest"
	"github.com/anki/sai-go-accounts/email"
	"github.com/anki/sai-go-accounts/testutil"
	"github.com/anki/sai-go-accounts/util"
	autil "github.com/anki/sai-go-util"
	"github.com/anki/sai-go-util/date"
	"github.com/anki/sai-go-util/postmarkapp"
	"github.com/anki/sai-go-util/timedtoken"
	"github.com/anki/sai-go-util/validate"
	uuid "github.com/satori/go.uuid"
	"github.com/stretchr/testify/suite"
)

var (
	GoodUser = &User{
		Status:     AccountActive,
		Username:   util.Pstring("good"),
		Email:      util.Pstring("a@example.com"),
		GivenName:  util.Pstring("name"),
		FamilyName: util.Pstring("name"),
		Gender:     util.Pstring("M"),
		DOB:        date.MustNew(2000, 1, 1),
	}
)

func init() {
}

func createNamedModelUa(t *testing.T, username string, remoteId string) *UserAccount {
	ua := &UserAccount{
		NewPassword: util.Pstring("hashme"),
		User: &User{
			Status:    AccountActive,
			Username:  util.Pstring(username),
			GivenName: util.Pstring("given name"),
			Email:     util.Pstring("a@b"),
			EmailLang: util.Pstring("en-US"),
		},
	}
	return ua
}

func createNamedModelUser(t *testing.T, username string, remoteId string) *UserAccount {
	ua := createNamedModelUa(t, username, remoteId)
	createFromUa(t, ua)
	return ua
}

func createUserWithState(t *testing.T, status AccountStatus, username, email, familyName string) *UserAccount {
	ua := &UserAccount{
		NewPassword: util.Pstring("hashme"),
		User: &User{
			Status:     status,
			Username:   &username,
			FamilyName: &familyName,
			Email:      &email,
		},
	}
	createFromUa(t, ua)
	return ua
}

func createFromUa(t *testing.T, ua *UserAccount) *UserAccount {
	errs := CreateUserAccount(ua)
	if errs != nil {
		t.Fatalf("Failed to create user %q account: %s", util.NS(ua.User.Username), errs)
	}
	return ua
}

func createModelUser(t *testing.T) *UserAccount {
	return createNamedModelUser(t, "user1", "")
}

func compareErrors(expected map[string]string, actual validate.FieldErrors) error {
	if len(expected) != len(actual) {
		return fmt.Errorf("Incorrect number of fields in error result.  "+
			"expected=%#v actual=%#v", expected, actual)
	}

	for fname, errCode := range expected {
		if len(actual[fname]) != 1 {
			return fmt.Errorf("Received incorrect number of error responses for field=%q "+
				"expected=1 actual=%d, errors=%v",
				fname, len(actual[fname]), map[string][]*validate.FieldError(actual))
		}
		if actual[fname][0].Code != errCode {
			return fmt.Errorf("Incorrect error code set for field=%q expected=%q actual=%q",
				fname, errCode, actual[fname][0].Code)
		}
	}
	return nil
}

type UserModelSuite struct {
	dbtest.DBSuite
}

func TestUserModelSuite(t *testing.T) {
	suite.Run(t, new(UserModelSuite))
}

const (
	// 4 byte utf8 emoji; MySQL cannot store these unless
	// using the utf8mb4 character set
	emojiWink       = "\xF0\x9F\x98\x89"
	emojiCry        = "\xF0\x9F\x98\xA2"
	emojiWeary      = "\xF0\x9F\x98\xA9"
	emojiHearNoEvil = "\xF0\x9F\x99\x89"
)

var unicodeTests = []string{
	"justascii",
	"abcdefghijk\xe2\x84\xa2",
	emojiWink + emojiCry + emojiWeary + emojiHearNoEvil,
}

func addBlockedEmail(addr string) {
	_, err := db.Dbmap.Db.Exec("INSERT INTO blocked_emails (email, time_blocked, blocked_by, reason) VALUES (?, ?, ?, ?)",
		addr, time.Now(), "test-user", "test")
	if err != nil {
		panic(fmt.Sprintf("Failed to add email %q to blocked_emails: %s", addr, err))
	}
}

func (suite *UserModelSuite) SetupTest() {
	suite.DBSuite.SetupTest()

	// A number of tests rely on having an entry in the blocked_emails table
	addBlockedEmail("blocked@example.net")
}

func (suite *UserModelSuite) TestUnicodeUsernames() {
	t := suite.T()

	for _, username := range unicodeTests {
		ua := &UserAccount{
			NewPassword: util.Pstring("hashme"),
			User: &User{
				Status:   AccountActive,
				Username: util.Pstring(username),
				Email:    util.Pstring("unicodetest@example.com"),
			},
		}

		// Fetch before creating the account; should be a negative result
		u, err := UserByUsername(username)
		if err != nil {
			t.Errorf("username=%q fetch failed err=%#v", username, err)
		}

		if u != nil {
			t.Errorf("Unexpected user returned for non-existant username %q user=%#v", username, u)
		}

		if err := CreateUserAccount(ua); err != nil {
			t.Errorf("username=%q create failed err=%v", username, err)
			continue
		}

		u, err = UserByUsername(username)
		if err != nil {
			t.Errorf("username=%q fetch failed err=%#v", username, err)
		}

		if *u.Username != username {
			t.Errorf("username=%q does not match stored copy=%q", username, *u.Username)
		}
	}
}

func (suite *UserModelSuite) TestAccountByUsernameOK() {
	t := suite.T()
	ua := createModelUser(t)
	for _, isUpper := range []bool{false, true} {
		username := *ua.User.Username
		if isUpper {
			username = strings.ToUpper(username)
		}
		out, err := AccountByUsername(username)
		if err != nil {
			t.Fatalf("Unexpected error err=%v", err)
		}
		if out == nil {
			t.Fatalf("Failed to fetch user %q", username)
		}
		if ua.User.Id != out.User.Id {
			t.Fatalf("Got wrong user id expected=%q actual=%q username=%q", ua.User.Id, out.User.Id, username)
		}
	}
}

func (suite *UserModelSuite) TestAccountByUsernameNotFound() {
	t := suite.T()
	// should not return an error, but out should be nil
	out, err := AccountByUsername("invalid")
	if err != nil {
		t.Fatalf("Unexpected error err=%v", err)
	}
	if out != nil {
		t.Fatalf("Got unexpected user=%#v", out)
	}
}

func (suite *UserModelSuite) TestUserByUsernameOK() {
	t := suite.T()
	ua := createModelUser(t)
	for _, isUpper := range []bool{false, true} {
		username := *ua.User.Username
		if isUpper {
			username = strings.ToUpper(username)
		}
		out, err := AccountByUsername(username)
		if err != nil {
			t.Fatalf("Unexpected error err=%v", err)
		}
		if out == nil {
			t.Fatalf("Failed to fetch user %q", username)
		}
		if ua.User.Id != out.Id {
			t.Fatalf("Got wrong user id expected=%q actual=%q username=%q", ua.User.Id, out.Id, username)
		}
	}
}

func (suite *UserModelSuite) TestSingleAccountByEmailOk() {
	t := suite.T()
	ua := createModelUser(t)
	for _, isUpper := range []bool{false, true} {
		email := *ua.User.Email
		if isUpper {
			email = strings.ToUpper(email)
		}
		accounts, err := AccountsByEmail(email)
		if err != nil {
			t.Fatalf("Unexpected error err=%v", err)
		}
		if accounts == nil || len(accounts) != 1 {
			t.Fatalf("Failed to fetch email=%q", email)
		}
		out := accounts[0]
		if ua.User.Id != out.Id {
			t.Fatalf("Got wrong user id expected=%q actual=%q email=%q", ua.User.Id, out.Id, email)
		}
	}
}

// TODO add TestMultiAccountByEmail

func (suite *UserModelSuite) TestUserByUsernameNotFound() {
	t := suite.T()
	// should not return an error, but out should be nil
	out, err := UserByUsername("invalid")
	if err != nil {
		t.Fatalf("Unexpected error err=%v", err)
	}
	if out != nil {
		t.Fatalf("Got unexpected user=%#v", out)
	}
}

var validationTests = []struct {
	name                string
	ua                  *UserAccount
	expectedFieldErrors map[string]string
}{
	{"bad_status", &UserAccount{
		User: &User{
			Status: "-",
		},
	}, map[string]string{"status": "not_one_of", "password": "bad_char_count"},
	},

	{"active_omitted_fields", &UserAccount{
		NewPassword: util.Pstring("valid password"),
		User: &User{
			Status: AccountActive,
		},
	}, map[string]string{"email": "invalid_email_address", "username": "bad_char_count"},
	},

	{"omitted_password", &UserAccount{
		User: &User{
			Status:   AccountActive,
			Username: util.Pstring("username"),
			Email:    util.Pstring("test@example.com"),
		},
	}, map[string]string{"password": "bad_char_count"},
	},

	{"active_bad_field_lengths", &UserAccount{
		NewPassword: util.Pstring("valid password"),
		User: &User{
			Status:     AccountActive,
			Username:   util.Pstring("x"), // too short
			Email:      util.Pstring("a"),
			GivenName:  util.Pstring(strings.Repeat("a", 101)),
			FamilyName: util.Pstring(strings.Repeat("a", 101)),
			Gender:     util.Pstring("Z"),
		},
	}, map[string]string{
		"email":       "invalid_email_address",
		"username":    "bad_char_count",
		"given_name":  "bad_char_count",
		"family_name": "bad_char_count",
		"gender":      "not_match_any"},
	},

	{"non_ascii_email", &UserAccount{
		NewPassword: util.Pstring("valid password"),
		User: &User{
			Status:   AccountActive,
			Username: util.Pstring("username"),
			Email:    util.Pstring("testç@example.com"),
		},
	}, map[string]string{"email": "invalid_email_address"},
	},

	{"dob-out-of-range", &UserAccount{
		NewPassword: util.Pstring("valid password"),
		User: &User{
			Status:   AccountActive,
			Username: util.Pstring("auser"),
			Email:    util.Pstring("test@example.com"),
			DOB:      date.MustNew(2200, 1, 1),
		},
	}, map[string]string{
		"dob": "not_match_any",
	}},
	{"dob-out-of-range-early", &UserAccount{
		NewPassword: util.Pstring("valid password"),
		User: &User{
			Status:   AccountActive,
			Username: util.Pstring("auser"),
			Email:    util.Pstring("test@example.com"),
			DOB:      date.MustNew(1899, 12, 31),
		},
	}, map[string]string{
		"dob": "not_match_any",
	}},
	{"dob-out-of-range-tomorrow", &UserAccount{
		NewPassword: util.Pstring("valid password"),
		User: &User{
			Status:   AccountActive,
			Username: util.Pstring("auser"),
			Email:    util.Pstring("test@example.com"),
			DOB:      autil.Must2(date.FromTime(time.Now().Add(24 * time.Hour))).(date.Date), // tomorrow
		},
	}, map[string]string{
		"dob": "not_match_any",
	}},
}

func (suite *UserModelSuite) TestValidateAccountFailures() {
	t := suite.T()

	for _, tst := range validationTests {
		errs := CreateUserAccount(tst.ua)

		if tst.expectedFieldErrors == nil {
			if errs != nil {
				t.Errorf("Unexpected errors from validation for test=%s: %#v", tst.name, errs)
			}
			continue
		}
		ferrs, ok := errs.(validate.FieldErrors)
		if !ok {
			t.Errorf("Unexpected error result from CreateUserAccount for test=%s: %s", tst.name, errs)
			continue
		}

		if testErr := compareErrors(tst.expectedFieldErrors, ferrs); testErr != nil {
			t.Errorf("Validation failed for test=%s: %s", tst.name, testErr)
		}
	}
}

// test dob date extremes are ok
func (suite *UserModelSuite) TestValidateAccountDob() {
	t := suite.T()
	for i, d := range []date.Date{date.MustNew(1900, 01, 01), date.Today()} {
		ac := &UserAccount{
			NewPassword: util.Pstring("valid password"),
			User: &User{
				Status:   AccountActive,
				Username: util.Pstring(fmt.Sprintf("user%d", i)),
				Email:    util.Pstring("test@example.com"),
				DOB:      d,
			},
		}

		errs := CreateUserAccount(ac)
		if errs != nil {
			t.Error("Unexpected error for DOB", d, errs)
		}
	}
}

// If no email address is provided at all, we should get an error
func (suite *UserModelSuite) TestCreateAccountNoEmail() {
	t := suite.T()

	user := *GoodUser // make a copy
	user.Email = nil
	ua := &UserAccount{
		NewPassword: util.Pstring("valid password"),
		User:        &user,
	}
	err := CreateUserAccount(ua)
	if err == nil {
		t.Fatal("Did not receive an error on create")
	}
	if err, ok := err.(validate.FieldErrors); ok {
		if len(err) != 1 || len(err["email"]) == 0 {
			t.Fatalf("Did not get expected errors; got %v", err)
		}
	} else {
		t.Fatalf("Did not get expected errors; got %#v", err)
	}
}

var passwordTests = []struct {
	password      string
	isComplex     bool
	expectedError error
}{
	{"abcdef-username", false, nil},
	{"abcdef", false, nil},
	{"ABCdef1.", true, nil},
}

func (suite *UserModelSuite) TestCreateAccountComplexPassword() {
	t := suite.T()
	for i, test := range passwordTests {
		user := *GoodUser // make a copy
		user.Username = util.Pstring("user" + strconv.Itoa(i))
		testPassword := strings.Replace(test.password, "username", *user.Username, -1)
		ua := &UserAccount{
			NewPassword: &testPassword,
			User:        &user,
		}
		err := CreateUserAccount(ua)
		if err != test.expectedError {
			t.Errorf("password=%q error mismatch expected=%v actual=%v",
				testPassword, test.expectedError, err)
		}
		if err != nil {
			continue
		}
		if ua.User.PasswordIsComplex != test.isComplex {
			t.Errorf("password=%q iscomplex mismatch expected=%t actual=%v",
				testPassword, test.isComplex, ua.User.PasswordIsComplex)
		}
	}
}

func (suite *UserModelSuite) TestCreateAccountMetadataFields() {
	t := suite.T()
	ua := &UserAccount{
		NewPassword: util.Pstring("changeme"),
		User: &User{
			Status:               AccountActive,
			Username:             util.Pstring("myuser"),
			Email:                util.Pstring("metadata-test@example.com"),
			DriveGuestId:         util.Pstring("drive-guest-id"),
			CreatedByAppName:     util.Pstring("app-name"),
			CreatedByAppVersion:  util.Pstring("app-version"),
			CreatedByAppPlatform: util.Pstring("app-platform"),
		},
	}

	if err := CreateUserAccount(ua); err != nil {
		t.Fatal("Unexpected error creating account", err)
	}

	// fetch
	ua2, err := AccountById(ua.User.Id)
	if err != nil {
		t.Fatal("Failed to fetch created account", err)
	}

	u := ua2.User
	// check fields are stripped correctly
	for _, tst := range []struct{ fn, fv, expected string }{
		{"username", *u.Username, "myuser"},
		{"drive_guest_id", *u.DriveGuestId, "drive-guest-id"},
		{"app_name", *u.CreatedByAppName, "app-name"},
		{"app_version", *u.CreatedByAppVersion, "app-version"},
		{"app_platform", *u.CreatedByAppPlatform, "app-platform"}} {
		if tst.fv != tst.expected {
			t.Errorf("Incorrect %s expected=%q actual=%q", tst.fn, tst.expected, tst.fv)
		}
	}
}

// ensure that whitespace is trimmed from text fields
func (suite *UserModelSuite) TestCreateAccountTrimSpaces() {
	t := suite.T()
	ua := &UserAccount{
		NewPassword: util.Pstring("changeme"),
		User: &User{
			Status:       AccountActive,
			Username:     util.Pstring("    myuser    "),
			FamilyName:   util.Pstring("   family-name    "),
			GivenName:    util.Pstring("   given-name    "),
			Email:        util.Pstring("   spacetest@example.com    "),
			Gender:       util.Pstring("   M    "),
			DriveGuestId: util.Pstring("   drive-guest-id    "),
		},
	}
	if err := CreateUserAccount(ua); err != nil {
		t.Fatal("Unexpected error creating account", err)
	}

	// fetch
	ua2, err := AccountById(ua.User.Id)
	if err != nil {
		t.Fatal("Failed to fetch created account", err)
	}

	u := ua2.User
	// check fields are stripped correctly
	for _, tst := range []struct{ fn, fv, expected string }{
		{"username", *u.Username, "myuser"},
		{"family_name", *u.FamilyName, "family-name"},
		{"given_name", *u.GivenName, "given-name"},
		{"email", *u.Email, "spacetest@example.com"},
		{"gender", *u.Gender, "M"},
		{"drive_guest_id", *u.DriveGuestId, "drive-guest-id"}} {
		if tst.fv != tst.expected {
			t.Errorf("Incorrect %s expected=%q actual=%q", tst.fn, tst.expected, tst.fv)
		}
	}
}

func (suite *UserModelSuite) TestUpdateAccountTrimSpaces() {
	t := suite.T()
	user := *GoodUser // make a copy
	ua := &UserAccount{
		NewPassword: util.Pstring("first password"),
		User:        &user,
	}
	if err := CreateUserAccount(ua); err != nil {
		t.Fatal("Failed to create user", err)
	}

	ua.Username = util.Pstring("    myuser    ")
	ua.FamilyName = util.Pstring("   family-name    ")
	ua.GivenName = util.Pstring("   given-name    ")
	ua.Email = util.Pstring("   spacetest@example.com    ")
	ua.Gender = util.Pstring("   M    ")
	ua.DriveGuestId = util.Pstring("   drive-guest-id    ")

	if err := UpdateUserAccount(ua); err != nil {
		t.Fatal("Failed to update user", err)
	}

	// fetch
	ua2, err := AccountById(ua.User.Id)
	if err != nil {
		t.Fatal("Failed to fetch created account", err)
	}

	u := ua2.User
	// check fields are stripped correctly
	for _, tst := range []struct{ fn, fv, expected string }{
		{"username", *u.Username, "myuser"},
		{"family_name", *u.FamilyName, "family-name"},
		{"given_name", *u.GivenName, "given-name"},
		{"email", *u.Email, "spacetest@example.com"},
		{"gender", *u.Gender, "M"},
		{"drive_guest_id", *u.DriveGuestId, "drive-guest-id"}} {
		if tst.fv != tst.expected {
			t.Errorf("Incorrect %s expected=%q actual=%q", tst.fn, tst.expected, tst.fv)
		}
	}
}

func (suite *UserModelSuite) TestCreateAccountBlockedEmail() {
	// attempts to create an account usikng a blocked email address should fail
	t := suite.T()
	ua := &UserAccount{
		NewPassword: util.Pstring("changeme"),
		User: &User{
			Status:   AccountActive,
			Username: util.Pstring("myuser"),
			Email:    util.Pstring("blocked@example.net"),
		},
	}
	if err := CreateUserAccount(ua); err != apierrors.EmailBlocked {
		t.Fatal("Incorrect error returned", err)
	}
}

func (suite *UserModelSuite) TestUpdateAccountComplexPassword() {
	t := suite.T()
	user := *GoodUser // make a copy
	ua := &UserAccount{
		NewPassword: util.Pstring("aaaaaa"),
		User:        &user,
	}
	if err := CreateUserAccount(ua); err != nil {
		t.Fatal("Failed to create user", err)
	}
	for _, test := range passwordTests {
		testPassword := strings.Replace(test.password, "username", *user.Username, -1)
		ua.NewPassword = &testPassword
		err := UpdateUserAccount(ua)
		if err != test.expectedError {
			t.Errorf("password=%q error mismatch expected=%v actual=%v",
				testPassword, test.expectedError, err)
		}
		if err != nil {
			continue
		}
		if ua.User.PasswordIsComplex != test.isComplex {
			t.Errorf("password=%q iscomplex mismatch expected=%t actual=%v",
				testPassword, test.isComplex, ua.User.PasswordIsComplex)
		}
	}
}

func (suite *UserModelSuite) TestUpdateAccountChangePassword() {
	t := suite.T()
	user := *GoodUser // make a copy
	ua := &UserAccount{
		NewPassword: util.Pstring("first password"),
		User:        &user,
	}
	if err := CreateUserAccount(ua); err != nil {
		t.Fatal("Failed to create user", err)
	}

	ua, _ = AccountById(ua.User.Id)
	// check login with original password
	if ok, _ := ua.IsPasswordCorrect("first password"); !ok {
		t.Errorf("Created password doesn't work")
	}

	// update the password
	ua.NewPassword = util.Pstring("second password")
	UpdateUserAccount(ua)

	ua, _ = AccountById(ua.User.Id)
	// check login with original password
	if ok, _ := ua.IsPasswordCorrect("second password"); !ok {
		t.Errorf("Updated password doesn't work")
	}
}

func (suite *UserModelSuite) TestEmailValidatation() {
	t := suite.T()
	start := time.Now().Add(-time.Second)
	user := *GoodUser // make a copy
	ua := &UserAccount{
		NewPassword: util.Pstring("first password"),
		User:        &user,
	}
	if err := CreateUserAccount(ua); err != nil {
		t.Fatal("Failed to create user", err)
	}

	if ua.EmailIsVerified {
		t.Fatal("email was already verified!")
	}

	if err := ua.SetEmailValidationToken("test-token"); err != nil {
		t.Fatal("Failed setting token", err)
	}

	ua, _ = AccountById(ua.User.Id)
	if ua.User.EmailIsVerified {
		t.Error("Email should not yet be verified")
	}
	if ua.User.EmailVerify == nil || *ua.User.EmailVerify != "test-token" {
		t.Errorf("Token was not set")
	}

	if err := ua.SetEmailValidated(); err != nil {
		t.Fatal("failed clearing token")
	}

	ua, _ = AccountById(ua.User.Id)
	if !ua.User.EmailIsVerified {
		t.Error("Email not verified")
	}
	if ua.User.EmailVerify != nil {
		t.Errorf("Token was not cleared")
	}

	if ua.User.TimeEmailFirstVerified == nil || ua.User.TimeEmailFirstVerified.Before(start) {
		t.Errorf("time email first verified not set")
	}
}

// check that reverifying an email address does not reset the time it was first verified
func (suite *UserModelSuite) TestEmailValidFirstVerified() {
	t := suite.T()
	validTime := time.Date(2014, 8, 27, 12, 0, 0, 0, time.UTC)
	user := *GoodUser // make a copy
	ua := &UserAccount{
		NewPassword: util.Pstring("first password"),
		User:        &user,
	}
	ua.User.TimeEmailFirstVerified = &validTime
	if err := CreateUserAccount(ua); err != nil {
		t.Fatal("Failed to create user", err)
	}

	if err := ua.SetEmailValidated(); err != nil {
		t.Fatal("failed clearing token")
	}

	ua, _ = AccountById(ua.User.Id)
	if !ua.User.EmailIsVerified {
		t.Error("Email not verified")
	}
	if ua.User.EmailVerify != nil {
		t.Errorf("Token was not cleared")
	}

	if ua.User.TimeEmailFirstVerified == nil || *ua.User.TimeEmailFirstVerified != validTime {
		t.Errorf("time email first verified  was changed to %#v", ua.User.TimeEmailFirstVerified)
	}
}

func (suite *UserModelSuite) TestCreateDuplicateUsername() {
	t := suite.T()
	// Create an account
	ua := &UserAccount{
		NewPassword: util.Pstring("123456"),
		User: &User{
			Username: util.Pstring("foobar"),
			Email:    util.Pstring("a@b"),
			Status:   AccountActive},
	}
	if err := CreateUserAccount(ua); err != nil {
		t.Fatalf("Failed to create account: %s", err)
	}
	err := CreateUserAccount(ua)
	if err != apierrors.DuplicateUsername {
		t.Fatalf("Didn't get expected error err=%#v", err)
	}
}

func (suite *UserModelSuite) TestUpdateDuplicateUsername() {
	t := suite.T()
	// Create an account
	ua1 := &UserAccount{
		NewPassword: util.Pstring("123456"),
		User: &User{Username: util.Pstring("user1"),
			Email:  util.Pstring("a@b"),
			Status: AccountActive},
	}
	ua2 := &UserAccount{
		NewPassword: util.Pstring("123456"),
		User: &User{Username: util.Pstring("user2"),
			Email:  util.Pstring("a@b"),
			Status: AccountActive},
	}
	if err := CreateUserAccount(ua1); err != nil {
		t.Fatalf("Failed to create account: %s", err)
	}
	if err := CreateUserAccount(ua2); err != nil {
		t.Fatalf("Failed to create account: %s", err)
	}
	// attempt to change user2 username to user1
	ua2.User.Username = util.Pstring("user1")
	if err := UpdateUserAccount(ua2); err != apierrors.DuplicateUsername {
		t.Fatalf("Didn't get expected error err=%#v", err)
	}
	// try again with user3; should be no error
	ua2.User.Username = util.Pstring("user3")
	if err := UpdateUserAccount(ua2); err != nil {
		t.Fatalf("Unexpected error %s", err)
	}
}

var validationOkTests = []struct {
	name string
	ua   *UserAccount
}{
	{"active_good_no_linked", &UserAccount{
		NewPassword: util.Pstring("valid password"),
		User: &User{
			Status:       AccountActive,
			DriveGuestId: util.Pstring("guest-id"),
			Username:     util.Pstring("good"),
			Email:        util.Pstring("a@example.com"),
			GivenName:    util.Pstring("name"),
			FamilyName:   util.Pstring("name"),
			Gender:       util.Pstring("M"),
		},
	},
	},
}

func (suite *UserModelSuite) TestCreateUserAccountOk() {
	// check that validation works
	t := suite.T()

	for _, tst := range validationOkTests {
		dbtest.RunDbTest(t, func() {
			errs := CreateUserAccount(tst.ua)
			if errs != nil {
				t.Errorf("Unexpected errors from validation for test=%s: %#v",
					tst.name, errs)
				return
			}
			if len(tst.ua.User.Id) < 15 {
				t.Errorf("New User entry didn't get an id for test=%s id=%q",
					tst.name, tst.ua.User.Id)
				return
			}
			readUa, err := AccountById(tst.ua.User.Id)
			if err != nil || readUa == nil {
				t.Errorf("Failed to load user entry for test=%s id=%q readUa=%v err=%s",
					tst.name, tst.ua.User.Id, readUa, err)
				return
			}
			if *readUa.User.Username != *tst.ua.User.Username {
				t.Errorf("Username did not save correctly for test=%s expected=%q actual=%q",
					tst.name, *tst.ua.User.Username, *readUa.User.Username)
			}
		})
	}
}

func (suite *UserModelSuite) TestCreateUserAccountPasswordHashed() {
	t := suite.T()
	ua := &UserAccount{
		NewPassword: util.Pstring("hashme"),
		User: &User{
			Status:   AccountActive,
			Username: util.Pstring("another"),
			Email:    util.Pstring("a@b"),
		}}
	errs := CreateUserAccount(ua)
	if errs != nil {
		t.Fatalf("Failed to create user account: %s", errs)
	}
	p, err := db.Dbmap.SelectNullStr("SELECT password_hash from users where user_id=?", ua.User.Id)
	if err != nil {
		t.Fatalf("Failed to load user account: %s", err)
	}
	if !p.Valid {
		t.Fatalf("Failed to load user account - no hashed row")
	}
	if !strings.HasPrefix(p.String, "scrypt$") {
		t.Errorf("Password does not appear to be hashed p=%q", p.String)
	}
}

func (suite *UserModelSuite) TestUserAccountPasswordHashed() {
	t := suite.T()
	ua := createModelUser(t)
	oldHash := ua.User.PasswordHash

	ua.NewPassword = util.Pstring("Edited password")
	err := UpdateUserAccount(ua)
	if err != nil {
		t.Fatalf("Update failed: %s", err)
	}

	ua, err = AccountById(ua.User.Id)
	if err != nil {
		t.Fatalf("Retrieve failed: %s", err)
	}

	if ua.User.PasswordHash == oldHash {
		t.Errorf("Password hash was not updated hash=%q", oldHash)
	}
}

func (suite *UserModelSuite) TestUserAccountEditField() {
	t := suite.T()
	ua := createModelUser(t)
	oldGivenName := *ua.User.GivenName

	ua.User.GivenName = util.Pstring("updated")

	err := UpdateUserAccount(ua)
	if err != nil {
		t.Fatalf("Update failed: %s", err)
	}

	ua, err = AccountById(ua.User.Id)
	if err != nil {
		t.Fatalf("Retrieve failed: %s", err)
	}

	if *ua.User.GivenName == oldGivenName {
		t.Errorf("Given name was not updated actual=%q", *ua.User.GivenName)
	}
}

func (suite *UserModelSuite) TestUserAccountDeleteOk() {
	t := suite.T()
	ua := createModelUser(t)
	userId := ua.User.Id
	username := *ua.User.Username

	err := DeactivateUserAccount(userId, "test-reason")
	if err != nil {
		t.Fatalf("Unexpected error while deactivating %v", err)
	}

	ua, _ = AccountById(userId)
	if ua.User.Status != AccountDeactivated {
		t.Errorf("Account in wrong state expected=%q actual=%q",
			AccountDeactivated, ua.User.Status)
	}
	if ua.User.Username != nil {
		t.Error("Account username was not set to nil")
	}
	if ua.User.DeactivatedUsername == nil || *ua.User.DeactivatedUsername != username {
		t.Errorf("Deactivated username incorrect; expected=%q actual=%#v",
			username, ua.User.DeactivatedUsername)
	}
	if ua.User.DeactivationReason == nil || *ua.User.DeactivationReason != "test-reason" {
		t.Errorf("Incorrect deactivation reason expected=%q actual=%q", "test-reason", util.NS(ua.User.DeactivationReason))
	}

}

// Check that users in a purged or deactivated state don't have any further
// action applied to them if deactivate is called again.
func (suite *UserModelSuite) TestUserAccountDeleteNoops() {
	t := suite.T()
	ua := createModelUser(t)
	user := ua.User
	userId := ua.User.Id

	for _, status := range []AccountStatus{AccountDeactivated, AccountPurged} {
		user.Status = status
		db.Dbmap.Update(user)
		err := DeactivateUserAccount(userId, "test")
		if err != nil {
			t.Fatalf("Unexpected error while deactivating %v", err)
		}

		ua, _ = AccountById(userId)
		if ua.User.Status != status {
			t.Errorf("Account in wrong state expected=%q actual=%q", status, ua.User.Status)
		}

	}
}

func (suite *UserModelSuite) TestUserAccountDeleteInvalidState() {
	t := suite.T()
	ua := createModelUser(t)
	user := ua.User
	userId := ua.User.Id
	user.Status = AccountStatus("-")
	db.Dbmap.Update(user)
	err := DeactivateUserAccount(userId, "test")
	if err == nil || err.Error() != "Account in unknown state" {
		t.Errorf("Didn't get expected error response %q", err)
	}
}

var purgeAccountTests = []struct {
	status        AccountStatus
	expectedError error
}{
	{AccountActive, apierrors.CannotPurgeActive},
	{AccountPurged, apierrors.AccountAlreadyPurged},
	{AccountDeactivated, nil},
}

func (suite *UserModelSuite) TestUserAccountPurge() {
	t := suite.T()
	ua := createModelUser(t)
	user := ua.User
	userId := user.Id

	for _, test := range purgeAccountTests {
		user.Status = test.status
		db.Dbmap.Update(user)
		err := PurgeUserAccount(userId, "test")

		if test.expectedError != nil {
			if err != test.expectedError {
				t.Errorf("status=%q did not get expected error expected=%v actual=%v",
					test.status, test.expectedError, err)
			}
		} else {
			if err != nil {
				t.Errorf("status=%q unexpected error=%v", test.status, err)
			} else {
				check, _ := AccountById(userId)
				if check.User.Status != AccountPurged {
					t.Errorf("status=%q Account not in purged state, got %q", test.status, check.User.Status)
				}
				if check.User.Email != nil {
					t.Errorf("status=%q Email not nil, got %v", test.status, *check.User.Email)
				}
				if !check.User.DOB.IsZero() {
					t.Errorf("status=%q DOB not cleared, got %v", test.status, check.User.DOB)
				}
				if check.User.GivenName != nil {
					t.Errorf("status=%q GivenName not nil, got %v", test.status, *check.User.GivenName)
				}
				if check.User.FamilyName != nil {
					t.Errorf("status=%q FamilyName not nil, got %v", test.status, *check.User.FamilyName)
				}
			}
		}
	}
}

var reactivateTests = []struct {
	initialState  AccountStatus
	newUsername   string
	expectedError error
}{
	{AccountDeactivated, "notdupe", nil},
	{AccountActive, "notdupe", apierrors.AccountAlreadyActive},
	{AccountPurged, "notdupe", apierrors.CannotReactivatePurged},
	{AccountDeactivated, "duplicate", apierrors.DuplicateUsername},
}

func (suite *UserModelSuite) TestUserAccountReactivate() {
	t := suite.T()
	ua := createModelUser(t)
	ua.User.Username = util.Pstring("duplicate")
	db.Dbmap.Update(ua.User)

	atime := time.Now()
	for _, test := range reactivateTests {
		ua := createModelUser(t)
		userId := ua.User.Id
		if err := DeactivateUserAccount(userId, "test"); err != nil {
			t.Fatal("Failed to deactivate", err)
		}
		ua.User.Status = test.initialState
		ua.User.DeactivationNoticeState = FirstDeactivationNoticeSent
		ua.User.TimeDeactivationStateUpdated = &atime
		db.Dbmap.Update(ua.User)

		err := ReactivateUserAccount(userId, test.newUsername)
		if err != test.expectedError {
			t.Errorf("state=%s newUsername=%q did not get expected error expected=%v actual=%v",
				test.initialState, test.newUsername, test.expectedError, err)
		}
		if err == nil {
			r, _ := AccountById(userId)
			if r.Status != AccountActive {
				t.Errorf("state=%s newUsername=%q state is not active, actual=%s",
					test.initialState, test.newUsername, r.Status)
			}
			if *r.Username != test.newUsername {
				t.Errorf("state=%s newUsername=%q has incorrect username expected=%q, actual=%q",
					test.initialState, test.newUsername, test.newUsername, *r.Username)
			}
			if r.DeactivationNoticeState != NoDeactivationNoticeSent {
				t.Errorf("state=%s newUsername=%q has incorrect deactivation_notice_state expected=%v, actual=%v",
					test.initialState, test.newUsername, NoDeactivationNoticeSent, r.DeactivationNoticeState)
			}
			if r.TimeDeactivationStateUpdated != nil {
				t.Errorf("state=%s newUsername=%q does not have nil time_deactivation_state_updated field",
					test.initialState, test.newUsername)
			}
		}

		db.Dbmap.Delete(ua.User)
	}
}

var inactiveTestAccounts = []struct {
	status            AccountStatus
	deactivationState DeactivationNoticeState
	isOld             bool
	timeVerifiedSet   bool
	emailVerifiedSet  bool
	expectedChanged   bool
}{
	{AccountActive, SecondDeactivationNoticeSent, true, false, false, true},   // 0 old account that was never activated; should be deactivated
	{AccountActive, SecondDeactivationNoticeSent, true, false, false, true},   // 1 old account that was never activated; should be deactivated
	{AccountActive, FirstDeactivationNoticeSent, true, false, false, false},   // 1 old account that was never activated; should be not be deactivated
	{AccountActive, NoDeactivationNoticeSent, true, false, false, false},      // 1 old account that was never activated; should be not be deactivated
	{AccountPurged, SecondDeactivationNoticeSent, true, true, false, false},   // 2 old account that was never activated; already purged; don't touch
	{AccountActive, SecondDeactivationNoticeSent, true, true, false, false},   // 3 old account, currently unverified but was verified before; don't touch
	{AccountActive, SecondDeactivationNoticeSent, false, false, false, false}, // 4 new unverified account; don't touch
	{AccountActive, SecondDeactivationNoticeSent, true, true, true, false},    // 5 old verified account; don't touch
}

func (suite *UserModelSuite) TestUpdateUnverifiedDefaultIdle() {
	t := suite.T()
	uas := make([]*UserAccount, len(inactiveTestAccounts))
	oldTime := time.Now().Add(-100 * 24 * time.Hour)
	now := time.Now().Add(-time.Second)
	for i, ac := range inactiveTestAccounts {
		is := strconv.Itoa(i)
		uas[i] = createNamedModelUser(t, "user"+is, "remote"+is)
		uas[i].NewPassword = nil
		uas[i].User.Status = ac.status
		uas[i].User.DeactivationNoticeState = ac.deactivationState
		if ac.status == AccountPurged {
			uas[i].User.Email = nil
			uas[i].User.GivenName = nil
			uas[i].User.FamilyName = nil
			uas[i].User.Username = nil
		}
		if ac.isOld {
			uas[i].User.TimeCreated = oldTime
			uas[i].User.TimeDeactivationStateUpdated = &oldTime
		}
		if ac.timeVerifiedSet {
			uas[i].User.TimeEmailFirstVerified = &oldTime
		}
		if ac.emailVerifiedSet {
			uas[i].User.EmailIsVerified = true
		}
		if err := UpdateUserAccount(uas[i]); err != nil {
			t.Fatalf("Failed to update ua %d - %v", i, err)
		}
	}

	// Run the job
	if err := UpdateUnverified(nil); err != nil {
		t.Fatal("UpdateUnverified returned error", err)
	}

	// Reload the data
	var err error
	for i, ua := range uas {
		if uas[i], err = AccountById(ua.User.Id); err != nil {
			t.Fatalf("Failed to reload ua %d - %v", i, err)
		}
	}

	// check the results
	for i, ac := range inactiveTestAccounts {
		ua := uas[i]
		is := strconv.Itoa(i)
		if ac.expectedChanged {
			if ua.Status != AccountDeactivated {
				t.Errorf("ua %d status was not deacitvated - Set to %v", i, ua.Status)
			}
			if ua.User.Username != nil {
				t.Errorf("ua %d username was not cleared - Set to %s", i, *ua.User.Username)
			}
			if ua.User.DeactivatedUsername == nil {
				t.Errorf("ua %d deactivated-username was not set", i)
			} else if *ua.User.DeactivatedUsername != "user"+is {
				t.Errorf("ua %d deactivated-username was not set to %q got %q", i, "user"+is, *ua.User.DeactivatedUsername)
			}
			if ua.User.TimeDeactivated == nil || (*ua.User.TimeDeactivated).Before(now) {
				t.Errorf("ua %d time_deactivated incorrect %#v", i, ua.User.TimeDeactivated)
			}
			if ua.User.DeactivationReason == nil || *ua.User.DeactivationReason != deactivationTimeoutReason {
				t.Errorf("ua %d incorrect deactivation_reason expected=%q actual=%q", i, deactivationTimeoutReason, *ua.User.DeactivationReason)
			}

		} else {
			// should not have changed
			if ua.Status != ac.status {
				t.Errorf("ua %d status was changed from %v to %v", i, ac.status, ua.Status)
			}
			if ac.status != AccountPurged && ua.User.Username == nil {
				t.Errorf("ua %d username was set to null", i)
			}
			if ua.User.DeactivatedUsername != nil {
				t.Errorf("ua %d deactivated-username is not null", i)
			}
		}
	}
}

func (suite *UserModelSuite) TestUpdateUnverifiedCustomMaxIdle() {
	t := suite.T()

	ua1 := createNamedModelUser(t, "user1", "")
	ua2 := createNamedModelUser(t, "user2", "")
	minus10 := time.Now().Add(-10 * time.Minute)
	minus20 := time.Now().Add(-20 * time.Minute)
	ua2.TimeCreated = time.Now().Add(-20 * time.Hour)
	ua1.TimeDeactivationStateUpdated = &minus10
	ua1.DeactivationNoticeState = SecondDeactivationNoticeSent
	UpdateUserAccount(ua1)
	ua2.TimeCreated = time.Now().Add(-20 * time.Hour)
	ua2.TimeDeactivationStateUpdated = &minus20
	ua2.DeactivationNoticeState = SecondDeactivationNoticeSent
	UpdateUserAccount(ua2)

	// Ask for all accounts older than 15 minutes to be deactivated
	frm := url.Values{"max-idle": {"15m"}}
	if err := UpdateUnverified(frm); err != nil {
		t.Fatal("UpdateUnverified returned error", err)
	}

	// reload
	ua1, _ = AccountById(ua1.User.Id)
	ua2, _ = AccountById(ua2.User.Id)

	if ua1.Status != AccountActive {
		t.Errorf("ua1 unexpectedly deactivated; new status: %s", ua1.Status)
	}
	if ua2.Status != AccountDeactivated {
		t.Errorf("ua2 status not deactivated; status: %s", ua2.Status)
	}
}

var purgeTestAccounts = []struct {
	status          AccountStatus
	isOld           bool
	expectedChanged bool
}{
	{AccountActive, true, false},       // 0 - active old account; don't touch
	{AccountDeactivated, true, true},   // 1 - deactived old account; purge
	{AccountDeactivated, false, false}, // 2 - deactived new account; don't touch
}

func (suite *UserModelSuite) TestPurgeDeactivatedDefaultIdle() {
	t := suite.T()
	// deactivated old
	// deactivated new
	// active old
	// check cleared fields
	uas := make([]*UserAccount, len(purgeTestAccounts))
	oldTime := time.Now().Add(-100 * 24 * time.Hour)
	now := time.Now().Add(-time.Second)
	for i, ac := range purgeTestAccounts {
		is := strconv.Itoa(i)
		uas[i] = createNamedModelUser(t, "user"+is, "remote"+is)
		uas[i].NewPassword = nil
		uas[i].User.Status = ac.status
		if ac.isOld {
			uas[i].User.TimeCreated = oldTime
		}
		if ac.status == AccountDeactivated {
			if ac.isOld {
				uas[i].User.TimeDeactivated = &oldTime
			} else {
				uas[i].User.TimeDeactivated = &now
			}
		}
		if err := UpdateUserAccount(uas[i]); err != nil {
			t.Fatalf("Failed to update ua %d - %v", i, err)
		}
	}

	// Run the job
	if err := PurgeDeactivated(nil); err != nil {
		t.Fatal("PurgeDeactivated returned error", err)
	}

	// Reload the data
	var err error
	for i, ua := range uas {
		if uas[i], err = AccountById(ua.User.Id); err != nil {
			t.Fatalf("Failed to reload ua %d - %v", i, err)
		}
	}

	// check the results
	for i, ac := range purgeTestAccounts {
		ua := uas[i]
		if ac.expectedChanged {
			if ua.User.Status != AccountPurged {
				t.Errorf("ua %d was not purged", i)
			}
			if ua.User.Email != nil {
				t.Errorf("ua %d email was not cleared", i)
			}
			if ua.User.FamilyName != nil {
				t.Errorf("ua %d family_name was not cleared", i)
			}
			if ua.User.GivenName != nil {
				t.Errorf("ua %d given_name was not cleared", i)
			}
			if !ua.User.DOB.IsZero() {
				t.Errorf("ua %d dob was not cleared dob=%#v", i, ua.User.DOB)
			}
			if ua.User.Gender != nil {
				t.Errorf("ua %d gender was not cleared", i)
			}
			if ua.User.TimePurged == nil || (*ua.User.TimePurged).Before(now) {
				t.Errorf("ua %d time_purged field incorrect", i)
			}
			if ua.User.PurgeReason == nil || *ua.User.PurgeReason != purgeTimeoutReason {
				t.Errorf("ua %d incorrect purge reason expected=%q actual=%q", i, purgeTimeoutReason, util.NS(ua.User.PurgeReason))
			}
		} else {
			if ua.User.Status != ac.status {
				t.Errorf("ua %d ustatus was changed from %s to %s", i, ac.status, ua.User.Status)
			}
			if ua.User.Email == nil {
				t.Errorf("ua %d PII was cleared", i)
			}
		}
	}
}

func (suite *UserModelSuite) TestPurgeDeactivatedCustomMaxIdle() {
	t := suite.T()
	ua1 := createNamedModelUser(t, "user1", "")
	ua2 := createNamedModelUser(t, "user2", "")
	tenago := time.Now().Add(-10 * time.Minute)
	twentyago := time.Now().Add(-20 * time.Minute)
	ua1.TimeDeactivated = &tenago
	ua1.Status = AccountDeactivated
	UpdateUserAccount(ua1)
	ua2.TimeDeactivated = &twentyago
	ua2.Status = AccountDeactivated
	UpdateUserAccount(ua2)

	// Ask for all accounts older than 15 minutes to be deactivated
	frm := url.Values{"max-idle": {"15m"}}
	if err := PurgeDeactivated(frm); err != nil {
		t.Fatal("PurgeDeactivated returned error", err)
	}

	// reload
	ua1, _ = AccountById(ua1.User.Id)
	ua2, _ = AccountById(ua2.User.Id)

	if ua1.Status != AccountDeactivated {
		t.Errorf("ua1 unexpectedly purged; new status: %s", ua1.Status)
	}
	if ua2.Status != AccountPurged {
		t.Errorf("ua2 status not purged; status: %s", ua2.Status)
	}
}

var deactivateNotifyTestAccounts = []struct {
	name                    string
	status                  AccountStatus
	ageHours                int
	timeVerifiedSet         bool
	deactivationNoticeState DeactivationNoticeState
	stateAge                int
}{
	// 8 day old account that was never activated; should be notified
	{"old-unactivated", AccountActive, 8*24 + 1, false, NoDeactivationNoticeSent, 0},

	// 1 hour till a notification should be sent
	{"almost-old-unactivated", AccountActive, 8*24 - 1, false, NoDeactivationNoticeSent, 0},

	// old account that has been validated at some point before
	{"old-activated", AccountActive, 100 * 24, true, FirstDeactivationNoticeSent, 7 * 24},

	// new account that has been validated at some point before
	{"old-activated2", AccountActive, 2 * 24, true, FirstDeactivationNoticeSent, 7 * 24},

	// old account that has been deactivated; should not be notified.
	{"old-deactivated", AccountDeactivated, 100 * 24, false, NoDeactivationNoticeSent, 0},

	// old account that has been purged; should not be notified.
	{"old-deactivated", AccountPurged, 100 * 24, false, NoDeactivationNoticeSent, 0},

	// old account that has had first notice sent, but not within 24 hours of deletion; no notice
	{"old-1stsent", AccountActive, 9 * 24, false, FirstDeactivationNoticeSent, 6*24 - 1},

	// old account that has had first notice sent, and is within 24 hours of deletion; should be notified
	{"old-2ndpending", AccountActive, 100*24 + 1, false, FirstDeactivationNoticeSent, 6*24 + 1},

	// old account that has had second notice sent, should not be notified (but would be deactivated)
	{"old-2ndsent", AccountActive, 100*24 + 1, false, SecondDeactivationNoticeSent, 8 * 24},
}

// Test that the correct account-deactivation notice emails are sent to the correct users
// accounts within 7 days of the 15 limit should be sent a first notice
// accounts within 24 hours of the 15 day limit should be send a second final notice
func (suite *UserModelSuite) TestDeactivateNotifyDefaultDuration() {
	email.InitAnkiEmailer()
	defer email.InitAnkiEmailer()
	t := suite.T()
	uas := make([]*UserAccount, len(deactivateNotifyTestAccounts))
	for i, ac := range deactivateNotifyTestAccounts {
		is := strconv.Itoa(i)
		oldTime := time.Now().Add(-time.Duration(ac.ageHours) * time.Hour)
		fmt.Println(ac.name, oldTime)
		uas[i] = createNamedModelUser(t, "user"+is, "remote"+is)
		uas[i].NewPassword = nil
		uas[i].User.Status = ac.status
		uas[i].User.Email = util.Pstring(fmt.Sprintf("%s@example.com", ac.name))
		uas[i].User.EmailVerify = util.Pstring(fmt.Sprintf("token%d", i))
		uas[i].User.DOB = date.MustNew(2000, 1, 1) // adult
		uas[i].DeactivationNoticeState = ac.deactivationNoticeState
		if ac.stateAge > 0 {
			tm := time.Now().Add(-time.Duration(ac.stateAge) * time.Hour)
			uas[i].TimeDeactivationStateUpdated = &tm
		}
		if ac.status == AccountPurged {
			uas[i].User.Email = nil
			uas[i].User.GivenName = nil
			uas[i].User.FamilyName = nil
			uas[i].User.Username = nil
		}
		uas[i].User.TimeCreated = oldTime
		if ac.timeVerifiedSet {
			uas[i].User.TimeEmailFirstVerified = &oldTime
		}
		if err := UpdateUserAccount(uas[i]); err != nil {
			t.Fatalf("Failed to update ua %d - %v", i, err)
		}
	}

	// Run the job
	f := func() error { return NotifyUnverified(nil) }
	msgs, err := testutil.CaptureEmails(f)
	if err != nil {
		t.Fatal("NotifyUnverified returned error", err)
	}

	expectedEmails := []string{"old-unactivated@example.com-Verification7dReminder", "old-2ndpending@example.com-Verification14dReminder"}
	var seenEmails []string
	for _, msg := range msgs {
		tpl := msg.Headers["X-Anki-Tmpl"][0]
		seenEmails = append(seenEmails, msg.To[0].Address+"-"+tpl)
	}
	sort.Strings(expectedEmails)
	sort.Strings(seenEmails)
	if !reflect.DeepEqual(expectedEmails, seenEmails) {
		t.Errorf("Incorrect email addresses.  expected=%#v actual=%#v", expectedEmails, seenEmails)
	}
}

func (suite *UserModelSuite) TestDeactivateNotifySkipInvalidEmail() {
	t := suite.T()

	ua1 := createNamedModelUser(t, "user1", "")
	ua1.User.Status = AccountActive
	ua1.DeactivationNoticeState = NoDeactivationNoticeSent
	ua1.TimeCreated = time.Now().Add(-100 * 24 * time.Hour)
	fmt.Println("TIME CREATED", ua1.TimeCreated)
	ua1.User.DOB = date.MustNew(2000, 1, 1) // adult
	ua1.User.Email = util.Pstring("ua1@example.com")
	ua1.User.EmailVerify = util.Pstring("ua1-verify")
	ua1.User.EmailFailureCode = util.Pstring("bad_email")

	if err := UpdateUserAccount(ua1); err != nil {
		t.Fatal("Failed to create user", err)
	}

	msgs, err := testutil.CaptureEmails(func() error {
		return NotifyUnverified(nil)
	})

	if err != nil {
		t.Fatal("Unexpected error", err)
	}
	if len(msgs) != 0 {
		t.Fatal("Unexpected messages sent", msgs)
	}

	if ua1, err = AccountById(ua1.User.Id); err != nil {
		t.Fatal("ua1 reload failed", err)
	}

	if ua1.DeactivationNoticeState != FirstDeactivationNoticeSent {
		t.Fatal("Account in incorrect state", ua1.DeactivationNoticeState)
	}

	if ua1.TimeDeactivationStateUpdated == nil {
		t.Fatal("Deactivation state time not updated")
	}
}

func (suite *UserModelSuite) TestDeactivateNotifySkipBlockedEmail() {
	t := suite.T()

	ua1 := createNamedModelUser(t, "user1", "")
	ua1.User.Status = AccountActive
	ua1.DeactivationNoticeState = NoDeactivationNoticeSent
	ua1.TimeCreated = time.Now().Add(-100 * 24 * time.Hour)
	ua1.User.DOB = date.MustNew(2000, 1, 1) // adult
	ua1.User.Email = util.Pstring("blocked@example.net")
	ua1.User.EmailVerify = util.Pstring("ua1-verify")

	if err := UpdateUserAccount(ua1); err != nil {
		t.Fatal("Failed to create user", err)
	}

	msgs, err := testutil.CaptureEmails(func() error {
		return NotifyUnverified(nil)
	})

	if err != nil {
		t.Fatal("Unexpected error", err)
	}
	if len(msgs) != 0 {
		//pretty.Println(msgs)
		t.Fatal("Unexpected messages sent")
	}

	if ua1, err = AccountById(ua1.User.Id); err != nil {
		t.Fatal("ua1 reload failed", err)
	}

	if ua1.DeactivationNoticeState != FirstDeactivationNoticeSent {
		t.Fatal("Account in incorrect state", ua1.DeactivationNoticeState)
	}

	if ua1.TimeDeactivationStateUpdated == nil {
		t.Fatal("Deactivation state time not updated")
	}
}

func (suite *UserModelSuite) TestDeactivateNotifyProgression() {
	email.InitAnkiEmailer()
	defer email.InitAnkiEmailer()
	t := suite.T()

	upd := func(ua *UserAccount) {
		if err := UpdateUserAccount(ua); err != nil {
			t.Fatal("Update failed ", ua)
		}
	}

	notify := func() []string {
		msgs, err := testutil.CaptureEmails(func() error {
			return NotifyUnverified(nil)
		})
		if err != nil {
			t.Fatal("Notify returned error", err)
		}
		var seenEmails []string
		for _, msg := range msgs {
			tpl := msg.Headers["X-Anki-Tmpl"][0]
			seenEmails = append(seenEmails, msg.To[0].Address+"-"+tpl)
		}
		sort.Strings(seenEmails)
		return seenEmails
	}

	ua1 := createNamedModelUser(t, "user1", "")
	ua1.User.Status = AccountActive
	ua1.DeactivationNoticeState = NoDeactivationNoticeSent
	ua1.TimeCreated = time.Now()
	ua1.User.DOB = date.MustNew(2000, 1, 1) // adult
	ua1.User.Email = util.Pstring("ua1@example.com")
	ua1.User.EmailVerify = util.Pstring("ua1-verify")
	upd(ua1)

	// create a sentinal that's not quite old enough to be notified
	// it should see no changes at all.
	sentinal := createNamedModelUser(t, "sentinal", "")
	sentinal.User.Status = AccountActive
	sentinal.DeactivationNoticeState = NoDeactivationNoticeSent
	sentinal.TimeCreated = time.Now().Add(-(7*24 - 1) * time.Hour)
	sentinal.User.DOB = date.MustNew(2000, 1, 1) // adult
	sentinal.User.Email = util.Pstring("sentinal@example.com")
	sentinal.User.EmailVerify = util.Pstring("sentinal-verify")
	upd(sentinal)

	// should not change state
	if emails := notify(); len(emails) != 0 {
		t.Fatal("Unepxected notifications (1)", emails)
	}

	// make ua1 old; should put it in state 1
	ua1.TimeCreated = time.Now().Add(-100 * 24 * time.Hour)
	upd(ua1)

	if emails := notify(); !reflect.DeepEqual(emails, []string{"ua1@example.com-Verification7dReminder"}) {
		t.Fatal("Incorrect email notifications", emails)
	}

	ua1, _ = AccountById(ua1.User.Id)
	if ua1.DeactivationNoticeState != FirstDeactivationNoticeSent {
		t.Fatal("Incorrect state", ua1.DeactivationNoticeState)
	}

	// push state change date back 5 days; run again; should remain in state 1
	// (ie. by 6 days it should be moving onto state 2)
	*ua1.TimeDeactivationStateUpdated = time.Now().Add(-5 * 24 * time.Hour)
	upd(ua1)
	if emails := notify(); len(emails) != 0 {
		t.Fatal("Unepxected notifications (1)", emails)
	}
	ua1, _ = AccountById(ua1.User.Id)
	if ua1.DeactivationNoticeState != FirstDeactivationNoticeSent {
		t.Fatal("Incorrect state", ua1.DeactivationNoticeState)
	}

	// move state change back 6 days; should move to state 2
	*ua1.TimeDeactivationStateUpdated = time.Now().Add(-(6*24 + 1) * time.Hour)
	upd(ua1)
	if emails := notify(); !reflect.DeepEqual(emails, []string{"ua1@example.com-Verification14dReminder"}) {
		t.Fatal("Incorrect email notifications", emails)
	}

	ua1, _ = AccountById(ua1.User.Id)
	if ua1.DeactivationNoticeState != SecondDeactivationNoticeSent {
		t.Fatal("Incorrect state", ua1.DeactivationNoticeState)
	}

	// Make sure the sentinal user remains untouched
	if sentinal.DeactivationNoticeState != NoDeactivationNoticeSent {
		t.Fatal("Sentinal user in incorrect state", sentinal.DeactivationNoticeState)
	}

	// Shouldn't yet actually deactivate
	UpdateUnverified(nil)
	ua1, _ = AccountById(ua1.User.Id)
	if ua1.User.Status != AccountActive {
		t.Fatal("User in incorrect state", ua1.User.Status)
	}

	// after one more day, should be deactivated
	*ua1.TimeDeactivationStateUpdated = time.Now().Add(-(7*24 + 1) * time.Hour)
	upd(ua1)

	UpdateUnverified(nil)
	ua1, _ = AccountById(ua1.User.Id)
	if ua1.User.Status != AccountDeactivated {
		t.Fatal("User in incorrect state", ua1.User.Status)
	}

	// shouldn't yet be purged
	PurgeDeactivated(nil)
	ua1, _ = AccountById(ua1.User.Id)
	if ua1.User.Status != AccountDeactivated {
		t.Fatal("User in incorrect state", ua1.User.Status)
	}

	// After 30 days the account should be purged
	*ua1.User.TimeDeactivated = time.Now().Add(-(30*24 + 1) * time.Hour)
	upd(ua1)
	PurgeDeactivated(nil)
	ua1, _ = AccountById(ua1.User.Id)
	if ua1.User.Status != AccountPurged {
		t.Fatal("User in incorrect state", ua1.User.Status)
	}

}

func (suite *UserModelSuite) TestDeactivateNotifyCustomDuration() {
	email.InitAnkiEmailer()
	defer email.InitAnkiEmailer()
	t := suite.T()
	ua1 := createNamedModelUser(t, "user1", "")
	ua2 := createNamedModelUser(t, "user2", "")
	tenago := time.Now().Add(-10 * time.Minute)
	twentyago := time.Now().Add(-20 * time.Minute)
	ua1.TimeCreated = tenago
	ua1.Status = AccountActive
	ua1.Email = util.Pstring("test1@example.com")
	ua1.EmailVerify = util.Pstring("test1")
	UpdateUserAccount(ua1)

	ua2.TimeCreated = twentyago
	ua2.Email = util.Pstring("test2@example.com")
	ua2.EmailVerify = util.Pstring("test2")
	ua2.Status = AccountActive
	UpdateUserAccount(ua2)

	frm := url.Values{"notify-older-than": {"15m"}}
	f := func() error { return NotifyUnverified(frm) }
	msgs, err := testutil.CaptureEmails(f)
	if err != nil {
		t.Fatal("NotifyUnverified returned error", err)
	}

	var addresses []string
	for _, msg := range msgs {
		addresses = append(addresses, msg.To[0].Address)
	}
	if len(addresses) != 1 || addresses[0] != "test2@example.com" {
		t.Error("Incorrect email addresses returned", addresses)
	}
}

// Once we've sent a notice to someone, we shouldn't send another
func (suite *UserModelSuite) TestDeactivateNotifyOnceOnly() {
	t := suite.T()
	oldTime := time.Now().Add(-360 * 24 * time.Hour) // BAW 10/23/14 Temp updated from 10 days to 360 days
	ua1 := createNamedModelUser(t, "user1", "")
	ua1.TimeCreated = oldTime
	ua1.Status = AccountActive
	ua1.Email = util.Pstring("test1@example.com")
	ua1.EmailVerify = util.Pstring("test1")
	UpdateUserAccount(ua1)

	f := func() error {
		if err := NotifyUnverified(nil); err != nil {
			return err
		}
		return NotifyUnverified(nil)
	}
	msgs, err := testutil.CaptureEmails(f)
	if err != nil {
		t.Fatal("NotifyUnverified returned error", err)
	}

	if len(msgs) != 1 {
		t.Errorf("Duplicate messages sent")
	}
}

// ensure postmark isn't called if there are no matching users to notify
func (suite *UserModelSuite) TestDeactivateNotifyNoMatch() {
	t := suite.T()
	email.InitAnkiEmailer()
	defer email.InitAnkiEmailer()

	fe := &failEmailer{}
	email.Emailer.PostmarkC = fe

	if err := NotifyUnverified(nil); err != nil {
		t.Fatal("Unexpected error", err)
	}
	if len(fe.sendBatchCalls) > 0 {
		t.Fatal("Call to SendBatch made")
	}
}

type failEmailer struct {
	addressResponses map[string]int // map addresses to response codes
	sendAddresses    []string
	sendBatchCalls   [][]*postmarkapp.Message
	msgs             []*postmarkapp.Message
}

func (e *failEmailer) Send(msg *postmarkapp.Message) (*postmarkapp.Result, error) {
	e.sendAddresses = append(e.sendAddresses, msg.To[0].Address)
	e.msgs = append(e.msgs, msg)
	code := e.addressResponses[msg.To[0].Address]
	if code < 0 {
		return nil, fmt.Errorf("Error code %d", code)
	}
	return &postmarkapp.Result{
		ErrorCode: code,
		Message:   fmt.Sprintf("Code %d", code),
	}, nil
}

func (e *failEmailer) SendBatch(msgs []*postmarkapp.Message) (results []*postmarkapp.Result, err error) {
	e.sendBatchCalls = append(e.sendBatchCalls, msgs)
	e.msgs = append(e.msgs, msgs...)
	for _, msg := range msgs {
		e.sendAddresses = append(e.sendAddresses, msg.To[0].Address)
		code := e.addressResponses[msg.To[0].Address]
		if code < 0 {
			return nil, fmt.Errorf("Error code %d", code)
		}
		results = append(results, &postmarkapp.Result{
			ErrorCode: code,
			Message:   fmt.Sprintf("Code %d", code),
		})
	}
	return results, nil
}

// Ensure messages to a bad address (as reported by Postmark) are flagged
func (suite *UserModelSuite) TestDeactivateNotifyInvalidEmail() {
	email.InitAnkiEmailer()
	defer email.InitAnkiEmailer()
	t := suite.T()

	fe := &failEmailer{addressResponses: map[string]int{"badaddr@example": email.InvalidEmail}}
	email.Emailer.PostmarkC = fe

	oldTime := time.Now().Add(-360 * 24 * time.Hour)
	// Create two accounts with the same address; should both be flagged
	// The other account should be processed ok.
	var ua = make([]*UserAccount, 3)
	for i := 0; i < 3; i++ {
		ua[i] = createNamedModelUser(t, fmt.Sprintf("user%d", i), "")
		ua[i].TimeCreated = oldTime
		ua[i].Status = AccountActive
		if i == 0 || i == 2 {
			ua[i].Email = util.Pstring("badaddr@example")
		} else {
			ua[i].Email = util.Pstring("test1@example.com")
		}
		ua[i].EmailVerify = util.Pstring("test1")
		UpdateUserAccount(ua[i])
	}

	// Make one of the bad address accounts activated already, so it won't be notified
	// but should still have its address marked as bad
	ua[0].TimeEmailFirstVerified = &oldTime
	ua[0].EmailIsVerified = true
	UpdateUserAccount(ua[0])

	if err := NotifyUnverified(nil); err != nil {
		t.Fatal("NotifyUnverified returned error", err)
	}

	// check accounts have been updated
	ua0, _ := AccountById(ua[0].User.Id)
	ua1, _ := AccountById(ua[1].User.Id)
	ua2, _ := AccountById(ua[2].User.Id)

	if ua0.EmailFailureCode == nil || *ua0.EmailFailureCode != "invalid_address" {
		t.Errorf("failure code was not set or incorrect for ua0.  code=%q", util.NS(ua0.EmailFailureCode))
	}
	if ua2.EmailFailureCode == nil || *ua2.EmailFailureCode != "invalid_address" {
		t.Errorf("failure code was not set or incorrect for ua2.  code=%q", util.NS(ua2.EmailFailureCode))
	}

	if ua1.EmailFailureCode != nil {
		t.Errorf("failure code was unexpectedly set for ua1.  code=%q", util.NS(ua1.EmailFailureCode))
	}

	// re-run and ensure no new emails are sent
	fe.sendAddresses = nil

	if err := NotifyUnverified(nil); err != nil {
		t.Fatal("NotifyUnverified returned error", err)
	}

	if len(fe.sendAddresses) != 0 {
		t.Errorf("Repeated call to NotifyUnverified unexpectly send emails to %v", fe.sendAddresses)
	}
}

// SAIP-486 Check that handling an email send failure for an individual user
// (with no existing txn) is handled correctly.
func (suite *UserModelSuite) TestSendEmailInvalid() {
	email.InitAnkiEmailer()
	defer email.InitAnkiEmailer()
	t := suite.T()

	fe := &failEmailer{addressResponses: map[string]int{"invalid@example.net": email.InvalidEmail}}
	email.Emailer.PostmarkC = fe

	ua := createUserWithState(t, AccountActive, "invalid", "invalid@example.net", "Test")
	msg := email.VerificationEmail(email.Emailer, "a", "b", "", email.AccountInfo{}, nil)
	msg.ToEmail = util.NS(ua.User.Email)
	if err := ua.sendEmail("test_email", msg); err == nil {
		t.Fatal("sendEmail did not return an error")
	}
}

// Check that sendEmail refuses to deliver messages to invalid/blocked email addresses
func (suite *UserModelSuite) TestSendEmailBlocked() {
	email.InitAnkiEmailer()
	defer email.InitAnkiEmailer()
	t := suite.T()

	blocked := createUserWithState(t, AccountActive, "blocked", "blocked2@example.net", "Test")
	ok := createUserWithState(t, AccountActive, "okuser", "ok@example.net", "Test")
	invalid := createUserWithState(t, AccountActive, "invalid", "invalid@example.com", "Test")
	invalid.User.EmailFailureCode = util.Pstring("test-failure")
	UpdateUserAccount(invalid)

	addBlockedEmail("blocked2@example.net")
	blocked, _ = AccountById(blocked.Id)

	msg := email.VerificationEmail(email.Emailer, "a", "b", "", email.AccountInfo{}, nil)

	for _, ua := range []*UserAccount{blocked, invalid} {
		msg.ToEmail = util.NS(ua.User.Email)
		if err := ua.User.sendEmail("email_verification", msg); err != InvalidEmail {
			t.Errorf("sendEmail did not return correct error for %s: %v", msg.ToEmail, err)
		}
	}

	msg.ToEmail = util.NS(ok.User.Email)
	if err := ok.User.sendEmail("email_verification", msg); err != nil {
		t.Errorf("sendEmail returned unexpected error: %v", err)
	}
}

// Ensure previously failed addresses are not sent to postmark
func (suite *UserModelSuite) TestDeactivateNotifyExistingFailedEmail() {
	email.InitAnkiEmailer()
	defer email.InitAnkiEmailer()
	t := suite.T()

	fe := &failEmailer{addressResponses: map[string]int{"badaddr@example": email.InvalidEmail}}
	email.Emailer.PostmarkC = fe

	oldTime := time.Now().Add(-360 * 24 * time.Hour)
	ua1 := createNamedModelUser(t, "user1", "")
	ua1.TimeCreated = oldTime
	ua1.Status = AccountActive
	ua1.Email = util.Pstring("test@example.com")
	ua1.EmailVerify = util.Pstring("test1")
	ua1.EmailFailureCode = util.Pstring("invalid_address")
	UpdateUserAccount(ua1)

	if err := NotifyUnverified(nil); err != nil {
		t.Fatal("NotifyUnverified returned error", err)
	}

	ua1, _ = AccountById(ua1.User.Id)

	// even though the address is marked as bad, the state should still be updated
	if ua1.DeactivationNoticeState != FirstDeactivationNoticeSent {
		t.Error("account state was not updated", ua1.DeactivationNoticeState)
	}

	if fe.sendAddresses != nil {
		t.Error("notify unexpectedly attempted to send mail to postmark", fe.sendAddresses)
	}
}

// Ensure a postmark failure fails the task
func (suite *UserModelSuite) TestDeactivateNotifyPostmarkFail() {
	email.InitAnkiEmailer()
	defer email.InitAnkiEmailer()
	t := suite.T()

	fe := &failEmailer{addressResponses: map[string]int{"badaddr@example": email.ServerError}}
	email.Emailer.PostmarkC = fe

	oldTime := time.Now().Add(-360 * 24 * time.Hour)

	var ua = make([]*UserAccount, 2)
	for i := 0; i < 2; i++ {
		ua[i] = createNamedModelUser(t, fmt.Sprintf("user%d", i), "")
		ua[i].TimeCreated = oldTime
		ua[i].Status = AccountActive
		if i == 0 {
			ua[i].Email = util.Pstring("badaddr@example")
		} else {
			ua[i].Email = util.Pstring("test1@example.com")
		}
		ua[i].EmailVerify = util.Pstring("test1")
		UpdateUserAccount(ua[i])
	}

	if err := NotifyUnverified(nil); err == nil {
		t.Error("NotifyUnverified did not return an error")
	}

	// Neither account should of been updated
	ua0, _ := AccountById(ua[0].User.Id)
	ua1, _ := AccountById(ua[1].User.Id)

	// postmark server errors should not result in the address being marked as bad
	if ua0.EmailFailureCode != nil || ua1.EmailFailureCode != nil {
		t.Error("EmailFailureCode unexpectedly set")
	}

	if ua0.DeactivationNoticeState != NoDeactivationNoticeSent || ua1.DeactivationNoticeState != NoDeactivationNoticeSent {
		t.Error("Account state unexpectedly updated")
	}
}

func (suite *UserModelSuite) TestForgotPasswordById() {
	email.InitAnkiEmailer()
	defer email.InitAnkiEmailer()
	t := suite.T()

	ua1 := createNamedModelUser(t, "user1", "")
	ua1.Email = util.Pstring("user1@example.com")
	UpdateUserAccount(ua1)
	ua2 := createNamedModelUser(t, "user2", "")
	ua2.Email = util.Pstring("user1@EXAMPLE.COM")
	UpdateUserAccount(ua2)
	ua3 := createNamedModelUser(t, "user3", "")
	ua3.Email = util.Pstring("user3@example.com")
	UpdateUserAccount(ua3)

	fe := &failEmailer{}
	email.Emailer.PostmarkC = fe

	uas, err := SendForgotPasswordByUserId(ua1.Id)
	if err != nil {
		t.Fatal("Unexpected error", err)
	}

	if len(fe.sendAddresses) != 1 || strings.ToLower(fe.sendAddresses[0]) != "user1@example.com" {
		t.Error("Incorrect email addresses", fe.sendAddresses)
	}

	if len(uas) != 1 || uas[0].Id != ua1.Id {
		t.Errorf("Incorrects uas returned: %v", uas)
	}

	if _, err := SendForgotPasswordByUserId("invalid"); err != apierrors.AccountNotFound {
		t.Error("Incorrect response", err)
	}
}

func (suite *UserModelSuite) TestForgotPasswordByUsernameEmail() {
	email.InitAnkiEmailer()
	defer email.InitAnkiEmailer()
	t := suite.T()

	ua1 := createNamedModelUser(t, "user1", "")
	ua1.Email = util.Pstring("user1@example.com")
	UpdateUserAccount(ua1)
	ua2 := createNamedModelUser(t, "user2", "")
	ua2.Email = util.Pstring("user1@EXAMPLE.COM")
	UpdateUserAccount(ua2)
	ua3 := createNamedModelUser(t, "user3", "")
	ua3.Email = util.Pstring("user3@example.com")
	UpdateUserAccount(ua3)

	fe := &failEmailer{}
	email.Emailer.PostmarkC = fe

	uas, err := SendForgotPasswordByUsername("user1")
	if err != nil {
		t.Fatal("Unexpected error", err)
	}

	if len(fe.sendAddresses) != 1 || strings.ToLower(fe.sendAddresses[0]) != "user1@example.com" {
		t.Error("Incorrect email addresses", fe.sendAddresses)
	}

	if len(uas) != 1 || uas[0].Id != ua1.Id {
		t.Errorf("Incorrects uas returned: %v", uas)
	}

	if _, err := SendForgotPasswordByUsername("invalid"); err != apierrors.AccountNotFound {
		t.Error("Incorrect response", err)
	}
}

func (suite *UserModelSuite) TestForgotPasswordByEmail() {
	email.InitAnkiEmailer()
	defer email.InitAnkiEmailer()
	t := suite.T()

	ua1 := createNamedModelUser(t, "user1", "")
	ua1.Email = util.Pstring("user1@example.com")
	UpdateUserAccount(ua1)
	ua2 := createNamedModelUser(t, "user2", "")
	ua2.Email = util.Pstring("user1@EXAMPLE.COM")
	UpdateUserAccount(ua2)
	ua3 := createNamedModelUser(t, "user3", "")
	ua3.Email = util.Pstring("user3@example.com")
	UpdateUserAccount(ua3)
	ua4 := createNamedModelUser(t, "user4", "")
	ua4.Email = util.Pstring("user1@example.com")
	ua4.Status = AccountDeactivated
	UpdateUserAccount(ua4)

	fe := &failEmailer{}
	email.Emailer.PostmarkC = fe

	uas, err := SendForgotPasswordByEmail("user1@example.com")
	if err != nil {
		t.Fatal("Unexpected error", err)
	}

	if len(fe.sendAddresses) != 1 || strings.ToLower(fe.sendAddresses[0]) != "user1@example.com" {
		t.Error("Incorrect email addresses", fe.sendAddresses)
	}
	if len(uas) != 2 {
		t.Errorf("Incorrect uas returned")
	} else {
		expectedIds := []string{uas[0].Id, uas[1].Id}
		sort.Strings(expectedIds)

		actualIds := []string{uas[0].Id, uas[1].Id}
		sort.Strings(actualIds)
		if !reflect.DeepEqual(expectedIds, actualIds) {
			t.Errorf("Incorrects uas returned: %v", uas)
		}
	}

	if _, err := SendForgotPasswordByEmail("invalid"); err != apierrors.AccountNotFound {
		t.Error("Incorrect response", err)
	}

	bodyBytes, _ := ioutil.ReadAll(fe.msgs[0].TextBody)
	body := string(bodyBytes)
	if !strings.Contains(body, ua1.User.Id) {
		t.Errorf("ua1 user id missing")
	}
	if !strings.Contains(body, ua2.User.Id) {
		t.Errorf("ua2 user id missing")
	}
	if strings.Contains(body, ua3.User.Id) {
		t.Errorf("ua3 user id unexpectedly present")
	}
	// deactivated user should not show up
	if strings.Contains(body, ua4.User.Id) {
		t.Errorf("ua4 user id unexpectedly present")
	}
}

func (suite *UserModelSuite) TestResetPasswordByToken() {
	email.InitAnkiEmailer()
	defer email.InitAnkiEmailer()
	t := suite.T()

	tt := timedtoken.Token()

	// default password is "hashme"
	ua := createNamedModelUser(t, "user1", "")
	ua.User.EmailVerify = util.Pstring("emailtoken") // should not be cleared
	ua.Email = util.Pstring("user1@example.com")
	ua.SetPasswordResetToken(tt)
	UpdateUserAccount(ua)

	// attempt to change with incorrect token should fail
	if err := ua.PasswordUpdateByToken(tt+"z", "newpw"); err != InvalidPasswordToken {
		t.Fatal("Did not receive invalid password token error", err)
	}
	// check that the password wasn't updated anyway
	if ok, _ := ua.IsPasswordCorrect("hashme"); !ok {
		t.Fatal("Password changed!")
	}

	// attempt to change with correct token
	if err := ua.PasswordUpdateByToken(tt, "newpassword"); err != nil {
		t.Error("Received an error when using correct token", err)
	}
	// check that the old password doesn't work
	if ok, _ := ua.IsPasswordCorrect("hashme"); ok {
		t.Error("Password wasn't updated!")
	}
	if ok, _ := ua.IsPasswordCorrect("newpassword"); !ok {
		t.Error("Password wasn't set to new password!")
	}

	if ua.User.EmailIsVerified {
		t.Error("Email is verififed flag was set when it shouldn't be")
	}

	// Try re-using the token
	if err := ua.PasswordUpdateByToken(tt, "newpw2"); err != InvalidPasswordToken {
		t.Error("Password token wasn't cleared after use")
	}

	if ua.User.EmailVerify == nil || *ua.User.EmailVerify != "emailtoken" {
		t.Error("Email token was unexpectedly updated")
	}
}

// Test that verification emails are sent correctly to over/under 13s
func (suite *UserModelSuite) TestSendVerificationEmail() {
	email.InitAnkiEmailer()
	defer email.InitAnkiEmailer()
	t := suite.T()

	ua1 := createNamedModelUser(t, "user1", "")
	ua1.Email = util.Pstring("user1@example.com")
	ua1.DOB = date.MustNew(1990, 1, 1)
	UpdateUserAccount(ua1)
	ua2 := createNamedModelUser(t, "user2", "")
	ua2.Email = util.Pstring("user2@example.com")
	ua2.DOB = date.MustNew(2014, 1, 1)
	UpdateUserAccount(ua2)

	for _, ua := range []*UserAccount{ua1, ua2} {
		fe := &failEmailer{}
		email.Emailer.PostmarkC = fe
		if err := ua.SendVerificationEmail(); err != nil {
			t.Errorf("Unexpected error for username=%q error=%v", *ua.User.Username, err)
			continue
		}
		if len(fe.msgs) != 1 {
			t.Errorf("Wrong number of messages for username=%q count=%d", *ua.User.Username, len(fe.msgs))
		}
		if fe.sendAddresses[0] != *ua.User.Email {
			t.Errorf("Incorrect to email address expected=%q actual=%q", *ua.User.Email, fe.sendAddresses[0])
		}
		tmpl := fe.msgs[0].Headers["X-Anki-Tmpl"][0]
		expectedTmpl := "Verification"
		if ua.DOB.String() == "2014-01-01" {
			expectedTmpl += "Under13"
		}

		if tmpl != expectedTmpl {
			t.Errorf("Wrong template for username=%q expected=%q actual=%q", *ua.User.Username, expectedTmpl, tmpl)
		}
	}
}

var (
	emojiName  = emojiWink + emojiCry + emojiWeary + emojiHearNoEvil
	emojiName2 = emojiWink + emojiCry + emojiWeary
	emojiName3 = emojiWink + emojiCry + emojiCry
	mbDigit    = "\U00000e54" // THAI DIGIT FOUR
)
var suggestTests = []struct {
	basename     string
	skipIfSqlite bool
	expected     []string
}{
	{"test", false, []string{"test1", "test3", "test5", "test7", "test8"}},
	{"test01", false, []string{"test1", "test3", "test5", "test7", "test8"}},
	{"Test01", true, []string{"Test1", "Test3", "Test5", "Test7", "Test8"}}, // will fail on sqlite unless COLLATE NOCASE is added to base query

	{" test ", false, []string{"test1", "test3", "test5", "test7", "test8"}},
	// trailing digits should be stripped
	{"test123", false, []string{"test123", "test1", "test3", "test5", "test7"}},
	// make sure final utf8 chacter is stripped to make room for digit
	{"abcdefghijk\xe2\x84\xa2", false, []string{"abcdefghijk\xe2\x84\xa2", "abcdefghijk2", "abcdefghijk3", "abcdefghijk4", "abcdefghijk5"}},
	// 12 character unicode string ending in 3 unicode characters
	// alternatives ending in 1-8 are already taken,false,  so should get first with 9 on the end with the last unicode character stripped
	// then 10+ with the last two unicode characters stripped to stay within the 12 character username limit
	{"aaaaaaaaa\xc2\xae\xc2\xa9\xe2\x84\xa2", false, []string{"aaaaaaaaa\xc2\xae\xc2\xa99", "aaaaaaaaa\xc2\xae10", "aaaaaaaaa\xc2\xae11", "aaaaaaaaa\xc2\xae12", "aaaaaaaaa\xc2\xae13"}},

	// username not in use should be included in the result set
	{"unused", false, []string{"unused", "unused1", "unused2", "unused3", "unused4"}},
	{"unused01", false, []string{"unused01", "unused1", "unused2", "unused3", "unused4"}},
	{"Unused01", false, []string{"Unused01", "Unused1", "Unused2", "Unused3", "Unused4"}},

	// minimum length
	{"aaa", false, []string{"aaa", "aaa1", "aaa2", "aaa3", "aaa4"}},

	// less than min length after digits stripped ; should keep minlength characters are start iterating
	{"a9234", false, []string{"a921", "a922", "a923", "a924", "a9234"}},

	// strip multibyte utf8 digits
	{"a" + mbDigit + mbDigit + mbDigit + mbDigit, false, []string{"a" + mbDigit + mbDigit + "1", "a" + mbDigit + mbDigit + "2", "a" + mbDigit + mbDigit + "3", "a" + mbDigit + mbDigit + "4", "a" + mbDigit + mbDigit + mbDigit + mbDigit}},

	// emoji username that's not already in use
	{emojiName, false, []string{emojiName, emojiName + "1", emojiName + "2", emojiName + "3", emojiName + "4"}},

	// emoji username that is already in use
	{emojiName2, false, []string{emojiName2 + "1", emojiName2 + "2", emojiName2 + "3", emojiName2 + "4", emojiName2 + "5"}},

	// This one will only pass in MySQL as it considers emojiName2 and emojiName3 to be equal (sqlite does not)
	{emojiName3, true, []string{emojiName3 + "1", emojiName3 + "2", emojiName3 + "3", emojiName3 + "4", emojiName3 + "5"}},
}

func quoteList(in []string) (out []string) {
	for _, s := range in {
		out = append(out, fmt.Sprintf("%+q", s))
	}
	return out
}

func (suite *UserModelSuite) TestUsernameSuggestions() {
	t := suite.T()
	// populate the users table with some existing usernames
	err := db.Dbmap.Insert(
		&User{Username: util.Pstring("test")},
		&User{Username: util.Pstring("test01")},
		&User{Username: util.Pstring("test2")},
		&User{Username: util.Pstring("test4")},
		&User{Username: util.Pstring("test6")},
		&User{Username: util.Pstring("aaaaaaaaa\xc2\xae\xc2\xa9\xe2\x84\xa2")},
		&User{Username: util.Pstring("abcdefghijk1")},
		&User{Username: &emojiName2},
	)
	for i := 0; i < 8; i++ {
		db.Dbmap.Insert(&User{Username: util.Pstring("aaaaaaaaa\xc2\xae\xc2\xa9" + strconv.Itoa(i+1))})
	}
	if err != nil {
		t.Fatal("Failed to create test users")
	}
	for _, test := range suggestTests {
		if test.skipIfSqlite && suite.IsSqlite() {
			continue
		}
		usernames, err := GenerateSuggestions(test.basename, 5)
		if err != nil {
			t.Fatal("Unexpected error", err)
		}
		sort.Strings(usernames)
		sort.Strings(test.expected)
		if !reflect.DeepEqual(usernames, test.expected) {
			t.Errorf("basename=%+q expected=%#v actual=%#v", test.basename, quoteList(test.expected), quoteList(usernames))
		}
	}
}

// ListAccounts Tests
var badModifierTests = []struct {
	name string
	mod  AccountListModifier
}{
	{"invalid account status", AccountListModifier{Status: []AccountStatus{"NotAStatus"}, OrderBy: Username, Limit: 1, Offset: 0}},
	{"invalid order", AccountListModifier{OrderBy: "NotAnOrder", Limit: 1, Offset: 0}},
	{"invalid limit", AccountListModifier{OrderBy: Username, Limit: 0, Offset: 0}},
	{"invalid filter name", AccountListModifier{Filters: []FilterField{{Name: "NotAFilterName"}}, OrderBy: Username, Limit: 1, Offset: 0}},
	{"invalid filter mode", AccountListModifier{Filters: []FilterField{{Mode: "NotAFilterMode"}}, OrderBy: Username, Limit: 1, Offset: 0}},
	{"invalid offset", AccountListModifier{Offset: -1, OrderBy: Username, Limit: 1}},
}

func (suite *UserModelSuite) TestListAccountBadModifiers() {
	t := suite.T()
	for _, test := range badModifierTests {
		if _, err := ListAccounts(test.mod); err == nil {
			t.Errorf("Didn't get error response from bad modifier: test=%q, err=%#v", test.name, err)
		}
	}
}

var goodModifierTests = []struct {
	name              string
	mod               AccountListModifier
	expectedUsernames []string
}{
	{"all accounts",
		AccountListModifier{Status: []AccountStatus{AccountActive, AccountDeactivated}, OrderBy: Username, Limit: 50, Offset: 0},
		[]string{"ben1", "ben2", "gareth1", "gareth2", "gareth3", "gareth4"},
	},
	{"all accounts limit to 3",
		AccountListModifier{Status: []AccountStatus{AccountActive, AccountDeactivated}, OrderBy: Username, Limit: 3, Offset: 0},
		[]string{"ben1", "ben2", "gareth1"},
	},
	{"all accounts limit to 3 offset 2",
		AccountListModifier{Status: []AccountStatus{AccountActive, AccountDeactivated}, OrderBy: Username, Limit: 3, Offset: 2},
		[]string{"gareth1", "gareth2", "gareth3"},
	},
	{"active only",
		AccountListModifier{Status: []AccountStatus{AccountActive}, OrderBy: Username, Limit: 50, Offset: 0},
		[]string{"ben1", "ben2", "gareth1", "gareth2"},
	},
	{"active only email sort",
		AccountListModifier{Status: []AccountStatus{AccountActive}, OrderBy: Email, Limit: 50, Offset: 0},
		[]string{"ben1", "ben2", "gareth2", "gareth1"},
	},
	{"active only email reverse sort",
		AccountListModifier{Status: []AccountStatus{AccountActive}, OrderBy: Email, Limit: 50, SortDesc: true, Offset: 0},
		[]string{"gareth1", "gareth2", "ben2", "ben1"},
	},
	{"username exact active only",
		AccountListModifier{Status: []AccountStatus{AccountActive}, Filters: []FilterField{{Name: Username, Mode: MatchFilter, Value: "gareth2"}}, OrderBy: Username, Limit: 50},
		[]string{"gareth2"},
	},
	{"username exact any status",
		AccountListModifier{Status: []AccountStatus{AccountActive, AccountDeactivated}, Filters: []FilterField{{Name: Username, Mode: MatchFilter, Value: "gareth2"}}, OrderBy: Username, Limit: 50},
		[]string{"gareth2"},
	},
	{"username exact no result",
		AccountListModifier{Status: []AccountStatus{AccountActive}, Filters: []FilterField{{Name: Username, Mode: MatchFilter, Value: "gareth"}}, OrderBy: Username, Limit: 50},
		[]string{},
	},
	{"username prefix active only",
		AccountListModifier{Status: []AccountStatus{AccountActive}, Filters: []FilterField{{Name: Username, Mode: PrefixFilter, Value: "gareth"}}, OrderBy: Username, Limit: 50},
		[]string{"gareth1", "gareth2"},
	},
	{"username prefix any status",
		AccountListModifier{Status: []AccountStatus{AccountActive, AccountDeactivated}, Filters: []FilterField{{Name: Username, Mode: PrefixFilter, Value: "gareth"}}, OrderBy: Username, Limit: 50},
		[]string{"gareth1", "gareth2", "gareth3", "gareth4"},
	},
	{"family name prefix active only",
		AccountListModifier{Status: []AccountStatus{AccountActive}, Filters: []FilterField{{Name: FamilyName, Mode: PrefixFilter, Value: "last"}}, OrderBy: Username, Limit: 50},
		[]string{"gareth1", "gareth2"},
	},
	{"family name prefix any status",
		AccountListModifier{Status: []AccountStatus{AccountActive, AccountDeactivated}, Filters: []FilterField{{Name: FamilyName, Mode: PrefixFilter, Value: "last"}}, OrderBy: Username, Limit: 50},
		[]string{"gareth1", "gareth2", "gareth3"},
	},
	{"family name exact no result",
		AccountListModifier{Status: []AccountStatus{AccountActive}, Filters: []FilterField{{Name: FamilyName, Mode: MatchFilter, Value: "last"}}, OrderBy: Username, Limit: 50},
		[]string{},
	},
	{"family name exact active only",
		AccountListModifier{Status: []AccountStatus{AccountActive}, Filters: []FilterField{{Name: FamilyName, Mode: MatchFilter, Value: "last1"}}, OrderBy: Username, Limit: 50},
		[]string{"gareth1"},
	},
	{"family name exact any status",
		AccountListModifier{Status: []AccountStatus{AccountActive, AccountDeactivated}, Filters: []FilterField{{Name: FamilyName, Mode: MatchFilter, Value: "last1"}}, OrderBy: Username, Limit: 50},
		[]string{"gareth1"},
	},
}

func (suite *UserModelSuite) TestListAccountGoodModifiers() {
	t := suite.T()
	createUserWithState(t, AccountActive, "gareth1", "gareth3@example.com", "Last1")
	createUserWithState(t, AccountActive, "gareth2", "gareth2@example.com", "Last2")
	createUserWithState(t, AccountDeactivated, "gareth3", "gareth1@example.com", "Last3")
	createUserWithState(t, AccountDeactivated, "gareth4", "gareth3@example.com", "FinalLast")
	createUserWithState(t, AccountActive, "ben1", "ben1@example.com", "Another1")
	createUserWithState(t, AccountActive, "ben2", "ben2@example.com", "Another2")

	var users []*UserAccount
	var err error
	for _, test := range goodModifierTests {
		if users, err = ListAccounts(test.mod); err != nil {
			t.Errorf("Error response for valid modifier: test=%q, err=%#v", test.name, err)
			continue
		}
		usernames := []string{}
		for _, ua := range users {
			usernames = append(usernames, util.NS(ua.Username))
		}
		if !reflect.DeepEqual(usernames, test.expectedUsernames) {
			t.Errorf("test=%q expected=%#v actual=%#v", test.name, test.expectedUsernames, usernames)
		}
	}
}

// Create a user then try to list it
func (suite *UserModelSuite) TestListAccountAfterCreate() {
	t := suite.T()
	username := "ListAccount"
	ua := &UserAccount{
		NewPassword: util.Pstring("hashme"),
		User: &User{
			Status:   AccountActive,
			Username: util.Pstring(username),
			Email:    util.Pstring("listaccounttest@example.com"),
		},
	}
	if err := CreateUserAccount(ua); err != nil {
		t.Fatalf("create failed when trying to create account, err=%v", err)
	}
	mod := AccountListModifier{
		Status:  []AccountStatus{AccountActive},
		OrderBy: Email,
		Limit:   MaxListLimit,
		Filters: []FilterField{{Name: Username, Value: username, Mode: MatchFilter}},
	}
	accounts, err := ListAccounts(mod)
	if err != nil {
		t.Errorf("Error response for listing user: err=%#v", err)
	}
	if len(accounts) > 1 {
		t.Errorf("Exact match should have returned one account but got %d: accounts=%v", len(accounts), accounts)
	}
}

var countryThresholds = []struct {
	locale         string
	thresholdYears int
}{
	{"en-US", 13},
	{"en-AU", 13}, // English, but in Australia (non-E)
	{"de-CH", 13}, // German language, but in Switzerland (non-EU)
	{"en-GB", 13},
	{"fr-FR", 15},
	{"de-FR", 15}, // German, but in France (lower threshold)
	{"de-DE", 16},
	{"be-BE", 16},
}

var isChildTests = []struct {
	yearOffset int
	dayOffset  int
	expected   bool
}{
	{0, 0, false},  // on their threshold birthday; adult
	{10, 0, false}, // 10 years older than threshold; adult
	{1, 0, false},  // +1 year past threshold; adult
	{0, 0, false},  // on their threshold birthday ; adult
	{0, -1, true},  // day before their threshold birthday; child
	{-1, 0, true},  // year before their threshold birthday; child
}

func (suite *UserModelSuite) TestIsChild() {
	t := suite.T()

	ua := createModelUser(t)

	for _, locale := range countryThresholds {
		for _, test := range isChildTests {
			dobYearOffset := -locale.thresholdYears - test.yearOffset
			ua.DOB = date.Today().AddDate(dobYearOffset, 0, -test.dayOffset)
			ua.EmailLang = &locale.locale

			isChild, err := ua.IsChild()
			expected := test.expected
			if err != nil {
				t.Fatalf("Unexpected error from isChild for locale=%s yearOFfset=%d dayOFfset=%d err=%v",
					locale.locale, test.yearOffset, test.dayOffset, err)
			}

			if isChild != expected {
				t.Errorf("Incorrect result for locale=%s yearOffset=%d dayOffset=%d ua.DOB=%q expected=%t actual=%t",
					locale.locale, test.yearOffset, test.dayOffset, ua.DOB, expected, isChild)
			}
		}
	}
}

func (suite *UserModelSuite) TestIsChildNoDOB() {
	t := suite.T()

	ua := createModelUser(t)
	ua.DOB = date.Zero
	_, err := ua.IsChild()
	if err != NoDOBError {
		t.Errorf("Expected NoDOBError, got %v", err)
	}
}

var csvTests = []struct {
	name          string
	startDate     string
	endDate       string
	expectedUsers []string
}{
	{"all-users", "", "", []string{"60ish", "30ish", "5ish", "nodob"}},
	{"just-children", time.Now().Add(-13 * time.Hour).Format(time.RFC3339), "", []string{"5ish", "nodob"}},
	{"just-adults", "", time.Now().Add(-13 * time.Hour).Format(time.RFC3339), []string{"60ish", "30ish"}},
	{"just-30ish", time.Now().Add(-40 * time.Hour).Format(time.RFC3339), time.Now().Add(-13 * time.Hour).Format(time.RFC3339), []string{"30ish"}},
}

func toCSV(entries []string) string {
	var b bytes.Buffer
	w := csv.NewWriter(&b)
	w.Write(entries)
	w.Flush()
	return b.String()
}

func (suite *UserModelSuite) TestCSV() {
	t := suite.T()
	t.Skip() // TOME - Temporarily Disabling CSV exporting- Test failure TBD
	// create a few users of varying ages
	// this sets the time_created field to now minus age-in-years hours for easy filtering in the test cases
	userEntries := map[string]string{}
	create := func(username string, ageYears int) *UserAccount {
		var dob date.Date
		var dobcsv string
		email := username + "@example.com"
		if ageYears > 0 {
			dob, _ = date.FromTime(time.Now().AddDate(-ageYears, 0, 0))
			dobcsv = dob.String()
		}
		ua := &UserAccount{
			NewPassword: util.Pstring("hashme"),
			User: &User{
				Status:           AccountActive,
				Username:         &username,
				Email:            util.Pstring(email),
				DOB:              dob,
				CreatedByAppName: util.Pstring("drive"),
			},
		}
		ua = createFromUa(t, ua)
		db.Dbmap.Exec("UPDATE users SET time_created=? WHERE user_id=?", time.Now().Add(-time.Hour*time.Duration(ageYears)), ua.User.Id)
		userEntries[username] = toCSV([]string{ua.User.Id, username, email, dobcsv, strconv.FormatBool(ageYears >= 13), "drive"})
		return ua
	}
	create("60ish", 60) // User around 60yo
	create("30ish", 30) // User around 30yo
	create("5ish", 5)   // User around 5yo

	for _, test := range csvTests {
		var w bytes.Buffer
		count, err := WriteCSV(&w, test.startDate, test.endDate, 0)
		if err != nil {
			t.Errorf("test=%s unexpected error=%v", test.name, err)
			continue
		}
		if count != len(test.expectedUsers) {
			t.Errorf("test=%s incorrect count expected=%d actual=%d", test.name, len(test.expectedUsers), count)
		}

		var expectedEntries []string
		for _, username := range test.expectedUsers {
			expectedEntries = append(expectedEntries, userEntries[username])
		}

		csvData := w.String()
		expectedCsv := strings.Join(expectedEntries, "")

		if expectedCsv != csvData {
			t.Errorf("test=%s incorrect respnose.  expected=\n%s\n actual=\n%s\n", test.name, expectedCsv, csvData)
		}
	}
}

var csvEmailTests = []struct {
	name        string
	email       string
	earliest    string
	latest      string
	shouldError bool
}{
	{"non-anki-email", "foo@example.com", "", "", true},    // only anki.com addresses work
	{"invalid-start", "foo@anki.com", "invalid", "", true}, // illegal start time
	{"invalid-end", "foo@anki.com", "", "invalid", true},   // illegal end time
	{"should-send", "foo@anki.com", "", "", false},         // should generate an email
}

func (suite *UserModelSuite) TestEmailCSV() {
	t := suite.T()

	// createa  user to return
	createNamedModelUser(t, "user1", "")

	for _, test := range csvEmailTests {
		frm := url.Values{
			"email":           {test.email},
			"earliest-create": {test.earliest},
			"latest-create":   {test.latest},
		}

		msgs, err := testutil.CaptureEmails(func() error {
			return EmailCSV(frm)
		})

		if test.shouldError {
			if err == nil {
				t.Errorf("test=%s no error response", test.name)
			}
			continue
		} else {
			if err != nil {
				t.Errorf("test=%s unexpected error %v", test.name, err)
				continue
			}
		}

		if len(msgs) != 1 {
			t.Errorf("test=%s invalid message count=%d", test.name, len(msgs))
		}
	}
}

var langoktests = []struct {
	name       string
	willerr    bool
	email_lang string
}{
	{"en-US", false, "en-US"},
	{"en-GB", false, "en-UK"},
	{"misc vals", false, "ab-CD"},
	{"upcase1", true, "EN-Us"},
	{"lowercase", true, "en-us"},
	{"empty", true, ""},
	{"badformat tooshort", true, "en"},
	{"badformat toolong", true, "en-us-us"},
	{"badformat spaces1", true, " n-us"},
	{"badformat spaces2", true, " en-us"},
	{"badformat spaces3", true, "en-us "},
	{"badformat underscore", true, "en_us"},
	{"badformat nosep1", true, "enus"},
	{"badformat nosep2", true, "enzus"},
	{"badformat nosep3", true, "en?us"},
}

func (suite *UserModelSuite) TestEmailLang() {
	t := suite.T()

	for i, dat := range langoktests {
		username := fmt.Sprintf("lang-u%d", i)

		ua := &UserAccount{
			NewPassword: util.Pstring("hashme"),
			User: &User{
				Status:    AccountActive,
				Username:  util.Pstring(username),
				GivenName: util.Pstring("given name"),
				Email:     util.Pstring("a@b"),
				EmailLang: util.Pstring(dat.email_lang),
			},
		}
		errs := CreateUserAccount(ua)

		if !dat.willerr && errs != nil {
			t.Fatalf("Failed to create user account %s (lang: %s): %s", username, dat.email_lang, errs)
		}
		if dat.willerr && errs == nil {
			t.Fatalf("EmailLang did not error as expected : %s", dat.email_lang)
		}
		if !dat.willerr {
			a, err := AccountByUsername(username)

			if err != nil {
				t.Fatal("Unexpected inability to pull account by usernames", err)
			}
			if a == nil && err != nil {
				t.Fatal("Unexpected inability to pull account by usernames", err)
			}

			ns := util.NS(a.EmailLang)

			if dat.email_lang != ns {
				t.Errorf("Unexpected EmailLang. expected %s => got %s", dat.email_lang, ns)
			}
		}
	}
}

func (suite *UserModelSuite) TestEmailNoLang() {
	t := suite.T()

	username := "lang2-u0"
	ua := &UserAccount{
		NewPassword: util.Pstring("hashme"),
		User: &User{
			Status:    AccountActive,
			Username:  util.Pstring(username),
			GivenName: util.Pstring("given name"),
			Email:     util.Pstring("a@b"),
		},
	}
	errs := CreateUserAccount(ua)

	if errs != nil {
		t.Fatalf("Expected OK to create UserAccount without EmailLanguage field specified (nil)")
	}
}

func (suite *UserModelSuite) TestRelated() {
	t := suite.T()

	// Create a bunch of users
	ua1 := createUserWithState(t, AccountActive, "a1-active", "a@example.com", "")
	ua2 := createUserWithState(t, AccountActive, "a2-active", "a@example.com", "")
	_ = createUserWithState(t, AccountDeactivated, "a3-deact", "a@example.com", "")
	ua4 := createUserWithState(t, AccountActive, "b1-active", "b@example.com", "")
	ua5 := createUserWithState(t, AccountActive, "a3-active", "a@example.com", "")

	expect := func(ua *UserAccount, expected ...string) {
		related, err := ua.Related()
		if err != nil {
			t.Fatalf("Unexpected error for username=%v: %v", ua.Username, err)
		}
		var actual []string
		for _, rel := range related {
			actual = append(actual, rel.Id)
		}
		sort.Strings(expected)
		sort.Strings(actual)
		if !reflect.DeepEqual(expected, actual) {
			t.Errorf("username=%s expected=%#v actual=%#v", *ua.Username, expected, actual)
		}
	}

	// confirm that accounts that share an email address in common return their active
	// (only) peers and not the original user id
	expect(ua1, ua2.Id, ua5.Id)
	expect(ua2, ua1.Id, ua5.Id)
	expect(ua4) // ua4 has no related accounts
	expect(ua5, ua1.Id, ua2.Id)
}

func (suite *UserModelSuite) TestCreatePlayerIDAlias() {
	t := suite.T()

	// creating an account with drive guest id set, should set player id
	ua := &UserAccount{
		NewPassword: util.Pstring("hashme"),
		User: &User{
			Status:       "A",
			Username:     util.Pstring("player"),
			Email:        util.Pstring("player@example.com"),
			DriveGuestId: util.Pstring("guest-id"),
		},
	}
	createFromUa(t, ua)
	if id := util.NS(ua.PlayerId); id != "guest-id" {
		t.Errorf("PlayerID incorrect: %q", id)
	}
	if id := util.NS(ua.DriveGuestId); id != "guest-id" {
		t.Errorf("DriveGuestId incorrect: %q", id)
	}
}

func (suite *UserModelSuite) TestUpdatePlayerIDAlias() {
	t := suite.T()

	// updating an account with drive guest id set, should set player id
	ua := &UserAccount{
		NewPassword: util.Pstring("hashme"),
		User: &User{
			Status:   "A",
			Username: util.Pstring("player"),
			Email:    util.Pstring("player@example.com"),
		},
	}
	createFromUa(t, ua)

	ua.User.DriveGuestId = util.Pstring("guest-id")

	err := UpdateUserAccount(ua)
	if err != nil {
		t.Fatal("Unexpected error", err)
	}

	if id := util.NS(ua.PlayerId); id != "guest-id" {
		t.Errorf("PlayerID incorrect: %q", id)
	}
	if id := util.NS(ua.DriveGuestId); id != "guest-id" {
		t.Errorf("DriveGuestId incorrect: %q", id)
	}
}

func (suite *UserModelSuite) TestLoadPlayerIDAlias() {
	t := suite.T()

	ua := &UserAccount{
		NewPassword: util.Pstring("hashme"),
		User: &User{
			Status:       "A",
			Username:     util.Pstring("player"),
			Email:        util.Pstring("player@example.com"),
			DriveGuestId: util.Pstring("guest-id"),
		},
	}
	createFromUa(t, ua)
	if id := util.NS(ua.PlayerId); id != "guest-id" {
		t.Errorf("PlayerID incorrect: %q", id)
	}
	if id := util.NS(ua.DriveGuestId); id != "guest-id" {
		t.Errorf("DriveGuestId incorrect: %q", id)
	}
	fmt.Println("Created ID", ua.User.Id)

	// reload
	reload, err := AccountById(ua.User.Id)
	if err != nil {
		t.Fatalf("Failed to reload UA: %v", err)
	}

	if id := util.NS(reload.PlayerId); id != "guest-id" {
		t.Errorf("PlayerID incorrect: %q", id)
	}
	if id := util.NS(reload.DriveGuestId); id != "guest-id" {
		t.Errorf("DriveGuestId incorrect: %q", id)
	}
}

func (suite *UserModelSuite) TestListPlayerIDAlias() {
	t := suite.T()

	ua := &UserAccount{
		NewPassword: util.Pstring("hashme"),
		User: &User{
			Status:       "A",
			Username:     util.Pstring("testlist"),
			Email:        util.Pstring("player@example.com"),
			DriveGuestId: util.Pstring("guest-id"),
		},
	}
	createFromUa(t, ua)

	mod := AccountListModifier{
		Status: []AccountStatus{AccountActive},
		Filters: []FilterField{{
			Name:  "username",
			Value: "testlist",
			Mode:  MatchFilter,
		}},
		Limit:   10,
		OrderBy: "username",
	}
	accounts, err := ListAccounts(mod)
	if err != nil {
		t.Fatal("List failed", err)
	}

	if len(accounts) != 1 {
		t.Fatalf("expected 1 account, got %d", len(accounts))
	}

	result := accounts[0]
	if id := util.NS(result.DriveGuestId); id != "guest-id" {
		t.Errorf(`drive_guest_id=%q expected="guest_id"`, id)
	}
	if id := util.NS(result.PlayerId); id != "guest-id" {
		t.Errorf(`player_id=%q expected="guest_id"`, id)
	}
}

func (suite *UserModelSuite) TestDefaultGuestID() {
	t := suite.T()

	// Creating a user without supplying a guest id should set a random default
	ua := &UserAccount{
		NewPassword: util.Pstring("hashme"),
		User: &User{
			Status:   "A",
			Username: util.Pstring("testlist"),
			Email:    util.Pstring("player@example.com"),
		},
	}
	createFromUa(t, ua)

	playerID := util.NS(ua.PlayerId)
	guestID := util.NS(ua.DriveGuestId)
	if playerID != guestID {
		t.Errorf("guest_id=%q player_id=%q", guestID, playerID)
	}

	if _, err := uuid.FromString(playerID); err != nil {
		t.Error("Failed to parse UUID", err)
	}
}

func (suite *UserModelSuite) TestSetGuestID() {
	t := suite.T()

	// Creating a user supplying a guest id should NOT set a random default
	ua := &UserAccount{
		NewPassword: util.Pstring("hashme"),
		User: &User{
			Status:       "A",
			Username:     util.Pstring("testlist"),
			Email:        util.Pstring("player@example.com"),
			DriveGuestId: util.Pstring("guest-id"),
		},
	}
	createFromUa(t, ua)

	playerID := util.NS(ua.PlayerId)
	guestID := util.NS(ua.DriveGuestId)
	if playerID != guestID {
		t.Errorf("guest_id=%q player_id=%q", guestID, playerID)
	}

	if guestID != "guest-id" {
		t.Errorf("expected drive_guest_id=%q actual=%q", "guest-id", playerID)
	}
}
