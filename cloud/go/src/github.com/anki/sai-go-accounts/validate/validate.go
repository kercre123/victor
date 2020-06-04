// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// The validate package provides a session validation HTTP handler
// that can operate against a remote Accounts API server, or
// a local database if a suitable SessionValidator is provided.
package validate

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"net/url"
	"strings"
	"time"

	"github.com/anki/sai-go-accounts/apierrors"
	"github.com/anki/sai-go-accounts/models/session"
	"github.com/anki/sai-go-util/http/apiclient"
	"github.com/anki/sai-go-util/jsonutil"
	"github.com/anki/sai-go-util/log"
	"github.com/gorilla/context"
)

const (
	SessionCtxKey      = "user-session"
	SessionTokenCtxKey = "session-token"
	ApiKeyCtxKey       = "apikey"
	ApiKeyTokenCtxKey  = "apikey-token"
	OriginalErrorField = "original_error"
)

type SessionStatus int

const (
	MaybeSession SessionStatus = iota
	NoSession
	RequireSession
)

var (
	// Set the DefaultSessionValidator to either an AccountsServer
	// instance, or a custom validator.
	// Left unchanged it will trigger a panic.
	DefaultSessionValidator SessionValidator = panicValidator{}

	UserAgent = apiclient.BuildInfoUserAgent()
)

func ActiveSession(r *http.Request) *session.UserSession {
	result := context.Get(r, SessionCtxKey)
	if result == nil {
		return nil
	}
	return result.(*session.UserSession)
}

func ActiveApiKey(r *http.Request) *session.ApiKey {
	result := context.Get(r, ApiKeyCtxKey)
	if result == nil {
		return nil
	}
	return result.(*session.ApiKey)
}

// Auth cookies (used with SAML logins) are commited to the .api.ank.com domain
// (except for localhost testing) so that both the accounts and ankival servers
// have access to it.  The cookie name is specific to the stack in use
func AuthCookieName(r *http.Request) (cookieName string, cookieDomain string) {
	// Determine the stack name from the host header
	host := strings.ToLower(r.Host)
	if i := strings.Index(host, ":"); i > -1 {
		host = host[0:i] // strip port
	}
	if !strings.HasSuffix(host, "api.anki.com") {
		return "anki-auth-" + host, ""
	}
	var stack string
	if dotpos := strings.Index(host, "."); dotpos > 0 {
		if dashpos := strings.Index(host[0:dotpos], "-"); dashpos > 0 {
			stack = host[dashpos+1 : dotpos]
		} else {
			stack = "prod"
		}
	}
	return "anki-auth-" + stack, ".api.anki.com"
}

type SessionValidator interface {
	ValidateSession(r *http.Request, apiToken, sessionToken string) (apiKey *session.ApiKey, ua *session.UserSession, remoteRequestId string, err error)
}

// throw a panic more useful than a nil pointer deref
type panicValidator struct{}

func (p panicValidator) ValidateSession(r *http.Request, apiToken, sessionToken string) (apiKey *session.ApiKey, ua *session.UserSession, remoteRequestId string, err error) {
	panic("No client.SessionValidator or DefaultSessionValidator set")
}

// The accounts server validate endpoint returns a json representation of
// this type.
type ValidateResponse struct {
	ApiKey      *session.ApiKey      `json:"apikey"`
	UserSession *session.UserSession `json:"usersession"`
}

type toktok struct {
	apiToken     string
	sessionToken string
}

type cacheEntry struct {
	t               time.Time
	apiKey          *session.ApiKey
	us              *session.UserSession
	remoteRequestId string
	cacheHits       int
}

type cacheReq struct {
	toktok toktok
	resp   chan *cacheEntry
}

type cacheStore struct {
	toktok toktok
	entry  *cacheEntry
}

// AccountsServer implements an HTTP client for validating user tokens against
// an accounts-api server that complies with the SessionValidator interface
// for use with the ValidationHandler type.
type AccountsServer struct {
	Server string // Accounts API server to connect to
	AppKey string // Client's app key (ie. the key belonging to the server wanting to make the check)

	cacheReq   chan cacheReq
	cacheStore chan cacheStore
	cacheDebug chan func(t chan time.Time, c map[toktok]*cacheEntry) // for unit tests
}

