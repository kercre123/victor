// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// The session package provides API handlers for session related tasks.
package session

import (
	"net/http"
	"strings"
	"time"

	"github.com/anki/sai-go-accounts/apierrors"
	"github.com/anki/sai-go-accounts/models/session"
	"github.com/anki/sai-go-accounts/models/user"
	"github.com/anki/sai-go-accounts/validate"
	"github.com/anki/sai-go-util/http/apirouter"
	"github.com/anki/sai-go-util/http/ratelimit"
	"github.com/anki/sai-go-util/jsonutil"
	"github.com/anki/sai-go-util/log"
	"github.com/gorilla/context"
)

type samlGroup struct {
	groupName string
	scope     session.Scope
}

var (
	handlers []apirouter.Handler

	// username/userid login requests are limited to an attempt every 2 seconds
	// An initial burst of up to 8 password attempts is allowed inside 2 seconds
	// before the rate limit will kick in, and then tokens will be refilled at 0.5/second
	LoginLimitRate     float64 = 0.5
	LoginLimitCapacity int64   = 8
	RateLimiter        *ratelimit.RateLimiter

	AdminPassword user.HashedPassword

	// Map Okta group membership to a session scope
	// Important: Highest privilege groups must be listed first so that users who are
	// members of multiple groups are assigned the correct privilege
	//samlGroupMapping = map[string]session.Scope{
	samlGroupMapping = []samlGroup{}
)

func AddSAMLGroup(groupName string, scope session.Scope) {
	samlGroupMapping = append(samlGroupMapping, samlGroup{groupName, scope})
}

func SetAdminPasswordHash(pw string) {
	AdminPassword = user.HashedPassword(pw)
}

func defineHandler(handler apirouter.Handler) {
	handlers = append(handlers, handler)
}

// RegisterAPI causes the session API handles to be registered with the http mux.
func RegisterAPI() {
	apirouter.RegisterHandlers(handlers)
}

// Task handlers

func init() {
	defineHandler(apirouter.Handler{
		Method: "GET,POST",
		Path:   "/1/tasks/sessions/purge-expired",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.NoSession,
			SessionScopeMask: session.ScopeMask(session.CronScope),
			Handler:          session.PurgeExpiredRunner,
		}})
}

// Define a rate limiter for login requests that will bucket based on the username or userid
// requested and then log once the rate limiting threshold is reached (but not every single
// instance after that, until requests are allowed again)
type loginLimiter struct{}

func (l loginLimiter) BucketId(w http.ResponseWriter, r *http.Request) interface{} {
	if username := r.PostFormValue("username"); username != "" {
		return "username-" + username
	} else if userid := r.PostFormValue("userid"); userid != "" {
		return "userid-" + userid
	}
	// we don't care about rate limiting non username/userid logins
	// beyond what the ip rate limiter already provides
	return nil
}

func (l loginLimiter) RequestLimited(r *http.Request, bucketId interface{}, exceedCount int) {
	if exceedCount > 1 {
		// we've already logged the event
		return
	}
	s := strings.SplitN(bucketId.(string), "-", 2)
	idType, idVal := s[0], s[1]
	alog.Warn{"action": "new_session_ratelimited", idType: idVal}.LogR(r)
}

// Register the NewSession endpoint
func init() {
	handler := jsonutil.JsonHandlerFunc(NewSession)
	RateLimiter = ratelimit.NewLimiter(loginLimiter{}, handler, LoginLimitRate, LoginLimitCapacity, time.Minute, nil)
	defineHandler(apirouter.Handler{
		Method: "POST",
		Path:   "/1/sessions",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.NoSession,
			SessionScopeMask: session.ScopeMask(session.UserScope),
			Handler:          RateLimiter,
		}})
}

// General handlers

// The NewSession endpoint attempts to verify a user and create a new session for them.
// It handles both username authentication and returns both the user
// information and the session details
//
// POST /sessions
func NewSession(w jsonutil.JsonWriter, r *http.Request) {
	username := r.PostFormValue("username")
	userid := r.PostFormValue("userid")
	if username != "" || userid != "" {
		newUserSession(w, r)
		return

	} else if pw := r.PostFormValue("todo_remove_me_admin_hack"); pw != "" && AdminPassword != "" {
		if ok, _ := AdminPassword.Validate([]byte(pw)); ok {
			// TODO Lose all this in favour of Okta.
			session, err := session.NewAdminSession(session.AdminScope, "gareth@anki.com")
			if err != nil {
				w.JsonServerError(r)
				alog.Error{"action": "new_admin_session", "status": "admin_session_create_failed", "error": err}.LogR(r)
				return
			}
			alog.Info{"action": "new_admin_session", "status": "session_created"}.LogR(r)
			w.JsonOkData(r, session)
			return
		} else {
			alog.Warn{"action": "new_admin_session", "status": "invalid_admin_password"}.LogR(r)
		}
	}

	w.JsonError(r, apierrors.MissingParameter)
}

