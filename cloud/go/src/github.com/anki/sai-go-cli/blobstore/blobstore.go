package blobstore

import (
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
)

func InitializeCommands(cmd *cli.Cmd, cfg **config.Config) {
	DownloadCommand(cmd, cfg)
	InfoCommand(cmd, cfg)
	SearchCommand(cmd, cfg)
	TailCommand(cmd, cfg)
	UploadCommand(cmd, cfg)
}
