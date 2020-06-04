package audit

import (
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
)

func InitializeCommands(cmd *cli.Cmd, cfg **config.Config) {
	WriteCommand(cmd, cfg)
	ReadCommand(cmd, cfg)
	ListEntriesCommand(cmd, cfg)
	SearchByAccountCommand(cmd, cfg)
	SearchByPrincipalCommand(cmd, cfg)
}
