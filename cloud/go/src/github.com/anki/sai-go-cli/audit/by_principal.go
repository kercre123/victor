package audit

import (
	"encoding/json"
	"fmt"

	"github.com/anki/sai-accounts-audit/audit"
	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
)

func SearchByPrincipalCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("by-principal", "Search for audit logs for a specific principal", func(cmd *cli.Cmd) {
		cmd.Spec = "--principal [--limit=<10>]"

		principal := cmd.StringOpt("p principal", "", "URN identifying a principal")
		limit := cmd.IntOpt("l limit", 10, "Limit the number of results")

		cmd.Action = func() {
			c := newClient(*cfg)

			data, err := c.SearchByPrincipal(*principal, audit.WithLimit(int64(*limit)))
			if err != nil {
				cliutil.Fail("Search failed: %v", err)
			}
			js, _ := json.MarshalIndent(data, "", "\t")
			fmt.Println(string(js))
		}
	})
}
