package das

import (
	"github.com/anki/sai-go-cli/config"
	cli "github.com/jawher/mow.cli"
)

func InitializeCommands(cmd *cli.Cmd, cfg **config.Config) {
	GrepCommand(cmd, cfg)
	PrintCommand(cmd, cfg)
	LoadOneDayCommand(cmd, cfg)
	ReplayOneDayCommand(cmd, cfg)
	CountEventsCommand(cmd, cfg)
	cmd.Command("sim", "Send simulated DAS traffic", func(c *cli.Cmd) {
		SimPingCommand(c, cfg)
		SimHeartbeatCommand(c, cfg)
	})
}
