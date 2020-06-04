package main

import (
	"fmt"
	"os"
	"time"

	"github.com/anki/sai-go-accounts/models/session"
	"github.com/anki/sai-go-util/cli"
	"github.com/anki/sai-go-util/envconfig"
	"github.com/anki/sai-go-util/log"
)

const (
	timeFormat = "2006-01-02T15:04:05"
	// Default to a 20 year lifetime for new keys
	defaultDuration = 20 * 385 * 24 * time.Hour
)

// Define a command that can create/insert a new app api key.
var createAppKeyCmd = &Command{
	Name:       "create-app-key",
	RequiresDb: true,
	Exec:       createAppKey,
	Flags:      createAppKeyFlags,
}

var (
	appKeyToken      string
	appKeyDesc       string
	appKeyExpiration string = time.Now().Add(defaultDuration).Format(timeFormat)
	appKeyScopeMask  string = "user"
)

func createAppKeyFlags(cmd *Command) {
	envconfig.String(&appKeyToken, "", "token", "Token to assign to app key")
	envconfig.String(&appKeyDesc, "", "desc", "Description for app key")
	envconfig.String(&appKeyExpiration, "", "expiry", "Time key will expire. Format YYYY-MM-DDTHH:MM:SS")
	envconfig.String(&appKeyScopeMask, "", "scopemask", "Comma separated list of scopes that may be used with this app key")
}

func createAppKey(cmd *Command, args []string) {
	const action = "create_app_key"

	// parse the scope mask
	scopeMask, err := session.ParseScopeMask(appKeyScopeMask)
	if err != nil {
		fmt.Println(err)
		os.Exit(2)
	}

	expire, err := time.Parse(timeFormat, appKeyExpiration)
	if err != nil {
		alog.Error{
			"action": action,
			"status": "error",
			"error":  fmt.Sprintf("Failed to parse expiration time: %s", err),
		}.Log()
		cli.Exit(2)
	}

	if appKeyToken == "" {
		cmd.Usage()
	}

	_, err = session.NewApiKey(appKeyToken, scopeMask, appKeyDesc, expire)
	if err != nil {
		alog.Error{
			"action": action,
			"status": "error",
			"error":  fmt.Sprintf("Failed to create app key: %s", err),
		}.Log()
		cli.Exit(2)
	}

	alog.Info{
		"action":     action,
		"status":     "app_key_inserted",
		"token":      appKeyToken,
		"scopemask":  scopeMask,
		"expiration": expire.Format(timeFormat),
	}.Log()
}
