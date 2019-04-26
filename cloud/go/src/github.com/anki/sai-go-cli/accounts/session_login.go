package accounts

import (
	"fmt"
	"net/http"

	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
	api "github.com/anki/sai-go-util/http/apiclient"
	"github.com/jawher/mow.cli"
)

func LoginCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("login", "Login and create a new persisted session", func(cmd *cli.Cmd) {
		cmd.Spec = "(--username | --userid) --password [--session-name]"

		username := cmd.StringOpt("u username", "", "Username of the user to log in")
		userid := cmd.StringOpt("i userid", "", "User ID of the user to log in")
		password := cmd.StringOpt("p password", "", "Password of the user")
		sessionName := cmd.StringOpt("s session-name", "default", "Name of session in configuration file")

		cmd.Action = func() {
			cfg := *cfg
			c := newClient(cfg)

			var resp *api.Response
			var err error

			if username != nil && *username != "" {
				resp, err = c.NewUserSession(*username, *password)
			} else if userid != nil && *userid != "" {
				resp, err = c.NewUserIdSession(*userid, *password)
			}
			if err != nil {
				cliutil.Fail("Failed to send request: %v", err)
			}

			if resp.StatusCode != http.StatusOK {
				apiutil.DefaultHandler(resp, err)
				return
			}

			jresp, err := resp.Json()
			if err != nil {
				cliutil.Fail("Failed to decode JSON response: %v", err)
			}

			s, err := cfg.NewSession(*sessionName)
			if err != nil {
				cliutil.Fail("Failed to create session %v", err)
			}

			if userID, _ := jresp.FieldStr("user", "user_id"); userID != "" {
				s.UserID = userID
			}

			token, err := jresp.FieldStr("session", "session_token")
			if err != nil {
				cliutil.Fail("Failed to extract session token: %v", err)
			}
			s.Token = token

			if err := s.Save(); err != nil {
				cliutil.Fail("Failed to save configuration: %v", err)
			}

			fmt.Println("Session updated")
		}
	})
}
