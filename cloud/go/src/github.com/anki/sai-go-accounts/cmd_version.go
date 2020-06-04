package main

import (
	"fmt"
	"os"

	"github.com/anki/sai-go-util/buildinfo"
	"github.com/anki/sai-go-util/cli"
)

// Define a command that will display build & version info
var versionCmd = &Command{
	Name:       "version",
	RequiresDb: false,
	Exec:       version,
}

func version(cmd *Command, args []string) {
	fmt.Println(os.Args[0])
	fmt.Println(buildinfo.String())
	cli.Exit(0)
}
