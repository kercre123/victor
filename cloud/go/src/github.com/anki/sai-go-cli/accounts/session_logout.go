package accounts

import (
	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
	cli "github.com/jawher/mow.cli"
)

func LogoutCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("logout", "Logout and delete the currently active session", func(cmd *cli.Cmd) {

		cmd.Action = func() {
			cfg := *cfg
			c := newClient(cfg)

			s := cfg.Session
			if s == nil {
				cliutil.Fail("No currently active session")
			}

			resp, err := c.DeleteSession(s.Token, false)
			if err != nil {
				cliutil.Fail("Failed to delete session: %v", err)
			}
			apiutil.DefaultHandler(resp, err)

			s.Delete()
		}
	})
}
