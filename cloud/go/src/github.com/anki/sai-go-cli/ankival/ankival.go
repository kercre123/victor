package ankival

import (
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
)

func InitializeCommands(cmd *cli.Cmd, cfg **config.Config) {
	GetUserDocCommand(cmd, cfg)
	CheckStatusCommand(cmd, cfg)
	SendTxnCommand(cmd, cfg)
}