func newUserSession(w jsonutil.JsonWriter, r *http.Request) {
	apiKey := context.Get(r, "apikey").(*session.ApiKey)
	username := r.PostFormValue("username")
	userid := r.PostFormValue("userid")
	password := r.PostFormValue("password")
	if password == "" {
		w.JsonError(r, apierrors.MissingParameter)
		return
	}
	var (
		us  *session.UserSession
		err error
	)
	if username != "" {
		us, err = session.NewByUsername(apiKey, username, password)
	} else {
		us, err = session.NewByUserId(apiKey, userid, password)
	}
	if err != nil {
		if jerr, ok := err.(jsonutil.JsonError); ok {
			alog.Info{"action": "new_session_api",
				"status":   "user_error",
				"username": username,
				"error":    jerr}.LogR(r)
			w.JsonError(r, jerr)
		} else {
			alog.Error{"action": "new_session_api",
				"status":   "unexpected_error",
				"username": username,
				"error":    err}.LogR(r)
			w.JsonServerError(r)
		}
		return
	}
	alog.Info{"action": "new_session_api",
		"status": "ok", "method": "username", "userid": us.User.Id}.LogR(r)
	w.JsonOkData(r, us)
}

// Register the DeactivateSession endpoint
func init() {
	defineHandler(apirouter.Handler{
		Method: "POST",
		Path:   "/1/sessions/delete",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.RequireSession,
			SessionScopeMask: session.ValidSessionScope,
			Handler:          jsonutil.JsonHandlerFunc(DeactivateSession),
		}})
}

// The DeactivateSession endpoint causes a session to be deleted, optionally
// including related sessions.
//
// POST /sessions/delete
func DeactivateSession(w jsonutil.JsonWriter, r *http.Request) {
	activeSession := validate.ActiveSession(r)
	sessionToken := r.PostFormValue("session_token")
	deleteRelatedOpt := r.PostFormValue("delete_related")
	deleteRelated := deleteRelatedOpt == "true"

	if sessionToken == "" {
		w.JsonError(r, apierrors.MissingParameter)
		return
	}

	us, err := session.ByToken(sessionToken)
	if err != nil {
		alog.Error{"action": "delete_session_api",
			"status":        "unexpected_error",
			"session_token": sessionToken,
			"error":         err}.LogR(r)
		w.JsonServerError(r)
		return
	}
	if us == nil {
		w.JsonError(r, apierrors.InvalidSession)
		return
	}

	canDeleteAny := session.NewScopeMask(session.SupportScope, session.AdminScope, session.RootScope)
	if !canDeleteAny.Contains(activeSession.Session.Scope) && us.User.Id != activeSession.User.Id {
		alog.Warn{"action": "update_user",
			"status":  "permission_denied",
			"user_id": activeSession.User.Id,
			"message": "User tried to delete session from another account"}.LogR(r)
		w.JsonError(r, apierrors.InvalidSessionPerms)
		return
	}

	err = us.Delete(deleteRelated)
	if err != nil {
		alog.Error{"action": "delete_session_api",
			"status":        "unexpected_error",
			"session_token": sessionToken,
			"error":         err}.LogR(r)
		w.JsonServerError(r)
	}

	w.JsonOk(r, "logged out")
}

// Register the ReupSession endpoint
func init() {
	defineHandler(apirouter.Handler{
		Method: "POST",
		Path:   "/1/sessions/reup",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.RequireSession,
			SessionScopeMask: session.ValidSessionScope,
			Handler:          jsonutil.JsonHandlerFunc(ReupSession),
		}})
}

// The ReupSession endpoint takes an existing, valid session and returns a
// new session with a reset expiry time.
// The old session passed in is immediately invalidated.
//
// POST /sessions/reup
func ReupSession(w jsonutil.JsonWriter, r *http.Request) {
	activeSession := validate.ActiveSession(r)
	newSession, err := activeSession.Reup()
	if err != nil {
		alog.Error{"action": "reup_session_api",
			"status":        "unexpected_error",
			"session_token": activeSession.Session.Id,
			"error":         err}.LogR(r)
		w.JsonServerError(r)
		return
	}

	alog.Info{"action": "reup_session_api", "status": "ok", "user_id": activeSession.User.Id}.LogR(r)

	// Just return the session data, not the whole user entry
	w.JsonOkData(r, newSession.Session)
}

// Register the ValidateSession endpoint
func init() {
	defineHandler(apirouter.Handler{
		Method: "POST",
		Path:   "/1/sessions/validate",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.NoSession,
			SessionScopeMask: session.ScopeMask(session.ServiceScope),
			Handler:          jsonutil.JsonHandlerFunc(ValidateSession),
		}})
}

// The ValidateSession endpoint returns user information associated with
// app and session tokens.
//
// POST /sessions/validate
func ValidateSession(w jsonutil.JsonWriter, r *http.Request) {
	apiToken := r.PostFormValue("app_token")
	sessionToken := r.PostFormValue("session_token")

	apiKey, us, _, err := validate.DefaultSessionValidator.ValidateSession(r, apiToken, sessionToken)
	if err != nil {
		switch err := err.(type) {
		case jsonutil.JsonError:
			alog.Info{"action": "validate_session_api",
				"status":        "invalid_session",
				"apiToken":      apiToken,
				"session_token": sessionToken}.LogR(r)
			w.JsonError(r, apierrors.NewValidateSessionError(err))
			return

		default:
			alog.Error{"action": "validate_session_api",
				"status":        "unexpected_error",
				"session_token": sessionToken,
				"apiToken":      apiToken,
				"error":         err}.LogR(r)
			w.JsonServerError(r)
			return
		}
	}

	w.JsonOkData(r, validate.ValidateResponse{
		ApiKey:      apiKey,
		UserSession: us,
	})
}
