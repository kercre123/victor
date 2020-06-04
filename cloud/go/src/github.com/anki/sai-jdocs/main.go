package main

import (
	"os"

	"github.com/anki/sai-jdocs/commands"
	"github.com/anki/sai-jdocs/server/jdocs" // package jdocssvc
	"github.com/anki/sai-service-framework/svc"
)

func main() {
	svc.ServiceMain(os.Args[0], "JSON Doc Store",
		// Commands to setup the backend, before start
		//commands.CreateTables(),  // TODO?
		commands.CheckTables(),

		// This is the actual service implementation command
		svc.WithServiceCommand("start", "Run the RPC service", jdocssvc.NewService()),

		// These are all examples of clients connecting to the service. Normally
		// these wouldn't be in the service binary, they're just here for
		// convenience.
		svc.WithCommandObject("echo", "Send Echo RPC test request to the server",
			commands.NewEchoClientCommand()),
		svc.WithCommandObject("write-doc", "Write a document",
			commands.NewWriteDocClientCommand()),
		svc.WithCommandObject("read-doc", "Read a document",
			commands.NewReadDocClientCommand()),
		svc.WithCommandObject("read-docs", "Read the latest version of one or more documents",
			commands.NewReadDocsClientCommand()),
		svc.WithCommandObject("delete-doc", "Delete a document",
			commands.NewDeleteDocClientCommand()),
		svc.WithCommandObject("blt", "Baby Load Test",
			commands.NewBabyLoadTestClientCommand()),
	)
}