const (
	sweepInterval = time.Minute
	maxCacheTime  = 30 * time.Minute // TODO: make this configurable
)

// testableTicker wraps NewTicker so as to provide a channel that can
// be written to by unit tests.
func testableTicker(interval time.Duration) chan time.Time {
	ch := make(chan time.Time, 1)
	t := time.NewTicker(interval)
	go func() {
		for {
			ch <- (<-t.C)
		}
	}()
	return ch
}

// startCache implements a very simple time bounded cache of successful validation
// responses from the server
func (c *AccountsServer) startCache() {
	c.cacheReq = make(chan cacheReq)
	c.cacheStore = make(chan cacheStore)
	c.cacheDebug = make(chan func(tick chan time.Time, c map[toktok]*cacheEntry))
	go func() {
		cache := make(map[toktok]*cacheEntry)
		sweep := testableTicker(sweepInterval)
		for {
			select {
			case req := <-c.cacheReq:
				entry := cache[req.toktok] // resp will be nil if no entry in the cache
				if entry != nil {
					entry.cacheHits++
				}
				req.resp <- entry

			case store := <-c.cacheStore:
				store.entry.t = time.Now()
				cache[store.toktok] = store.entry

			case f := <-c.cacheDebug:
				f(sweep, cache) // synchronous update for unit testing

			case <-sweep:
				count := len(cache)
				hits := 0
				for k, entry := range cache {
					if time.Since(entry.t) > maxCacheTime {
						hits += entry.cacheHits
						delete(cache, k) // legal to do in the loop according to the Go spec
					}
				}
				alog.Info{"action": "remote_validation_cache", "status": "sweep",
					"before_len": count, "after_len": len(cache),
					"total_hits": hits, "sweep_interval": sweepInterval.String()}.Log()
			}
		}
	}()
}

// getCached returns a cache hit, or nil on cache miss
func (c *AccountsServer) getCached(apiToken, sessionToken string) *cacheEntry {
	req := cacheReq{
		toktok: toktok{apiToken: apiToken, sessionToken: sessionToken},
		resp:   make(chan *cacheEntry, 1),
	}
	c.cacheReq <- req
	return <-req.resp
}

// cacheOK adds a positive entry to the in-memory resposne cache
func (c *AccountsServer) cacheOK(apiToken, sessionToken string, apiKey *session.ApiKey, us *session.UserSession, remoteRequestId string) {
	c.cacheStore <- cacheStore{
		toktok: toktok{apiToken: apiToken, sessionToken: sessionToken},
		entry: &cacheEntry{
			apiKey:          apiKey,
			us:              us,
			remoteRequestId: remoteRequestId,
		},
	}
}

// Init sets up in-memory caching
func (c *AccountsServer) Init() {
	c.startCache()
}

