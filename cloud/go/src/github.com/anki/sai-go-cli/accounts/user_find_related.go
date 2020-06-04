package accounts

import (
	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
)

func FindRelatedCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("find-related", "Find users that share an email address - requires a service appkey (speicfy --appkey)", func(cmd *cli.Cmd) {
		cmd.Spec = "--userid"

		userid := cmd.StringOpt("i userid", "", "User ID of the user to lookup")

		cmd.Action = func() {
			c := newClient(*cfg)

			resp, _, err := c.FetchRelatedAccounts(*userid)
			apiutil.DefaultHandler(resp, err)
		}

	})
}
