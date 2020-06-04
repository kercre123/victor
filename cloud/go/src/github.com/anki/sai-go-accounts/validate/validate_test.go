package validate

import (
	"errors"
	"fmt"
	"net/http"
	"net/http/httptest"
	"reflect"
	"sync"
	"testing"
	"time"

	"github.com/anki/sai-go-accounts/apierrors"
	"github.com/anki/sai-go-accounts/models/session"
	"github.com/anki/sai-go-accounts/models/user"
	"github.com/anki/sai-go-util/jsonutil"
	"github.com/gorilla/context"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	_ "github.com/anki/sai-go-accounts/testutil"
)

type testValidator struct {
	returnSessionScope *session.Scope
	returnApiScope     *session.ScopeMask
	returnError        error
}

func (v *testValidator) ValidateSession(r *http.Request, apiToken, sessionToken string) (apiKey *session.ApiKey, ua *session.UserSession, remoteRequestId string, err error) {
	if v.returnSessionScope != nil {
		ua = &session.UserSession{
			User: &user.UserAccount{
				User: &user.User{
					Id: "1",
				},
			},
			Session: &session.Session{
				Scope: *v.returnSessionScope,
			},
		}
	}
	if v.returnApiScope != nil {
		apiKey = &session.ApiKey{
			Scopes: *v.returnApiScope,
		}
	}
	return apiKey, ua, "reqid", v.returnError
}

var (
	userScope      = session.UserScope
	adminScope     = session.AdminScope
	rootScope      = session.RootScope
	userScopeMask  = session.ScopeMask(session.UserScope)
	adminScopeMask = session.ScopeMask(session.AdminScope)
	rootScopeMask  = session.ScopeMask(session.RootScope)
	noScopeMask    = session.ScopeMask(session.NoSessionScope)
	anyScopeMask   = session.AnyScope
)

var validationTests = []struct {
	name                 string
	handlerScopeMask     session.ScopeMask
	handlerSessionStatus SessionStatus
	handlerNoAppKey      bool
	returnSessionScope   *session.Scope
	returnApiScope       *session.ScopeMask
	expectedError        error
}{
	{
		name:                 "ok-test-1",
		handlerScopeMask:     userScopeMask,
		handlerSessionStatus: RequireSession,
		handlerNoAppKey:      false,
		returnSessionScope:   &userScope,
		returnApiScope:       &userScopeMask,
		expectedError:        nil,
	}, {
		name:                 "ok-no-app-key-or-session-key",
		handlerScopeMask:     anyScopeMask,
		handlerSessionStatus: NoSession,
		handlerNoAppKey:      true,
		returnSessionScope:   nil,
		returnApiScope:       nil,
		expectedError:        nil,
	}, {
		name:                 "fail-no-app-key",
		handlerScopeMask:     anyScopeMask,
		handlerSessionStatus: NoSession,
		handlerNoAppKey:      false,
		returnSessionScope:   nil,
		returnApiScope:       nil,
		expectedError:        apierrors.MissingApiKey,
	}, {
		name:                 "app-key-scope-fail",
		handlerScopeMask:     userScopeMask, // require userScope, but we're going to get NoSessionScope
		handlerSessionStatus: NoSession,     // don't want a session
		handlerNoAppKey:      true,          // not supplying an app key is ok
		returnSessionScope:   nil,           // won't be called
		returnApiScope:       nil,           // won't be called
		expectedError:        apierrors.InvalidSessionPerms,
	}, {
		name:                 "app-key-scope-mismatch",
		handlerScopeMask:     userScopeMask, // require userScope, but we're going to get NoSessionScope
		handlerSessionStatus: RequireSession,
		handlerNoAppKey:      false,
		returnSessionScope:   &userScope,
		returnApiScope:       &adminScopeMask, // user trying to use an admin scope app key
		expectedError:        apierrors.InvalidSessionPerms,
	}, {
		name:                 "session-scope-mismatch",
		handlerScopeMask:     adminScopeMask, // require adminScope, but supplying userScope session & app key
		handlerSessionStatus: RequireSession,
		handlerNoAppKey:      false,
		returnSessionScope:   &userScope,
		returnApiScope:       &userScopeMask,
		expectedError:        apierrors.InvalidSessionPerms,
	}, {
		name:                 "no-session-appkey-scope-mismatch",
		handlerScopeMask:     adminScopeMask, // require adminScope, but supplying userScope session & app key
		handlerSessionStatus: NoSession,      // relying on Appkey only
		handlerNoAppKey:      false,
		returnSessionScope:   nil,
		returnApiScope:       &userScopeMask,
		expectedError:        apierrors.InvalidSessionPerms,
	}, {
		name:                 "no-session-appkey-ok",
		handlerScopeMask:     userScopeMask, //
		handlerSessionStatus: NoSession,     // relying on Appkey only
		handlerNoAppKey:      false,
		returnSessionScope:   nil,
		returnApiScope:       &userScopeMask, // matches what we're asking for
		expectedError:        nil,
	}, {
		name:                 "no-session-appkey-maybe-session-ok",
		handlerScopeMask:     userScopeMask, //
		handlerSessionStatus: MaybeSession,  // relying on Appkey only
		handlerNoAppKey:      false,
		returnSessionScope:   nil,
		returnApiScope:       &userScopeMask, // matches what we're asking for
		expectedError:        nil,
	}, {
		name:                 "no-session-appkey-require-session-fail",
		handlerScopeMask:     userScopeMask,  //
		handlerSessionStatus: RequireSession, // but we're not going to supply one
		handlerNoAppKey:      false,
		returnSessionScope:   nil,
		returnApiScope:       &userScopeMask, // matches what we're asking for
		expectedError:        apierrors.InvalidSession,
	}, {
		name:                 "supply-session-but-dont-want-one",
		handlerScopeMask:     userScopeMask,
		handlerSessionStatus: NoSession,
		handlerNoAppKey:      false,
		returnSessionScope:   &userScope,
		returnApiScope:       &userScopeMask,
		expectedError:        apierrors.InvalidSessionPerms,
	}, {
		name:                 "rootscope-can-do-anything",
		handlerScopeMask:     userScopeMask,
		handlerSessionStatus: RequireSession,
		handlerNoAppKey:      false,
		returnSessionScope:   &rootScope, // we iz root, your requirement for userScope is ignored!
		returnApiScope:       &rootScopeMask,
		expectedError:        nil,
	},
}

