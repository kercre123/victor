// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

package session

import (
	"errors"
	"math/rand"
	"net/url"
	"testing"
	"time"

	"github.com/anki/sai-go-accounts/apierrors"
	"github.com/anki/sai-go-accounts/db"
	"github.com/anki/sai-go-accounts/db/dbtest"
	"github.com/anki/sai-go-accounts/models/user"
	"github.com/anki/sai-go-accounts/models/user/usertest"
	_ "github.com/anki/sai-go-accounts/testutil"
	"github.com/stretchr/testify/suite"
)

func init() {
}

type SessionModelSuite struct {
	dbtest.DBSuite
}

func TestSessionModelSuite(t *testing.T) {
	suite.Run(t, new(SessionModelSuite))
}

func genApiKey(t *testing.T, k *ApiKey) *ApiKey {
	err := db.Dbmap.Insert(k)
	if err != nil {
		t.Fatalf("Failed to create apikey: %s", err)
	}
	return k
}
func genApiKeys(t *testing.T) map[string]*ApiKey {
	return map[string]*ApiKey{
		"user-key":  genApiKey(t, &ApiKey{"", "user-key", ScopeMask(UserScope), "", time.Now(), time.Now()}),
		"admin-key": genApiKey(t, &ApiKey{"", "admin-key", ScopeMask(AdminScope), "", time.Now(), time.Now()}),
	}
}

var userSessionFailures = []struct {
	name          string
	apikey        string
	username      string
	password      string
	expectedError error
}{
	{"no-account", "user-key", "invalid_username", "pw", apierrors.InvalidUser},
	{"bad-pw", "user-key", "user1", "invalid", apierrors.InvalidPassword},
	//{"bad-apikey", "invalid", "user1", "hashme", InvalidApiKey},
	{"bad-scope", "admin-key", "user1", "hashme", apierrors.InvalidScopeForApiKey},
}

func (suite *SessionModelSuite) TestUserSessionFailures() {
	t := suite.T()
	apiKeys := genApiKeys(t)
	usertest.CreateModelUser(t)

	for _, test := range userSessionFailures {
		_, err := NewByUsername(apiKeys[test.apikey], test.username, test.password)
		if err == nil {
			t.Errorf("Error was not returned for test=%q", test.name)
			continue
		}
		if err != test.expectedError {
			t.Errorf("Incorrect error for test=%q expected=%#v actual=%#v", test.name, test.expectedError, err)
		}
	}
}

func validateSession(t *testing.T, ua *user.UserAccount, us *UserSession) {
	if ua.User.Id != us.User.Id || *ua.Username != *us.User.Username {
		t.Errorf("User mismatch expected user_id=%q username=%q actual user_id=%q username=%q %#v", ua.User.Id, *ua.User.Username, us.User.Id, *us.User.Username, us.User)
	}

	if len(us.Session.Token()) < 10 {
		t.Errorf("Session token too short token=%q", us.Session.Token())
	}

	if us.Session.Scope != UserScope {
		t.Errorf("Session has incorrect scope=%s", us.Session.Scope)
	}

	if us.Session.UserId != ua.User.Id {
		t.Errorf("Session has incorrect userid=%s", us.Session.UserId)
	}

	if us.Session.TimeExpires.Before(time.Now().Add(24 * 89 * time.Hour)) {
		t.Errorf("Session has incorrect expire_date=%s", us.Session.TimeExpires)
	}
}

func (suite *SessionModelSuite) TestUserSessionCreateOk() {
	t := suite.T()
	apiKeys := genApiKeys(t)
	ua := usertest.CreateModelUser(t)

	us, err := NewByUsername(apiKeys["user-key"], "user1", "hashme")
	if err != nil {
		t.Fatalf("Unexpected error %s", err)
	}
	if us == nil {
		t.Fatalf("session is nil")
	}

	validateSession(t, ua, us)
}

var testError = errors.New("Test error")

func newSession(t usertest.Test, apiKey *ApiKey, ua *user.UserAccount) *UserSession {
	us, err := NewByUsername(apiKey, *ua.User.Username, "hashme")
	if err != nil {
		t.Fatalf("Unexpected error %s", err)
	}
	if us == nil {
		t.Fatalf("session is nil")
	}
	return us
}

