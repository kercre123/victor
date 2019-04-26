package audit

import (
	"encoding/json"
	"fmt"

	"github.com/anki/sai-accounts-audit/audit"
	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
)

func SearchByAccountCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("by-account", "Search for audit logs for a specific account", func(cmd *cli.Cmd) {
		cmd.Spec = "--account [--limit=<10>]"

		account := cmd.StringOpt("a account", "", "Account id")
		limit := cmd.IntOpt("l limit", 10, "Limit the number of results")

		cmd.Action = func() {
			c := newClient(*cfg)

			data, err := c.SearchByAccount(*account, audit.WithLimit(int64(*limit)))
			if err != nil {
				cliutil.Fail("Command failed: %v", err)
			}
			js, _ := json.MarshalIndent(data, "", "\t")
			fmt.Println(string(js))
		}
	})
}
