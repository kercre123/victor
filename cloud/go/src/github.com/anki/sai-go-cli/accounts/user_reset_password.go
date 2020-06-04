package accounts

import (
	"net/url"

	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/anki/sai-go-util/http/apiclient"
	"github.com/jawher/mow.cli"
)

func ResetPasswordCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("reset-password", "Reset password by username or email address", func(cmd *cli.Cmd) {
		cmd.Spec = "-u | -e"

		username := cmd.StringOpt("u username", "", "Username to reset")
		email := cmd.StringOpt("e email", "", "Email address to reset")

		cmd.Action = func() {
			c := newClient(*cfg)

			frm := url.Values{}
			if username != nil && *username != "" {
				frm.Set("username", *username)
			} else if email != nil && *email != "" {
				frm.Set("email", *email)
			}
			r, _ := c.NewRequest("POST", "/1/reset_user_password", apiclient.WithFormBody(frm))
			apiutil.DefaultHandler(c.Do(r))
		}
	})
}