func (suite *SessionModelSuite) TestSessionByTokenOk() {
	t := suite.T()
	apiKeys := genApiKeys(t)
	ua := usertest.CreateModelUser(t) // user1
	us := newSession(t, apiKeys["user-key"], ua)

	us, err := ByToken(us.Session.Token())
	if err != nil {
		t.Fatalf("Received unexpected error from ByToken error=%v", err)
	}
	if us == nil {
		t.Fatalf("Received empty session from ByToken")
	}

	validateSession(t, ua, us)
}

func (suite *SessionModelSuite) TestSessionByIdOk() {
	t := suite.T()
	apiKeys := genApiKeys(t)

	ua := usertest.CreateModelUser(t) // user1
	us, err := NewByUserId(apiKeys["user-key"], ua.User.Id, "hashme")
	if err != nil {
		t.Fatalf("Unexpected error %s", err)
	}
	if us == nil {
		t.Fatalf("session is nil")
	}

	us, err = ByToken(us.Session.Token())
	if err != nil {
		t.Fatalf("Received unexpected error from ByToken error=%v", err)
	}
	if us == nil {
		t.Fatalf("Received empty session from ByToken")
	}
}

func (suite *SessionModelSuite) TestSessionByTokenInvalid() {
	t := suite.T()
	apiKeys := genApiKeys(t)

	usertest.CreateModelUser(t) // user1
	us, err := NewByUsername(apiKeys["user-key"], "user1", "hashme")
	if err != nil {
		t.Fatalf("Unexpected error %s", err)
	}
	if us == nil {
		t.Fatalf("session is nil")
	}

	us, err = ByToken("invalid")
	if err != nil {
		t.Fatalf("Received unexpected error from ByToken error=%v", err)
	}
	if us != nil {
		t.Fatalf("Received unexpected session %#v", us)
	}
}

func (suite *SessionModelSuite) TestSessionByTokenExpired() {
	t := suite.T()
	apiKeys := genApiKeys(t)

	usertest.CreateModelUser(t) // user1
	us, err := NewByUsername(apiKeys["user-key"], "user1", "hashme")
	if err != nil {
		t.Fatalf("Unexpected error %s", err)
	}
	if us == nil {
		t.Fatalf("session is nil")
	}

	// give session an expired time
	us.Session.TimeExpires = time.Now().Add(-time.Hour)
	db.Dbmap.Update(us.Session)

	// should not get the expired session back from ByToken
	us, err = ByToken(us.Session.Token())
	if err != nil {
		t.Fatalf("Received unexpected error from ByToken error=%v", err)
	}
	if us != nil {
		t.Fatalf("Received unexpected session %#v", us)
	}
}

func (suite *SessionModelSuite) TestDeleteSingleSession() {
	// Create a couple of sessions and make sure that we can delete just one.
	t := suite.T()
	apiKeys := genApiKeys(t)
	ua := usertest.CreateModelUser(t) // user1
	us1 := newSession(t, apiKeys["user-key"], ua)
	us2 := newSession(t, apiKeys["user-key"], ua)

	token1 := us1.Session.Token()
	token2 := us2.Session.Token()
	if token1 == token2 {
		t.Fatal("Tokens are identical??")
	}

	us1.Delete(false)

	fetch1, err1 := ByToken(token1)
	fetch2, err2 := ByToken(token2)

	if fetch1 != nil || err1 != nil {
		t.Errorf("Unexpected reuslt for token1  fetch1=%#v err1=%#v", fetch1, err1)
	}

	// token2 should still be valid
	if fetch2 == nil || err2 != nil {
		t.Errorf("Unexpected reuslt for token2 fetch1=%#v err1=%#v", fetch2, err2)
	}
}

