package jdocs

// crud.go - Create, Read, Update, and Delete JDOCS documents.

import (
	"github.com/anki/sai-jdocs/commands"
	"github.com/jawher/mow.cli"
)

func WriteDocCommand(parent *cli.Cmd) {
	clientCmd := commands.NewWriteDocClientCommand()
	parent.Command("write-doc", "Write a document", func(cmd *cli.Cmd) {
		clientCmd.CommandInitialize(cmd)
		cmd.Spec = clientCmd.CommandSpec()
	})
}

func ReadDocCommand(parent *cli.Cmd) {
	clientCmd := commands.NewReadDocClientCommand()
	parent.Command("read-doc", "Read a document", func(cmd *cli.Cmd) {
		clientCmd.CommandInitialize(cmd)
		cmd.Spec = clientCmd.CommandSpec()
	})
}

func ReadDocsCommand(parent *cli.Cmd) {
	clientCmd := commands.NewReadDocsClientCommand()
	parent.Command("read-docs", "Read the latest version of one or more documents", func(cmd *cli.Cmd) {
		clientCmd.CommandInitialize(cmd)
		cmd.Spec = clientCmd.CommandSpec()
	})
}

func DeleteDocCommand(parent *cli.Cmd) {
	clientCmd := commands.NewDeleteDocClientCommand()
	parent.Command("delete-doc", "Delete a document", func(cmd *cli.Cmd) {
		clientCmd.CommandInitialize(cmd)
		cmd.Spec = clientCmd.CommandSpec()
	})
}
