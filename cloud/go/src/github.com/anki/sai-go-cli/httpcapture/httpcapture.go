package httpcapture

import (
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
)

func InitializeCommands(cmd *cli.Cmd, cfg **config.Config) {
	OpenCommand(cmd, cfg)
	TailCommand(cmd, cfg)
}