func (suite *SessionModelSuite) TestDeleteRelatedSessions() {
	// Create a couple of sessions and make sure that we can delete just one.
	// Also make sure we leave other user's sessions untouched.
	t := suite.T()
	apiKeys := genApiKeys(t)
	ua1 := usertest.CreateModelUser(t)  // user1
	ua2 := usertest.CreateModelUser2(t) // user2

	// create two sessions for user1 and a session for user2
	ua1us1 := newSession(t, apiKeys["user-key"], ua1)
	ua1us2 := newSession(t, apiKeys["user-key"], ua1)
	ua2us1 := newSession(t, apiKeys["user-key"], ua2)

	token1 := ua1us1.Session.Token()
	token2 := ua1us2.Session.Token()
	if token1 == token2 {
		t.Fatal("Tokens are identical??")
	}

	ua1us1.Delete(true)

	fetch1, err1 := ByToken(token1)
	fetch2, err2 := ByToken(token2)

	if fetch1 != nil || err1 != nil {
		t.Errorf("Unexpected reuslt for token1  fetch1=%#v err1=%#v", fetch1, err1)
	}

	// token2 should still also be invalid
	if fetch2 != nil || err2 != nil {
		t.Errorf("Unexpected reuslt for token2 fetch1=%#v err1=%#v", fetch2, err2)
	}

	//ua2's session should still be good
	ua2fetch, err := ByToken(ua2us1.Session.Token())
	if ua2fetch == nil || err != nil {
		t.Errorf("Unexpected reuslt for ua2's token fetch1=%#v err1=%#v", ua2fetch, err)
	}
}
func (suite *SessionModelSuite) TestDeleteOtherByUserId() {
	t := suite.T()
	apiKeys := genApiKeys(t)
	ua1 := usertest.CreateModelUser(t)  // user1
	ua2 := usertest.CreateModelUser2(t) // user2
	// create 3 sessions for ua1
	us0 := newSession(t, apiKeys["user-key"], ua1)
	us1 := newSession(t, apiKeys["user-key"], ua1)
	us2 := newSession(t, apiKeys["user-key"], ua1)
	// create one session for ua2 that shouldn't be touched
	us3 := newSession(t, apiKeys["user-key"], ua2)

	if err := DeleteOtherByUserId(ua1.User.Id, us1.Session); err != nil {
		t.Fatal("Error from DeleteOtherByUserId", err)
	}

	// attempt to refetch the sessions, only us0 and us2 should have been deleted
	var fetch []*UserSession
	for i, us := range []*UserSession{us0, us1, us2, us3} {
		f, err := ByToken(us.Session.Token())
		if err != nil {
			t.Fatal("Error fetching session by token", i, err)
		}
		fetch = append(fetch, f)
	}

	if fetch[0] != nil || fetch[2] != nil {
		t.Errorf("Sessions weren't deleted {us0=%p, us2=%p}", fetch[0], fetch[2])
	}
	if fetch[1] == nil || fetch[3] == nil {
		t.Errorf("Sessions were unexpectedly deleted {us1=%p, us3=%p}", fetch[1], fetch[3])
	}
}

func (suite *SessionModelSuite) TestReup() {
	t := suite.T()
	apiKeys := genApiKeys(t)
	ua := usertest.CreateModelUser(t) // user1

	us1 := newSession(t, apiKeys["user-key"], ua)
	us2 := newSession(t, apiKeys["user-key"], ua)

	token1 := us1.Session.Token()
	token2 := us2.Session.Token()

	newUs1, err := us1.Reup()
	if err != nil {
		t.Fatalf("Reup failed: %v", err)
	}

	if newUs1.Session.Token() == token1 {
		t.Errorf("Token was not updated")
	}

	// confirm original token is now invalid
	if uscheck, err := ByToken(token1); uscheck != nil || err != nil {
		t.Errorf("Original token was not invalidated uscheck=%#v err=%#v", uscheck, err)
	}

	// refetch sessions; make sure session1 is updated and session2 is unchanged
	if new1, err := ByToken(newUs1.Session.Token()); new1 == nil || err != nil {
		t.Errorf("Error fetching reupped token new1=%#v err=%#v", new1, err)
	}

	if check2, err := ByToken(token2); check2 == nil || err != nil {
		t.Errorf("Error confirming token2 is unchanged check2=%#v err2=%#v", check2, err)
	}
}

