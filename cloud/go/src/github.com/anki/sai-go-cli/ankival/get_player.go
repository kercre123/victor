package ankival

import (
	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
)

func GetUserDocCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("get-doc", "Fetch a player profile", func(cmd *cli.Cmd) {
		cmd.Spec = "[--userid]"

		userid := cmd.StringOpt("u userid", "", "Id of the user profile to fetch.  Defaults to the userid of the active session")

		cmd.Action = func() {
			cfg := *cfg
			c := newClient(cfg)

			if *userid == "" {
				*userid = cfg.Session.UserID
				if *userid == "" {
					cliutil.Fail("Current session has unknown userid")
				}
			}
			apiutil.DefaultHandler(c.GetUserDoc(*userid))
		}
	})
}
