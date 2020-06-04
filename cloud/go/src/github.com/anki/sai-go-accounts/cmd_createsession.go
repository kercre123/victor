package main

import (
	"fmt"
	"time"

	"github.com/anki/sai-go-accounts/models/session"
	"github.com/anki/sai-go-util/cli"
	"github.com/anki/sai-go-util/envconfig"
	"github.com/anki/sai-go-util/log"
)

// Define a command that will create an arbitrary session.
// Used exclusivley by integration tests.
var createSessionCmd = &Command{
	Name:       "create-session",
	RequiresDb: true,
	Exec:       createSession,
	Flags:      createSessionFlags,
}

const (
	sessionTimeFormat = "2006-01-02T15:04:05"
	// Default to a 1 hour session
	sessionDefaultDuration = time.Hour
)

var (
	sessionExpiration string = time.Now().Add(sessionDefaultDuration).Format(sessionTimeFormat)
	sessionScope             = "user"
	sessionOwnerId    string
	sessionId         string
)

func createSessionFlags(cmd *Command) {
	envconfig.String(&sessionOwnerId, "", "ownerid", "Owner id (email address)")
	envconfig.String(&sessionExpiration, "", "expiry", "Time session will expire. Format YYYY-MM-DDTHH:MM:SS")
	envconfig.String(&sessionScope, "", "scope", "Name of scope to assign to session")
	envconfig.String(&sessionId, "", "sessionid", "An optional ID to use for the session")
}

func createSession(cmd *Command, args []string) {
	const action = "create_session_token"

	scope, ok := session.ScopeInverse[sessionScope]
	if !ok {
		alog.Error{"action": action, "status": "error", "error": "Invalid scope name"}.Log()
		cli.Exit(1)
	}

	if sessionOwnerId == "" {
		alog.Error{"action": action, "status": "error", "error": "No owner id specified"}.Log()
		cli.Exit(1)
	}

	var s *session.UserSession
	var err error

	if sessionId != "" {
		s, err = session.NewAdminSessionWithId(scope, sessionOwnerId, sessionId)
	} else {
		s, err = session.NewAdminSession(scope, sessionOwnerId)
	}
	if err != nil {
		alog.Error{"action": action, "status": "error", "error": err}.Log()
		cli.Exit(1)
	}
	fmt.Println(s.Session.Token())
	cli.Exit(0)
}
