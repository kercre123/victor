package jdocs

import (
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
)

func InitializeCommands(cmd *cli.Cmd, cfg **config.Config) {
	EchoCommand(cmd)
	WriteDocCommand(cmd)
	ReadDocCommand(cmd)
	ReadDocsCommand(cmd)
	DeleteDocCommand(cmd)
}
