package accounts

import (
	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
)

func DeleteSessionCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("delete-session", "Delete an existing session", func(cmd *cli.Cmd) {
		cmd.Spec = "-t [-r]"

		token := cmd.StringOpt("t token", "", "Session token to be deleted")
		related := cmd.BoolOpt("r related", false, "Delete related")

		cmd.Action = func() {
			c := newClient(*cfg)

			apiutil.DefaultHandler(c.DeleteSession(*token, *related))
		}
	})
}
