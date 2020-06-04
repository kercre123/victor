// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// Utiilty routines shared by api tests
package apitest

import (
	"bytes"
	"io/ioutil"
	"net/http"
	"net/url"
	"testing"
	"time"

	"github.com/anki/sai-go-accounts/db"
	"github.com/anki/sai-go-accounts/db/dbtest"
	"github.com/anki/sai-go-accounts/models/session"
	"github.com/anki/sai-go-accounts/models/user"
	"github.com/anki/sai-go-accounts/util"
	"github.com/anki/sai-go-accounts/validate"
	"github.com/anki/sai-go-util/date"
	"github.com/anki/sai-go-util/http/apirouter"
	"github.com/anki/sai-go-util/jsonutil"
)

var (
	enUS = "en-US"
)

func init() {
	// Ensure the ValiationHandler queries the local database.
	validate.DefaultSessionValidator = &session.LocalValidator{}
}

func GenApiKey(t *testing.T, k *session.ApiKey) *session.ApiKey {
	err := db.Dbmap.Insert(k)
	if err != nil {
		t.Fatalf("Failed to create apikey: %s", err)
	}
	return k
}

func genApiKeys(t *testing.T) map[string]*session.ApiKey {
	return map[string]*session.ApiKey{
		"user-key": GenApiKey(t, &session.ApiKey{
			Id:          "",
			Token:       "user-key",
			Scopes:      session.ScopeMask(session.UserScope),
			Description: "",
			TimeCreated: time.Now(),
			TimeExpires: time.Now()}),
		"admin-key": GenApiKey(t, &session.ApiKey{
			Id:          "",
			Token:       "admin-key",
			Scopes:      session.ScopeMask(session.AdminScope),
			Description: "",
			TimeCreated: time.Now(),
			TimeExpires: time.Now()}),
		"service-key": GenApiKey(t, &session.ApiKey{
			Id:          "",
			Token:       "service-key",
			Scopes:      session.ScopeMask(session.ServiceScope),
			Description: "",
			TimeCreated: time.Now(),
			TimeExpires: time.Now()}),
		"cron-key": GenApiKey(t, &session.ApiKey{
			Id:          "",
			Token:       "cron-key",
			Scopes:      session.ScopeMask(session.CronScope),
			Description: "",
			TimeCreated: time.Now(),
			TimeExpires: time.Now()}),
	}
}

func CreateNamedUserSession(t *testing.T, username string, status user.AccountStatus, remoteId string) *session.UserSession {
	ua := &user.UserAccount{
		NewPassword: util.Pstring("hashme"),
		User: &user.User{
			Status:    status,
			Username:  util.Pstring(username),
			GivenName: util.Pstring("given name"),
			Email:     util.Pstring("test@example.com"),
			DOB:       date.MustNew(1970, 1, 1),
			EmailLang: &enUS,
		}}

	errs := user.CreateUserAccount(ua)
	if errs != nil {
		t.Fatalf("Failed to create user account: %s", errs)
		return nil
	}

	us, err := session.NewUserSession(ua)
	if err != nil {
		t.Fatalf("Failed to create user session: %s", err)
		return nil
	}
	return us
}

func CreateAdminSession(t *testing.T, scope session.Scope, email string) *session.UserSession {
	us, err := session.NewAdminSession(scope, email)
	if err != nil {
		t.Fatalf("Failed to create admin session: %s", err)
		return nil
	}
	return us
}

func GenUsers(t *testing.T) map[string]*session.UserSession {
	result := map[string]*session.UserSession{
		// active users
		"u1-unlinked": CreateNamedUserSession(t, "u1-unlinked", user.AccountActive, ""),
		"u2-unlinked": CreateNamedUserSession(t, "u2-unlinked", user.AccountActive, ""),
		"u3-unlinked": CreateNamedUserSession(t, "u3-unlinked", user.AccountActive, ""),
		"u4-unlinked": CreateNamedUserSession(t, "u4-unlinked", user.AccountActive, ""),
		// deactivated users
		"d5-unlinked": CreateNamedUserSession(t, "d5-unlinked", user.AccountDeactivated, ""),
		"d6-unlinked": CreateNamedUserSession(t, "d6-unlinked", user.AccountDeactivated, ""),
		// service scoped account
		"s1-unlinked": CreateAdminSession(t, session.ServiceScope, "admin@example.com"),
		// purged users
		"p1-unlinked": CreateNamedUserSession(t, "p1-unlinked", user.AccountDeactivated, ""),
		// admin scoped account
		"admin": CreateAdminSession(t, session.AdminScope, "admin@example.com"),
	}

	// Purge the purged account and reload data from DB
	if err := user.PurgeUserAccount(result["p1-unlinked"].User.Id, "test"); err != nil {
		t.Fatalf("Failed to create purged user: %s", err)
	}
	result["p1-unlinked"].User, _ = user.AccountById(result["p1-unlinked"].User.Id)

	return result
}

type TestRequest struct {
	*http.Request
}

func (req *TestRequest) Run() (record *jsonutil.JsonWriterRecorder) {
	record = jsonutil.NewWriterRecorder()
	apirouter.MuxRouter.ServeHTTP(record, req.Request)
	return record
}

func MakeRequest(apiToken string, us *session.UserSession, method, path string, body []byte, formValues url.Values) *TestRequest {
	if apiToken == "match" {
		if us != nil && us.Session != nil && us.Session.Scope == session.AdminScope {
			apiToken = "admin-key"
		} else {
			apiToken = "user-key"
		}
	}
	jsonHeaders := http.Header{
		"Content-Type": []string{"text/json"},
		//"Anki-App-Key": []string{apiToken},
	}
	if apiToken != "" {
		jsonHeaders["Anki-App-Key"] = []string{apiToken}
	}
	u, _ := url.Parse(path)
	req := &http.Request{
		Method: method,
		URL:    u,
		//URL:    &url.URL{Path: path},
		Header: jsonHeaders,
	}
	if us != nil {
		req.Header.Set("Authorization", "Anki "+us.Session.Token())
	}
	if body != nil {
		req.Body = ioutil.NopCloser(bytes.NewReader(body))
	}
	if formValues != nil {
		req.PostForm = formValues
	}

	return &TestRequest{req}
}

func RunRequest(apiToken string, us *session.UserSession, method, path string, body []byte, formValues url.Values) (*http.Request, *jsonutil.JsonWriterRecorder) {
	req := MakeRequest(apiToken, us, method, path, body, formValues)
	record := req.Run()
	return req.Request, record
}

type APITestSuite struct {
	dbtest.DBSuite
	ApiKeys map[string]*session.ApiKey
	Users   map[string]*session.UserSession
}

func (s *APITestSuite) SetupTest() {
	t := s.T()
	s.DBSuite.SetupTest()
	s.ApiKeys = genApiKeys(t)
	apirouter.ResetMux()
	s.Users = GenUsers(t)
}
