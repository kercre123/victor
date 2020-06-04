package main

import (
	"fmt"
	"os"
	"sort"

	"github.com/anki/sai-go-util/cli"
	"github.com/anki/sai-go-util/dynamoutil"
	"github.com/anki/sai-go-util/envconfig"
	"github.com/anki/sai-go-util/log"
)

var (
	commands = []*Command{
		listTablesCmd,
		createTablesCmd,
		versionCmd,
		runServerCmd,
		//dumpTablesCmd,
	}
)

func init() {
	cli.InitLogFlags("sai_blobstore")
}

// Command maps the first command line parameter to a function to run
// and the flags associated with that.
type Command struct {
	Name       string // maps to os.Args[1]
	RequiresDb bool
	Exec       func(cmd *Command, args []string) // The function associated with this command
	Flags      func(cmd *Command)                // function to assign additional flags prior to Parse being called
}

func (c *Command) Run(args []string) {
	c.Exec(c, args)
}

func (c *Command) AssignFlags() {
	if c.Flags != nil {
		c.Flags(c)
	}
}

func (c *Command) Usage() {
	cmdUsage()
}

func cmdUsage() {
	fmt.Fprintf(os.Stderr, "Usage: %s %s [args...]\n", os.Args[0], os.Args[1])
	envconfig.PrintDefaults()
	cli.Exit(1)
}

func usage() {
	cmds := []string{}
	for _, cmd := range commands {
		cmds = append(cmds, cmd.Name)
	}
	sort.Strings(cmds)
	fmt.Printf("Usage: %s command [args...]\n", os.Args[0])
	fmt.Println("Available commands")
	for _, cmd := range cmds {
		fmt.Println(cmd)
	}
	cli.Exit(0)
}

func setupDb(checkTables bool) {
	err := dynamoutil.OpenDynamo(checkTables)
	if err != nil {
		alog.Error{
			"action": "initialize_database",
			"status": "error",
			"error":  err,
		}.Log()
		cli.Exit(1)
	}
}

func main() {
	defer cli.CleanupAndExit()
	cli.SetupLogging()

	envconfig.DefaultConfig.Flags.Usage = cmdUsage

	if len(os.Args) < 2 {
		usage()
	}

	for _, cmd := range commands {
		if cmd.Name == os.Args[1] {
			cmd.AssignFlags()
			envconfig.DefaultConfig.Flags.Parse(os.Args[2:])
			cmd.Run(os.Args[2:])
			return
		}
	}

	usage()
}