func (suite *SessionModelSuite) TestPurgeExpired() {
	t := suite.T()
	// Generate some artificially expired sessions
	apiKeys := genApiKeys(t)
	ua := usertest.CreateModelUser(t) // user1
	us1 := newSession(t, apiKeys["user-key"], ua)
	us2 := newSession(t, apiKeys["user-key"], ua)
	us3 := newSession(t, apiKeys["user-key"], ua)
	us4 := newSession(t, apiKeys["user-key"], ua)
	us1.Session.TimeExpires = time.Now().Add(-time.Minute)
	us2.Session.TimeExpires = time.Now().Add(-time.Minute)
	db.Dbmap.Update(us1.Session, us2.Session)

	if err := PurgeExpired(nil); err != nil {
		t.Fatal("Error while expiring:", err)
	}

	s1, err1 := ByToken(us1.Session.Token())
	s2, err2 := ByToken(us2.Session.Token())
	s3, err3 := ByToken(us3.Session.Token())
	s4, err4 := ByToken(us4.Session.Token())
	if err1 != nil || err2 != nil || err3 != nil || err4 != nil {
		t.Fatal("Errors from ByToken", err1, err2, err3, err4)
	}
	if s1 != nil || s2 != nil {
		t.Fatalf("Sessions not nil s1=%#v s2=%#v", s1, s2)
	}
	if s3 == nil || s4 == nil {
		t.Errorf("Sessions unexpectedly deleteed s3=%#v s4=%#v", s3, s4)
	}
}

func (suite *SessionModelSuite) TestPurgeAll() {
	t := suite.T()
	apiKeys := genApiKeys(t)
	ua := usertest.CreateModelUser(t) // user1
	us1 := newSession(t, apiKeys["user-key"], ua)
	us2 := newSession(t, apiKeys["user-key"], ua)
	us3 := newSession(t, apiKeys["user-key"], ua)
	us4 := newSession(t, apiKeys["user-key"], ua)
	us1.Session.TimeExpires = time.Now().Add(-time.Minute)
	us2.Session.TimeExpires = time.Now().Add(-time.Minute)
	db.Dbmap.Update(us1.Session, us2.Session)

	frm := url.Values{"purge-all": {"true"}}
	if err := PurgeExpired(frm); err != nil {
		t.Fatal("Error while expiring:", err)
	}

	for i, us := range []*UserSession{us1, us2, us3, us4} {
		s, err := ByToken(us.Session.Token())
		if err != nil {
			t.Fatalf("%d - Error from ByToken: %v", i, err)
		}
		if s != nil {
			t.Errorf("%d - Token wasn't deleted", i)
		}
	}
}

var validatorTests = []struct {
	validAppToken     bool
	validSessionToken bool
	expectedError     error
}{
	{true, true, nil},
	{false, true, apierrors.InvalidApiKey},
	{false, false, apierrors.InvalidApiKey}, // only as we know apikey is tested first
	{true, false, apierrors.InvalidSession},
}

func (suite *SessionModelSuite) TestLocalValidator() {
	t := suite.T()
	v := &LocalValidator{}
	apiKeys := genApiKeys(t)
	ua := usertest.CreateModelUser(t) // user1
	us1 := newSession(t, apiKeys["user-key"], ua)

	for _, test := range validatorTests {
		appToken, sessionToken := "invalid", "invalid"
		if test.validAppToken {
			appToken = "user-key"
		}
		if test.validSessionToken {
			sessionToken = us1.Session.Token()
		}

		apiKey, us, _, err := v.ValidateSession(nil, appToken, sessionToken)

		if err != test.expectedError {
			t.Errorf("test=%#v incorrect error response expected=%#v actual=%#v",
				test, test.expectedError, err)
		}
		if err != nil {
			continue
		}

		if test.validAppToken {
			if apiKey == nil {
				t.Errorf("test=%#v apikey=nil", test)
			} else if apiKey.Token != "user-key" {
				t.Errorf("test=%#v incorrect apikey %#v", test, apiKey)
			}
		}
		if test.validSessionToken {
			if us == nil {
				t.Errorf("test=%#v usersession=nil", test)
			} else if us.Session.Token() != sessionToken {
				t.Errorf("test=%#v incorrect session token %#v", test, us)
			} else if us.User == nil {
				t.Errorf("test=%#v user was not set", test)
			}
		}
	}
}

