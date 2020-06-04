// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// +build !accounts_no_db

package session

import (
	"database/sql"
	"net/http"
	"net/url"
	"time"

	"github.com/anki/sai-go-accounts/apierrors"
	"github.com/anki/sai-go-accounts/db"
	"github.com/anki/sai-go-accounts/models/user"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-go-util/task"
	"github.com/coopernurse/gorp"
)

var (
	UserSessionLifetime  = 365 * 24 * time.Hour
	AdminSessionLifetime = 3 * time.Hour
)

func init() {
	db.RegisterTable("sessions", Session{})
}

func (s *Session) PreInsert(sql gorp.SqlExecutor) (err error) {
	if s.Id == "" {
		if err = db.AssignId(s); err != nil {
			return err
		}
	}
	s.TimeCreated = time.Now()
	return nil
}

func NewUserSession(ua *user.UserAccount) (*UserSession, error) {
	// create the session
	s := &Session{
		UserId:      ua.User.Id,
		Scope:       UserScope,
		TimeExpires: time.Now().Add(UserSessionLifetime),
	}
	if err := db.Dbmap.Insert(s); err != nil {
		return nil, err
	}
	return &UserSession{
		Session: s,
		User:    ua,
	}, nil
}

// Create a new admin session with a normal, randomly assigned session
// ID.
func NewAdminSession(scope Scope, remoteId string) (*UserSession, error) {
	// create the session
	s := &Session{
		RemoteId:    remoteId,
		Scope:       scope,
		TimeExpires: time.Now().Add(AdminSessionLifetime),
	}
	if err := db.Dbmap.Insert(s); err != nil {
		return nil, err
	}
	return &UserSession{
		Session: s,
	}, nil
}

// Create a new admin session with a pre-assigned ID, for integration
// testing with known session IDs. Only to be used for testing.
func NewAdminSessionWithId(scope Scope, remoteId string, sessionId string) (*UserSession, error) {
	s := &Session{
		Id:          sessionId,
		RemoteId:    remoteId,
		Scope:       scope,
		TimeExpires: time.Now().Add(AdminSessionLifetime),
	}
	if err := db.Dbmap.Insert(s); err != nil {
		return nil, err
	}
	return &UserSession{
		Session: s,
	}, nil
}

// NewByUsername validates a user account's username, password, apikey and scope
// and returns a new UserSession for that user.
func NewByUser(apiKey *ApiKey, ua *user.UserAccount, password string) (us *UserSession, err error) {
	var correct bool
	if ua.User.Status != user.AccountActive {
		return nil, apierrors.InvalidUser
	}
	if correct, err = ua.IsPasswordCorrect(password); err != nil {
		return nil, err
	}
	if !correct {
		return nil, apierrors.InvalidPassword
	}
	if !apiKey.Scopes.Contains(UserScope) {
		return nil, apierrors.InvalidScopeForApiKey
	}

	return NewUserSession(ua)
}

func NewByUsername(apiKey *ApiKey, username, password string) (us *UserSession, err error) {
	var ua *user.UserAccount
	if ua, err = user.AccountByUsername(username); err != nil {
		return nil, err
	}
	if ua == nil {
		return nil, apierrors.InvalidUser
	}
	return NewByUser(apiKey, ua, password)
}

func NewByUserId(apiKey *ApiKey, userId, password string) (us *UserSession, err error) {
	var ua *user.UserAccount
	if ua, err = user.AccountById(userId); err != nil {
		return nil, err
	}
	if ua == nil {
		return nil, apierrors.InvalidUser
	}
	return NewByUser(apiKey, ua, password)
}


/*
func NewByGoogle(apiKeyToken, googleToken string) (UserSession, error) {
}
*/

// ByToken returns the Session object that matches the supplied token.
//
// If token has expired then it will be deleted and not returned.
func ByToken(token string) (us *UserSession, err error) {
	var ua *user.UserAccount

	obj, err := db.Dbmap.Get(Session{}, token)
	if obj == nil || err != nil {
		return nil, err
	}
	s := obj.(*Session)

	if s.HasExpired() {
		return nil, s.Delete(false)
	}

	if ua, err = user.AccountById(s.UserId); err != nil {
		return nil, err
	}

	return &UserSession{
		Session: s,
		User:    ua,
	}, nil
}

