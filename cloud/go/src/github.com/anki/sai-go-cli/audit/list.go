package audit

import (
	"encoding/json"
	"fmt"

	"github.com/anki/sai-accounts-audit/audit"
	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
)

func ListEntriesCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("list", "List all audit log entries", func(cmd *cli.Cmd) {
		cmd.Spec = "[--limit]"

		limit := cmd.IntOpt("l limit", 10, "Limit the number of results")

		cmd.Action = func() {
			c := newClient(*cfg)

			data, err := c.List(audit.WithLimit(int64(*limit)))
			if err != nil {
				cliutil.Fail("Command failed: %v", err)
			}
			js, _ := json.MarshalIndent(data, "", "\t")
			fmt.Println(string(js))
		}
	})
}