func TestValidationHandler(t *testing.T) {
	for _, test := range validationTests {
		if false && test.name != "fail-no-app-key" {
			continue
		}
		// the validator is just responsible for fetching/validating the user's session and app tokens
		// if they're set.  Our test validator doesn't check them; just returns whatever our
		// test case has asked to return so they can be passed on to the ValidationHandler
		// which will check whether thse scope/scopemask those tokens have is sufficient
		// to access the handler it wraps
		validator := &testValidator{
			returnSessionScope: test.returnSessionScope,
			returnApiScope:     test.returnApiScope,
		}

		handlerCalled := false
		dummyHandler := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			handlerCalled = true
		})

		handler := &ValidationHandler{
			SessionValidator: validator,
			Handler:          dummyHandler,
			SessionScopeMask: test.handlerScopeMask,
			SessionStatus:    test.handlerSessionStatus,
			NoAppKey:         test.handlerNoAppKey,
		}

		r, _ := http.NewRequest("GET", "/", nil)
		w := jsonutil.NewWriterRecorder()

		if test.returnApiScope != nil {
			r.Header.Set("Anki-App-Key", "key-token")
		}

		r.Header.Set("Authorization", "Anki sessiontoken")

		handler.ServeHTTP(w, r)

		if w.ErrorResponse != test.expectedError {
			t.Errorf("test=%q did not get expected error expected=%#v actual=%#v",
				test.name, test.expectedError, w.ErrorResponse)
		}

		if test.expectedError == nil && !handlerCalled {
			t.Error("handler was not called")
		} else if test.expectedError != nil && handlerCalled {
			t.Error("handler was called despite expected error response")
		}

		if test.expectedError == nil {
			for _, contextKey := range []string{ApiKeyTokenCtxKey, ApiKeyCtxKey, SessionTokenCtxKey, SessionCtxKey} {
				if context.Get(r, contextKey) == nil {
					t.Errorf("test=%q context key %q wasn't set on context", test.name, contextKey)
				}
			}
		}
	}
}

var validatorErrorTests = []struct {
	errorResponse                error
	expectedInternalServiceError bool
}{
	{apierrors.AccountNotFound, false},
	{errors.New("Borked"), true},
}

func TestValidationValidatorErrors(t *testing.T) {
	for _, test := range validatorErrorTests {
		validator := &testValidator{
			returnError: test.errorResponse,
		}
		dummyHandler := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {})

		handler := &ValidationHandler{
			SessionValidator: validator,
			Handler:          dummyHandler,
			SessionScopeMask: userScopeMask,
			SessionStatus:    RequireSession,
			NoAppKey:         true,
		}

		r, _ := http.NewRequest("GET", "/", nil)
		w := jsonutil.NewWriterRecorder()
		r.Header.Set("Anki-App-Key", "key-token")
		r.Header.Set("Authorization", "Anki sessiontoken")

		handler.ServeHTTP(w, r)

		if test.expectedInternalServiceError {
			if !w.ServerFailure {
				t.Error("ServeFailure was not set")
			}
		} else {
			if test.errorResponse != w.ErrorResponse {
				t.Errorf("Did not get expected error expected=%#v actual=%#v", test.errorResponse, w.ErrorResponse)
			}
		}
	}
}