func (us *UserSession) Delete(includeRelated bool) error {
	return us.Session.Delete(includeRelated)
}

// DeleteByUserid deletes all sessions belonging to a user
// used by the users api when an account is deactivated.
func DeleteByUserId(userId string) error {
	_, err := db.Dbmap.Exec("DELETE FROM sessions WHERE user_id=?", userId)
	return err
}

// DeleteOtherByUserId deletes all sessions belonging to a user
// except one explicilty passed as a paramter
func DeleteOtherByUserId(userId string, except *Session) error {
	_, err := db.Dbmap.Exec("DELETE FROM sessions WHERE user_id=? AND session_id != ?", userId, except.Id)
	return err
}

// Delete deletes a session.  If related is true then all sessions belonging
// to the user that holds the session are deleted as well.
func (s *Session) Delete(includeRelated bool) error {
	if includeRelated {
		return DeleteByUserId(s.UserId)
	}
	_, err := db.Dbmap.Delete(s)
	return err
}

// Re-up creates a new session from the existing one and deletes itself.
func (s *Session) Reup() (newSession *Session, err error) {
	// create the session
	newSession = &Session{
		UserId:      s.UserId,
		Scope:       s.Scope,
		TimeExpires: time.Now().Add(UserSessionLifetime),
	}
	if err = db.Dbmap.Insert(newSession); err != nil {
		return nil, err
	}
	if err = s.Delete(false); err != nil {
		return nil, err
	}
	return newSession, nil
}

func (us *UserSession) Reup() (*UserSession, error) {
	newSession, err := us.Session.Reup()
	if err != nil {
		return nil, err
	}
	us.Session = newSession
	return us, nil
}

// LocalValidator implements the client.SessionValidator interface
// so that HTTP calls to APIs query the local database to validate sessions
type LocalValidator struct{}

func (v *LocalValidator) ValidateSession(r *http.Request, apiToken, sessionToken string) (apiKey *ApiKey, ua *UserSession, remoteRequestId string, err error) {
	if apiToken != "" {
		apiKey, err = ApiKeyByToken(apiToken)
		if err != nil {
			alog.Error{"action": "validate_api_key", "status": "unexpected_error", "apikey": apiToken, "error": err}.LogR(r)
			return nil, nil, "", err
		}
		if apiKey == nil {
			alog.Info{"action": "validate_api_key", "status": "invalid_key", "apikey": apiToken}.LogR(r)
			return nil, nil, "", apierrors.InvalidApiKey
		}
	}

	if sessionToken != "" {
		ua, err = ByToken(sessionToken)
		if err != nil {
			alog.Error{"action": "validate_session", "status": "unexpected_error", "token": sessionToken, "error": err}.LogR(r)
			return apiKey, nil, "", err
		}
		if ua == nil {
			alog.Info{"action": "validate_session", "status": "invalid_token", "token": sessionToken, "error": err}.LogR(r)
			return apiKey, nil, "", apierrors.InvalidSession
		}
	}
	return apiKey, ua, "", nil
}

var (
	PurgeExpiredRunner = task.NewRunner(task.TaskFunc(PurgeExpired))
)

// Delete all expired sessions
// This may block for a while; use PurgeExpiredTask.Run() instead to run in the background
func PurgeExpired(frm url.Values) error {
	var (
		result sql.Result
		err    error
	)
	start := time.Now()
	if frm.Get("purge-all") == "true" {
		result, err = db.Dbmap.Exec("DELETE FROM sessions")
	} else {

		result, err = db.Dbmap.Exec("DELETE FROM sessions WHERE time_expires < ?", start)
	}
	rowcount, _ := result.RowsAffected()
	duration := time.Now().Sub(start)

	if err != nil {
		alog.Error{"action": "session_purge_expired", "status": "dberror", "error": err, "duration_seconds": duration.Seconds()}.Log()
	} else {
		alog.Info{"action": "session_purge_expired", "status": "ok", "count_purged": rowcount, "duration_seconds": duration.Seconds()}.Log()
	}

	return err
}
