package accounts

import (
	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
)

func ValidateSessionCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("validate-session", "Validate a session token", func(cmd *cli.Cmd) {
		cmd.Spec = "-t -a"
		var (
			token = cmd.StringOpt("t token", "", "Session token to be validated")
			app   = cmd.StringOpt("a app", "", "App key of the user's session to be validated (the service's appkey must still be passed. eg. using the global --appkey option)")
		)
		cmd.Action = func() {
			cfg := *cfg
			c := newClient(cfg)

			// Must not supply a session token when calling validate
			cfg.Session.Token = ""

			apiutil.DefaultHandler(c.ValidateSession(*app, *token))
		}
	})
}
