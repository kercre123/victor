package vrscmd

import (
	"github.com/anki/sai-go-cli/config"
	cli "github.com/jawher/mow.cli"
)

// InitializeCommands registers commands with the CLI library.
func InitializeCommands(cmd *cli.Cmd, cfg **config.Config) {
	findRewardsCommand(cmd, cfg)
	redeemRewardsCommand(cmd, cfg)
	addToQueueCommand(cmd, cfg)
}
