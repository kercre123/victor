package accounts

import (
	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
)

func ResendVerifyCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("resend-verify", "Resend the activate account email", func(cmd *cli.Cmd) {
		cmd.Spec = "[--userid]"

		userid := cmd.StringOpt("u userid", "", "User ID to reactivate - Defaults to the active session")

		cmd.Action = func() {
			cfg := *cfg
			c := newClient(cfg)

			if *userid == "" {
				*userid = cfg.Session.UserID
				if *userid == "" {
					cliutil.Fail("No user ID defined")
				}
			}
			apiutil.DefaultHandler(c.ResendVerify(*userid))
		}
	})
}
