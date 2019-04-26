package ankival

import (
	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
)

func CheckStatusCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("status", "Fetch API status", func(cmd *cli.Cmd) {
		cmd.Action = func() {
			c := newClient(*cfg)

			apiutil.GetStatus(c.Client)
		}
	})
}
