package accounts

import (
	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
)

func ReactivateUsercommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("reactivate-user", "Reactivate a single user", func(cmd *cli.Cmd) {
		cmd.Spec = "-u -n"

		userid := cmd.StringOpt("u userid", "", "User ID to reactivate")
		username := cmd.StringOpt("n username", "", "User name to reactivate")

		cmd.Action = func() {
			c := newClient(*cfg)

			apiutil.DefaultHandler(c.ReactivateUser(*userid, *username))
		}
	})
}
