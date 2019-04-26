package main

import (
	"github.com/anki/sai-go-accounts/db"
	"github.com/anki/sai-go-util/cli"
	"github.com/anki/sai-go-util/log"
)

// Define a command that will create all of the database tables.
var createTablesCmd = &Command{
	Name:       "create-tables",
	RequiresDb: true,
	Exec:       createTables,
}

func createTables(cmd *Command, args []string) {
	const action = "create_db_tables"
	alog.Info{"action": action, "status": "starting"}.Log()
	if err := db.CreateTables(); err != nil {
		alog.Error{"action": action, "status": "failed", "error": err}.Log()
		cli.Exit(1)
	}
	alog.Info{"action": action, "status": "completed"}.Log()
	cli.Exit(0)
}
