package vrscmd

import (
	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
	cli "github.com/jawher/mow.cli"
)

func findRewardsCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("find", "Find rewards for a GUID or device prefix", func(cmd *cli.Cmd) {
		cmd.Spec = "--guid"

		guid := cmd.StringOpt("g guid", "", "Guid or device prefix to search against")

		cmd.Action = func() {
			cfg := *cfg

			if *guid == "" {
				cliutil.Fail("No GUID specified")
			}

			c := newClient(cfg)
			resp, _, err := c.FindRewards(*guid)
			apiutil.DefaultHandler(resp, err)
		}
	})

}
