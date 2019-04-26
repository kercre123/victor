package accounts

import (
	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/anki/sai-go-util/http/apiclient"
	"github.com/jawher/mow.cli"
)

func CreateAccountCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("create-user", "Create a new user", func(cmd *cli.Cmd) {
		cmd.Spec = "-u -p -e [-g] [-f] [-b] [-n]"

		// Required
		username := cmd.StringOpt("u username", "", "Username")
		password := cmd.StringOpt("p password", "", "Password")
		email := cmd.StringOpt("e email", "", "Email address")

		// Optional
		givenName := cmd.StringOpt("g given-name", "", "Given (first) name")
		familyName := cmd.StringOpt("f family-name", "", "Family name (surname)")
		dateOfBirth := cmd.StringOpt("b dob", "", "Date of birth")
		gender := cmd.StringOpt("n gender", "", "Gender")

		cmd.Action = func() {
			c := newClient(*cfg)

			userInfo := make(map[string]interface{})

			userInfo["username"] = username
			userInfo["password"] = password
			userInfo["email"] = email

			if *givenName != "" {
				userInfo["given_name"] = *givenName
			}
			if *familyName != "" {
				userInfo["family_name"] = *familyName
			}
			if *dateOfBirth != "" {
				userInfo["dob"] = *dateOfBirth
			}
			if *gender != "" {
				userInfo["gender"] = *gender
			}

			r, _ := c.NewRequest("POST", "/1/users", apiclient.WithJsonBody(userInfo))
			apiutil.DefaultHandler(c.Do(r))
		}
	})
}
