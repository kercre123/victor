package jdocs

import (
	"github.com/anki/sai-jdocs/commands"
	"github.com/jawher/mow.cli"
)

func EchoCommand(parent *cli.Cmd) {
	clientCmd := commands.NewEchoClientCommand()
	parent.Command("echo", "Echo a string", func(cmd *cli.Cmd) {
		clientCmd.CommandInitialize(cmd)
		cmd.Spec = clientCmd.CommandSpec()
	})
}
