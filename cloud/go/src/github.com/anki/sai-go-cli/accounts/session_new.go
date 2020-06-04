package accounts

import (
	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
)

func NewSessionCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("new-session", "Create a new user session", func(cmd *cli.Cmd) {
		cmd.Spec = "(-u | -i) -p"

		username := cmd.StringOpt("u username", "", "Username of the user to log in")
		userid := cmd.StringOpt("i userid", "", "User ID of the user to log in")
		password := cmd.StringOpt("p password", "", "Password of the user")

		cmd.Action = func() {
			c := newClient(*cfg)

			if username != nil && *username != "" {
				apiutil.DefaultHandler(c.NewUserSession(*username, *password))
			} else if userid != nil && *userid != "" {
				apiutil.DefaultHandler(c.NewUserIdSession(*userid, *password))
			}
		}
	})
}