func TestAccountsServerValidationErrors(t *testing.T) {
	errors := []*jsonutil.JsonErrorResponse{
		apierrors.AccountNotFound, apierrors.MissingApiKey, apierrors.InvalidSession,
		apierrors.InvalidSessionPerms, apierrors.InvalidApiKey,
		apierrors.InvalidUser, apierrors.InvalidScopeForApiKey,
	}

	for _, expected := range errors {
		ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			jw := &jsonutil.JsonWriterAdapter{ResponseWriter: w}
			jw.JsonError(r, expected)
		}))

		as := &AccountsServer{Server: ts.URL + "/", AppKey: "appkey"}
		as.Init()

		_, _, _, err := as.ValidateSession(nil, "app", "session")
		assert.NotNil(t, err)
		jsErr := err.(*jsonutil.JsonErrorResponse)
		assert.NotNil(t, jsErr, "Error response was not a JSON error as expected")

		assert.Equal(t, apierrors.BadGatewayCode, jsErr.Code)
		orig, ok := jsErr.Metadata[OriginalErrorField]
		require.Equal(t, true, ok, "Missing original_error field in error response")

		origErr := orig.(*jsonutil.JsonErrorResponse)
		require.NotNil(t, origErr, "Embedded original_error was not a JSON error as expected")
		assert.Equal(t, expected.Message, origErr.Message)
		assert.Equal(t, expected.Code, origErr.Code)

		ts.Close()
	}

}

func TestAccountsServerCacheHit(t *testing.T) {
	responses := []struct {
		status int
		resp   string
	}{
		{200, `{"apikey": {"id": "apikey00"}, "usersession": {"session": {"session_token": "s00"}}}`},
	}

	ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.Header().Add("Anki-Request-Id", "Remote-Id")
		if len(responses) > 0 {
			resp := responses[0]
			responses = responses[1:]
			w.WriteHeader(resp.status)
			w.Write([]byte(resp.resp))
		} else {
			w.WriteHeader(500)
			fmt.Fprintln(w, "out of responses")
		}
	}))

	as := &AccountsServer{Server: ts.URL + "/", AppKey: "appkey"}
	as.Init()

	for i := 0; i < 3; i++ {
		apikey, ua, reqId, err := as.ValidateSession(nil, "apptoken", "stoken")
		if err != nil {
			t.Fatal("err", i, err)
		}

		if reqId != "Remote-Id" {
			t.Error(i, "request id invalid", reqId)
		}

		if apikey.Id != "apikey00" {
			t.Error(i, "apikey invalid", apikey)
		}

		if ua == nil || ua.Session.Id != "s00" {
			t.Errorf("%d session invalid %#v", i, ua)
		}
	}

	var wg sync.WaitGroup
	wg.Add(1)
	as.cacheDebug <- func(s chan time.Time, c map[toktok]*cacheEntry) {
		defer wg.Done()
		if len(c) != 1 {
			t.Error("Incorrect cache size", len(c))
		}
		for _, entry := range c {
			if entry.cacheHits != 2 {
				t.Error("Incorrect cache hit count", entry.cacheHits)
			}
			break
		}
	}
	wg.Wait()
}

func TestAccountsServerCacheSweep(t *testing.T) {
	as := new(AccountsServer)
	as.Init()
	as.cacheDebug <- func(s chan time.Time, c map[toktok]*cacheEntry) {
		c[toktok{"1", "1"}] = &cacheEntry{t: time.Now().Add(-time.Hour), remoteRequestId: "one"}
		c[toktok{"2", "2"}] = &cacheEntry{t: time.Now().Add(-time.Minute), remoteRequestId: "two"}
		c[toktok{"3", "3"}] = &cacheEntry{t: time.Now().Add(-time.Hour), remoteRequestId: "three"}
		s <- time.Now()
	}

	// wait for sweep to run
	var wg sync.WaitGroup
	wg.Add(1)
	as.cacheDebug <- func(s chan time.Time, c map[toktok]*cacheEntry) {
		defer wg.Done()
		if len(c) != 1 {
			t.Error("Incorrect cache entry count after sweep", len(c))
		}
		for k, v := range c {
			if !reflect.DeepEqual(k, toktok{"2", "2"}) || v.remoteRequestId != "two" {
				t.Error("Incorrect cache entry remains", k)
			}
		}
	}
	wg.Wait()
}