func (suite *SessionModelSuite) TestAdminSession() {
	t := suite.T()
	_, err := NewAdminSession(AdminScope, "admin@example.com")
	if err != nil {
		t.Fatal("Failed to create admin session", err)
	}
}

func (suite *SessionModelSuite) TestAdminSessionWithId() {
	t := suite.T()
	us1, err := NewAdminSessionWithId(AdminScope, "admin@example.com", "sessionid")
	if err != nil {
		t.Fatal("Failed to create admin session", err)
	}

	o2, err2 := db.Dbmap.Get(Session{}, "sessionid")
	if err2 != nil {
		t.Fatal("Failed to retrieve admin session", err)
	}
	s2 := o2.(*Session)
	if s2.Id != us1.Session.Id {
		t.Errorf("Incorrect session token. Expected: %v, actual: %v",
			us1.Session.Id, s2.Id)
	}
}

//
// Benchmarks
//

var (
	benchSessionModelSuite *SessionModelSuite
)

func benchSetupSessionModelSuite() *SessionModelSuite {
	benchSessionModelSuite = new(SessionModelSuite)
	benchSessionModelSuite.SetupSuite()
	benchSessionModelSuite.SetupTest()
	return benchSessionModelSuite
}

func benchTeardownSessionModelSuite() {
	if benchSessionModelSuite != nil {
		benchSessionModelSuite.TearDownSuite()
		benchSessionModelSuite = nil
	}
}

// This benchmark is cpu bound by the shash implementation
func BenchmarkSessionCreationWithHash(b *testing.B) {
	benchSetupSessionModelSuite()
	defer benchTeardownSessionModelSuite()

	apiKey := &ApiKey{"", "user-key", ScopeMask(UserScope), "", time.Now(), time.Now()}
	if err := db.Dbmap.Insert(apiKey); err != nil {
		b.Fatal("Failed to generate apikey", err)
	}

	ua := usertest.CreateModelUser(b) // user1

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		newSession(b, apiKey, ua)
	}
}

func BenchmarkSessionCreationNoHash(b *testing.B) {
	benchSetupSessionModelSuite()
	defer benchTeardownSessionModelSuite()

	apiKey := &ApiKey{"", "user-key", ScopeMask(UserScope), "", time.Now(), time.Now()}
	if err := db.Dbmap.Insert(apiKey); err != nil {
		b.Fatal("Failed to generate apikey", err)
	}

	ua := usertest.CreateModelUser(b) // user1

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, err := NewUserSession(ua)
		if err != nil {
			b.Fatal("Failed to create session", err)
		}
	}
}

func BenchmarkSessionFetch(b *testing.B) {
	benchSetupSessionModelSuite()
	defer benchTeardownSessionModelSuite()

	apiKey := &ApiKey{"", "user-key", ScopeMask(UserScope), "", time.Now(), time.Now()}
	if err := db.Dbmap.Insert(apiKey); err != nil {
		b.Fatal("Failed to generate apikey", err)
	}

	ua := usertest.CreateModelUser(b) // user1
	tokens := []string{}
	for i := 0; i < 10000; i++ {
		// call NewUserSession directly to generate tokens without the overhead of password validation
		us, err := NewUserSession(ua)
		if err != nil {
			b.Fatal("Failed to create session", err)
		}
		tokens = append(tokens, us.Session.Token())
	}

	// use a fixed seed so tests are repeatable
	rand.Seed(100)
	b.ResetTimer()
	var token string
	for i := 0; i < b.N; i++ {
		token = tokens[rand.Intn(len(tokens))]
		if _, err := ByToken(token); err != nil {
			b.Fatal("ByToken failed ", err)
		}
	}
}

// invalid session
// fb failure
// invalid account type

/*
account not found
invalid password
invalid api key
bad api key scope
db insert failed
*/
