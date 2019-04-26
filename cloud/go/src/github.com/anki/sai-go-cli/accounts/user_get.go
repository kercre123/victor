package accounts

import (
	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
)

func GetUserCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("get-user", "View a user's account details", func(cmd *cli.Cmd) {
		cmd.Spec = "[--userid]"

		userid := cmd.StringOpt("i userid", "", "User ID of the user to view - Defaults to \"me\"")

		cmd.Action = func() {
			c := newClient(*cfg)

			uid := *userid
			if uid == "" {
				uid = "me"
			}

			apiutil.DefaultHandler(c.UserById(uid))
		}

	})
}
