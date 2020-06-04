package cli

import (
	"fmt"
	"os"

	"github.com/anki/sai-go-util/buildinfo"
)

func init() {
	RegisterCommand("version", &versionCommand{})
}

type versionCommand struct{}

func (c *versionCommand) Run(args []string) {
	fmt.Println(os.Args[0])
	fmt.Println(buildinfo.Info().String())
}
