package vrscmd

import (
	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/config"
	cli "github.com/jawher/mow.cli"
)

func redeemRewardsCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("redeem", "Mark GUIDs as redeemed", func(cmd *cli.Cmd) {
		cmd.Spec = "--device-id --guid..."

		deviceID := cmd.StringOpt("d device-id", "", "Device ID to collect redemption for")
		guids := cmd.StringsOpt("g guid", nil, "Guid or device prefix to search against")

		cmd.Action = func() {
			cfg := *cfg

			c := newClient(cfg)
			resp, err := c.MarkRedeemed(*deviceID, *guids...)
			apiutil.DefaultHandler(resp, err)
		}
	})
}