func (c *AccountsServer) ValidateSession(r *http.Request, apiToken, sessionToken string) (apiKey *session.ApiKey, ua *session.UserSession, remoteRequestId string, err error) {
	var response ValidateResponse

	if entry := c.getCached(apiToken, sessionToken); entry != nil {
		return entry.apiKey, entry.us, entry.remoteRequestId, nil
	}

	sessionUrl := c.Server + "1/sessions/validate"
	data := url.Values{
		"session_token": {sessionToken},
		"app_token":     {apiToken},
	}
	body := strings.NewReader(data.Encode())
	request, _ := http.NewRequest("POST", sessionUrl, body)
	request.Header.Set("Anki-App-Key", c.AppKey)
	request.Header.Set("Content-Type", "application/x-www-form-urlencoded")
	request.Header.Set("User-Agent", UserAgent)
	resp, err := http.DefaultClient.Do(request)
	if err != nil {
		return nil, nil, "", err
	}

	remoteRequestId = resp.Header.Get("Anki-Request-Id")

	respBody, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return nil, nil, remoteRequestId, err
	}

	if resp.StatusCode == http.StatusOK {
		if err = json.Unmarshal(respBody, &response); err != nil {
			return nil, nil, remoteRequestId, fmt.Errorf("Failed to parse JSON response "+
				"from Accounts API session endpoint: %s", err)
		}
		c.cacheOK(apiToken, sessionToken, response.ApiKey, response.UserSession, remoteRequestId)
		return response.ApiKey, response.UserSession, remoteRequestId, nil
	}

	var jsonError *jsonutil.JsonErrorResponse
	if err = json.Unmarshal(respBody, &jsonError); err != nil {
		// didn't unmarhsal means we didn't get a JsonErrorResponse back.. not so good.
		return nil, nil, remoteRequestId, fmt.Errorf("Unexpected error from Accounts API server, "+
			"status code %d, errbody=%q", resp.StatusCode, string(respBody))
	}

	// The ValidateSessionErrorCode response is a special case with an
	// embedded user validation error to be returned.
	if jsonError.Code == apierrors.ValidateSessionErrorCode {
		// extract the error the api wants to return to the user
		// TODO clean this up!
		if original, ok := jsonError.Metadata[OriginalErrorField].(map[string]interface{}); ok {
			if _, ok = original["HttpStatus"]; ok {
				userError := &jsonutil.JsonErrorResponse{
					HttpStatus: int(original["HttpStatus"].(float64)),
					Code:       original["Code"].(string),
					Message:    original["Message"].(string),
				}
				return nil, nil, remoteRequestId, userError
			}
		}
	}

	// else it's a problem with our interface to the server
	return nil, nil, remoteRequestId, &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusBadGateway,
		Code:       apierrors.BadGatewayCode,
		Message:    "Unexpected error from Accounts API server",
		Metadata: map[string]interface{}{
			OriginalErrorField: jsonError,
		},
	}
}

// ValidationHandler provides an HTTP handler that extracts the App and Session
// tokens from the appropriate request headers, converts them to AppKey
// and UserSession objects and validates them against the requested Scope
// permissions, etc.
//
// It uses a SessionValidator to actually convert the tokens to objects.
//
// In all cases, the user's session is checked against SessionScopeMask
// and the request is denied if the user's session does not have a subset scope.
// The exception is if a session is not required, but NoAppKey is false.
//
// In that case, SessionScopeMask is applied against the app token's scope.
type ValidationHandler struct {
	SessionValidator SessionValidator

	// The handler to run if the validation checks pass.
	Handler http.Handler

	// The ScopeMask that either the user's session or apikey Scope
	// must match for validatio nto pass
	SessionScopeMask session.ScopeMask

	// One of:
	// RequireSession - The user must have a session and it must comply with ScopeMask
	// MaybeSsession - The user may or may not supply a session, but if they do
	//     it'll be validated and must match the ScopeMask
	// NoSession - The user must not supply a session token at all.
	SessionStatus SessionStatus

	// If NoAppKey is true then the request is allowed through,
	// even if no App token is supplied.  If one is supplied, it's
	// still validated though.
	NoAppKey bool
}

