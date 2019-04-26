package cli

import (
	"errors"
	"fmt"
	"os"
	"sort"

	"github.com/anki/sai-go-util/envconfig"
	"github.com/anki/sai-go-util/log"
)

var commands = map[string]Command{}

type Initializable interface {
	Initialize() error
}

type Configurable interface {
	// Flags can assign addition flags prior to command line arguments
	// being parsed.
	Flags()
}

type Command interface {
	// Run executes the command, with the remaining command line
	// arguments following the command name passed as args.
	Run(args []string)
}

func RegisterCommand(name string, cmd Command) error {
	if _, ok := commands[name]; ok {
		return errors.New("Command " + name + " already registered")
	}
	commands[name] = cmd
	return nil
}

func Usage() {
	names := []string{}
	for key, _ := range commands {
		names = append(names, key)
	}
	sort.Strings(names)
	fmt.Printf("Usage: %s command [args...]\n", os.Args[0])
	fmt.Println("Available commands")
	for _, name := range names {
		fmt.Println(name)
	}
	Exit(0)
}

func Run() {
	defer CleanupAndExit()
	SetupLogging()

	if len(os.Args) < 2 {
		Usage()
		return
	}

	contextUsage := func() {
		if envconfig.DefaultConfig.SubCmd == "" {
			Usage()
		} else {
			envconfig.DefaultConfig.Usage()
		}
	}
	envconfig.DefaultConfig.Flags.Usage = contextUsage

	for key, cmd := range commands {
		if key == os.Args[1] {
			alog.Command = key
			envconfig.DefaultConfig.SubCmd = key

			if cfg, ok := cmd.(Configurable); ok {
				cfg.Flags()
			}

			envconfig.DefaultConfig.Flags.Parse(os.Args[2:])

			if init, ok := cmd.(Initializable); ok {
				err := init.Initialize()
				if err != nil {
					alog.Error{"action": "initialize", "status": "error", "error": err}.Log()
					contextUsage()
					return
				}
			}

			cmd.Run(os.Args[2:])
			return
		}
	}

	Usage()
}
