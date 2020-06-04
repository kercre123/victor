package accounts

import (
	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
)

func CheckUsernameCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("check-username", "Check if username exists", func(cmd *cli.Cmd) {
		cmd.Spec = "-u"
		username := cmd.StringOpt("u username", "", "Username to check")

		cmd.Action = func() {
			c := newClient(*cfg)

			r, _ := c.NewRequest("GET", "/1/usernames/"+*username)
			apiutil.DefaultHandler(c.Do(r))
		}
	})
}