func (h *ValidationHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	var (
		apiToken, sessionToken string
		apiScopeMask           session.ScopeMask

		activeScope = session.NoSessionScope
		validator   = h.SessionValidator
	)

	// Default active scope
	if validator == nil {
		validator = DefaultSessionValidator
	}

	jw := jsonutil.ToJsonWriter(w)
	apiToken = r.Header.Get("Anki-App-Key")
	if apiToken == "" && !h.NoAppKey {
		jw.JsonError(r, apierrors.MissingApiKey)
		return
	}
	context.Set(r, ApiKeyTokenCtxKey, apiToken)

	// Fetch session authorization, if supplied from header:
	// Authorization: Anki ssseion-token
	if header := r.Header.Get("Authorization"); len(header) > len("Anki ") {
		sessionToken = header[len("Anki "):]
		for sessionToken[0] == ' ' {
			// strip leading spaces
			sessionToken = sessionToken[1:]
		}
	} else {
		cn, _ := AuthCookieName(r)
		if c, err := r.Cookie(cn); err == nil {
			// cookie sessions are only supported over AJAX and *must* have passed CORS
			// to prevent CSRF attacks
			if r.Header.Get("Origin") != "" {
				sessionToken = c.Value
			}
		}
	}

	// validate
	apiKey, us, accountsRequestId, err := validator.ValidateSession(r, apiToken, sessionToken)
	if err != nil {
		if err == apierrors.InvalidSession && h.SessionStatus == MaybeSession {
			alog.Info{"action": "validate_session", "status": "invalid_token", "accounts_request_id": accountsRequestId,
				"token": sessionToken, "disposition": "downgrade_to_no_session"}.LogR(r)
			us = nil
		} else if jerr, ok := err.(*jsonutil.JsonErrorResponse); ok {
			alog.Info{"action": "validate_session", "status": "invalid_token", "accounts_request_id": accountsRequestId,
				"token": sessionToken, "error": jerr.Message, "code": jerr.Code}.LogR(r)
			jw.JsonError(r, jerr)
			return
		} else {

			alog.Error{"action": "validate_session", "status": "unexpected_error",
				"accounts_request_id": accountsRequestId, "token": sessionToken, "error": err}.LogR(r)
			jw.JsonServerError(r)
			return
		}
	}

	userId := "none"
	sessionScope := "none"
	if us != nil {
		activeScope = us.Session.Scope
		sessionScope = us.Session.Scope.String()
		if us.User != nil {
			userId = us.User.Id
		} else {
			userId = us.Session.RemoteId
		}
	}

	apiScopeString := "none"
	if apiKey == nil {
		// Default api key scope if one isn't provided and NoAppKey is set
		apiScopeMask = session.ScopeMask(session.NoSessionScope)
		apiScopeString = apiScopeMask.String()
	} else {
		apiScopeMask = apiKey.Scopes
		apiScopeString = apiScopeMask.String()
	}

	context.Set(r, ApiKeyCtxKey, apiKey)
	context.Set(r, SessionTokenCtxKey, sessionToken)
	context.Set(r, SessionCtxKey, us)

	if h.SessionStatus == RequireSession && us == nil {
		alog.Info{"action": "validate_session", "status": "session_required",
			"accounts_request_id": accountsRequestId, "error": "no_session_supplied"}.LogR(r)
		jw.JsonError(r, apierrors.InvalidSession)
		return
	}

	if h.SessionStatus == NoSession && us != nil {
		alog.Info{"action": "validate_session", "status": "no_session_required",
			"accounts_request_id": accountsRequestId, "error": "session_was_supplied"}.LogR(r)
		jw.JsonError(r, apierrors.InvalidSessionPerms)
		return
	}

	// Check sessionscope if a session is set
	if us != nil {
		if activeScope != session.RootScope && !h.SessionScopeMask.Contains(activeScope) {
			alog.Info{"action": "validate_session", "status": "permission_denied",
				"accounts_request_id": accountsRequestId, "active_scope": activeScope,
				"required_scopemask": h.SessionScopeMask.String()}.LogR(r)
			jw.JsonError(r, apierrors.InvalidSessionPerms)
			return
		}

		// If there's an app key, make sure the user's scope is a subset of the appkeys scope mask
		// (eg. you can't use an admin session with a user scope mask)
		if apiKey != nil && !apiScopeMask.Contains(activeScope) {
			alog.Info{"action": "validate_session", "status": "permission_denied",
				"accounts_request_id": accountsRequestId, "active_scope": activeScope,
				"apikey_scopemask":   apiScopeMask.String(),
				"required_scopemask": h.SessionScopeMask.String()}.LogR(r)
			jw.JsonError(r, apierrors.InvalidSessionPerms)
			return
		}

	} else {
		// if there is no session set, and an app key is required,
		// then check the active apikey overlaps with the requested handler scopemask
		if h.SessionScopeMask&apiScopeMask == 0 {
			alog.Info{"action": "validate_session", "status": "bad_api_scope",
				"accounts_request_id": accountsRequestId, "with_session": "false",
				"api_scope_mask": apiScopeMask.String(), "required_scopemask": h.SessionScopeMask.String()}.LogR(r)
			jw.JsonError(r, apierrors.InvalidSessionPerms)
			return
		}
	}

	alog.Info{"action": "validate_session", "status": "ok",
		"accounts_request_id": accountsRequestId, "user_id": userId,
		"session_scope": sessionScope, "apikey_scope": apiScopeString}.LogR(r)

	h.Handler.ServeHTTP(w, r)
}
